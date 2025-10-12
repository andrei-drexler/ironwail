#include "quakedef.h"
#include "q3shader.h"

#include <ctype.h>

static q3shader_t *q3shader_registry;

static const struct
{
        const char              *name;
        q3shader_surfaceparm_t   flag;
} q3shader_surfaceparms[] =
{
        { "nodraw",             Q3SURF_NODRAW },
        { "nonsolid",           Q3SURF_NONSOLID },
        { "nomarks",            Q3SURF_NOMARKS },
        { "nolightmap",         Q3SURF_NOLIGHTMAP },
        { "sky",                Q3SURF_SKY },
        { "lava",               Q3SURF_LAVA },
        { "slime",              Q3SURF_SLIME },
        { "water",              Q3SURF_WATER },
        { "fog",                Q3SURF_FOG },
        { "hint",               Q3SURF_HINT },
        { "skip",               Q3SURF_SKIP },
        { "detail",             Q3SURF_DETAIL },
        { "structural",         Q3SURF_STRUCTURAL },
        { "trans",              Q3SURF_TRANSLUCENT },
        { "slick",              Q3SURF_SLICK },
        { "nodamage",           Q3SURF_NODAMAGE },
        { "noimpact",           Q3SURF_NOIMPACT },
        { "nodrop",             Q3SURF_NODROP },
        { "playerclip",         Q3SURF_PLAYERCLIP },
        { "monsterclip",        Q3SURF_MONSTERCLIP },
        { "botclip",            Q3SURF_BOTCLIP },
        { "origin",             Q3SURF_ORIGIN },
        { "lightfilter",        Q3SURF_LIGHTFILTER },
        { "alphashadow",        Q3SURF_ALPHASHADOW },
        { "areaportal",         Q3SURF_AREAPORTAL },
        { "clusterportal",      Q3SURF_CLUSTERPORTAL },
        { "donotenter",         Q3SURF_DONOTENTER },
        { "dust",               Q3SURF_DUST },
        { "ladder",             Q3SURF_LADDER },
        { "nosteps",            Q3SURF_NOSTEPS },
        { "nodlight",           Q3SURF_NODLIGHT },
};

static void Q3Shader_FreeDirective (q3shader_directive_t *directive);
static void Q3Shader_FreeStage (q3shader_stage_t *stage);
static void Q3Shader_FreeShader (q3shader_t *shader);
static void Q3Shader_Insert (q3shader_t *shader);
static uint32_t Q3Shader_SurfaceParmFlag (const char *name);
static qboolean Q3Shader_AddUniquePath (char ***vec, const char *path);
static const char *Q3Shader_ParseShaderBlock (const char *data, const char *filename, q3shader_t *shader);
static const char *Q3Shader_ParseStageBlock (const char *data, const char *filename, const q3shader_t *shader, q3shader_stage_t *stage);
static const char *Q3Shader_ParseDirectiveArgs (const char *data, const char *filename, const char *shadername, q3shader_directive_t *directive);
static qboolean Q3Shader_AddDirectiveArg (q3shader_directive_t *directive, const char *value);
static void Q3Shader_AddDirective (q3shader_directive_t **list, q3shader_directive_t *directive);
static qboolean Q3Shader_SkipWhitespace (const char **pdata, qboolean *hit_newline);
static char *Q3Shader_CopyString (const char *src);
static void Q3Shader_FreeStringVec (char ***vec);
static void Q3Shader_ApplyShaderDirective (q3shader_t *shader, const q3shader_directive_t *directive);
static void Q3Shader_ApplyStageDirective (q3shader_stage_t *stage, const q3shader_directive_t *directive);
static char *Q3Shader_JoinArgs (const q3shader_directive_t *directive, size_t first_index);
static void Q3Shader_SetBlendFunc (q3shader_stage_t *stage, const q3shader_directive_t *directive);
static q3shader_t *Q3Shader_FindMutable (const char *name);

void Q3Shader_Init (void)
{
        Q3Shader_Clear ();
}

