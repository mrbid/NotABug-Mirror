#include <stddef.h>

#include "../inc/gl.h"

#ifndef OLD_ASSETS_H
#define OLD_ASSETS_H

extern GLfloat ufo_damaged_vertices[];
extern const GLfloat ufo_damaged_normals[];
extern GLfloat ufo_damaged_uvmap[];  // needs flipping with flipUV()
extern const GLushort ufo_damaged_indices[];
extern const GLsizeiptr ufo_damaged_numind;
extern const GLsizeiptr ufo_damaged_numvert;
extern const size_t ufo_damaged_vertices_size;
extern const size_t ufo_damaged_normals_size;
extern const size_t ufo_damaged_uvmap_size;
extern const size_t ufo_damaged_indices_size;

extern GLfloat ufo_tri_vertices[];
extern const GLubyte ufo_tri_indices[];
extern const GLsizeiptr ufo_tri_numvert;
extern const GLsizeiptr ufo_tri_numind;
extern const size_t ufo_tri_vertices_size;
extern const size_t ufo_tri_indices_size;

extern GLfloat pod_vertices[];
extern const GLfloat pod_normals[];
extern const GLfloat pod_uvmap[];
extern const GLushort pod_indices[];
extern const GLsizeiptr pod_numind;
extern const GLsizeiptr pod_numvert;
extern const size_t pod_vertices_size;
extern const size_t pod_normals_size;
extern const size_t pod_uvmap_size;
extern const size_t pod_indices_size;

extern const unsigned char tex_pod[]; // 800x533x3

#endif
