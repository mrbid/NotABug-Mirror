/*
    December 2022

    Requires vec.h: https://gist.github.com/mrbid/77a92019e1ab8b86109bf103166bd04e

    Credits:
    Aaftab Munshi, Dan Ginsburg, Dave Shreiner, James William Fletcher, Intel, Gabriel Cramer, Test_User
*/

#ifndef MAT_H
#define MAT_H

#include "vec.h"

typedef struct
{
    float m[4][4];
} mat;

void mIdent(mat *m);
void mCopy(mat *restrict r, const mat *restrict v);
void mMul(mat *restrict r, const mat *restrict a, const mat *restrict b);
void mMulP(vec *restrict r, const mat *restrict a, const float x, const float y, const float z);
void mMulV(vec *restrict r, const mat *restrict a, const vec v);
void mScale(mat *r, const float x, const float y, const float z);
void mTranslate(mat *r, const float x, const float y, const float z);
void mRotate(mat *r, const float radians, float x, float y, float z); // rotate axis (rotate X to move Y up and down)
void mRotX(mat *r, const float radians); // rotate on axis
void mRotY(mat *r, const float radians); // rotate on axis
void mRotZ(mat *r, const float radians); // rotate on axis
void mAngleAxisRotate(mat *r, const mat view, const float xrot, const float yrot, const float zrot); // gimbal free rotations
void mFrustum(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ);
void mPerspective(mat *r, const float fovy, const float aspect, const float nearZ, const float farZ);
void mOrtho(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ);
void mLookAt(mat *r, const vec origin, const vec unit_dir);
void mInvert(float *restrict dst, const float *restrict mat);
void mTranspose(mat *restrict r, const mat *restrict m);
void mSetViewDir(mat *r, const vec dir_norm, const vec up_norm);
void mGetViewDir(vec *r, const mat matrix);
void mGetViewX(vec *r, const mat matrix);
void mGetViewY(vec *r, const mat matrix);
void mGetViewZ(vec *r, const mat matrix);
void mSetDir(mat *r, const vec dir_norm, const vec up_norm);
void mGetDirX(vec *r, const mat matrix);
void mGetDirY(vec *r, const mat matrix);
void mGetDirZ(vec *r, const mat matrix);
void mGetPos(vec *r, const mat matrix);
void mSetPos(mat *r, const vec pos);
void mDump(const mat matrix);

//

void mIdent(mat *m)
{
    memset(m, 0x0, sizeof(mat));
    m->m[0][0] = 1.0f;
    m->m[1][1] = 1.0f;
    m->m[2][2] = 1.0f;
    m->m[3][3] = 1.0f;
}

void mCopy(mat *restrict r, const mat *restrict v)
{
    memcpy(r, v, sizeof(mat));
}

void mMul(mat *restrict r, const mat *restrict a, const mat *restrict b)
{
    mat tmp;
    for(int i = 0; i < 4; i++)
    {
        tmp.m[i][0] =	(a->m[i][0] * b->m[0][0]) +
                        (a->m[i][1] * b->m[1][0]) +
                        (a->m[i][2] * b->m[2][0]) +
                        (a->m[i][3] * b->m[3][0]) ;

        tmp.m[i][1] =	(a->m[i][0] * b->m[0][1]) + 
                        (a->m[i][1] * b->m[1][1]) +
                        (a->m[i][2] * b->m[2][1]) +
                        (a->m[i][3] * b->m[3][1]) ;

        tmp.m[i][2] =	(a->m[i][0] * b->m[0][2]) + 
                        (a->m[i][1] * b->m[1][2]) +
                        (a->m[i][2] * b->m[2][2]) +
                        (a->m[i][3] * b->m[3][2]) ;

        tmp.m[i][3] =	(a->m[i][0] * b->m[0][3]) + 
                        (a->m[i][1] * b->m[1][3]) +
                        (a->m[i][2] * b->m[2][3]) +
                        (a->m[i][3] * b->m[3][3]) ;
    }
    memcpy(r, &tmp, sizeof(mat));
}

