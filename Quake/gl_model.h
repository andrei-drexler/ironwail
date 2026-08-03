/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef GL_MODEL_H
#define GL_MODEL_H

#include "modelgen.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

typedef enum {
	TEXTYPE_DEFAULT,
	TEXTYPE_CUTOUT,
	TEXTYPE_SKY,
	TEXTYPE_LAVA,
	TEXTYPE_SLIME,
	TEXTYPE_TELE,
	TEXTYPE_WATER,

	TEXTYPE_COUNT,

	TEXTYPE_FIRSTLIQUID = TEXTYPE_LAVA,
	TEXTYPE_LASTLIQUID = TEXTYPE_WATER,
	TEXTYPE_NUMLIQUIDS = TEXTYPE_LASTLIQUID + 1 - TEXTYPE_FIRSTLIQUID,
} textype_t;

#define TEXTYPE_ISLIQUID(x) ((unsigned)((x) - TEXTYPE_FIRSTLIQUID) < (unsigned)TEXTYPE_NUMLIQUIDS)

typedef struct texture_s
{
	char				name[16];
	unsigned			width, height;
	unsigned			shift;		// Q64
	textype_t			type;
	struct gltexture_s	*gltexture; //johnfitz -- pointer to gltexture
	struct gltexture_s	*fullbright; //johnfitz -- fullbright mask texture
	int					anim_total;				// total tenths in sequence ( 0 = no)
	int					anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s	*anim_next;		// in the animation sequence
	struct texture_s	*alternate_anims;	// bmodels in frmae 1 use these
} texture_t;


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
#define SURF_NOTEXTURE		0x100 //johnfitz
#define SURF_DRAWFENCE		0x200
#define SURF_DRAWLAVA		0x400
#define SURF_DRAWSLIME		0x800
#define SURF_DRAWTELE		0x1000
#define SURF_DRAWWATER		0x2000

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned int	v[2];
} medge_t;

typedef struct
{
	float		vecs[2][4];
	int			texnum;
	int			flags;
} mtexinfo_t;

typedef struct glvert_s {
	vec3_t		pos;
	float		st[4];
	float		lmofs;
	unsigned	styles;
} glvert_t;

typedef struct msurface_s
{
	mplane_t	*plane;
	float		mins[3];		// johnfitz -- for frustum culling
	float		maxs[3];		// johnfitz -- for frustum culling
	int			flags;

	int			vbo_firstvert;		// index of this surface's first vert in the VBO
	int			firstedge;			// look up in model->surfedges[], negative numbers
	short		numedges;			// are backwards edges

	short		lightmaptexturenum;
	short		extents[2];
	short		light_s, light_t;	// gl lightmap coordinates

	byte		styles[MAXLIGHTMAPS];
	byte		*samples;			// [numstyles*surfsize]

	int			texturemins[2];
	mtexinfo_t	*texinfo;
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];

	unsigned int		firstsurface;
	unsigned int		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	byte		*compressed_vis;

	int			*firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

//johnfitz -- for clipnodes>32k
typedef struct mclipnode_s
{
	int			planenum;
	int			children[2]; // negative numbers are contents
} mclipnode_t;
//johnfitz

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	mclipnode_t	*clipnodes; //johnfitz -- was dclipnode_t
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s
{
	int					width, height;
	float				up, down, left, right;
	float				smax, tmax; //johnfitz -- image might be padded
	struct gltexture_s	*gltexture;
} mspriteframe_t;

typedef struct
{
	int				numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t	type;
	mspriteframe_t		*frameptr;
} mspriteframedesc_t;

