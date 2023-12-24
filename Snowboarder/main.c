/*
--------------------------------------------------
    James William Fletcher (james@voxdsp.com)
        December 2021
--------------------------------------------------
    C & SDL / OpenGL ES2 / GLSL ES

    Colour Converter: https://www.easyrgb.com
*/

#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include "esAux2.h"

#include "assets/tunnel.h"
#include "assets/dead.h"
#include "assets/three.h"
#include "assets/two.h"
#include "assets/one.h"
#include "assets/lslick.h"
#include "assets/mslick.h"
#include "assets/rslick.h"
#include "assets/lgem.h"
#include "assets/mgem.h"
#include "assets/rgem.h"
#include "assets/lboost.h"
#include "assets/mboost.h"
#include "assets/rboost.h"
#include "assets/lboarder.h"
#include "assets/mboarder.h"
#include "assets/rboarder.h"

#include "assets/res.h"

#define uint GLushort
#define sint GLshort
#define f32 GLfloat

//*************************************
// globals
//*************************************
char appTitle[] = "Snowboarder";
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Surface* s_icon = NULL;
Uint32 winw = 1024, winh = 768;
f32 aspect;
f32 t = 0.f;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// render state inputs
vec lightpos = {0.f, 0.f, 0.f};

// models
uint bindstate = 0;
ESModel mdlTunnel;
ESModel mdlDead;
ESModel mdlThree;
ESModel mdlTwo;
ESModel mdlOne;
ESModel mdlLslick;
ESModel mdlMslick;
ESModel mdlRslick;
ESModel mdlLgem;
ESModel mdlMgem;
ESModel mdlRgem;
ESModel mdlLboost;
ESModel mdlMboost;
ESModel mdlRboost;
ESModel mdlLboarder;
ESModel mdlMboarder;
ESModel mdlRboarder;

// simulation / game vars
f32 dt = 0;
double rt = 0;
uint state = 3;
f32 camdist = -5.0f;
uint keystate[2] = {0};
uint player_x = 1;
uint score = 0;

#define ARRAY_MAX 6
typedef struct
{
    f32 z;
    uint x;
    uint s;
} gi;

uint max_slicks = 0;
uint max_gems = 0;
uint max_boosts = 0;
gi array_slick[ARRAY_MAX];
gi array_gem[ARRAY_MAX];
gi array_boost[ARRAY_MAX];

#define NEWGAME_SEED 1337
#define MINSPEED -9.f
#define MAXSPEED -32.f
#define DRAGRATE 3.3f
f32 pz = 0.f;
f32 ps = MINSPEED;
f32 pr = 0.f;
f32 po = 1.0f;

vec c1 = {1.000,0.325,0.251};
vec c2 = {0.404,1.000,0.663};
vec c3 = {0.827,0.400,1.000};

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    memcpy(s->pixels, data, s->pitch*h);
    return s;
}

//*************************************
// render functions
//*************************************
void rTunnel()
{
    bindstate = 0;

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlTunnel.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlTunnel.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlTunnel.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlTunnel.iid);

    glDrawElements(GL_TRIANGLES, tunnel_numind, GL_UNSIGNED_SHORT, 0);
}

void rDead(f32 z)
{
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, 0.f, 0.f, z);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlDead.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlDead.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlDead.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlDead.iid);

    glDrawElements(GL_TRIANGLES, dead_numind, GL_UNSIGNED_SHORT, 0);
}

void rCountDown(f32 z, uint c)
{
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, 0.f, 0.f, z);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    if(c == 3)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlThree.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlThree.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlThree.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlThree.iid);

        glDrawElements(GL_TRIANGLES, three_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(c == 2)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlTwo.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlTwo.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlTwo.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlTwo.iid);

        glDrawElements(GL_TRIANGLES, two_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(c == 1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlOne.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlOne.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlOne.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlOne.iid);

        glDrawElements(GL_TRIANGLES, one_numind, GL_UNSIGNED_SHORT, 0);
    }
}