void mMulP(vec *restrict r, const mat *restrict a, const float x, const float y, const float z)
{
    r->x =  (a->m[0][0] * x) +
            (a->m[0][1] * x) +
            (a->m[0][2] * x) +
            (a->m[0][3] * x) ;

    r->y =  (a->m[1][0] * y) +
            (a->m[1][1] * y) +
            (a->m[1][2] * y) +
            (a->m[1][3] * y) ;

    r->z =  (a->m[2][0] * z) +
            (a->m[2][1] * z) +
            (a->m[2][2] * z) +
            (a->m[2][3] * z) ;
}

void mMulV(vec *restrict r, const mat *restrict a, const vec v)
{
    r->x =  (a->m[0][0] * v.x) +
            (a->m[0][1] * v.x) +
            (a->m[0][2] * v.x) +
            (a->m[0][3] * v.x) ;

    r->y =  (a->m[1][0] * v.y) +
            (a->m[1][1] * v.y) +
            (a->m[1][2] * v.y) +
            (a->m[1][3] * v.y) ;

    r->z =  (a->m[2][0] * v.z) +
            (a->m[2][1] * v.z) +
            (a->m[2][2] * v.z) +
            (a->m[2][3] * v.z) ;

    r->w =  (a->m[3][0] * v.w) +
            (a->m[3][1] * v.w) +
            (a->m[3][2] * v.w) +
            (a->m[3][3] * v.w) ;
}

void mScale(mat *r, const float x, const float y, const float z)
{
    r->m[0][0] *= x;
    r->m[0][1] *= x;
    r->m[0][2] *= x;
    r->m[0][3] *= x;

    r->m[1][0] *= y;
    r->m[1][1] *= y;
    r->m[1][2] *= y;
    r->m[1][3] *= y;

    r->m[2][0] *= z;
    r->m[2][1] *= z;
    r->m[2][2] *= z;
    r->m[2][3] *= z;
}

void mTranslate(mat *r, const float x, const float y, const float z)
{
    r->m[3][0] += (r->m[0][0] * x + r->m[1][0] * y + r->m[2][0] * z);
    r->m[3][1] += (r->m[0][1] * x + r->m[1][1] * y + r->m[2][1] * z);
    r->m[3][2] += (r->m[0][2] * x + r->m[1][2] * y + r->m[2][2] * z);
    r->m[3][3] += (r->m[0][3] * x + r->m[1][3] * y + r->m[2][3] * z);
}

void mRotate(mat *r, const float radians, float x, float y, float z)
{
    const float mag = rsqrtss(x * x + y * y + z * z);
    const float sinAngle = sinf(radians);
    const float cosAngle = cosf(radians);
    if(mag > 0.0f)
    {
        x *= mag;
        y *= mag;
        z *= mag;

        const float xx = x * x;
        const float yy = y * y;
        const float zz = z * z;
        const float xy = x * y;
        const float yz = y * z;
        const float zx = z * x;
        const float xs = x * sinAngle;
        const float ys = y * sinAngle;
        const float zs = z * sinAngle;
        const float oneMinusCos = 1.0f - cosAngle;

        mat rotMat;
        rotMat.m[0][0] = (oneMinusCos * xx) + cosAngle;
        rotMat.m[0][1] = (oneMinusCos * xy) - zs;
        rotMat.m[0][2] = (oneMinusCos * zx) + ys;
        rotMat.m[0][3] = 0.0F; 

        rotMat.m[1][0] = (oneMinusCos * xy) + zs;
        rotMat.m[1][1] = (oneMinusCos * yy) + cosAngle;
        rotMat.m[1][2] = (oneMinusCos * yz) - xs;
        rotMat.m[1][3] = 0.0F;

        rotMat.m[2][0] = (oneMinusCos * zx) - ys;
        rotMat.m[2][1] = (oneMinusCos * yz) + xs;
        rotMat.m[2][2] = (oneMinusCos * zz) + cosAngle;
        rotMat.m[2][3] = 0.0F; 

        rotMat.m[3][0] = 0.0F;
        rotMat.m[3][1] = 0.0F;
        rotMat.m[3][2] = 0.0F;
        rotMat.m[3][3] = 1.0F;

        mMul(r, &rotMat, r);
    }
}

