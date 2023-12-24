/*
--------------------------------------------------
    James William Fletcher (github.com/mrbid)
        February 2023
--------------------------------------------------
    C & SDL / OpenGL ES2 / GLSL ES
    Colour Converter: https://www.easyrgb.com
*/

#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include "esAux4.h"

#include "assets/res.h"
#include "assets/scene.h"
#include "assets/dynamic.h"
#include "assets/minball.h"

#define uint GLuint
#define sint GLint
#define f32 GLfloat
#define forceinline __attribute__((always_inline)) inline

//*************************************
// globals
//*************************************
char appTitle[] = "Snowling";
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Surface* s_icon = NULL;
Uint32 winw = 1024, winh = 768;
f32 aspect, t = 0.f;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;
GLint texcoord_id;
GLint sampler_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// render state inputs
vec lightpos = {0.f, 7.f, 0.f};

// models
ESModel mdlPlane;
    GLuint tex_skyplane;
ESModel mdlScene;
ESModel mdlDynamic;
ESModel mdlMinball;
uint bindstate = 0;

// simulation / game vars
uint ground = 0;        // current round number
uint score = 0;         // total score
uint rscore = 0;        // round score
uint penalty = 0;       // penalty count
uint state = 0;         // game state
double s0lt = 0;        // time offset controls ball swing offset
vec bp;                 // ball position
f32 stepspeed = 0.f;    // ball speed
f32 hardness = 0.f;     // ball hardness

//*************************************
// utility functions
//*************************************
void timestamp(char* ts){const time_t tt = time(0);strftime(ts, 16, "%H:%M:%S", localtime(&tt));}
forceinline f32 f32Time(){return ((f32)SDL_GetTicks())*0.001f;}

//*************************************
// gl functions
//*************************************
void doPerspective()
{
    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.1f, 80.f);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
}

//*************************************
// generation functions
//*************************************
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    memcpy(s->pixels, data, s->pitch*h);
    return s;
}

void blotColour2(f32 r, f32 g, f32 b, f32 rad)
{
    const GLushort rci = esRand(0, dynamic_numvert-1) * 3;
    const vec tv = (vec){dynamic_vertices[rci], dynamic_vertices[rci+1], dynamic_vertices[rci+2]};

    if(tv.x > 9.6f) // not too close to spawn
        return;

    const GLushort tc = (dynamic_numvert-1)*3;
    for(GLushort i = 0; i < tc; i += 3)
    {
        const vec nv = (vec){dynamic_vertices[i], dynamic_vertices[i+1], dynamic_vertices[i+2]};
        if(vDist(tv, nv) < rad)
        {
            dynamic_colors[i] = r;
            dynamic_colors[i+1] = g;
            dynamic_colors[i+2] = b;
        }
    }
}

void blotColour(f32 r, f32 g, f32 b, GLushort streak)
{
    const GLushort rci = esRand(0, dynamic_numvert-1-streak) * 3;
    for(GLushort i = 0; i < streak; i++)
    {
        dynamic_colors[rci+(i*3)] = r;
        dynamic_colors[rci+(i*3)+1] = g;
        dynamic_colors[rci+(i*3)+2] = b;
    }
}

void gNewRound()
{
    // count & log
    ground++;
    char strts[16];
    timestamp(&strts[0]);
    printf("[%s] STARTING ROUND %u - SCORE %u - PENALTY %u\n", strts, ground, score, penalty);
    if(ground > 1)
    {
        char title[256];
        sprintf(title, "ROUND %u - SCORE %u - PENALTY %u", ground, score, penalty);
        SDL_SetWindowTitle(wnd, title);
    }

    // reset
    const GLushort tc = dynamic_numvert*3;
    for(GLushort i = 0; i < tc; i++)
        dynamic_colors[i] = 1.f;

    // ice
    for(uint i = 0; i < 6; i++)
    {
        if(esRand(0, 1000) < 500)
            blotColour(0.f, 1.f, 1.f, 32);
        else
            blotColour2(0.f, 1.f, 1.f, esRandFloat(0.1f, 0.9f));
    }

    // boost
    for(uint i = 0; i < 6; i++)
    {
        if(esRand(0, 1000) < 500)
            blotColour(0.50196f, 0.00000f, 0.50196f, 6);
        else
            blotColour2(0.50196f, 0.00000f, 0.50196f, esRandFloat(0.1f, 0.3f));
    }

    // lava
    for(uint i = 0; i < 2; i++)
    {
        if(esRand(0, 1000) < 500)
            blotColour(0.81176f, 0.06275f, 0.12549f, 6);
        else
            blotColour2(0.81176f, 0.06275f, 0.12549f, esRandFloat(0.1f, 0.3f));
    }

    // rebind
    glBindBuffer(GL_ARRAY_BUFFER, mdlDynamic.cid);
    glBufferData(GL_ARRAY_BUFFER, sizeof(dynamic_colors), dynamic_colors, GL_STATIC_DRAW);
}

