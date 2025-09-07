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
// gl_mesh.c: triangle model functions

#include "quakedef.h"


/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/

/*
================
GL_MakeAliasModelDisplayLists

Saves data needed to build the VBO for this model on the hunk. Afterwards this
is copied to Mod_Extradata.

Original code by MH from RMQEngine
================
*/
void GL_MakeAliasModelDisplayLists (qmodel_t *aliasmodel, aliashdr_t *paliashdr)
{
	int i, j;
	int mark;
	trivertx_t *verts;
	unsigned short *indexes;
	unsigned short *remap;
	aliasmesh_t *desc;

	// first, copy the verts onto the hunk
	verts = (trivertx_t *) Hunk_AllocNoFill (paliashdr->numposes * paliashdr->numverts * sizeof(trivertx_t));
	paliashdr->vertexes = (byte *)verts - (byte *)paliashdr;
	for (i=0 ; i<paliashdr->numposes ; i++)
		for (j=0 ; j<paliashdr->numverts ; j++)
			verts[i*paliashdr->numverts + j] = poseverts[i][j];

	// there can never be more than this number of verts and we just put them all on the hunk
	// (each vertex can be used twice, once with the original UVs and once with the seam adjustment)
	desc = (aliasmesh_t *) Hunk_Alloc (sizeof (aliasmesh_t) * pheader->numverts * 2);

	// there will always be this number of indexes
	indexes = (unsigned short *) Hunk_Alloc (sizeof (unsigned short) * pheader->numtris * 3);

	pheader->indexes = (intptr_t) indexes - (intptr_t) pheader;
	pheader->meshdesc = (intptr_t) desc - (intptr_t) pheader;
	pheader->numindexes = 0;
	pheader->numverts_vbo = 0;

	mark = Hunk_LowMark ();

	// each pair of elements in the remap array corresponds to one source vertex
	// each value is the final index + 1, or 0 if the corresponding vertex hasn't been emitted yet
	remap = (unsigned short *) Hunk_Alloc (paliashdr->numverts * 2 * sizeof (remap[0]));

	for (i = 0; i < pheader->numtris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			// index into hdr->vertexes
			unsigned short vertindex = triangles[i].vertindex[j];

			// index into remap table
			int v = vertindex * 2;

			// check for back side
			if (!triangles[i].facesfront && stverts[vertindex].onseam)
				v++;

			// emit new vertex if it doesn't already exist
			if (!remap[v])
			{
				// basic s/t coords
				int s = stverts[vertindex].s;
				int t = stverts[vertindex].t;

				// check for back side and adjust texcoord s
				if (v & 1)
					s += paliashdr->skinwidth / 2;

				desc[pheader->numverts_vbo].vertindex = vertindex;
				desc[pheader->numverts_vbo].st[0] = s;
				desc[pheader->numverts_vbo].st[1] = t;

				remap[v] = ++pheader->numverts_vbo;
			}

			// emit index
			indexes[pheader->numindexes++] = remap[v] - 1;
		}
	}

	// free temporary data
	Hunk_FreeToLowMark (mark);

	// upload immediately
	GLMesh_LoadVertexBuffer (aliasmodel, pheader);
}

