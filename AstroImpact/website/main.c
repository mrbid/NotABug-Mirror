/*
--------------------------------------------------
    James William Fletcher (github.com/mrbid)
        June 2023 - AstroImpact.net
--------------------------------------------------
    Emscripten / C & SDL / OpenGL ES2 / GLSL ES
*/

#include <emscripten.h>
#include <emscripten/html5.h>

#include <SDL.h>
#include <SDL_opengles2.h>

#define NOSSE
#define SEIR_RAND

#include "../inc/esAux4.h"
#include "../assets/models.h"

#define uint GLuint
#define f32 GLfloat

//*************************************
// globals
//*************************************
char appTitle[] = "AstroImpact.net";
SDL_Window* wnd;
SDL_GLContext glc;
Uint32 winw = 0, winh = 0;
f32 aspect, t;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;

// render state matrices
mat projection;
mat view;

// models
ESModel mdlPlanet;

// sim vars
vec lightpos = {0.f, 0.f, 0.f};

//*************************************
// emscripten/gl functions
//*************************************
void doPerspective()
{
    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.01f, 120.f); 
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*)&projection.m[0][0]);
}

EM_BOOL emscripten_resize_event(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
    winw = uiEvent->documentBodyClientWidth;
    winh = uiEvent->documentBodyClientHeight;
    doPerspective();
    emscripten_set_canvas_element_size("canvas", winw, winh);
    return EM_FALSE;
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static f32 lt = 0, ts = -1.3f;
    t = ((f32)SDL_GetTicks())*0.001f;
    const f32 dt = t-lt;
    lt = t;
    ts += dt*0.01f;

//*************************************
// camera & simulation
//*************************************
    static f32 xrot = 0.f, yrot = 0.f;

    mIdent(&view);
    mSetPos(&view, (vec){0.f, 0.f, -3.f});
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 0.f, 1.f);

    // yrot += dt*0.01f;
    // xrot += dt*0.01f;
    // glUniform3f(lightpos_id, sinf(ts) * x2PI, cosf(ts) * x2PI, sinf(ts) * x2PI);

    yrot = sinf(ts)*x2PI;
    xrot += dt*0.01f;
    const f32 ls = ts*0.5f;
    glUniform3f(lightpos_id, sinf(ls) * x2PI, cosf(ls) * x2PI, sinf(ls) * x2PI);

//*************************************
// render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (GLfloat*) &view.m[0][0]);
    glDrawElements(GL_TRIANGLES, exo_numind, GL_UNSIGNED_INT, 0);
    SDL_GL_SwapWindow(wnd);
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
//*************************************
// setup render context / window
//*************************************
    SDL_Init(SDL_INIT_VIDEO);
    
    double width, height;
    emscripten_get_element_css_size("body", &width, &height);
    winw = (Uint32)width, winh = (Uint32)height;
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GL_SetSwapInterval(0);
    glc = SDL_GL_CreateContext(wnd);

//*************************************
// bind vertex and index buffers
//*************************************
    const GLsizeiptr s = exo_numvert*3;
    for(GLsizeiptr i = 0; i < s; i+=3)
    {
        const f32 g = (exo_colors[i] + exo_colors[i+1] + exo_colors[i+2]) * 0.3333333433f;
        const f32 h = (1.f-g)*1.3f;
        vec v = {exo_vertices[i], exo_vertices[i+1], exo_vertices[i+2]};
        vNorm(&v);
        vMulS(&v, v, h);
        exo_vertices[i]   -= v.x;
        exo_vertices[i+1] -= v.y;
        exo_vertices[i+2] -= v.z;
        exo_vertices[i]   *= 0.01f;
        exo_vertices[i+1] *= 0.01f;
        exo_vertices[i+2] *= 0.01f;
    }
    esBind(GL_ARRAY_BUFFER, &mdlPlanet.vid, exo_vertices, exo_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPlanet.cid, exo_colors, exo_colors_size, GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPlanet.iid, exo_indices, exo_indices_size, GL_STATIC_DRAW);

//*************************************
// projection
//*************************************
    doPerspective();

//*************************************
// compile & link shader program
//*************************************
    makeLambert2();

//*************************************
// configure render options
//*************************************
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);

    shadeLambert2(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (GLfloat*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPlanet.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPlanet.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPlanet.iid);

//*************************************
// execute update / render loop
//*************************************
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE, emscripten_resize_event);
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}

