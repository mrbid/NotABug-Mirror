/*
    James William Fletcher (github.com/mrbid)
        December 2021 - November 2023
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define uint GLushort
#define f32 GLfloat

#include "gl.h"
#define GLFW_INCLUDE_NONE
#include "glfw3.h"

#include "esAux2.h"
#include "res.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024, winh = 768;
double t = 0.0, x, y, rww, rwh, ww, wh, ww3, wh3;
f32 dt = 0.f, ft = 0.f, aspect;

// render state id's
GLint projection_id, modelview_id; // uniform matrices
GLint lightpos_id, color_id, opacity_id; // uniform floats
GLint position_id, normal_id; // attrib

// render/shader vars
mat projection, view, model, modelview;
const vec lightpos = {0.f, 0.f, 0.f};
ESModel mdlCube;

// game vars
#define NEWGAME_SEED 1337
#define FAR_DISTANCE 512.f
#define ARRAY_MAX 128
typedef struct
{
    uint health;
    vec pos;
    f32 r,g,b;
} gi;
gi arCu[ARRAY_MAX]; // enemy cube array
vec arBu[ARRAY_MAX];// bullet array
vec pp;
int score = 0;
uint hit = 0, miss = 0, enable_mouse = 1;

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
static inline f32 nsat(f32 f)
{
    if(f > 1.f){f = 1.f;}else if(f < 0.f){f = 0.f;}
    return f;
}

//*************************************
// render functions
//*************************************
void rCube(f32 x, f32 y, f32 z, f32 lightness, uint light_mode, f32 r, f32 g, f32 b)
{
    // transform
    mIdent(&model);
    mTranslate(&model, x, y, z);
    if(light_mode == 4)
    {
        const f32 s = 0.5f - (fabsf(z) * 0.0078125f); // fabsf(z) / 128.f
        mScale(&model, s, s, s);
        light_mode = 2;
    }
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    
    // compute lightness
    f32 lf = fabsf(sinf(ft+r+g+b)); // it's the `+r+g+b` causing the light strobe to de-synchronise from the cube color strobe, but meh it's a game not a simulation. I prefer it this way, the game actually has a very subtle uneasy feeling to it due to these mild oddities.
    if(lf < 0.3f){lf = 0.3f;}
    lightness /= 6.f * lf; // make this a multiplication?
    if(lightness > 2.f){lightness = 2.f;}
    lightness = 2.f-lightness;

    // shading
    if(light_mode == 0)
    {
        f32 r0 = nsat(sinf(x+ft*0.3f)), g0 = nsat(cosf(y+ft*0.3f)), b0 = nsat(cosf(z+ft*0.3f));
        if(r0 <= 0.f){r0 = 0.1f;} /**/ if(g0 <= 0.f){g0 = 0.1f;} /**/ if(b0 <= 0.f){b0 = 0.1f;}
        glUniform3f(color_id, r0 * 0.2f + (r0 * lightness), g0 * 0.2f + (g0 * lightness), b0 * 0.2f + (b0 * lightness));
    }
    else if(light_mode == 1){glUniform3f(color_id, r * lightness, g * lightness, b * lightness);}
    else if(light_mode == 2){glUniform3f(color_id, r, g, b);}

    // draw
    glDrawElements(GL_TRIANGLES, cube_numind, GL_UNSIGNED_BYTE, 0);
}

//*************************************
// game functions
//*************************************
void rndCube(uint i, f32 far)
{
    arCu[i].health = 1;
    arCu[i].pos = (vec){(1 + esRand(0, 14)) * 2, (1 + esRand(0, 8)) * 2, far, 1.f + randf()*19.f};
    arCu[i].r = randf(), arCu[i].g = randf(), arCu[i].b = randf();
}
void newGame()
{
    glfwSetWindowTitle(window, "!!! GAME START !!!");
    score = 0, hit = 0, miss = 0;
    pp = (vec){16.f, 4.f, -6.f};
    for(uint i = 0; i < ARRAY_MAX; i++){rndCube(i, esRandFloat(-64.f, -FAR_DISTANCE));}
    t = glfwGetTime();
}