void rSlick(f32 z, uint x)
{
    mIdent(&model);
    mTranslate(&model, 0.f, 0.f, z);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    if(x == 0)
    {
        if(bindstate != 1)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlLslick.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlLslick.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlLslick.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlLslick.iid);

            bindstate = 1;
        }

        glDrawElements(GL_TRIANGLES, lslick_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 1)
    {
        if(bindstate != 2)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlMslick.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlMslick.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlMslick.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMslick.iid);

            bindstate = 2;
        }

        glDrawElements(GL_TRIANGLES, mslick_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 2)
    {
        if(bindstate != 3)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlRslick.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlRslick.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlRslick.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRslick.iid);

            bindstate = 3;
        }

        glDrawElements(GL_TRIANGLES, rslick_numind, GL_UNSIGNED_SHORT, 0);
    }
}

void rGem(f32 z, uint x)
{
    mIdent(&model);
    mTranslate(&model, 0.f, 0.f, z);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    if(x == 0)
    {
        if(bindstate != 4)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlLgem.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlLgem.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlLgem.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlLgem.iid);

            bindstate = 4;
        }

        glDrawElements(GL_TRIANGLES, lgem_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 1)
    {
        if(bindstate != 5)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlMgem.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlMgem.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlMgem.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMgem.iid);

            bindstate = 5;
        }

        glDrawElements(GL_TRIANGLES, mgem_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 2)
    {
        if(bindstate != 6)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlRgem.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlRgem.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlRgem.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRgem.iid);

            bindstate = 6;
        }

        glDrawElements(GL_TRIANGLES, rgem_numind, GL_UNSIGNED_SHORT, 0);
    }
}

void rBoost(f32 z, uint x)
{
    mIdent(&model);
    mTranslate(&model, 0.f, 0.f, z);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    if(x == 0)
    {
        if(bindstate != 7)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlLboost.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlLboost.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlLboost.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlLboost.iid);

            bindstate = 7;
        }

        glDrawElements(GL_TRIANGLES, lboost_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 1)
    {
        if(bindstate != 8)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlMboost.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlMboost.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlMboost.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMboost.iid);

            bindstate = 8;
        }

        glDrawElements(GL_TRIANGLES, mboost_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 2)
    {
        if(bindstate != 9)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlRboost.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlRboost.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlRboost.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRboost.iid);

            bindstate = 9;
        }

        glDrawElements(GL_TRIANGLES, rboost_numind, GL_UNSIGNED_SHORT, 0);
    }
}

void rBoarder(f32 z, uint x, f32 r)
{
    mIdent(&model);
    mTranslate(&model, 0.f, 0.f, z);

    if(x == 1)
        mRotX(&model, r);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

    if(po < 1.0f)
        po += 1.0f * dt;
    else
        po = 1.0f;
    glUniform1f(opacity_id, po);

    if(po != 1.0f)
        glEnable(GL_BLEND);

    if(x == 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlLboarder.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlLboarder.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlLboarder.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlLboarder.iid);

        glDrawElements(GL_TRIANGLES, lboarder_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlMboarder.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlMboarder.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlMboarder.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMboarder.iid);

        glDrawElements(GL_TRIANGLES, mboarder_numind, GL_UNSIGNED_SHORT, 0);
    }
    else if(x == 2)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlRboarder.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlRboarder.nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlRboarder.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRboarder.iid);

        glDrawElements(GL_TRIANGLES, rboarder_numind, GL_UNSIGNED_SHORT, 0);
    }

    if(po != 1.0f)
        glDisable(GL_BLEND);
}

//*************************************
// gl functions
//*************************************
void doPerspective()
{
    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 1.0f, 160.f);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static f32 lt = 0;
    t = ((float)SDL_GetTicks())*0.001f;
    dt = t-lt;
    lt = t;