void Q3Shader_Shutdown (void)
{
        Q3Shader_Clear ();
}

void Q3Shader_Clear (void)
{
        size_t i;
        if (!q3shader_registry)
                return;

        for (i = 0; i < VEC_SIZE (q3shader_registry); ++i)
                Q3Shader_FreeShader (&q3shader_registry[i]);
        VEC_FREE (q3shader_registry);
        q3shader_registry = NULL;
}

static q3shader_t *Q3Shader_FindMutable (const char *name)
{
        size_t i;
        if (!name || !q3shader_registry)
                return NULL;

        for (i = 0; i < VEC_SIZE (q3shader_registry); ++i)
        {
                if (!q_strcasecmp (q3shader_registry[i].name, name))
                        return &q3shader_registry[i];
        }
        return NULL;
}

static void Q3Shader_Insert (q3shader_t *shader)
{
        q3shader_t *existing;

        if (!shader || !shader->name)
                return;

        existing = Q3Shader_FindMutable (shader->name);
        if (existing)
        {
                Q3Shader_FreeShader (existing);
                *existing = *shader;
        }
        else
        {
                VEC_GROW ((void **) &q3shader_registry, sizeof (q3shader_registry[0]), 1);
                q3shader_registry[VEC_HEADER (q3shader_registry).size++] = *shader;
        }

        memset (shader, 0, sizeof (*shader));
}

int Q3Shader_LoadFile (const char *path)
{
        byte    *buffer;
        const char *data;
        const char *filename;
        unsigned int path_id = 0;
        int count = 0;

        if (!path || !path[0])
                return -1;

        filename = path;
        buffer = COM_LoadMallocFile (path, &path_id);
        if (!buffer)
        {
                Con_Printf ("Q3Shader_LoadFile: couldn't load %s\n", path);
                return -1;
        }

        data = (const char *) buffer;

        while (1)
        {
                q3shader_t shader;

                data = COM_Parse (data);
                if (!data)
                        break;

                if (!com_token[0])
                        continue;

                if (com_token[0] == '{' || com_token[0] == '}')
                {
                        Con_Printf ("Q3Shader_LoadFile: unexpected token '%s' in %s\n", com_token, filename);
                        count = -1;
                        break;
                }

                memset (&shader, 0, sizeof (shader));
                shader.name = Q3Shader_CopyString (com_token);
                shader.source_file = Q3Shader_CopyString (path);
                shader.source_path_id = path_id;
                shader.cullmode = Q3CULL_BACK;
                shader.qer_trans = -1.0f;
                shader.q3map_surfacelight = 0.0f;

                data = Q3Shader_ParseShaderBlock (data, filename, &shader);
                if (!data)
                {
                        Q3Shader_FreeShader (&shader);
                        count = -1;
                        break;
                }

                Q3Shader_Insert (&shader);
                count++;
        }

        free (buffer);
        return count;
}

int Q3Shader_LoadAll (int *files_loaded, int *files_failed)
{
        searchpath_t *search;
        char        **paths = NULL;
        int             total_shaders = 0;
        int             successful_files = 0;
        int             failed_files = 0;
        size_t          i;

        for (search = com_searchpaths; search; search = search->next)
        {
                if (search->pack)
                {
                        pack_t *pak = search->pack;
                        int j;

                        for (j = 0; j < pak->numfiles; ++j)
                        {
                                const char *name = pak->files[j].name;

                                if (q_strncasecmp (name, "scripts/", 8) != 0)
                                        continue;

                                if (q_strcasecmp (COM_FileGetExtension (name), "shader"))
                                        continue;

                                Q3Shader_AddUniquePath (&paths, name);
                        }
                }
                else if (search->filename[0])
                {
                        char dir[MAX_OSPATH];
                        findfile_t *find;

                        if (q_snprintf (dir, sizeof (dir), "%s/scripts", search->filename) >= (int) sizeof (dir))
                                continue;

                        for (find = Sys_FindFirst (dir, "shader"); find; find = Sys_FindNext (find))
                        {
                                char relpath[MAX_QPATH];

                                if (find->attribs & FA_DIRECTORY)
                                        continue;

                                if (q_snprintf (relpath, sizeof (relpath), "scripts/%s", find->name) >= (int) sizeof (relpath))
                                        continue;

                                Q3Shader_AddUniquePath (&paths, relpath);
                        }
                }
        }

        for (i = 0; i < VEC_SIZE (paths); ++i)
        {
                int loaded = Q3Shader_LoadFile (paths[i]);

                if (loaded >= 0)
                {
                        successful_files++;
                        total_shaders += loaded;
                }
                else
                        failed_files++;
        }

        Q3Shader_FreeStringVec (&paths);

        if (files_loaded)
                *files_loaded = successful_files;
        if (files_failed)
                *files_failed = failed_files;

        return total_shaders;
}