//*************************************
// update & render
//*************************************
void main_loop()
{
    // dilate time
    t += sin(t * 0.05) * 10.0; // double

    // calc delta time
    static f32 lt = 0;
    ft = (f32)t;
    dt = ft-lt;
    lt = ft;

    // clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // simulate & render player
    const f32 red = nsat(1.f - ((f32)miss / (f32)hit)); // you are the health bar
    if(enable_mouse == 1) // mouse move
    {
        glfwGetCursorPos(window, &x, &y); // double
        x -= ww3;
        y -= wh3;
        x *= 3.0;
        y *= 3.0;
        x = 2.0  + (x * rww) *  30.0;
        y = 18.0 + (y * rwh) * -18.0;
        if(x < 2.0){x = 2.0;}else if(x > 30.0){x = 30.0;}
        if(y < 2.0){y = 2.0;}else if(y > 18.0){y = 18.0;}
        x = floor(x+0.5);
        y = floor(y+0.5);
        const double x2 = x*0.5;
        const double y2 = y*0.5;
        if(x2-floor(x2) != 0.0){x += 1.0;}
        if(y2-floor(y2) != 0.0){y += 1.0;}
        pp.x = x, pp.y = y;
    }
    rCube(pp.x, pp.y, pp.z, 0.f, 2, 1.f,red,0.f); // render
    static double lx = 0.f, ly = 0.f; // auto-shoot
    if(lx != x || ly != y)
    {
        for(uint i = 0; i < ARRAY_MAX; i++)
        {
            if(arBu[i].z == 0.f)
            {
                arBu[i].x = pp.x;
                arBu[i].y = pp.y;
                arBu[i].z = -8.f;
                break;
            }
        }
    }
    lx = x, ly = y;

    // simulate & render bullets
    for(uint i = 0; i < ARRAY_MAX; i++)
    {
        // is bullet alive
        if(arBu[i].z != 0.f)
        {
            arBu[i].z += -30.f * dt; // simulate bullet
            for(uint j = 0; j < ARRAY_MAX; j++) // check collisions with cubes
            {
                if(arCu[j].health == 0 || arCu[j].health == 1337){continue;}
                if( arBu[i].x == arCu[j].pos.x && 
                    arBu[i].y == arCu[j].pos.y &&
                    arBu[i].z < arCu[j].pos.z )
                {
                    arCu[j].health = 0;
                    arBu[i].z = 0.f;
                    score++, hit++;
                    char title[256];
                    if(miss == 0){sprintf(title, "PERFECT %u", hit);}
                    else{sprintf(title, "HIT %u - MISS %u - SCORE %i", hit, miss, score);}
                    glfwSetWindowTitle(window, title);
                }  
            }
            // check bullet death and draw
            if(arBu[i].z < -64.f){arBu[i].z = 0.f;}
            else if(arBu[i].z != 0.f){rCube(arBu[i].x, arBu[i].y, arBu[i].z, 0.f, 4, 1.f, 1.f, 0.f);}
        }
    }

    // simulate & render cubes
    for(uint i = 0; i < ARRAY_MAX; i++)
    {
        arCu[i].pos.z += arCu[i].pos.w * dt; // simulate
        if(arCu[i].pos.z > 20.f || arCu[i].pos.y < 2.f){rndCube(i, -FAR_DISTANCE);} // cube reset
        if(arCu[i].health == 0){arCu[i].pos.y -= arCu[i].pos.w * dt;} // cube death
        if(arCu[i].health == 1 && arCu[i].pos.z > -6.f) // cube got past you alive
        {
            arCu[i].health = 1337;
            score--, miss++;
            char title[256];
            if(miss == 1)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] PERFECT %u REACHED\n", strts, hit);
            }
            if(miss == 0){sprintf(title, "PERFECT %u", hit);}
            else{sprintf(title, "HIT %u - MISS %u - SCORE %i", hit, miss, score);}
            glfwSetWindowTitle(window, title);
        }
        // draw cube red if its past you, or original colour if not
        if(arCu[i].pos.z > -6.f && arCu[i].health != 0)
            rCube(arCu[i].pos.x, arCu[i].pos.y, arCu[i].pos.z, 3.f, 1, 1.f,0.f,0.f);
        else
            rCube(arCu[i].pos.x, arCu[i].pos.y, arCu[i].pos.z, 3.f, 1, arCu[i].r, arCu[i].g, arCu[i].b);
    }

    // draw grid [per wall](find nearest light source, render cube)
    for(uint x = 0; x < 17; x++)
    {
        for(uint y = 0; y < 32; y++)
        {
            const f32 fx =  2.0f * (f32)(x);
            const f32 fy = -2.0f * (f32)(y);
            f32 l = 60.f;                                       // there
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                f32 nl = vDist((vec){fx, 0.f, fy}, arCu[i].pos);
                if(nl < l){l = nl;}
                if(arBu[i].z != 0.f)
                {
                    nl = vDist((vec){fx, 0.f, fy}, arBu[i]);
                    if(nl < l){l = nl;}
                }
            }
            rCube(fx, 0.f, fy, l, 0, 0.f,0.f,0.f);
            l = 60.f;                                           // are
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                f32 nl = vDist((vec){fx, 20.f, fy}, arCu[i].pos);
                if(nl < l){l = nl;}
                if(arBu[i].z != 0.f)
                {
                    nl = vDist((vec){fx, 20.f, fy}, arBu[i]);
                    if(nl < l){l = nl;}
                }
            }
            rCube(fx, 20.f, fy, l, 0, 0.f,0.f,0.f);
            l = 60.f;                                           // four
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                f32 nl = vDist((vec){0.f, fx, fy}, arCu[i].pos);
                if(nl < l){l = nl;}
                if(arBu[i].z != 0.f)
                {
                    nl = vDist((vec){0.f, fx, fy}, arBu[i]);
                    if(nl < l){l = nl;}
                }
            }
            rCube(0.f, fx, fy, l, 0, 0.f,0.f,0.f);
            l = 60.f;                                           // walls
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                f32 nl = vDist((vec){32.f, fx, fy}, arCu[i].pos);
                if(nl < l){l = nl;}
                if(arBu[i].z != 0.f)
                {
                    nl = vDist((vec){32.f, fx, fy}, arBu[i]);
                    if(nl < l){l = nl;}
                }
            }
            rCube(32.f, fx, fy, l, 0, 0.f,0.f,0.f);
        }
    }

    // swap buffers / display render
    glfwSwapBuffers(window);
}