//*************************************
// input handling
//*************************************
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
            {
                if(event.key.keysym.sym == SDLK_LEFT){ keystate[0] = 1; player_x = 0; }
                else if(event.key.keysym.sym == SDLK_RIGHT){ keystate[1] = 1; player_x = 2; }
            }
            break;

            case SDL_KEYUP:
            {
                if(event.key.keysym.sym == SDLK_LEFT){ keystate[0] = 0; }
                else if(event.key.keysym.sym == SDLK_RIGHT){ keystate[1] = 0; }

                if(keystate[0] == 1){ player_x = 0; }
                else if(keystate[1] == 1){ player_x = 2; }
                else { player_x = 1; }
            }
            break;

            case SDL_FINGERDOWN:
            {
                if(event.tfinger.x < 0.3f){keystate[0] = 1; player_x = 0;} // left
                else if(event.tfinger.x > 0.6f){keystate[1] = 1; player_x = 2;} // right
                else{keystate[0] = 0; keystate[1] = 0; player_x = 1;} // center
            }
            break;

            case SDL_FINGERMOTION:
            {
                if(event.tfinger.x < 0.3f){keystate[0] = 1; player_x = 0;} // left
                else if(event.tfinger.x > 0.6f){keystate[1] = 1; player_x = 2;} // right
                else{keystate[0] = 0; keystate[1] = 0; player_x = 1;} // center
            }
            break;

            case SDL_FINGERUP:
            {
                keystate[0] = 0;
                keystate[1] = 0;
                player_x = 1;
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    keystate[0] = 1; player_x = 0;
                }
                else if(event.button.button == SDL_BUTTON_RIGHT)
                {
                    keystate[1] = 1; player_x = 2;
                }
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT){ keystate[0] = 0; }
                else if(event.button.button == SDL_BUTTON_RIGHT){ keystate[1] = 0; }
                if(keystate[0] == 1){ player_x = 0; }
                else if(keystate[1] == 1){ player_x = 2; }
                else { player_x = 1; }
            }
            break;

            case SDL_WINDOWEVENT:
            {
                if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    winw = event.window.data1;
                    winh = event.window.data2;
                    doPerspective();
                }
            }
            break;

            case SDL_QUIT:
            {
                SDL_GL_DeleteContext(glc);
                SDL_FreeSurface(s_icon);
                SDL_DestroyWindow(wnd);
                SDL_Quit();
                exit(0);
            }
            break;
        }
    }

