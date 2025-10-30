#ifndef Q3SHADER_H
#define Q3SHADER_H

#include "common.h"

#define Q3SHADER_MAX_NAME        256
#define Q3SHADER_MAX_DIRECTIVE   64

typedef enum
{
        Q3SURF_NONE               = 0u,
        Q3SURF_NODRAW             = 1u << 0,
        Q3SURF_NONSOLID           = 1u << 1,
        Q3SURF_NOMARKS            = 1u << 2,
        Q3SURF_NOLIGHTMAP         = 1u << 3,
        Q3SURF_SKY                = 1u << 4,
        Q3SURF_LAVA               = 1u << 5,
        Q3SURF_SLIME              = 1u << 6,
        Q3SURF_WATER              = 1u << 7,
        Q3SURF_FOG                = 1u << 8,
        Q3SURF_HINT               = 1u << 9,
        Q3SURF_SKIP               = 1u << 10,
        Q3SURF_DETAIL             = 1u << 11,
        Q3SURF_STRUCTURAL         = 1u << 12,
        Q3SURF_TRANSLUCENT        = 1u << 13,
        Q3SURF_SLICK              = 1u << 14,
        Q3SURF_NODAMAGE           = 1u << 15,
        Q3SURF_NOIMPACT           = 1u << 16,
        Q3SURF_NODROP             = 1u << 17,
        Q3SURF_PLAYERCLIP         = 1u << 18,
        Q3SURF_MONSTERCLIP        = 1u << 19,
        Q3SURF_BOTCLIP            = 1u << 20,
        Q3SURF_ORIGIN             = 1u << 21,
        Q3SURF_LIGHTFILTER        = 1u << 22,
        Q3SURF_ALPHASHADOW        = 1u << 23,
        Q3SURF_AREAPORTAL         = 1u << 24,
        Q3SURF_CLUSTERPORTAL      = 1u << 25,
        Q3SURF_DONOTENTER         = 1u << 26,
        Q3SURF_DUST               = 1u << 27,
        Q3SURF_LADDER             = 1u << 28,
        Q3SURF_NOSTEPS            = 1u << 29,
        Q3SURF_NODLIGHT           = 1u << 30,
} q3shader_surfaceparm_t;

typedef enum
{
        Q3CULL_BACK,
        Q3CULL_FRONT,
        Q3CULL_TWOSIDED,
} q3shader_cullmode_t;

typedef struct q3shader_directive_s
{
        char    name[Q3SHADER_MAX_DIRECTIVE];
        char  **args;
} q3shader_directive_t;

typedef struct q3shader_stage_s
{
        q3shader_directive_t     *directives;
        char                     *map;
        char                     *clampmap;
        float                     anim_frequency;
        char                    **anim_maps;
        qboolean                  depthwrite;
        char                     *blendfunc_src;
        char                     *blendfunc_dst;
        char                     *alphaFunc;
        char                     *rgbGen;
        char                     *alphaGen;
        char                     *tcGen;
        char                    **tcMods;
} q3shader_stage_t;

typedef struct q3shader_s
{
        char                     *name;
        char                     *source_file;
        unsigned int              source_path_id;
        q3shader_directive_t     *directives;
        q3shader_stage_t         *stages;
        uint32_t                  surfaceparms;
        q3shader_cullmode_t       cullmode;
        float                     qer_trans;
        char                     *qer_editorimage;
        float                     q3map_surfacelight;
        char                     *q3map_lightimage;
} q3shader_t;

void                    Q3Shader_Init (void);
void                    Q3Shader_Shutdown (void);
void                    Q3Shader_Clear (void);
int                     Q3Shader_LoadFile (const char *path);
int                     Q3Shader_LoadAll (int *files_loaded, int *files_failed);

size_t                  Q3Shader_Count (void);
const q3shader_t       *Q3Shader_GetByIndex (size_t index);
const q3shader_t       *Q3Shader_Find (const char *name);

size_t                  Q3Shader_DirectiveCount (const q3shader_t *shader);
const q3shader_directive_t *Q3Shader_GetDirective (const q3shader_t *shader, size_t index);

size_t                  Q3ShaderStage_Count (const q3shader_t *shader);
const q3shader_stage_t *Q3Shader_GetStage (const q3shader_t *shader, size_t index);

size_t                  Q3ShaderStage_DirectiveCount (const q3shader_stage_t *stage);
const q3shader_directive_t *Q3ShaderStage_GetDirective (const q3shader_stage_t *stage, size_t index);

size_t                  Q3ShaderDirective_ArgCount (const q3shader_directive_t *directive);
const char             *Q3ShaderDirective_GetArg (const q3shader_directive_t *directive, size_t index);

qboolean                Q3Shader_HasSurfaceParm (const q3shader_t *shader, q3shader_surfaceparm_t parm);
q3shader_cullmode_t     Q3Shader_GetCullMode (const q3shader_t *shader);
float                   Q3Shader_GetEditorTransparency (const q3shader_t *shader);
const char             *Q3Shader_GetEditorImage (const q3shader_t *shader);
float                   Q3Shader_GetSurfaceLight (const q3shader_t *shader);
const char             *Q3Shader_GetSurfaceLightImage (const q3shader_t *shader);
void                    Q3Shader_DescribeSurfaceParms (const q3shader_t *shader, char *buffer, size_t bufsize);

size_t                  Q3ShaderStage_GetAnimMapCount (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetAnimMap (const q3shader_stage_t *stage, size_t index);
float                   Q3ShaderStage_GetAnimFrequency (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetMap (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetClampMap (const q3shader_stage_t *stage);
qboolean                Q3ShaderStage_GetDepthWrite (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetBlendFuncSrc (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetBlendFuncDst (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetAlphaFunc (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetRgbGen (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetAlphaGen (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetTcGen (const q3shader_stage_t *stage);
size_t                  Q3ShaderStage_GetTcModCount (const q3shader_stage_t *stage);
const char             *Q3ShaderStage_GetTcMod (const q3shader_stage_t *stage, size_t index);

#endif /* Q3SHADER_H */