//*************************************
// input handelling
//*************************************
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action != GLFW_PRESS && action != GLFW_REPEAT){return;} // only presses and repeats
    if(key == GLFW_KEY_LEFT)        { pp.x -= 2.f; if(pp.x < 2.f ){pp.x = 2.f; } } // moveit!
    else if(key == GLFW_KEY_RIGHT)  { pp.x += 2.f; if(pp.x > 30.f){pp.x = 30.f;} }
    else if(key == GLFW_KEY_UP)     { pp.y += 2.f; if(pp.y > 18.f){pp.y = 18.f;} }
    else if(key == GLFW_KEY_DOWN)   { pp.y -= 2.f; if(pp.y < 2.f ){pp.y = 2.f; } }
    else if(key == GLFW_KEY_ESCAPE) // userland
    {
        enable_mouse = 0;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if(key == GLFW_KEY_SPACE) // fire bullet
    {
        for(uint i = 0; i < ARRAY_MAX; i++)
        {
            if(arBu[i].z == 0.f)
            {
                arBu[i].x = pp.x;
                arBu[i].y = pp.y;
                arBu[i].z = -8.f;
                break;
            }
        }
    }
    else if(key == GLFW_KEY_R) // random seed
    {
        char strts[16];
        timestamp(&strts[0]);
        printf("[%s] HIT %u - MISS %u - SCORE %i\n", strts, hit, miss, score);
        const uint nr = (uint)(t*100.0);
        srand(nr);
        srandf(nr);
        printf("[%s] New Game. Seed: %u\n", strts, nr);
        for(uint i = 0; i < ARRAY_MAX; i++){rndCube(i, esRandFloat(-64.f, -FAR_DISTANCE));}
        newGame();
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action != GLFW_PRESS){return;} // only presses
    enable_mouse = 1 - enable_mouse;
    if(enable_mouse == 1){glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);}
    else{glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);}
}
void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width, winh = height;
    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw, wh = winh;
    ww3 = ww*0.333333333, wh3 = wh*0.333333333; // / 3.0
    rww = 1.0/ww, rwh = 1.0/wh;
    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 1.0f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
}

//*************************************
// process entry point
//*************************************
int main(int argc, char** argv)
{
    // help
    printf("Cube Shooter\n");
    printf("James William Fletcher (github.com/mrbid)\n\n");
    printf("Just move around your mouse - OR - you can use your keyboard,\n");
    printf("just click to toggle between mouse and keyboard control.\n");
    printf("KEYBOARD: arrow keys to move and spacebar to shoot.\n\n");
    printf("Press 'R' to reset the game.\n\n");

    // init glfw
    if(!glfwInit()){exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 16);
    window = glfwCreateWindow(winw, winh, "Cube Shooter", NULL, NULL);
    if(!window){glfwTerminate(); exit(EXIT_FAILURE);}
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop (wont work on wayland)
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // configure render pipeline
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    makeLambert1();
    shadeLambert1(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    mIdent(&view);
    mTranslate(&view, -16.f, -9.f, -15.f);
    esBind(GL_ARRAY_BUFFER, &mdlCube.vid, cube_vertices, sizeof(cube_vertices), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);
    esBind(GL_ARRAY_BUFFER, &mdlCube.nid, cube_normals, sizeof(cube_normals), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mdlCube.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlCube.iid, cube_indices, sizeof(cube_indices), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlCube.iid);
    window_size_callback(window, winw, winh); // set projection

    // new game and loop
    srand(NEWGAME_SEED);
    newGame();
    while(glfwWindowShouldClose(window) == 0)
    {
        t = glfwGetTime();
        glfwPollEvents();
        main_loop();
    }

    // final score, quit
    char strts[16];
    timestamp(&strts[0]);
    printf("[%s] HIT %u - MISS %u - SCORE %i\n\n", strts, hit, miss, score);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