//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

    // camera
    mIdent(&view);
    mTranslate(&view, 0.f, -2.f, camdist);

    if(state == 0) // next tunnel
    {
        // new slick array
        max_slicks = esRand(0, ARRAY_MAX);
        const uint empty_lane = esRand(0, 2);
        for(uint i = 0; i < max_slicks; i++)
        {
            array_slick[i].z = esRandFloat(-41.f, -70.f);
            array_slick[i].x = esRand(0, 3);
            if(array_slick[i].x == empty_lane)
                array_slick[i].s = 0;
            else
                array_slick[i].s = 1;
        }

        // new gem array
        max_gems = esRand(0, ARRAY_MAX);
        for(uint i = 0; i < max_gems; i++)
        {
            array_gem[i].z = esRandFloat(-11.f, -70.f);
            array_gem[i].x = esRand(0, 3);
            array_gem[i].s = 1;
        }

        // new boost array
        max_boosts = esRand(0, ARRAY_MAX/2);
        for(uint i = 0; i < max_boosts; i++)
        {
            array_boost[i].z = esRandFloat(-11.f, -70.f);
            array_boost[i].x = esRand(0, 3);
            array_boost[i].s = 1;
        }

        // reset vars
        pz = 0;
        pr = 0.f;

        // transition tunnel colours
        vec cn;
        cn.x = esRandFloat(0.f, 1.f);
        cn.y = esRandFloat(0.f, 1.f);
        cn.z = esRandFloat(0.f, 1.f);
        const uint mi = tunnel_numvert*3;
        for(uint i = 0; i < mi; i += 3)
        {
            if(tunnel_colors[i] == c1.x && tunnel_colors[i+1] == c1.y && tunnel_colors[i+2] == c1.z)
            {
                tunnel_colors[i] = c2.x;
                tunnel_colors[i+1] = c2.y;
                tunnel_colors[i+2] = c2.z;
            }
            else if(tunnel_colors[i] == c2.x && tunnel_colors[i+1] == c2.y && tunnel_colors[i+2] == c2.z)
            {
                tunnel_colors[i] = c3.x;
                tunnel_colors[i+1] = c3.y;
                tunnel_colors[i+2] = c3.z;
            }
            else if(tunnel_colors[i] == c3.x && tunnel_colors[i+1] == c3.y && tunnel_colors[i+2] == c3.z)
            {
                tunnel_colors[i] = cn.x;
                tunnel_colors[i+1] = cn.y;
                tunnel_colors[i+2] = cn.z;
            }
        }
        c1.x = c2.x;
        c1.y = c2.y;
        c1.z = c2.z;
        c2.x = c3.x;
        c2.y = c3.y;
        c2.z = c3.z;
        c3.x = cn.x;
        c3.y = cn.y;
        c3.z = cn.z;
        glBindBuffer(GL_ARRAY_BUFFER, mdlTunnel.cid);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tunnel_colors), tunnel_colors, GL_STATIC_DRAW);

        // reposition next tunnel angle
        const f32 r1 = esRandFloat(-6.f, 6.f);
        const f32 r2 = esRandFloat(-91.f, -130.f);
        for(uint i = 0; i < mi; i += 3)
        {
            if(tunnel_vertices[i+2] < -91.f)
            {
                tunnel_vertices[i] += -12.f * sinf(r1);
                tunnel_vertices[i+1] += -12.f * cosf(r1);
                tunnel_vertices[i+2] = r2;
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, mdlTunnel.vid);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tunnel_vertices), tunnel_vertices, GL_STATIC_DRAW);

        // next state
        state = 1;

        // log
        char strts[16];
        timestamp(&strts[0]);
        printf("[%s] New Tunnel\n", strts);
    }
    else if(state == 1) // simulate game
    {
        // render slicks
        for(uint i = 0; i < max_slicks; i++)
        {
            if(array_slick[i].s == 1)
            {
                rSlick(array_slick[i].z, array_slick[i].x);
                if(player_x == array_slick[i].x && pz < array_slick[i].z+0.75f && pz > array_slick[i].z-0.75f)
                {
                    rt = t+3;
                    state = 2;
                    array_slick[i].s = 0;
                    char strts[16];
                    timestamp(&strts[0]);
                    printf("[%s] Slick: %.2f %u %u\n", strts, array_slick[i].z, array_slick[i].x, array_slick[i].s);
                    printf("[%s] >>> YOU DIED, SCORE %u <<<\n", strts, score);
                }
            }
        }

        // render gems
        for(uint i = 0; i < max_gems; i++)
        {
            if(array_gem[i].s == 1)
            {
                rGem(array_gem[i].z, array_gem[i].x);
                if(player_x == array_gem[i].x && pz < array_gem[i].z+0.3f && pz > array_gem[i].z-0.3f)
                {
                    array_gem[i].s = 0;
                    score++;
                    po = 0.3f;

                    char title[256];
                    sprintf(title, "SCORE %u", score);
                    SDL_SetWindowTitle(wnd, title);

                    char strts[16];
                    timestamp(&strts[0]);
                    printf("[%s] Gem: %.2f %u %u\n", strts, array_gem[i].z, array_gem[i].x, array_gem[i].s);
                }
            }
        }

        // render boosts
        for(uint i = 0; i < max_boosts; i++)
        {
            rBoost(array_boost[i].z, array_boost[i].x);
            if(array_boost[i].s == 1 && player_x == array_boost[i].x && pz < array_boost[i].z+1.6f && pz > array_boost[i].z-1.6f)
            {
                ps *= 2.3f;
                array_boost[i].s = 0;
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] Boost: %.2f %u %u\n", strts, array_boost[i].z, array_boost[i].x, array_boost[i].s);
            }
        }

        // move player
        rBoarder(pz, player_x, pr);
        pz += ps * dt;
        if(ps < MINSPEED)
            ps += DRAGRATE * dt;
        else
            ps = MINSPEED;
        if(ps < MAXSPEED)
            ps = MAXSPEED;
        camdist = -pz + -5.0f;
        
        // reset
        if(pz < -80.f)
            state = 0;
    }
    else if(state == 2) // you died
    {
        pr += 1.2f * dt;
        rBoarder(pz, 1, pr);
        rDead(pz-6.0f);

        if(t > rt)
        {
            // reset game
            srand(time(0));
            SDL_SetWindowTitle(wnd, appTitle);
            ps = MINSPEED;
            score = 0;
            state = 3;
            rt = t + 3;
            pz = 0.f;
            camdist = -5.0f;
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] *** New Game\n", strts);
        }
    }
    else if(state == 3) // round count down
    {
        static double lt = 0;
        static f32 z = 0.f;
        const double ft = floor(rt-t);
        if(lt != ft)
        {
            z = 0.f;
            lt = ft;
        }
        z -= 6.0f * dt;
        rCountDown(z, rt-t+1);
        if(t > rt)
            state = 0;
    }

    // render tunnel
    rTunnel();