void mRotX(mat *r, const float radians)
{
    const float s = sinf(radians);
    const float c = cosf(radians);
    const mat t = { c, 0.f, s, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    -s, 0.f, c, 0.f,
                    0.f, 0.f, 0.f, 1.f };
    mMul(r, &t, r);
}

void mRotY(mat *r, const float radians)
{
    const float s = sinf(radians);
    const float c = cosf(radians);
    const mat t = { 1.f, 0.f, 0.f, 0.f,
                    0.f, c, -s, 0.f,
                    0.f, s, c, 0.f,
                    0.f, 0.f, 0.f, 1.f };
    mMul(r, &t, r);
}

void mRotZ(mat *r, const float radians)
{
    const float s = sinf(radians);
    const float c = cosf(radians);
    const mat t = { c, -s, 0.f, 0.f,
                    s, c, 0.f, 0.f,
                    0.f, 0.f, 1.f, 0.f,
                    0.f, 0.f, 0.f, 1.f };
    mMul(r, &t, r);
}

void mAngleAxisRotate(mat *r, const mat view, const float xrot, const float yrot, const float zrot)
{
    // Test_User angle-axis rotation
    // https://en.wikipedia.org/wiki/Axis%E2%80%93angle_representation
    // https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
    // this is incrementing the rotation of the provided view matrix by the xrot,yrot,zrot
    vec tmp0, tmp1;

    vec vecview[3] = {
        {view.m[0][0], view.m[1][0], view.m[2][0]}, // r
        {view.m[0][1], view.m[1][1], view.m[2][1]}, // u
        {view.m[0][2], view.m[1][2], view.m[2][2]}  // f
    };

    // left/right
    vMulS(&tmp0, vecview[0], cosf(xrot));

    vCross(&tmp1, vecview[1], vecview[0]);
    vMulS(&tmp1, tmp1, sinf(xrot));

    vMulS(&vecview[0], vecview[1], vDot(vecview[1], vecview[0]) * (1.f - cosf(xrot)));

    vAdd(&vecview[0], vecview[0], tmp0);
    vAdd(&vecview[0], vecview[0], tmp1);

    // up/down
    vMulS(&tmp0, vecview[1], cosf(yrot));

    vCross(&tmp1, vecview[0], vecview[1]);
    vMulS(&tmp1, tmp1, sinf(yrot));

    vMulS(&vecview[1], vecview[0], vDot(vecview[0], vecview[1]) * (1.f - cosf(yrot)));

    vAdd(&vecview[1], vecview[1], tmp0);
    vAdd(&vecview[1], vecview[1], tmp1);

    vCross(&vecview[2], vecview[0], vecview[1]);
    vCross(&vecview[1], vecview[2], vecview[0]);

    // roll
    vMulS(&tmp0, vecview[0], cosf(zrot));

    vCross(&tmp1, vecview[2], vecview[0]);
    vMulS(&tmp1, tmp1, sinf(zrot));

    vMulS(&vecview[0], vecview[2], vDot(vecview[2], vecview[0]) * (1.f - cosf(zrot)));

    vAdd(&vecview[0], vecview[0], tmp0);
    vAdd(&vecview[0], vecview[0], tmp1);

    vCross(&vecview[1], vecview[2], vecview[0]);

    vNorm(&vecview[0]);
    vNorm(&vecview[1]);
    vNorm(&vecview[2]);

    *r = (mat){
        vecview[0].x, vecview[1].x, vecview[2].x, 0.f,
        vecview[0].y, vecview[1].y, vecview[2].y, 0.f,
        vecview[0].z, vecview[1].z, vecview[2].z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

void mFrustum(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    const float dX = right - left;
    const float dY = top - bottom;
    const float dZ = farZ - nearZ;
    const float rdX = 1.f/dX;
    const float rdY = 1.f/dY;
    const float rdZ = 1.f/dZ;
    mat frust;

    if(nearZ <= 0.0f || farZ <= 0.0f || dX <= 0.0f || dY <= 0.0f || dZ <= 0.0f)
        return;

    frust.m[0][0] = 2.0f * nearZ * rdX;
    frust.m[0][1] = frust.m[0][2] = frust.m[0][3] = 0.0f;

    frust.m[1][1] = 2.0f * nearZ * rdY;
    frust.m[1][0] = frust.m[1][2] = frust.m[1][3] = 0.0f;

    frust.m[2][0] = (right + left) * rdX;
    frust.m[2][1] = (top + bottom) * rdY;
    frust.m[2][2] = -(nearZ + farZ) * rdZ;
    frust.m[2][3] = -1.0f;

    frust.m[3][2] = -2.0f * nearZ * farZ * rdZ;
    frust.m[3][0] = frust.m[3][1] = frust.m[3][3] = 0.0f;

    mMul(r, &frust, r);
}

void mPerspective(mat *r, const float fovy, const float aspect, const float nearZ, const float farZ)
{
   float frustumW, frustumH;
   frustumH = tanf(fovy * 0.002777778078f * PI ) * nearZ; // 0x3b360b62
   frustumW = frustumH * aspect;
   mFrustum(r, -frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
}

void mOrtho(mat *r, const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    const float dX = right - left;
    const float dY = top - bottom;
    const float dZ = farZ - nearZ;
    const float rdX = 1.f/dX;
    const float rdY = 1.f/dY;
    const float rdZ = 1.f/dZ;
    mat ortho;

    if(dX == 0.0f || dY == 0.0f || dZ == 0.0f)
        return;

    mIdent(&ortho);
    ortho.m[0][0] = 2.0f * rdX;
    ortho.m[3][0] = -(right + left) * rdX;
    ortho.m[1][1] = 2.0f * rdY;
    ortho.m[3][1] = -(top + bottom) * rdY;
    ortho.m[2][2] = -2.0f * rdZ;
    ortho.m[3][2] = -(nearZ + farZ) * rdZ;

    mMul(r, &ortho, r);
}

void mLookAt(mat *r, const vec origin, const vec unit_dir)
{
    vec up;
    up.x = 0.f, up.y = 1.f, up.z = 0.f;

    vec dirn;
    dirn.x = unit_dir.x;
    dirn.y = unit_dir.y;
    dirn.z = unit_dir.z;

    vec c;
    vCross(&c, up, dirn);
    vNorm(&c);

    vec rup;
    vCross(&rup, dirn, c);

    r->m[0][0] = c.x;
    r->m[0][1] = c.y;
    r->m[0][2] = c.z;

    r->m[1][0] = rup.x;
    r->m[1][1] = rup.y;
    r->m[1][2] = rup.z;

    r->m[2][0] = dirn.x;
    r->m[2][1] = dirn.y;
    r->m[2][2] = dirn.z;

    r->m[3][0] = origin.x;
    r->m[3][1] = origin.y;
    r->m[3][2] = origin.z;
}

void mInvert(float *restrict dst, const float *restrict src)
{
    // original source: ftp://download.intel.com/design/PentiumIII/sml/24504301.pdf
    // mirrored: https://github.com/esAux/esAux-Menger/raw/main/SIMD%20Matrix%20Inverse.pdf

#ifdef NOSSE
    // Cramer Invert
    float tmp[12]; /* temp array for pairs */
    float tsrc[16]; /* array of transpose source matrix */
    float det; /* determinant */

    /* transpose matrix */
    for(int i = 0; i < 4; i++)
    {
        tsrc[i] = src[i*4];
        tsrc[i + 4] = src[i*4 + 1];
        tsrc[i + 8] = src[i*4 + 2];
        tsrc[i + 12] = src[i*4 + 3];
    }

    /* calculate pairs for first 8 elements (cofactors) */
    tmp[0] = tsrc[10] * tsrc[15];
    tmp[1] = tsrc[11] * tsrc[14];
    tmp[2] = tsrc[9] * tsrc[15];
    tmp[3] = tsrc[11] * tsrc[13];
    tmp[4] = tsrc[9] * tsrc[14];
    tmp[5] = tsrc[10] * tsrc[13];
    tmp[6] = tsrc[8] * tsrc[15];
    tmp[7] = tsrc[11] * tsrc[12];
    tmp[8] = tsrc[8] * tsrc[14];
    tmp[9] = tsrc[10] * tsrc[12];
    tmp[10] = tsrc[8] * tsrc[13];
    tmp[11] = tsrc[9] * tsrc[12];

    /* calculate first 8 elements (cofactors) */
    dst[0] = tmp[0]*tsrc[5] + tmp[3]*tsrc[6] + tmp[4]*tsrc[7];
    dst[0] -= tmp[1]*tsrc[5] + tmp[2]*tsrc[6] + tmp[5]*tsrc[7];
    dst[1] = tmp[1]*tsrc[4] + tmp[6]*tsrc[6] + tmp[9]*tsrc[7];
    dst[1] -= tmp[0]*tsrc[4] + tmp[7]*tsrc[6] + tmp[8]*tsrc[7];
    dst[2] = tmp[2]*tsrc[4] + tmp[7]*tsrc[5] + tmp[10]*tsrc[7];
    dst[2] -= tmp[3]*tsrc[4] + tmp[6]*tsrc[5] + tmp[11]*tsrc[7];
    dst[3] = tmp[5]*tsrc[4] + tmp[8]*tsrc[5] + tmp[11]*tsrc[6];
    dst[3] -= tmp[4]*tsrc[4] + tmp[9]*tsrc[5] + tmp[10]*tsrc[6];
    dst[4] = tmp[1]*tsrc[1] + tmp[2]*tsrc[2] + tmp[5]*tsrc[3];
    dst[4] -= tmp[0]*tsrc[1] + tmp[3]*tsrc[2] + tmp[4]*tsrc[3];
    dst[5] = tmp[0]*tsrc[0] + tmp[7]*tsrc[2] + tmp[8]*tsrc[3];
    dst[5] -= tmp[1]*tsrc[0] + tmp[6]*tsrc[2] + tmp[9]*tsrc[3];
    dst[6] = tmp[3]*tsrc[0] + tmp[6]*tsrc[1] + tmp[11]*tsrc[3];
    dst[6] -= tmp[2]*tsrc[0] + tmp[7]*tsrc[1] + tmp[10]*tsrc[3];
    dst[7] = tmp[4]*tsrc[0] + tmp[9]*tsrc[1] + tmp[10]*tsrc[2];
    dst[7] -= tmp[5]*tsrc[0] + tmp[8]*tsrc[1] + tmp[11]*tsrc[2];

    /* calculate pairs for second 8 elements (cofactors) */
    tmp[0] = tsrc[2]*tsrc[7];
    tmp[1] = tsrc[3]*tsrc[6];
    tmp[2] = tsrc[1]*tsrc[7];
    tmp[3] = tsrc[3]*tsrc[5];
    tmp[4] = tsrc[1]*tsrc[6];
    tmp[5] = tsrc[2]*tsrc[5];
    tmp[6] = tsrc[0]*tsrc[7];
    tmp[7] = tsrc[3]*tsrc[4];
    tmp[8] = tsrc[0]*tsrc[6];
    tmp[9] = tsrc[2]*tsrc[4];
    tmp[10] = tsrc[0]*tsrc[5];
    tmp[11] = tsrc[1]*tsrc[4];

    /* calculate second 8 elements (cofactors) */
    dst[8] = tmp[0]*tsrc[13] + tmp[3]*tsrc[14] + tmp[4]*tsrc[15];
    dst[8] -= tmp[1]*tsrc[13] + tmp[2]*tsrc[14] + tmp[5]*tsrc[15];
    dst[9] = tmp[1]*tsrc[12] + tmp[6]*tsrc[14] + tmp[9]*tsrc[15];
    dst[9] -= tmp[0]*tsrc[12] + tmp[7]*tsrc[14] + tmp[8]*tsrc[15];
    dst[10] = tmp[2]*tsrc[12] + tmp[7]*tsrc[13] + tmp[10]*tsrc[15];
    dst[10]-= tmp[3]*tsrc[12] + tmp[6]*tsrc[13] + tmp[11]*tsrc[15];
    dst[11] = tmp[5]*tsrc[12] + tmp[8]*tsrc[13] + tmp[11]*tsrc[14];
    dst[11]-= tmp[4]*tsrc[12] + tmp[9]*tsrc[13] + tmp[10]*tsrc[14];
    dst[12] = tmp[2]*tsrc[10] + tmp[5]*tsrc[11] + tmp[1]*tsrc[9];
    dst[12]-= tmp[4]*tsrc[11] + tmp[0]*tsrc[9] + tmp[3]*tsrc[10];
    dst[13] = tmp[8]*tsrc[11] + tmp[0]*tsrc[8] + tmp[7]*tsrc[10];
    dst[13]-= tmp[6]*tsrc[10] + tmp[9]*tsrc[11] + tmp[1]*tsrc[8];
    dst[14] = tmp[6]*tsrc[9] + tmp[11]*tsrc[11] + tmp[3]*tsrc[8];
    dst[14]-= tmp[10]*tsrc[11] + tmp[2]*tsrc[8] + tmp[7]*tsrc[9];
    dst[15] = tmp[10]*tsrc[10] + tmp[4]*tsrc[8] + tmp[9]*tsrc[9];
    dst[15]-= tmp[8]*tsrc[9] + tmp[11]*tsrc[10] + tmp[5]*tsrc[8];

    /* calculate determinant */
    det = tsrc[0]*dst[0]+tsrc[1]*dst[1]+tsrc[2]*dst[2]+tsrc[3]*dst[3];

    /* calculate matrix inverse */
    det = 1.0f/det;
    for(int j = 0; j < 16; j++){dst[j] *= det;}
#else
    memcpy(dst, src, sizeof(mat));

    __m128 minor0, minor1, minor2, minor3, det;
    __m128 tmp1 = _mm_setzero_ps();
    __m128 row0 = _mm_setzero_ps();
    __m128 row1 = _mm_setzero_ps();
    __m128 row2 = _mm_setzero_ps();
    __m128 row3 = _mm_setzero_ps();

    tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(dst)), (__m64*)(dst+ 4));
    row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(dst+8)), (__m64*)(dst+12));
    row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
    row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
    tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(dst+ 2)), (__m64*)(dst+ 6));
    row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(dst+10)), (__m64*)(dst+14));
    row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
    row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

    tmp1 = _mm_mul_ps(row2, row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor0 = _mm_mul_ps(row1, tmp1);
    minor1 = _mm_mul_ps(row0, tmp1);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
    minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
    minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

    tmp1 = _mm_mul_ps(row1, row2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
    minor3 = _mm_mul_ps(row0, tmp1);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
    minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);

    tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    row2 = _mm_shuffle_ps(row2, row2, 0x4E);
    minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
    minor2 = _mm_mul_ps(row0, tmp1);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
    minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

    tmp1 = _mm_mul_ps(row0, row1);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

    tmp1 = _mm_mul_ps(row0, row3);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
    minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

    tmp1 = _mm_mul_ps(row0, row2);
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
    tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

    det = _mm_mul_ps(row0, minor0);
    det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
    det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
    tmp1 = _mm_rcp_ss(det);
    det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
    det = _mm_shuffle_ps(det, det, 0x00);
    minor0 = _mm_mul_ps(det, minor0);
    _mm_storel_pi((__m64*)(dst), minor0);
    _mm_storeh_pi((__m64*)(dst+2), minor0);
    minor1 = _mm_mul_ps(det, minor1);
    _mm_storel_pi((__m64*)(dst+4), minor1);
    _mm_storeh_pi((__m64*)(dst+6), minor1);
    minor2 = _mm_mul_ps(det, minor2);
    _mm_storel_pi((__m64*)(dst+ 8), minor2);
    _mm_storeh_pi((__m64*)(dst+10), minor2);
    minor3 = _mm_mul_ps(det, minor3);
    _mm_storel_pi((__m64*)(dst+12), minor3);
    _mm_storeh_pi((__m64*)(dst+14), minor3);
#endif
}

