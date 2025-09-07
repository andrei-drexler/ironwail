/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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

#ifndef __MATHLIB_H
#define __MATHLIB_H

// mathlib.h

#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define M_PI_DIV_180	(M_PI / 180.0) //johnfitz
#define DEG2RAD(a)		((a) * M_PI_DIV_180)
#define RAD2DEG(a)		((a) * (180.0 / M_PI))

struct mplane_s;

extern vec3_t vec3_origin;
extern vec4_t vec4_origin;

#define	nanmask		(255 << 23)	/* 7F800000 */
#if 0	/* macro is violating strict aliasing rules */
#define	IS_NAN(x)	(((*(int *) (char *) &x) & nanmask) == nanmask)
#else
static inline int IS_NAN (float x) {
	union { float f; int i; } num;
	num.f = x;
	return ((num.i & nanmask) == nanmask);
}
#endif

#define Q_rint(x) ((x) > 0 ? (int)((x) + 0.5) : (int)((x) - 0.5)) //johnfitz -- from joequake

#define DotProduct(x,y)					((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define DoublePrecisionDotProduct(x,y)	((double)(x)[0]*(y)[0]+(double)(x)[1]*(y)[1]+(double)(x)[2]*(y)[2])
#define VectorSubtract(a,b,dst)			do {(dst)[0]=(a)[0]-(b)[0];(dst)[1]=(a)[1]-(b)[1];(dst)[2]=(a)[2]-(b)[2];} while (0)
#define VectorAdd(a,b,dst)				do {(dst)[0]=(a)[0]+(b)[0];(dst)[1]=(a)[1]+(b)[1];(dst)[2]=(a)[2]+(b)[2];} while (0)
#define VectorCopy(src,dst)				do {(dst)[0]=(src)[0];(dst)[1]=(src)[1];(dst)[2]=(src)[2];} while (0)
#define VectorSet(v,x,y,z)				do {(v)[0]=(x);(v)[1]=(y);(v)[2]=(z);} while (0)
#define VectorLengthSquared(v)			DotProduct(v,v)

//johnfitz -- courtesy of lordhavoc
// QuakeSpasm: To avoid strict aliasing violations, use a float/int union instead of type punning.
#define VectorNormalizeFast(_v)\
do\
{\
	union { float f; int i; } _y, _number;\
	_number.f = DotProduct((_v), (_v));\
	if (_number.f != 0.0)\
	{\
		_y.i = 0x5f3759df - (_number.i >> 1);\
		_y.f = _y.f * (1.5f - (_number.f * 0.5f * _y.f * _y.f));\
		VectorScale((_v), _y.f, (_v));\
	}\
} while (0)

void VectorAngles (const vec3_t forward, vec3_t angles); //johnfitz

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);
void VectorLerp (const vec3_t veca, const vec3_t vecb, float frac, vec3_t dst);

vec_t _DotProduct (const vec3_t v1, const vec3_t v2);
void _VectorSubtract (const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorAdd (const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorCopy (const vec3_t in, vec3_t out);

int VectorCompare (const vec3_t v1, const vec3_t v2);
vec_t VectorLength (const vec3_t v);
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
float VectorNormalize (vec3_t v);		// returns vector length
float DistanceSquared (const vec3_t a, const vec3_t b);
float Distance (const vec3_t a, const vec3_t b);
void VectorInverse (vec3_t v);
void VectorScale (const vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);
int Q_nextPow2(int val);

float GetFraction (float val, float minval, float maxval);
float GetClampedFraction (float val, float minval, float maxval);

float Log2f (float val);
float Exp2f (float val);
float GetLogFraction (float val, float minval, float maxval);
float GetClampedLogFraction (float val, float minval, float maxval);
float LogLerp (float minval, float maxval, float t);

float EaseInOut (float t);

uint32_t Interleave0 (uint16_t x);
uint32_t Interleave (uint16_t even, uint16_t odd);
uint16_t DeinterleaveEven (uint32_t x);
void DecodeMortonIndex (uint16_t index, int *x, int *y);

void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]);

void FloorDivMod (double numer, double denom, int *quotient,
		int *rem);
fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor (int i1, int i2);

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct mplane_s *plane);
float	anglemod(float a);

float NormalizeAngle (float degrees);
float AngleDifference (float dega, float degb);
float LerpAngle (float degfrom, float degto, float frac);

void MatrixMultiply(float left[16], float right[16]);
void RotationMatrix(float matrix[16], float angle, int axis);
void TranslationMatrix(float matrix[16], float x, float y, float z);
void ScaleMatrix(float matrix[16], float x, float y, float z);
void IdentityMatrix(float matrix[16]);
void ApplyScale(float matrix[16], float x, float y, float z);
void ApplyTranslation(float matrix[16], float x, float y, float z);
void MatrixTranspose4x3(const float src[16], float dst[12]);
void ProjectVector(const vec3_t src, const float matrix[16], vec3_t dst);

qboolean RayVsBox (const vec3_t org, const vec3_t rcpdelta, const vec3_t mins, const vec3_t maxs, float *frac);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

/*==========================================================================*/

/* SIMD */
#if (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))) || (defined(__GNUC__) && defined(__SSE__) && defined(__SSE2__))
	#define USE_SIMD
	#define USE_SSE2
	#include <emmintrin.h>
#endif

/*==========================================================================*/

#endif	/* __MATHLIB_H */