//*************************************
// swap buffers / display render
//*************************************
    SDL_GL_SwapWindow(wnd);
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    printf("Snowboarder, A simple snowboarding game.\n");
    printf("James William Fletcher (github.com/mrbid)\n\n");
    printf("Use your keyboard arrow keys to move from left to right or left and right click.\n\n");

//*************************************
// setup render context / window
//*************************************
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) < 0)
    {
        printf("ERROR: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    while(wnd == NULL)
    {
        msaa--;
        if(msaa == 0)
        {
            printf("ERROR: SDL_CreateWindow(): %s\n", SDL_GetError());
            return 1;
        }
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
        wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    }
    SDL_GL_SetSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync
    glc = SDL_GL_CreateContext(wnd);
    if(glc == NULL)
    {
        printf("ERROR: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    // set icon
    unsigned char* ipd = (unsigned char*)&icon_image2.pixel_data;
    srand(time(0));
    const uint r = esRand(0, 2);
    if(r == 0)      {ipd = (unsigned char*)&icon_image.pixel_data;}
    else if(r == 1) {ipd = (unsigned char*)&icon_image1.pixel_data;}
    else            {ipd = (unsigned char*)&icon_image2.pixel_data;}
    s_icon = surfaceFromData((Uint32*)ipd, 16, 16);
    SDL_SetWindowIcon(wnd, s_icon);

    // seed random
    srand(time(0));

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND TUNNEL *****
    esBind(GL_ARRAY_BUFFER, &mdlTunnel.vid, tunnel_vertices, sizeof(tunnel_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTunnel.nid, tunnel_normals, sizeof(tunnel_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTunnel.cid, tunnel_colors, sizeof(tunnel_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlTunnel.iid, tunnel_indices, sizeof(tunnel_indices), GL_STATIC_DRAW);

    // ***** BIND DEAD *****
    esBind(GL_ARRAY_BUFFER, &mdlDead.vid, dead_vertices, sizeof(dead_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlDead.nid, dead_normals, sizeof(dead_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlDead.cid, dead_colors, sizeof(dead_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlDead.iid, dead_indices, sizeof(dead_indices), GL_STATIC_DRAW);
    
    // ***** BIND THREE *****
    esBind(GL_ARRAY_BUFFER, &mdlThree.vid, three_vertices, sizeof(three_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlThree.nid, three_normals, sizeof(three_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlThree.cid, three_colors, sizeof(three_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlThree.iid, three_indices, sizeof(three_indices), GL_STATIC_DRAW);

    // ***** BIND TWO *****
    esBind(GL_ARRAY_BUFFER, &mdlTwo.vid, two_vertices, sizeof(two_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTwo.nid, two_normals, sizeof(two_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTwo.cid, two_colors, sizeof(two_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlTwo.iid, two_indices, sizeof(two_indices), GL_STATIC_DRAW);

    // ***** BIND ONE *****
    esBind(GL_ARRAY_BUFFER, &mdlOne.vid, one_vertices, sizeof(one_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlOne.nid, one_normals, sizeof(one_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlOne.cid, one_colors, sizeof(one_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlOne.iid, one_indices, sizeof(one_indices), GL_STATIC_DRAW);

    // ***** BIND LSLICK *****
    esBind(GL_ARRAY_BUFFER, &mdlLslick.vid, lslick_vertices, sizeof(lslick_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLslick.nid, lslick_normals, sizeof(lslick_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLslick.cid, lslick_colors, sizeof(lslick_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlLslick.iid, lslick_indices, sizeof(lslick_indices), GL_STATIC_DRAW);

    // ***** BIND MSLICK *****
    esBind(GL_ARRAY_BUFFER, &mdlMslick.vid, mslick_vertices, sizeof(mslick_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMslick.nid, mslick_normals, sizeof(mslick_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMslick.cid, mslick_colors, sizeof(mslick_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMslick.iid, mslick_indices, sizeof(mslick_indices), GL_STATIC_DRAW);

    // ***** BIND RSLICK *****
    esBind(GL_ARRAY_BUFFER, &mdlRslick.vid, rslick_vertices, sizeof(rslick_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRslick.nid, rslick_normals, sizeof(rslick_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRslick.cid, rslick_colors, sizeof(rslick_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRslick.iid, rslick_indices, sizeof(rslick_indices), GL_STATIC_DRAW);

    // ***** BIND LGEM *****
    esBind(GL_ARRAY_BUFFER, &mdlLgem.vid, lgem_vertices, sizeof(lgem_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLgem.nid, lgem_normals, sizeof(lgem_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLgem.cid, lgem_colors, sizeof(lgem_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlLgem.iid, lgem_indices, sizeof(lgem_indices), GL_STATIC_DRAW);

    // ***** BIND MGEM *****
    esBind(GL_ARRAY_BUFFER, &mdlMgem.vid, mgem_vertices, sizeof(mgem_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMgem.nid, mgem_normals, sizeof(mgem_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMgem.cid, mgem_colors, sizeof(mgem_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMgem.iid, mgem_indices, sizeof(mgem_indices), GL_STATIC_DRAW);

    // ***** BIND RGEM *****
    esBind(GL_ARRAY_BUFFER, &mdlRgem.vid, rgem_vertices, sizeof(rgem_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRgem.nid, rgem_normals, sizeof(rgem_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRgem.cid, rgem_colors, sizeof(rgem_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRgem.iid, rgem_indices, sizeof(rgem_indices), GL_STATIC_DRAW);

    // ***** BIND LBOOST *****
    esBind(GL_ARRAY_BUFFER, &mdlLboost.vid, lboost_vertices, sizeof(lboost_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLboost.nid, lboost_normals, sizeof(lboost_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLboost.cid, lboost_colors, sizeof(lboost_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlLboost.iid, lboost_indices, sizeof(lboost_indices), GL_STATIC_DRAW);

    // ***** BIND MBOOST *****
    esBind(GL_ARRAY_BUFFER, &mdlMboost.vid, mboost_vertices, sizeof(mboost_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMboost.nid, mboost_normals, sizeof(mboost_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMboost.cid, mboost_colors, sizeof(mboost_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMboost.iid, mboost_indices, sizeof(mboost_indices), GL_STATIC_DRAW);

    // ***** BIND RBOOST *****
    esBind(GL_ARRAY_BUFFER, &mdlRboost.vid, rboost_vertices, sizeof(rboost_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRboost.nid, rboost_normals, sizeof(rboost_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRboost.cid, rboost_colors, sizeof(rboost_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRboost.iid, rboost_indices, sizeof(rboost_indices), GL_STATIC_DRAW);

    // ***** BIND LBOARDER *****
    esBind(GL_ARRAY_BUFFER, &mdlLboarder.vid, lboarder_vertices, sizeof(lboarder_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLboarder.nid, lboarder_normals, sizeof(lboarder_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLboarder.cid, lboarder_colors, sizeof(lboarder_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlLboarder.iid, lboarder_indices, sizeof(lboarder_indices), GL_STATIC_DRAW);

    // ***** BIND MBOARDER *****
    esBind(GL_ARRAY_BUFFER, &mdlMboarder.vid, mboarder_vertices, sizeof(mboarder_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMboarder.nid, mboarder_normals, sizeof(mboarder_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlMboarder.cid, mboarder_colors, sizeof(mboarder_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMboarder.iid, mboarder_indices, sizeof(mboarder_indices), GL_STATIC_DRAW);

    // ***** BIND RBOARDER *****
    esBind(GL_ARRAY_BUFFER, &mdlRboarder.vid, rboarder_vertices, sizeof(rboarder_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRboarder.nid, rboarder_normals, sizeof(rboarder_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRboarder.cid, rboarder_colors, sizeof(rboarder_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRboarder.iid, rboarder_indices, sizeof(rboarder_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader program
//*************************************
    makeLambert3();

//*************************************
// configure render options
//*************************************
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    doPerspective();

//*************************************
// execute update / render loop
//*************************************
    t = ((float)SDL_GetTicks())*0.001f;
    rt = t+3;

    while(1){main_loop();}
    return 0;
}