void mTranspose(mat *r, const mat *restrict m)
{
    r->m[0][0] = m->m[0][0];
    r->m[1][0] = m->m[0][1];
    r->m[2][0] = m->m[0][2];
    r->m[3][0] = m->m[0][3];

    r->m[0][1] = m->m[1][0];
    r->m[1][1] = m->m[1][1];
    r->m[2][1] = m->m[1][2];
    r->m[3][1] = m->m[1][3];

    r->m[0][2] = m->m[2][0];
    r->m[1][2] = m->m[2][1];
    r->m[2][2] = m->m[2][2];
    r->m[3][2] = m->m[2][3];

    r->m[0][3] = m->m[3][0];
    r->m[1][3] = m->m[3][1];
    r->m[2][3] = m->m[3][2];
    r->m[3][3] = m->m[3][3];
}

//

void mSetViewDir(mat *r, const vec dir_norm, const vec up_norm)
{
    vec c;
    vCross(&c, up_norm, dir_norm);
    vNorm(&c);

    vec rup;
    vCross(&rup, dir_norm, c);

    r->m[0][0] = -c.x;
    r->m[1][0] = -c.y;
    r->m[2][0] = -c.z;

    r->m[0][1] = -rup.x;
    r->m[1][1] = -rup.y;
    r->m[2][1] = -rup.z;

    r->m[0][2] = -dir_norm.x;
    r->m[1][2] = -dir_norm.y;
    r->m[2][2] = -dir_norm.z;
}