static const char *Q3Shader_ParseShaderBlock (const char *data, const char *filename, q3shader_t *shader)
{
        const char *p = data;

        if (!shader)
                return NULL;

        Q3Shader_SkipWhitespace (&p, NULL);
        data = COM_Parse (p);
        if (!data || com_token[0] != '{')
        {
                Con_Printf ("Q3Shader: %s: expected '{' after shader %s\n", filename, shader->name ? shader->name : "<unknown>");
                return NULL;
        }

        while (1)
        {
                q3shader_directive_t directive;

                data = COM_Parse (data);
                if (!data)
                {
                        Con_Printf ("Q3Shader: %s: unexpected EOF while parsing shader %s\n", filename, shader->name ? shader->name : "<unknown>");
                        return NULL;
                }

                if (!com_token[0])
                        continue;

                if (com_token[0] == '}')
                        return data;

                if (com_token[0] == '{')
                {
                        q3shader_stage_t stage;
                        memset (&stage, 0, sizeof (stage));

                        data = Q3Shader_ParseStageBlock (data, filename, shader, &stage);
                        if (!data)
                        {
                                Q3Shader_FreeStage (&stage);
                                return NULL;
                        }

                        VEC_GROW ((void **) &shader->stages, sizeof (shader->stages[0]), 1);
                        shader->stages[VEC_HEADER (shader->stages).size++] = stage;
                        continue;
                }

                memset (&directive, 0, sizeof (directive));
                q_strlcpy (directive.name, com_token, sizeof (directive.name));

                data = Q3Shader_ParseDirectiveArgs (data, filename, shader->name, &directive);
                if (!data)
                {
                        Q3Shader_FreeDirective (&directive);
                        return NULL;
                }

                Q3Shader_ApplyShaderDirective (shader, &directive);
                Q3Shader_AddDirective (&shader->directives, &directive);
        }
}

static const char *Q3Shader_ParseStageBlock (const char *data, const char *filename, const q3shader_t *shader, q3shader_stage_t *stage)
{
        while (1)
        {
                q3shader_directive_t directive;

                data = COM_Parse (data);
                if (!data)
                {
                        Con_Printf ("Q3Shader: %s: unexpected EOF in shader stage for %s\n", filename, shader && shader->name ? shader->name : "<unknown>");
                        return NULL;
                }

                if (!com_token[0])
                        continue;

                if (com_token[0] == '}')
                        return data;

                if (com_token[0] == '{')
                {
                        Con_Printf ("Q3Shader: %s: unexpected '{' inside shader stage for %s\n", filename, shader && shader->name ? shader->name : "<unknown>");
                        return NULL;
                }

                memset (&directive, 0, sizeof (directive));
                q_strlcpy (directive.name, com_token, sizeof (directive.name));

                data = Q3Shader_ParseDirectiveArgs (data, filename, shader && shader->name ? shader->name : NULL, &directive);
                if (!data)
                {
                        Q3Shader_FreeDirective (&directive);
                        return NULL;
                }

                Q3Shader_ApplyStageDirective (stage, &directive);
                Q3Shader_AddDirective (&stage->directives, &directive);
        }
}