uint checkCollisions(const f32 bpy)
{
    // static double ccl = 0;
    // if(t < ccl)
    //     return 0;
    // else
    //     ccl = t + 0.05; // limit checkCollisions() execution frequency
    vec lbp = bp;
    lbp.z -= bpy;

    static double tt1 = 0, tt2 = 0, tt3 = 0;
    static const GLushort tc = (dynamic_numvert-1)*3;
    for(GLushort i = 0; i < tc; i+=3)
    {
        if(t > tt1 && dynamic_colors[i] == 0.f && dynamic_colors[i+1] == 1.f && dynamic_colors[i+2] == 1.f)
        {
            const vec cp = {dynamic_vertices[i], dynamic_vertices[i+1], dynamic_vertices[i+2]};
            if(vDistSq(cp, lbp) < 0.0169f)
            {
                hardness += 1.f;
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] hit ice: %u\n", strts, (uint)hardness);
                tt1 = t+0.3; // freq limiter
                return 1;
            }
        }
        else if(t > tt2 && dynamic_colors[i] == 0.50196f && dynamic_colors[i+1] == 0.f && dynamic_colors[i+2] == 0.50196f)
        {
            const vec cp = {dynamic_vertices[i], dynamic_vertices[i+1], dynamic_vertices[i+2]};
            if(vDistSq(cp, lbp) < 0.0169f)
            {
                stepspeed *= 3.4f;
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] hit boost: %.1f\n", strts, stepspeed);
                tt2 = t+0.3; // freq limiter
                return 2;
            }
        }
        else if(t > tt3 && dynamic_colors[i] == 0.81176f && dynamic_colors[i+1] == 0.06275f && dynamic_colors[i+2] == 0.12549f)
        {
            const vec cp = {dynamic_vertices[i], dynamic_vertices[i+1], dynamic_vertices[i+2]};
            if(vDistSq(cp, lbp) < 0.0169f)
            {
                penalty++;
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] hit lava: %u\n", strts, penalty);
                tt3 = t+0.3; // freq limiter
                return 3;
            }
        }
    }
    
    return 0;
}

//*************************************
// render functions
//*************************************
void rSkyPlane()
{
    bindstate = 0;

    static mat skyplane_model = {0.f};
    static f32 la = 0;
    if(skyplane_model.m[0][0] == 0.f || la != aspect)
    {
        mIdent(&skyplane_model);
        mSetPos(&skyplane_model, (vec){-40.f, 0.f, 0.f});
        mRotY(&skyplane_model, -90.f*DEG2RAD);
        mRotX(&skyplane_model, -90.f*DEG2RAD);
        mScale(&skyplane_model, 40.f*aspect, 23.f*aspect, 0);
        la = aspect;
    }

    mMul(&modelview, &skyplane_model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPlane.tid);
    glVertexAttribPointer(texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(texcoord_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_skyplane);
    glUniform1i(sampler_id, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPlane.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPlane.iid);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void rStaticScene()
{
    bindstate = 0;

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);

    glBindBuffer(GL_ARRAY_BUFFER, mdlScene.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlScene.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlScene.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlScene.iid);

    glDrawElements(GL_TRIANGLES, scene_numind, GL_UNSIGNED_SHORT, 0);
}

void rDynamicScene()
{
    bindstate = 0;

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);

    glBindBuffer(GL_ARRAY_BUFFER, mdlDynamic.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlDynamic.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlDynamic.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlDynamic.iid);

    glDrawElements(GL_TRIANGLES, dynamic_numind, GL_UNSIGNED_SHORT, 0);
}