void mGetViewDir(vec *r, const mat matrix)
{
    mGetViewZ(r, matrix);
}

void mGetViewX(vec *r, const mat matrix)
{
    r->x = -matrix.m[0][0];
    r->y = -matrix.m[1][0];
    r->z = -matrix.m[2][0];
}

void mGetViewY(vec *r, const mat matrix)
{
    r->x = -matrix.m[0][1];
    r->y = -matrix.m[1][1];
    r->z = -matrix.m[2][1];
}

void mGetViewZ(vec *r, const mat matrix)
{
    r->x = -matrix.m[0][2];
    r->y = -matrix.m[1][2];
    r->z = -matrix.m[2][2];
}

//

void mSetDir(mat *r, const vec dir_norm, const vec up_norm)
{
    vec c;
    vCross(&c, up_norm, dir_norm);
    vNorm(&c);

    vec rup;
    vCross(&rup, dir_norm, c);

    r->m[0][0] = c.x;
    r->m[0][1] = c.y;
    r->m[0][2] = c.z;

    r->m[1][0] = rup.x;
    r->m[1][1] = rup.y;
    r->m[1][2] = rup.z;

    r->m[2][0] = dir_norm.x;
    r->m[2][1] = dir_norm.y;
    r->m[2][2] = dir_norm.z;
}

