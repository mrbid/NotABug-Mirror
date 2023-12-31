#include <stddef.h>

#ifndef __EMSCRIPTEN__
    #include "../inc/gl.h"
#else
    #include <SDL.h>
    #include <SDL_opengles2.h>
#endif

#ifndef models_H
#define models_H

extern GLfloat exo_vertices[];

#ifndef EXO_VERTICES_ONLY
extern GLfloat exo_colors[];
extern const GLuint exo_indices[];
extern const GLsizeiptr exo_numind;
#endif

extern const GLsizeiptr exo_numvert;
extern const size_t exo_vertices_size;

#ifndef EXO_VERTICES_ONLY
extern const size_t exo_colors_size;
extern const size_t exo_indices_size;

extern GLfloat ncube_vertices[];
extern const GLuint ncube_indices[];
extern const GLsizeiptr ncube_numind;
extern const GLsizeiptr ncube_numvert;
extern const size_t ncube_vertices_size;
extern const size_t ncube_indices_size;

extern GLfloat inner_colors[];
extern const size_t inner_colors_size;

extern const GLfloat rock8_vertices[];
extern const GLfloat rock8_normals[];
extern const GLfloat rock8_uvmap[];
extern const GLushort rock8_indices[];
extern const GLsizeiptr rock8_numind;
extern const GLsizeiptr rock8_numvert;
extern const size_t rock8_vertices_size;
extern const size_t rock8_normals_size;
extern const size_t rock8_uvmap_size;
extern const size_t rock8_indices_size;

extern const GLfloat rock7_vertices[];
extern const GLfloat rock7_normals[];
extern const GLfloat rock7_uvmap[];
extern const GLushort rock7_indices[];
extern const GLsizeiptr rock7_numind;
extern const GLsizeiptr rock7_numvert;
extern const size_t rock7_vertices_size;
extern const size_t rock7_normals_size;
extern const size_t rock7_uvmap_size;
extern const size_t rock7_indices_size;

extern const GLfloat rock6_vertices[];
extern const GLfloat rock6_normals[];
extern const GLfloat rock6_uvmap[];
extern const GLushort rock6_indices[];
extern const GLsizeiptr rock6_numind;
extern const GLsizeiptr rock6_numvert;
extern const size_t rock6_vertices_size;
extern const size_t rock6_normals_size;
extern const size_t rock6_uvmap_size;
extern const size_t rock6_indices_size;

extern const GLfloat rock5_vertices[];
extern const GLfloat rock5_normals[];
extern const GLfloat rock5_uvmap[];
extern const GLushort rock5_indices[];
extern const GLsizeiptr rock5_numind;
extern const GLsizeiptr rock5_numvert;
extern const size_t rock5_vertices_size;
extern const size_t rock5_normals_size;
extern const size_t rock5_uvmap_size;
extern const size_t rock5_indices_size;

extern const GLfloat rock4_vertices[];
extern const GLfloat rock4_normals[];
extern const GLfloat rock4_uvmap[];
extern const GLushort rock4_indices[];
extern const GLsizeiptr rock4_numind;
extern const GLsizeiptr rock4_numvert;
extern const size_t rock4_vertices_size;
extern const size_t rock4_normals_size;
extern const size_t rock4_uvmap_size;
extern const size_t rock4_indices_size;

extern const GLfloat rock3_vertices[];
extern const GLfloat rock3_normals[];
extern const GLfloat rock3_uvmap[];
extern const GLushort rock3_indices[];
extern const GLsizeiptr rock3_numind;
extern const GLsizeiptr rock3_numvert;
extern const size_t rock3_vertices_size;
extern const size_t rock3_normals_size;
extern const size_t rock3_uvmap_size;
extern const size_t rock3_indices_size;

extern const GLfloat rock2_vertices[];
extern const GLfloat rock2_normals[];
extern const GLfloat rock2_uvmap[];
extern const GLushort rock2_indices[];
extern const GLsizeiptr rock2_numind;
extern const GLsizeiptr rock2_numvert;
extern const size_t rock2_vertices_size;
extern const size_t rock2_normals_size;
extern const size_t rock2_uvmap_size;
extern const size_t rock2_indices_size;

extern const GLfloat rock1_vertices[];
extern const GLfloat rock1_normals[];
extern const GLfloat rock1_uvmap[];
extern const GLushort rock1_indices[];
extern const GLsizeiptr rock1_numind;
extern const GLsizeiptr rock1_numvert;
extern const size_t rock1_vertices_size;
extern const size_t rock1_normals_size;
extern const size_t rock1_uvmap_size;
extern const size_t rock1_indices_size;

extern GLfloat beam_vertices[];
extern const GLfloat beam_normals[];
extern const GLfloat beam_uvmap[];
extern const GLubyte beam_indices[];
extern const GLsizeiptr beam_numind;
extern const GLsizeiptr beam_numvert;
extern const size_t beam_vertices_size;
extern const size_t beam_normals_size;
extern const size_t beam_uvmap_size;
extern const size_t beam_indices_size;

extern GLfloat ufo_vertices[];
extern const GLfloat ufo_normals[];
extern GLfloat ufo_uvmap[]; // needs flipping with flipUV()
extern const GLushort ufo_indices[];
extern const GLsizeiptr ufo_numind;
extern const GLsizeiptr ufo_numvert;
extern const size_t ufo_vertices_size;
extern const size_t ufo_normals_size;
extern const size_t ufo_uvmap_size;
extern const size_t ufo_indices_size;

extern GLfloat ufo_lights_vertices[];
extern const GLubyte ufo_lights_indices[];
extern const GLsizeiptr ufo_lights_numvert;
extern const GLsizeiptr ufo_lights_numind;
extern const size_t ufo_lights_vertices_size;
extern const size_t ufo_lights_indices_size;

extern GLfloat ufo_shield_vertices[];
extern const GLushort ufo_shield_indices[];
extern const GLfloat ufo_shield_colors[];
extern const GLsizeiptr ufo_shield_numvert;
extern const GLsizeiptr ufo_shield_numind;
extern const size_t ufo_shield_vertices_size;
extern const size_t ufo_shield_indices_size;
extern const size_t ufo_shield_colors_size;

extern GLfloat ufo_interior_vertices[];
extern const GLubyte ufo_interior_indices[];
extern const GLfloat ufo_interior_colors[];
extern const GLsizeiptr ufo_interior_numvert;
extern const GLsizeiptr ufo_interior_numind;
extern const size_t ufo_interior_vertices_size;
extern const size_t ufo_interior_indices_size;
extern const size_t ufo_interior_colors_size;

#endif
#endif