static const char *Q3Shader_ParseDirectiveArgs (const char *data, const char *filename, const char *shadername, q3shader_directive_t *directive)
{
        while (1)
        {
                const char *p = data;
                qboolean hit_newline = false;

                if (!Q3Shader_SkipWhitespace (&p, &hit_newline))
                        return NULL;

                if (hit_newline)
                {
                        data = p;
                        return data;
                }

                if (!p[0] || p[0] == '{' || p[0] == '}')
                {
                        data = p;
                        return data;
                }

                data = COM_Parse (p);
                if (!data)
                {
                        Con_Printf ("Q3Shader: %s: unexpected EOF while reading directive '%s' in shader %s\n", filename, directive->name, shadername ? shadername : "<unknown>");
                        return NULL;
                }

                if (!com_token[0])
                        continue;

                if (!Q3Shader_AddDirectiveArg (directive, com_token))
                        return NULL;
        }
}

static qboolean Q3Shader_SkipWhitespace (const char **pdata, qboolean *hit_newline)
{
        const char *data;
        qboolean newline = false;

        if (!pdata || !*pdata)
                return false;

        data = *pdata;

        while (*data)
        {
                if (*data == '\n')
                {
                        newline = true;
                        data++;
                        continue;
                }
                if (*data <= ' ')
                {
                        data++;
                        continue;
                }
                if (*data == '/' && data[1] == '/')
                {
                        data += 2;
                        while (*data && *data != '\n')
                                data++;
                        continue;
                }
                if (*data == '/' && data[1] == '*')
                {
                        data += 2;
                        while (*data && !(*data == '*' && data[1] == '/'))
                        {
                                if (*data == '\n')
                                        newline = true;
                                data++;
                        }
                        if (*data)
                                data += 2;
                        continue;
                }
                break;
        }

        if (hit_newline)
                *hit_newline = newline;
        *pdata = data;
        return true;
}

static qboolean Q3Shader_AddDirectiveArg (q3shader_directive_t *directive, const char *value)
{
        char *copy;

        if (!directive || !value)
                return false;

        copy = Q3Shader_CopyString (value);
        if (!copy)
        {
                        Con_Printf ("Q3Shader: failed to allocate argument for '%s'\n", directive->name);
                        return false;
        }

        VEC_GROW ((void **) &directive->args, sizeof (directive->args[0]), 1);
        directive->args[VEC_HEADER (directive->args).size++] = copy;
        return true;
}

static void Q3Shader_AddDirective (q3shader_directive_t **list, q3shader_directive_t *directive)
{
        if (!list || !directive)
                return;

        VEC_GROW ((void **) list, sizeof ((*list)[0]), 1);
        (*list)[VEC_HEADER (*list).size++] = *directive;
        directive->args = NULL;
}

static qboolean Q3Shader_AddUniquePath (char ***vec, const char *path)
{
        size_t i;
        char *copy;

        if (!vec || !path || !path[0])
                return false;

        for (i = 0; i < VEC_SIZE (*vec); ++i)
        {
                if (!q_strcasecmp ((*vec)[i], path))
                        return false;
        }

        copy = Q3Shader_CopyString (path);
        if (!copy)
        {
                Con_Printf ("Q3Shader: failed to allocate memory for path '%s'\n", path);
                return false;
        }

        VEC_GROW ((void **) vec, sizeof ((*vec)[0]), 1);
        (*vec)[VEC_HEADER (*vec).size++] = copy;
        return true;
}

static char *Q3Shader_CopyString (const char *src)
{
        size_t len;
        char *copy;

        if (!src)
                return NULL;
        len = strlen (src);
        copy = (char *) malloc (len + 1);
        if (!copy)
                return NULL;
        memcpy (copy, src, len + 1);
        return copy;
}