/*
================
GLMesh_LoadVertexBuffer

Upload the given alias model's mesh to a VBO

Original code by MH from RMQEngine
================
*/
void GLMesh_LoadVertexBuffer (qmodel_t *m, aliashdr_t *mainhdr)
{
	int totalvbosize = 0;
	int animsize = 0;
	const aliasmesh_t *desc;
	const trivertx_t *trivertexes;
	byte *ebodata;
	byte *vbodata;
	int f;
	aliashdr_t *hdr;
	unsigned int numindexes, numverts;
	intptr_t stofs;
	intptr_t vertofs;
	intptr_t poseofs;

	if (isDedicated)
		return;

	//count how much space we're going to need.
	for(hdr = mainhdr, numverts = 0, numindexes = 0; ; )
	{
		switch(hdr->poseverttype)
		{
		case PV_QUAKE1:
			totalvbosize += (hdr->numposes * hdr->numverts_vbo * sizeof (meshxyz_t)); // ericw -- what RMQEngine called nummeshframes is called numposes in QuakeSpasm
			break;
		case PV_IQM:
			totalvbosize += (hdr->numposes * hdr->numverts_vbo * sizeof (iqmvert_t));
			animsize += hdr->numboneposes * hdr->numbones * sizeof (bonepose_t);
			break;
		default:
			Sys_Error ("Bad vert type %i for %s", hdr->poseverttype, m->name);
			break;
		}

		numverts += hdr->numverts_vbo;
		numindexes += hdr->numindexes;

		if (hdr->nextsurface)
			hdr = (aliashdr_t*)((byte*)hdr + hdr->nextsurface);
		else
			break;
	}
	hdr = NULL;

	vertofs = 0;
	totalvbosize = (totalvbosize + ssbo_align) & ~ssbo_align;	//align it.

	stofs = totalvbosize;
	if (mainhdr->poseverttype == PV_QUAKE1)
		totalvbosize += (numverts * sizeof (meshst_t));
	totalvbosize = (totalvbosize + ssbo_align) & ~ssbo_align;	//align it.

	poseofs = totalvbosize;
	totalvbosize += animsize;
	totalvbosize = (totalvbosize + ssbo_align) & ~ssbo_align;	//align it.

	if (!totalvbosize) return;
	if (!numindexes) return;

	//create an elements buffer
	ebodata = (byte *) malloc(numindexes * sizeof(unsigned short));
	if (!ebodata)
		return;	//fatal

	// create the vertex buffer (empty)
	vbodata = (byte *) malloc(totalvbosize);
	if (!vbodata)
	{	//fatal
		free(ebodata);
		return;
	}
	memset(vbodata, 0, totalvbosize);

	numindexes = 0;

	for(hdr = mainhdr, numverts = 0, numindexes = 0; ; )
	{
		// grab the pointers to data in the extradata
		desc = (aliasmesh_t *) ((byte *) hdr + hdr->meshdesc);
		trivertexes = (const trivertx_t *) ((byte *)hdr + hdr->vertexes);

		//submit the index data.
		hdr->eboofs = numindexes * sizeof (unsigned short);
		numindexes += hdr->numindexes;
		memcpy(ebodata + hdr->eboofs, (short *) ((byte *) hdr + hdr->indexes), hdr->numindexes * sizeof (unsigned short));

		hdr->vbovertofs = vertofs;

		// fill in the vertices at the start of the buffer
		switch(hdr->poseverttype)
		{
		case PV_QUAKE1:
			for (f = 0; f < hdr->numposes; f++) // ericw -- what RMQEngine called nummeshframes is called numposes in QuakeSpasm
			{
				int v;
				meshxyz_t *xyz = (meshxyz_t *) (vbodata + vertofs);
				const trivertx_t *tv = (const trivertx_t*)trivertexes + (hdr->numverts * f);
				vertofs += hdr->numverts_vbo * sizeof (*xyz);

				for (v = 0; v < hdr->numverts_vbo; v++)
				{
					trivertx_t trivert = tv[desc[v].vertindex];

					xyz[v].xyz[0] = trivert.v[0];
					xyz[v].xyz[1] = trivert.v[1];
					xyz[v].xyz[2] = trivert.v[2];
					xyz[v].xyz[3] = 1;	// need w 1 for 4 byte vertex compression

					// map the normal coordinates in [-1..1] to [-127..127] and store in an unsigned char.
					// this introduces some error (less than 0.004), but the normals were very coarse
					// to begin with
					xyz[v].normal[0] = 127 * r_avertexnormals[trivert.lightnormalindex][0];
					xyz[v].normal[1] = 127 * r_avertexnormals[trivert.lightnormalindex][1];
					xyz[v].normal[2] = 127 * r_avertexnormals[trivert.lightnormalindex][2];
					xyz[v].normal[3] = 0;	// unused; for 4-byte alignment
				}
			}
			break;
		case PV_IQM:
			for (f = 0; f < hdr->numposes; f++) // ericw -- what RMQEngine called nummeshframes is called numposes in QuakeSpasm
			{
				int v;
				iqmvert_t *xyz = (iqmvert_t *) (vbodata + vertofs);
				const iqmvert_t *tv = (const iqmvert_t*)trivertexes + (hdr->numverts_vbo * f);
				vertofs += hdr->numverts_vbo * sizeof (*xyz);

				for (v = 0; v < hdr->numverts_vbo; v++, tv++)
					xyz[v] = *tv;
			}

			// copy bone poses
			hdr->vboposeofs = poseofs;
			memcpy (vbodata + hdr->vboposeofs, (byte *) hdr + hdr->boneposedata, hdr->numboneposes * hdr->numbones * sizeof (bonepose_t));
			poseofs += hdr->numboneposes * hdr->numbones * sizeof (bonepose_t);

			break;
		}

		// fill in the ST coords at the end of the buffer
		if (hdr->poseverttype == PV_QUAKE1)
		{
			meshst_t *st;
			float hscale, vscale;

			//johnfitz -- padded skins
			hscale = 1.0f / (float)TexMgr_PadConditional(hdr->skinwidth);
			vscale = 1.0f / (float)TexMgr_PadConditional(hdr->skinheight);
			//johnfitz

			hdr->vbostofs = stofs; 
			st = (meshst_t *) (vbodata + stofs);
			stofs += hdr->numverts_vbo*sizeof(*st);
			for (f = 0; f < hdr->numverts_vbo; f++)
			{
				st[f].st[0] = hscale * ((float) desc[f].st[0] + 0.5f);
				st[f].st[1] = vscale * ((float) desc[f].st[1] + 0.5f);
			}
		}

		if (hdr->nextsurface)
			hdr = (aliashdr_t*)((byte*)hdr + hdr->nextsurface);
		else
			break;
	}
	hdr = NULL;

	// upload indexes buffer
	GL_DeleteBuffer (m->meshindexesvbo);
	m->meshindexesvbo = GL_CreateBuffer (GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, va ("%s indices", m->name), numindexes * sizeof (unsigned short), ebodata);

	// upload vertexes buffer
	GL_DeleteBuffer (m->meshvbo);
	m->meshvbo = GL_CreateBuffer (GL_ARRAY_BUFFER, GL_STATIC_DRAW, va ("%s vertices", m->name), totalvbosize, vbodata);

	free (vbodata);
	free (ebodata);
}

/*
================
GLMesh_LoadVertexBuffers

Loop over all precached alias models, and upload each one to a VBO.
================
*/
void GLMesh_LoadVertexBuffers (void)
{
	int j;
	qmodel_t *m;
	aliashdr_t *hdr;

	for (j = 1; j < MAX_MODELS; j++)
	{
		if (!(m = cl.model_precache[j])) break;
		if (m->type != mod_alias) continue;

		hdr = (aliashdr_t *) Mod_Extradata (m);
		
		GLMesh_LoadVertexBuffer (m, hdr);
	}
}

/*
================
GLMesh_DeleteVertexBuffers

Delete VBOs for all loaded alias models
================
*/
void GLMesh_DeleteVertexBuffers (void)
{
	int j;
	qmodel_t *m;
	
	if (isDedicated)
		return;

	for (j = 1; j < MAX_MODELS; j++)
	{
		if (!(m = cl.model_precache[j])) break;
		if (m->type != mod_alias) continue;
		
		GL_DeleteBuffersFunc (1, &m->meshvbo);
		m->meshvbo = 0;

		GL_DeleteBuffersFunc (1, &m->meshindexesvbo);
		m->meshindexesvbo = 0;
	}
	
	GL_ClearBufferBindings ();
}