void rMinballRGB(f32 x, f32 y, f32 z, f32 r, f32 g, f32 b, f32 s)
{
    mIdent(&model);
    mSetPos(&model, (vec){x, y, z});
    if(s != 1.f)
        mScale(&model, s, s, s);
    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(color_id, r, g, b);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);

    if(bindstate == 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlMinball.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMinball.iid);
        bindstate = 1;
    }

    glDrawElements(GL_TRIANGLES, minball_numind, GL_UNSIGNED_SHORT, 0);
}

static inline void rMinball(f32 x, f32 y, f32 z, f32 s)
{
    rMinballRGB(x, y, z, 1.f, 1.f, 1.f, s);
}

void rPin(f32 x, f32 y, f32 z, uint down)
{
    if(down == 1)
    {
        rMinballRGB(x, y, z, 1.f, 0.f, 0.f, 2.2f);
    }
    else
    {
        rMinball(x, y, z, 2.2f);
        rMinball(x, y, z+0.14f, 1.45f);
        rMinball(x, y, z+0.24f, 0.9f);
    }
}

void rPinSet()
{
    rPin(-0.9, 0.f, 0.f, rscore >= 1 ? 1 : 0);

    rPin(-1.2, -0.14f, 0.f, rscore >= 2 ? 1 : 0);
    rPin(-1.2, 0.14f, 0.f, rscore >= 3 ? 1 : 0);

    rPin(-1.5, 0.f, 0.f, rscore >= 4 ? 1 : 0);
    rPin(-1.5, -0.28f, 0.f, rscore >= 5 ? 1 : 0);
    rPin(-1.5, 0.28f, 0.f, rscore >= 6 ? 1 : 0);

    rPin(-1.8, -0.14f, 0.f, rscore >= 7 ? 1 : 0);
    rPin(-1.8, 0.14f, 0.f, rscore >= 8 ? 1 : 0);
    rPin(-1.8, -0.42f, 0.f, rscore >= 9 ? 1 : 0);
    rPin(-1.8, 0.42f, 0.f, rscore >= 10 ? 1 : 0);
}

//*************************************
// interpolators and steppers for simulation
//*************************************
typedef struct { f32 y,z; } lt;
static inline f32 lerp(lt* a, lt* b, f32 y)
{
    return a->z + (((y - a->y) / (b->y - a->y)) * (b->z - a->z));
}

lt hlt[25];
f32 getHeight(f32 y)
{
    y = fabs(y);

    for(uint i = 0; i < 24; i++)
        if(y >= hlt[i].y && y < hlt[i+1].y)
            return lerp(&hlt[i], &hlt[i+1], y);

    return -1.f; // bad times if this happens
}

static inline f32 smoothStepN(f32 v)
{
    return v * v * (3.f - 2.f * v);
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
    t = f32Time();
    const f32 dt = t-lt;
    lt = t;

//*************************************
// input handling
//*************************************
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_FINGERDOWN)
        {
            if(stepspeed == 0.f)
            {
                stepspeed = 0.5f + randf()*6.f;
                s0lt = t - randf()*x2PI;
            }
        }
        else if(event.type == SDL_KEYDOWN)
        {
            if(stepspeed == 0.f)
            {
                stepspeed = 0.5f + randf()*6.f;
                s0lt = t - randf()*x2PI;
            }
        }
        else if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            if(stepspeed == 0.f)
            {
                stepspeed = 0.5f + randf()*6.f;
                s0lt = t - randf()*x2PI;
            }
        }
        else if(event.type == SDL_QUIT)
        {
            SDL_GL_DeleteContext(glc);
            SDL_FreeSurface(s_icon);
            SDL_DestroyWindow(wnd);
            SDL_Quit();
            exit(0);
        }
        else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            winw = event.window.data1;
            winh = event.window.data2;
            doPerspective();
        }
    }

//*************************************
// camera control
//*************************************
    static f32 camdist = -15.f;
    mIdent(&view);
    mSetPos(&view, (vec){0.f, -0.5f, camdist});
    mRotY(&view, 1.396263361f);
    mRotZ(&view, 1.570796371f);