static char *Q3Shader_JoinArgs (const q3shader_directive_t *directive, size_t first_index)
{
        size_t count, i;
        size_t total = 0;
        char *result;
        size_t pos = 0;

        if (!directive || !directive->args)
                return NULL;

        count = VEC_SIZE (directive->args);
        if (first_index >= count)
                return NULL;

        for (i = first_index; i < count; ++i)
        {
                total += strlen (directive->args[i]);
                if (i + 1 < count)
                        total++;
        }

        result = (char *) malloc (total + 1);
        if (!result)
                return NULL;

        for (i = first_index; i < count; ++i)
        {
                size_t arglen = strlen (directive->args[i]);
                memcpy (result + pos, directive->args[i], arglen);
                pos += arglen;
                if (i + 1 < count)
                        result[pos++] = ' ';
        }
        result[pos] = '\0';
        return result;
}

static void Q3Shader_SetBlendFunc (q3shader_stage_t *stage, const q3shader_directive_t *directive)
{
        size_t argc;
        const char *src = NULL, *dst = NULL;

        if (!stage || !directive)
                return;

        argc = VEC_SIZE (directive->args);
        if (argc == 1)
        {
                const char *token = directive->args[0];
                if (!q_strcasecmp (token, "add"))
                {
                        src = "GL_ONE";
                        dst = "GL_ONE";
                }
                else if (!q_strcasecmp (token, "filter"))
                {
                        src = "GL_DST_COLOR";
                        dst = "GL_ZERO";
                }
                else if (!q_strcasecmp (token, "blend"))
                {
                        src = "GL_SRC_ALPHA";
                        dst = "GL_ONE_MINUS_SRC_ALPHA";
                }
                else
                {
                        src = token;
                        dst = NULL;
                }
        }
        else if (argc >= 2)
        {
                src = directive->args[0];
                dst = directive->args[1];
        }

        if (src)
        {
                free (stage->blendfunc_src);
                stage->blendfunc_src = Q3Shader_CopyString (src);
        }
        if (dst)
        {
                free (stage->blendfunc_dst);
                stage->blendfunc_dst = Q3Shader_CopyString (dst);
        }
        else
        {
                free (stage->blendfunc_dst);
                stage->blendfunc_dst = NULL;
                if (!src)
                {
                        free (stage->blendfunc_src);
                        stage->blendfunc_src = NULL;
                }
        }
}