typedef struct
{
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

//-- from RMQEngine
// split out to keep vertex sizes down
typedef struct aliasmesh_s
{
	short st[2];
	unsigned short vertindex;
} aliasmesh_t;

typedef struct meshxyz_s
{
	byte xyz[4];
	signed char normal[4];
} meshxyz_t;

typedef struct meshst_s
{
	float st[2];
} meshst_t;
//--

typedef struct
{
	int					firstpose;
	int					numposes;
	float				interval;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;

#define	MAX_SKINS	32
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;

	//ericw -- used to populate vbo
	int			numverts_vbo;   // number of verts with unique x,y,z,s,t
	intptr_t		meshdesc;       // offset into extradata: numverts_vbo aliasmesh_t
	int			numindexes;
	intptr_t		indexes;        // offset into extradata: numindexes unsigned shorts
	intptr_t		vertexes;       // offset into extradata: numposes*vertsperframe trivertx_t

	intptr_t	vbovertofs;
	intptr_t	vbostofs;
	intptr_t	vboposeofs;
	intptr_t	eboofs;
	//ericw --

	int					numposes;
	intptr_t			nextsurface;		//spike
	int					numbones;			//spike -- for iqm
	intptr_t			boneinfo;			//spike -- for iqm, boneinfo_t[numbones]
	intptr_t			boneposedata;		//spike -- for iqm, bonepose_t[numboneposes*numbones]
	enum
	{
		PV_QUAKE1,		//trivertx_t
		PV_IQM,			//iqmvert_t
		PV_MD3
	} poseverttype;	//spike
	struct gltexture_s	*gltextures[MAX_SKINS][4]; //johnfitz
	struct gltexture_s	*fbtextures[MAX_SKINS][4]; //johnfitz
	int					texels[MAX_SKINS];	// only for player skins
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

static inline aliashdr_t *Mod_NextSurface (aliashdr_t *hdr) { return hdr->nextsurface ? (aliashdr_t*)((byte*)hdr + hdr->nextsurface) : NULL; }

typedef struct
{
	float		xyz[3];
	int8_t		norm[4];
	float		st[2];
	uint8_t		weight[4];
	uint8_t		idx[4];
} iqmvert_t;
typedef struct
{
	float mat[12];
} bonepose_t; //pose data for a single bone.
typedef struct
{
	int parent; //-1 for a root bone
	char name[32];
	bonepose_t inverse;
} boneinfo_t;
typedef struct
{
	uint16_t	xyz[3];
	uint8_t		normal[2]; // spherical coords
} md3pose_t; // Total size is now 8 bytes
#define	MAXALIASVERTS		0x7fff //16-bit index buffer + onseam duplication
#define	MAXALIASVERTS_QS	2000 //johnfitz -- was 1024
#define	MAXALIASFRAMES		1024 //spike -- was 256
#define	MAXALIASTRIS_QS		4096 //ericw -- was 2048
extern	aliashdr_t			*pheader;
extern	const stvert_t		*stverts;
extern	const dtriangle_t	*triangles;
extern	trivertx_t			*poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

typedef enum {mod_brush, mod_alias, mod_sprite, mod_numtypes} modtype_t;

#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail
#define	MF_HOLEY	(1u<<14)		// MarkV/QSS -- make index 255 transparent on mdl's

//johnfitz -- extra flags for rendering
#define	MOD_NOLERP		256		//don't lerp when animating
#define	MOD_NOSHADOW	512		//don't cast a shadow
#define	MOD_FBRIGHTHACK	1024	//when fullbrights are disabled, use a hack to render this model brighter
//johnfitz

//
// Entity sort keys (20 bits)
// 19.....4 | 3....0
// model:16 | anim:4
//
enum
{
	MODSORT_FRAMEBITS			= 4,
	MODSORT_MODELBITS			= 16,
	MODSORT_BITS				= MODSORT_FRAMEBITS + MODSORT_MODELBITS,
	MODSORT_FRAMEMASK			= (1 << MODSORT_FRAMEBITS) - 1,
	MODSORT_MODELMASK			= (1 << MODSORT_MODELBITS) - 1,
	MODSORT_MASK				= (1 << MODSORT_BITS) - 1,
	MODSORT_ALIAS_ALPHATEST		= 1 << (MODSORT_BITS - 1),
};

typedef struct qmodel_s
{
	char		name[MAX_QPATH];
	unsigned int	path_id;		// path id of the game directory
							// that this model came from
	qboolean	needload;		// bmodels and sprites don't cache normally

	modtype_t	type;
	int			numframes;
	synctype_t	synctype;

	int			flags;
	uint32_t	sortkey;

//
// volume occupied by the model graphics
//
	vec3_t		mins, maxs;
	vec3_t		ymins, ymaxs; //johnfitz -- bounds for entities with nonzero yaw
	vec3_t		rmins, rmaxs; //johnfitz -- bounds for entities with nonzero pitch or roll
	//johnfitz -- removed float radius;

//
// solid volume for clipping
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	dmodel_t	*submodels;

	int			numplanes;
	mplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	mclipnode_t	*clipnodes; //johnfitz -- was dclipnode_t

	int			nummarksurfaces;
	int			*marksurfaces;

	hull_t		hulls[MAX_MAP_HULLS];

	int			numtextures;
	texture_t	**textures;
	int			firstcmd; // index of first indirect draw command
	int			texofs[TEXTYPE_COUNT + 1]; // index of first texture of the given type in the usedtextures array
	int			*usedtextures;

	byte		*visdata;
	byte		*lightdata;
	char		*entities;

	qboolean	litfile;
	qboolean	viswarn; // for Mod_DecompressVis()

	int			bspversion;
	int			contentstransparent;	//spike -- added this so we can disable glitchy wateralpha where its not supported.
	qboolean	haslitwater;

//
// alias model
//

	GLuint		meshvbo;
	GLuint		meshindexesvbo;

//
// additional model data
//
	cache_user_t	cache;		// only access through Mod_Extradata

} qmodel_t;

//
// MD3 structs
//
#define MD3_IDENT			('I' + ('D' << 8) + ('P' << 16) + ('3' << 24))
#define MD3_VERSION			15
#define MD3_XYZ_SCALE		(1.0/64.0)
	typedef struct { int ident, version; char name[MAX_QPATH]; int flags; int numFrames, numTags, numSurfaces, numSkins; int ofsFrames, ofsTags, ofsSurfaces, ofsEnd; } md3Header_t;
	typedef struct { int ident; char name[MAX_QPATH]; int flags; int numFrames, numShaders, numVerts, numTriangles; int ofsTriangles, ofsShaders, ofsSt, ofsXyzNormal, ofsEnd; } md3Surface_t;
	typedef struct { int indexes[3]; } md3Triangle_t;
	typedef struct { float st[2]; } md3TexCoord_t;
	typedef struct { short xyz[3]; unsigned char normal[2]; } md3Vertex_t;

//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
void	Mod_ResetAll (void); // for gamedir changes (Host_Game_f)
qmodel_t *Mod_ForName (const char *name, qboolean crash);
void	*Mod_Extradata (qmodel_t *mod);	// handles caching
void	Mod_TouchModel (const char *name);

#define VIS_ALIGN			16						// vis buffer size alignment (in bytes)
#define VIS_ALIGN_MASK		(VIS_ALIGN - 1)			// alignment - 1, to simplify alignment code

mleaf_t *Mod_PointInLeaf (vec3_t p, qmodel_t *model);
byte	*Mod_LeafPVS (mleaf_t *leaf, qmodel_t *model);
byte	*Mod_NoVisPVS (qmodel_t *model);

void Mod_SetExtraFlags (qmodel_t *mod);
size_t Mod_SanitizeMapDescription (char *dst, size_t dstsize, const char *src);
qboolean Mod_LoadMapDescription (char *desc, size_t maxchars, const char *map);

#endif	/* GL_MODEL_H */