//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

    // render sky plane
    shadeFullbrightT(&position_id, &projection_id, &modelview_id, &texcoord_id, &sampler_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
    rSkyPlane();
    
    // render static and dynamic scenes
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    rStaticScene();
    rDynamicScene();

    // only rendering icospheres from here on out
    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    rPinSet();

    // simulate the bowling snowball & transition the game states
    static double s1lt = 0;
    static f32 x = 10.5f;
    static f32 ms = -1.f;

    if(state == 0) // bowl simulation state
    {
        if(stepspeed == 0.f)
        {
            rMinball(10.5f, 0.f-0.017f, 0.f, 1.f);
        }
        else
        {
            const f32 ddt = stepspeed * dt;
            x -= ddt;
            camdist += ddt;
            if(x <= -1.f)
                state = 1;

            const f32 h = sinf(t-s0lt)*(1.38f-((10.5f-x)*0.1f));

            f32 ns = (10.5f-x)*0.4f;
            if(ns < 1.f)
                ns = 1.f;

            lightpos.y = 7.f * smoothStepN(x*0.09523809701f);

            bp.x = x;
            bp.y = h-0.017f;
            const f32 ho = ((1.f-(bp.x*0.09523809701f))*0.08f);
            bp.z = getHeight(h) + ho;
            rMinballRGB(bp.x, bp.y, bp.z, 1.f-(hardness*0.22f), 1.f, 1.f, ns);

            if(checkCollisions(ho) == 3)
                state = 3;
        }
    }
    else if(state == 1) // pins knocked down freeze state
    {
        if(s1lt == 0)
        {
            // calc score
            if(hardness > 0.f)
            {
                const f32 fscore = ((hardness * stepspeed) * 0.3f)+0.5f;
                rscore = (uint)fscore;
                if(rscore == 0)
                {
                    char strts[16];
                    timestamp(&strts[0]);
                    printf("[%s] ~~ PENALTY YOU FAILED TO KNOCK DOWN ANY PINS ~~\n", strts);
                    penalty++;
                    s1lt = t + 1;
                }
                else if(rscore >= 10)
                {
                    rscore = 20;
                    char strts[16];
                    timestamp(&strts[0]);
                    printf("[%s] !!! STRIKE !!! - ROUND %u - SCORE %u\n", strts, ground, rscore);
                    s1lt = t + 3;
                }
                else
                {
                    char strts[16];
                    timestamp(&strts[0]);
                    printf("[%s] ~~ ROUND %u SCORE %u ~~\n", strts, ground, rscore);
                    s1lt = t + 1;
                }
                score += rscore;
            }
            else
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] ~~ PENALTY YOU FAILED TO KNOCK DOWN ANY PINS ~~\n", strts);
                penalty++;
                s1lt = t + 1;
            }
        }
        if(t > s1lt)
            state = 2;
    }
    else if(state == 2) // new round state
    {
        // reset
        lightpos.y = 7.f;
        x = 10.5f;
        camdist = -15.f;
        stepspeed = 0.f;
        hardness = 0.f;
        state = 0;
        s1lt = 0;
        rscore = 0;
        ms = -1.f;
        gNewRound();
    }
    else if(state == 3) // hit lava, melting state (worst animation ever)
    {
        static f32 rsms = 0.f;
        if(ms == -1.f)
        {
            ms = (10.5f-x)*0.4f;
            rsms = 1.f/ms;
            if(ms < 1.f)
                ms = 1.f;
        }

        ms -= 1.f * dt;
        if(ms <= 0.f)
            state = 2;

        bp.z -= 0.075f * dt;

        rMinballRGB(bp.x, bp.y, bp.z, 1.f, 1.f - (1.f-ms*rsms), 1.f - (1.f-ms*rsms), ms);
    }

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
//*************************************
// setup render context / window
//*************************************
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}
    printf("James William Fletcher (github.com/mrbid)\nIt's like a fruit machine, random chance, you click or press the \"any key\" and it bowls a random bowl - a game of chance/luck.\n\nThe alpine background image was taken by Karl Köhler (https://unsplash.com/photos/landscape-photo-of-snowy-mountain-N_MXyBUV5hU).\n\n");
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
    srandf(time(0));
    srand(time(0));

    // gen height table
    hlt[0].y = 0;         hlt[0].z = 0.016813;
    hlt[1].y = 0.102108;  hlt[1].z = 0.018762;
    hlt[2].y = 0.191487;  hlt[2].z = 0.024604;
    hlt[3].y = 0.280097;  hlt[3].z = 0.034311;
    hlt[4].y = 0.367561;  hlt[4].z = 0.047843;
    hlt[5].y = 0.453504;  hlt[5].z = 0.065142;
    hlt[6].y = 0.537558;  hlt[6].z = 0.086133;
    hlt[7].y = 0.619362;  hlt[7].z = 0.110728;
    hlt[8].y = 0.698569;  hlt[8].z = 0.138821;
    hlt[9].y = 0.774836;  hlt[9].z = 0.170289;
    hlt[10].y = 0.847838; hlt[10].z = 0.205001;
    hlt[11].y = 0.917262; hlt[11].z = 0.242807;
    hlt[12].y = 0.982812; hlt[12].z = 0.283543;
    hlt[13].y = 1.04421;  hlt[13].z = 0.327039;
    hlt[14].y = 1.10118;  hlt[14].z = 0.373106;
    hlt[15].y = 1.15349;  hlt[15].z = 0.421545;
    hlt[16].y = 1.20092;  hlt[16].z = 0.472152;
    hlt[17].y = 1.24325;  hlt[17].z = 0.524709;
    hlt[18].y = 1.28032;  hlt[18].z = 0.57899;
    hlt[19].y = 1.31195;  hlt[19].z = 0.634764;
    hlt[20].y = 1.33803;  hlt[20].z = 0.69179;
    hlt[21].y = 1.35843;  hlt[21].z = 0.749827;
    hlt[22].y = 1.37305;  hlt[22].z = 0.808624;
    hlt[23].y = 1.38185;  hlt[23].z = 0.867931;
    hlt[24].y = 1.38479;  hlt[24].z = 0.927491;

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND SKY PLANE *****
    f32  plane_vert[] = {1.000000,-1.000000,0.000000,
                        -1.000000,1.000000,0.000000,
                        -1.000000,-1.000000,0.000000,
                         1.000000,1.000000,0.000000};
    GLushort plane_indi[] = {0,1,2,0,3,1};
    esBindModel(&mdlPlane, plane_vert, 9, plane_indi, 6);
    f32 plane_texc[] = {1.f,1.f,
                        0.f,0.f,
                        0.f,1.f,
                        1.f,0.f};
    esBind(GL_ARRAY_BUFFER, &mdlPlane.tid, plane_texc, sizeof(plane_texc), GL_STATIC_DRAW);
    tex_skyplane = esLoadTexture(1469, 981, &alpinebg_tex[0], 1);

    // ***** BIND STATIC SCENE *****
    esBind(GL_ARRAY_BUFFER, &mdlScene.vid, scene_vertices, sizeof(scene_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlScene.nid, scene_normals, sizeof(scene_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlScene.cid, scene_colors, sizeof(scene_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlScene.iid, scene_indices, sizeof(scene_indices), GL_STATIC_DRAW);

    // ***** BIND DYNAMIC SCENE *****
    esBind(GL_ARRAY_BUFFER, &mdlDynamic.vid, dynamic_vertices, sizeof(dynamic_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlDynamic.nid, dynamic_normals, sizeof(dynamic_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlDynamic.cid, dynamic_colors, sizeof(dynamic_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlDynamic.iid, dynamic_indices, sizeof(dynamic_indices), GL_STATIC_DRAW);

    // ***** BIND MIN BALL *****
    esBind(GL_ARRAY_BUFFER, &mdlMinball.vid, minball_vertices, sizeof(minball_vertices), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMinball.iid, minball_indices, sizeof(minball_indices), GL_STATIC_DRAW);

//*************************************
// projection
//*************************************
    doPerspective();

//*************************************
// compile & link shader program
//*************************************
    makeFullbrightT();
    makeLambert();
    makeLambert3();

//*************************************
// configure render options
//*************************************
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.20000f, 0.34510f, 0.48627f, 0.0f);

//*************************************
// execute update / render loop
//*************************************
    gNewRound();
    while(1){main_loop();}
    return 0;
}