static void Q3Shader_ApplyShaderDirective (q3shader_t *shader, const q3shader_directive_t *directive)
{
        if (!shader || !directive)
                return;

        if (!q_strcasecmp (directive->name, "surfaceparm"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                        shader->surfaceparms |= Q3Shader_SurfaceParmFlag (directive->args[0]);
        }
        else if (!q_strcasecmp (directive->name, "cull"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                {
                        const char *value = directive->args[0];
                        if (!q_strcasecmp (value, "front"))
                                shader->cullmode = Q3CULL_FRONT;
                        else if (!q_strcasecmp (value, "back"))
                                shader->cullmode = Q3CULL_BACK;
                        else if (!q_strcasecmp (value, "twosided") || !q_strcasecmp (value, "disable") || !q_strcasecmp (value, "none"))
                                shader->cullmode = Q3CULL_TWOSIDED;
                }
        }
        else if (!q_strcasecmp (directive->name, "qer_editorimage"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                {
                        free (shader->qer_editorimage);
                        shader->qer_editorimage = Q3Shader_CopyString (directive->args[0]);
                }
        }
        else if (!q_strcasecmp (directive->name, "qer_trans"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                        shader->qer_trans = (float) Q_atof (directive->args[0]);
        }
        else if (!q_strcasecmp (directive->name, "q3map_surfacelight"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                        shader->q3map_surfacelight = (float) Q_atof (directive->args[0]);
        }
        else if (!q_strcasecmp (directive->name, "q3map_lightimage"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                {
                        free (shader->q3map_lightimage);
                        shader->q3map_lightimage = Q3Shader_CopyString (directive->args[0]);
                }
        }
}

static void Q3Shader_ApplyStageDirective (q3shader_stage_t *stage, const q3shader_directive_t *directive)
{
        if (!stage || !directive)
                return;

        if (!q_strcasecmp (directive->name, "map"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                {
                        free (stage->map);
                        stage->map = Q3Shader_CopyString (directive->args[0]);
                }
        }
        else if (!q_strcasecmp (directive->name, "clampmap"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                {
                        free (stage->clampmap);
                        stage->clampmap = Q3Shader_CopyString (directive->args[0]);
                }
        }
        else if (!q_strcasecmp (directive->name, "animmap"))
        {
                size_t i;
                if (VEC_SIZE (directive->args) >= 2)
                {
                        stage->anim_frequency = (float) Q_atof (directive->args[0]);
                        Q3Shader_FreeStringVec (&stage->anim_maps);
                        for (i = 1; i < VEC_SIZE (directive->args); ++i)
                        {
                                VEC_GROW ((void **) &stage->anim_maps, sizeof (stage->anim_maps[0]), 1);
                                stage->anim_maps[VEC_HEADER (stage->anim_maps).size++] = Q3Shader_CopyString (directive->args[i]);
                        }
                }
        }
        else if (!q_strcasecmp (directive->name, "blendfunc"))
        {
                Q3Shader_SetBlendFunc (stage, directive);
        }
        else if (!q_strcasecmp (directive->name, "alphafunc"))
        {
                if (VEC_SIZE (directive->args) >= 1)
                {
                        free (stage->alphaFunc);
                        stage->alphaFunc = Q3Shader_JoinArgs (directive, 0);
                }
        }
        else if (!q_strcasecmp (directive->name, "rgbgen"))
        {
                free (stage->rgbGen);
                stage->rgbGen = Q3Shader_JoinArgs (directive, 0);
        }
        else if (!q_strcasecmp (directive->name, "alphagen"))
        {
                free (stage->alphaGen);
                stage->alphaGen = Q3Shader_JoinArgs (directive, 0);
        }
        else if (!q_strcasecmp (directive->name, "tcgen"))
        {
                free (stage->tcGen);
                stage->tcGen = Q3Shader_JoinArgs (directive, 0);
        }
        else if (!q_strcasecmp (directive->name, "tcmod"))
        {
                char *value = Q3Shader_JoinArgs (directive, 0);
                if (value)
                {
                        VEC_GROW ((void **) &stage->tcMods, sizeof (stage->tcMods[0]), 1);
                        stage->tcMods[VEC_HEADER (stage->tcMods).size++] = value;
                }
        }
        else if (!q_strcasecmp (directive->name, "depthwrite"))
        {
                stage->depthwrite = true;
        }
}

static void Q3Shader_FreeDirective (q3shader_directive_t *directive)
{
        size_t i;
        if (!directive || !directive->args)
                return;

        for (i = 0; i < VEC_SIZE (directive->args); ++i)
                free (directive->args[i]);
        VEC_FREE (directive->args);
        directive->args = NULL;
}

static void Q3Shader_FreeStringVec (char ***vec)
{
        size_t i;
        if (!vec || !*vec)
                return;

        for (i = 0; i < VEC_SIZE (*vec); ++i)
                free ((*vec)[i]);
        VEC_FREE (*vec);
        *vec = NULL;
}

static void Q3Shader_FreeStage (q3shader_stage_t *stage)
{
        size_t i;
        if (!stage)
                return;

        if (stage->directives)
        {
                for (i = 0; i < VEC_SIZE (stage->directives); ++i)
                        Q3Shader_FreeDirective (&stage->directives[i]);
                VEC_FREE (stage->directives);
                stage->directives = NULL;
        }

        free (stage->map);
        stage->map = NULL;
        free (stage->clampmap);
        stage->clampmap = NULL;
        free (stage->blendfunc_src);
        stage->blendfunc_src = NULL;
        free (stage->blendfunc_dst);
        stage->blendfunc_dst = NULL;
        free (stage->alphaFunc);
        stage->alphaFunc = NULL;
        free (stage->rgbGen);
        stage->rgbGen = NULL;
        free (stage->alphaGen);
        stage->alphaGen = NULL;
        free (stage->tcGen);
        stage->tcGen = NULL;

        Q3Shader_FreeStringVec (&stage->anim_maps);
        Q3Shader_FreeStringVec (&stage->tcMods);
        stage->anim_frequency = 0.0f;
        stage->depthwrite = false;
}

static void Q3Shader_FreeShader (q3shader_t *shader)
{
        size_t i;
        if (!shader)
                return;

        free (shader->name);
        shader->name = NULL;
        free (shader->source_file);
        shader->source_file = NULL;
        free (shader->qer_editorimage);
        shader->qer_editorimage = NULL;
        free (shader->q3map_lightimage);
        shader->q3map_lightimage = NULL;

        if (shader->directives)
        {
                for (i = 0; i < VEC_SIZE (shader->directives); ++i)
                        Q3Shader_FreeDirective (&shader->directives[i]);
                VEC_FREE (shader->directives);
                shader->directives = NULL;
        }

        if (shader->stages)
        {
                for (i = 0; i < VEC_SIZE (shader->stages); ++i)
                        Q3Shader_FreeStage (&shader->stages[i]);
                VEC_FREE (shader->stages);
                shader->stages = NULL;
        }

        shader->surfaceparms = 0;
        shader->cullmode = Q3CULL_BACK;
        shader->qer_trans = -1.0f;
        shader->q3map_surfacelight = 0.0f;
}

size_t Q3Shader_Count (void)
{
        return VEC_SIZE (q3shader_registry);
}

const q3shader_t *Q3Shader_GetByIndex (size_t index)
{
        if (!q3shader_registry || index >= VEC_SIZE (q3shader_registry))
                return NULL;
        return &q3shader_registry[index];
}

const q3shader_t *Q3Shader_Find (const char *name)
{
        return Q3Shader_FindMutable (name);
}

size_t Q3Shader_DirectiveCount (const q3shader_t *shader)
{
        if (!shader || !shader->directives)
                return 0;
        return VEC_SIZE (shader->directives);
}

const q3shader_directive_t *Q3Shader_GetDirective (const q3shader_t *shader, size_t index)
{
        if (!shader || !shader->directives || index >= VEC_SIZE (shader->directives))
                return NULL;
        return &shader->directives[index];
}

size_t Q3ShaderStage_Count (const q3shader_t *shader)
{
        if (!shader || !shader->stages)
                return 0;
        return VEC_SIZE (shader->stages);
}

const q3shader_stage_t *Q3Shader_GetStage (const q3shader_t *shader, size_t index)
{
        if (!shader || !shader->stages || index >= VEC_SIZE (shader->stages))
                return NULL;
        return &shader->stages[index];
}

size_t Q3ShaderStage_DirectiveCount (const q3shader_stage_t *stage)
{
        if (!stage || !stage->directives)
                return 0;
        return VEC_SIZE (stage->directives);
}

const q3shader_directive_t *Q3ShaderStage_GetDirective (const q3shader_stage_t *stage, size_t index)
{
        if (!stage || !stage->directives || index >= VEC_SIZE (stage->directives))
                return NULL;
        return &stage->directives[index];
}

size_t Q3ShaderDirective_ArgCount (const q3shader_directive_t *directive)
{
        if (!directive || !directive->args)
                return 0;
        return VEC_SIZE (directive->args);
}

const char *Q3ShaderDirective_GetArg (const q3shader_directive_t *directive, size_t index)
{
        if (!directive || !directive->args || index >= VEC_SIZE (directive->args))
                return NULL;
        return directive->args[index];
}

qboolean Q3Shader_HasSurfaceParm (const q3shader_t *shader, q3shader_surfaceparm_t parm)
{
        if (!shader || !parm)
                return false;
        return (shader->surfaceparms & parm) != 0;
}

q3shader_cullmode_t Q3Shader_GetCullMode (const q3shader_t *shader)
{
        return shader ? shader->cullmode : Q3CULL_BACK;
}

float Q3Shader_GetEditorTransparency (const q3shader_t *shader)
{
        return shader ? shader->qer_trans : -1.0f;
}

const char *Q3Shader_GetEditorImage (const q3shader_t *shader)
{
        return shader ? shader->qer_editorimage : NULL;
}

float Q3Shader_GetSurfaceLight (const q3shader_t *shader)
{
        return shader ? shader->q3map_surfacelight : 0.0f;
}

const char *Q3Shader_GetSurfaceLightImage (const q3shader_t *shader)
{
        return shader ? shader->q3map_lightimage : NULL;
}

void Q3Shader_DescribeSurfaceParms (const q3shader_t *shader, char *buffer, size_t bufsize)
{
        size_t i;

        if (!buffer || !bufsize)
                return;

        buffer[0] = '\0';

        if (!shader)
        {
                q_strlcpy (buffer, "<none>", bufsize);
                return;
        }

        for (i = 0; i < Q_COUNTOF (q3shader_surfaceparms); ++i)
        {
                if (shader->surfaceparms & q3shader_surfaceparms[i].flag)
                {
                        if (buffer[0])
                                q_strlcat (buffer, " ", bufsize);
                        q_strlcat (buffer, q3shader_surfaceparms[i].name, bufsize);
                }
        }

        if (!buffer[0])
                q_strlcpy (buffer, "<none>", bufsize);
}

size_t Q3ShaderStage_GetAnimMapCount (const q3shader_stage_t *stage)
{
        if (!stage || !stage->anim_maps)
                return 0;
        return VEC_SIZE (stage->anim_maps);
}

const char *Q3ShaderStage_GetAnimMap (const q3shader_stage_t *stage, size_t index)
{
        if (!stage || !stage->anim_maps || index >= VEC_SIZE (stage->anim_maps))
                return NULL;
        return stage->anim_maps[index];
}

float Q3ShaderStage_GetAnimFrequency (const q3shader_stage_t *stage)
{
        return stage ? stage->anim_frequency : 0.0f;
}

const char *Q3ShaderStage_GetMap (const q3shader_stage_t *stage)
{
        return stage ? stage->map : NULL;
}

const char *Q3ShaderStage_GetClampMap (const q3shader_stage_t *stage)
{
        return stage ? stage->clampmap : NULL;
}

qboolean Q3ShaderStage_GetDepthWrite (const q3shader_stage_t *stage)
{
        return stage ? stage->depthwrite : false;
}

const char *Q3ShaderStage_GetBlendFuncSrc (const q3shader_stage_t *stage)
{
        return stage ? stage->blendfunc_src : NULL;
}

const char *Q3ShaderStage_GetBlendFuncDst (const q3shader_stage_t *stage)
{
        return stage ? stage->blendfunc_dst : NULL;
}

const char *Q3ShaderStage_GetAlphaFunc (const q3shader_stage_t *stage)
{
        return stage ? stage->alphaFunc : NULL;
}

const char *Q3ShaderStage_GetRgbGen (const q3shader_stage_t *stage)
{
        return stage ? stage->rgbGen : NULL;
}

const char *Q3ShaderStage_GetAlphaGen (const q3shader_stage_t *stage)
{
        return stage ? stage->alphaGen : NULL;
}

const char *Q3ShaderStage_GetTcGen (const q3shader_stage_t *stage)
{
        return stage ? stage->tcGen : NULL;
}

size_t Q3ShaderStage_GetTcModCount (const q3shader_stage_t *stage)
{
        if (!stage || !stage->tcMods)
                return 0;
        return VEC_SIZE (stage->tcMods);
}

const char *Q3ShaderStage_GetTcMod (const q3shader_stage_t *stage, size_t index)
{
        if (!stage || !stage->tcMods || index >= VEC_SIZE (stage->tcMods))
                return NULL;
        return stage->tcMods[index];
}

static uint32_t Q3Shader_SurfaceParmFlag (const char *name)
{
        size_t i;
        if (!name)
                return 0;
        for (i = 0; i < Q_COUNTOF (q3shader_surfaceparms); ++i)
        {
                if (!q_strcasecmp (q3shader_surfaceparms[i].name, name))
                        return q3shader_surfaceparms[i].flag;
        }
        return 0;
}
