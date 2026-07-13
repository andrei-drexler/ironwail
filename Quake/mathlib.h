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

#define VectorOps(n,op,a,b,dst) \
MULTI_LINE_MACRO_BEGIN \
_Pragma("omp simd") \
for(size_t i = 0; i < n; i++) \
	(dst)[i] = (__typeof__((dst)[0]))(a)[i] op (b)[i]; \
MULTI_LINE_MACRO_END

#define VectorNCopy(n,src,dst) \
MULTI_LINE_MACRO_BEGIN \
_Pragma("omp simd") \
for(size_t i = 0; i < n; i++) \
	(dst)[i] = (__typeof__((src)[0]))(src)[i]; \
MULTI_LINE_MACRO_END
#define VectorCopy(src,dst) VectorNCopy(3,src,dst)

#define VectorSet(v,...) \
MULTI_LINE_MACRO_BEGIN \
_Pragma("omp simd") \
for(size_t i = 0; i < countargs(__VA_ARGS__); i++) \
	(v)[i] = (__typeof__((v)[0]))(__typeof__(__VA_ARGS__)[]){ __VA_ARGS__ }[i]; \
MULTI_LINE_MACRO_END

#define VectorAdd(a,b,dst) VectorOps(3,+,a,b,dst)
#define VectorSub(a,b,dst) VectorOps(3,-,a,b,dst)
#define VectorMul(a,b,dst) VectorOps(3,*,a,b,dst)
#define VectorDiv(a,b,dst) VectorOps(3,/,a,b,dst)

FUNC_INLINE vecf_t dotf(size_t n, const vecf_t a[], const vecf_t b[])
{
        vecf_t dst = (vecf_t)0;
        _Pragma("omp simd reduction(+:dst)")
        for(size_t i = 0; i < n; i++)
                dst += a[i] * b[i];
        return dst;
}
FUNC_INLINE vecd_t dotftod(size_t n, const vecf_t a[], const vecf_t b[])
{
        vecd_t dst = (vecd_t)0;
        _Pragma("omp simd reduction(+:dst)")
        for(size_t i = 0; i < n; i++)
                dst += (vecd_t)a[i] * b[i];
        return dst;
}
FUNC_INLINE vecd_t dotd(size_t n, const vecd_t a[], const vecd_t b[])
{
        vecd_t dst = (vecd_t)0;
        _Pragma("omp simd reduction(+:dst)")
        for(size_t i = 0; i < n; i++)
                dst += a[i] * b[i];
        return dst;
}
FUNC_INLINE vecf_t dotdtof(size_t n, const vecd_t a[], const vecd_t b[])
{
        vecf_t dst = (vecf_t)0;
        _Pragma("omp simd reduction(+:dst)")
        for(size_t i = 0; i < n; i++)
                dst += (vecf_t)a[i] * b[i];
        return dst;
}

#define cross3(a,b,dst)                 ((dst)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],\
                                         (dst)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],\
                                         (dst)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])

#define CrossProduct(a,b,cross)         cross3(a,b,cross)

#define DotProduct(x,y)                 dotf(3,(const vecf_t*)(x),(const vecf_t*)(y))
#define DotProduct4(x,y)                dotf(4,(const vecf_t*)(x),(const vecf_t*)(y))
#define DoublePrecisionDotProduct(x,y)  dotftod(3,(const vecf_t*)(x),(const vecf_t*)(y))
#define DoublePrecisionDotProduct4(x,y) dotftod(4,(const vecf_t*)(x),(const vecf_t*)(y))
#define VectorLengthSquared(v)          DotProduct(v,v)

//johnfitz -- courtesy of lordhavoc
// QuakeSpasm: To avoid strict aliasing violations, use a float/int union instead of type punning.
#define VectorNormalizeFast(_v) \
MULTI_LINE_MACRO_BEGIN \
	union { float f; int i; } _y, _number; \
	_number.f = DotProduct((_v),(_v)); \
	if (_number.f != 0.0) \
	{ \
		_y.i = 0x5f3759df - (_number.i >> 1); \
		_y.f = _y.f * (1.5f - (_number.f * 0.5f * _y.f * _y.f)); \
		VectorScale((_v), _y.f, (_v)); \
	} \
MULTI_LINE_MACRO_END

#define QuatMul(qa,qb,qdst) \
MULTI_LINE_MACRO_BEGIN \
	VectorSet(qdst,                                                                   \
	((qa)[3] * (qb)[0] + (qa)[0] * (qb)[3] + (qa)[1] * (qb)[2] - (qa)[2] * (qb)[1]),  \
	((qa)[3] * (qb)[1] + (qa)[1] * (qb)[3] + (qa)[2] * (qb)[0] - (qa)[0] * (qb)[2]),  \
	((qa)[3] * (qb)[2] + (qa)[2] * (qb)[3] + (qa)[0] * (qb)[1] - (qa)[1] * (qb)[0]),  \
	((qa)[3] * (qb)[3] - (qa)[0] * (qb)[0] - (qa)[1] * (qb)[1] - (qa)[2] * (qb)[2])); \
MULTI_LINE_MACRO_END

#define QuatMul2(qadst,qb) \
MULTI_LINE_MACRO_BEGIN     \
quat_t tmp;                \
VectorNCopy(4,qadst,tmp);  \
QuatMul(tmp,qb,qadst);     \
MULTI_LINE_MACRO_END

void VectorAngles (const vec3_t forward, vec3_t angles); //johnfitz

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);
void VectorLerp (const vec3_t veca, const vec3_t vecb, float frac, vec3_t dst);

int VectorCompare (const vec3_t v1, const vec3_t v2);
vec_t VectorLength (const vec3_t v);
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