void mGetDirX(vec *r, const mat matrix)
{
    r->x = matrix.m[0][0];
    r->y = matrix.m[0][1];
    r->z = matrix.m[0][2];
}

void mGetDirY(vec *r, const mat matrix)
{
    r->x = matrix.m[1][0];
    r->y = matrix.m[1][1];
    r->z = matrix.m[1][2];
}

void mGetDirZ(vec *r, const mat matrix)
{
    r->x = matrix.m[2][0];
    r->y = matrix.m[2][1];
    r->z = matrix.m[2][2];
}

void mGetPos(vec *r, const mat matrix)
{
    r->x = matrix.m[3][0];
    r->y = matrix.m[3][1];
    r->z = matrix.m[3][2];
}

void mSetPos(mat *r, const vec pos)
{
    r->m[3][0] = pos.x;
    r->m[3][1] = pos.y;
    r->m[3][2] = pos.z;
}

void mDump(const mat matrix)
{
    printf("%+.2f %+.2f %+.2f %+.2f\n", matrix.m[0][0], matrix.m[0][1], matrix.m[0][2], matrix.m[0][3]);
    printf("%+.2f %+.2f %+.2f %+.2f\n", matrix.m[1][0], matrix.m[1][1], matrix.m[1][2], matrix.m[1][3]);
    printf("%+.2f %+.2f %+.2f %+.2f\n", matrix.m[2][0], matrix.m[2][1], matrix.m[2][2], matrix.m[2][3]);
    printf("%+.2f %+.2f %+.2f %+.2f\n", matrix.m[3][0], matrix.m[3][1], matrix.m[3][2], matrix.m[3][3]);
    printf("---\n");
}

#endif
