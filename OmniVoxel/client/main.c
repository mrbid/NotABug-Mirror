/*
--------------------------------------------------
    James William Fletcher (github.com/mrbid)
        July 2023
--------------------------------------------------
    C & SDL / OpenGL ES2 / GLSL ES
    Colour Converter: https://www.easyrgb.com

    VoxelPainter / OmniVoxel / Space game thing
*/

//#define NO_COMPRESSION
#define VERBOSE

#include <time.h>
#ifndef NO_COMPRESSION
    #include <zlib.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#ifdef __linux__
    #include <sys/time.h>
    #include <locale.h>
#endif

#include "inc/esVoxel.h"
#include "assets.h"

#define uint GLuint
#define sint GLint
#define uchar unsigned char
#define forceinline __attribute__((always_inline)) inline

//*************************************
// globals
//*************************************
const char appTitle[] = "OmniVoxel";
char *basedir, *appdir;
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Surface* s_icon = NULL;
int winw = 1024, winh = 768;
int winw2 = 512, winh2 = 384;
float ww, wh;
float aspect, t = 0.f;
uint ks[10] = {0};      // keystate
uint focus_mouse = 0;   // mouse lock
float ddist = 4096.f;   // view distance
float ddist2 = 4096.f*4096.f;
vec ipp;                // inverse player position
vec look_dir;           // camera look direction
int lray = 0;           // pointed at node index
float ptt = 0.f;        // place timing trigger (for repeat place)
float dtt = 0.f;        // delete timing trigger (for repeat delete)
uint fks = 0;           // F-Key state (fast mode toggle)
void drawHud();

//*************************************
// utility functions
//*************************************
void timestamp(char* ts){const time_t tt = time(0);strftime(ts, 16, "%H:%M:%S", localtime(&tt));}
forceinline float fTime(){return ((float)SDL_GetTicks())*0.001f;}
#ifdef __linux__
    uint64_t microtime()
    {
        struct timeval tv;
        struct timezone tz;
        memset(&tz, 0, sizeof(struct timezone));
        gettimeofday(&tv, &tz);
        return 1000000 * tv.tv_sec + tv.tv_usec;
    }
#endif

//*************************************
// game state functions
//*************************************

// game data (for fast save and load)
#define max_voxels 64000
//#define header_size 68
#define header_size (sizeof(g) - sizeof(g.voxels))
//#define max_data_size 1024068 // (max_voxels*sizeof(vec)) + header_size
#define max_data_size sizeof(g)

typedef struct
{
    vec pp;     // player position
    vec pb;     // place block pos
    float sens; // mouse sensitivity
    float xrot; // camera x-axis rotation
    float yrot; // camera y-axis rotation
    float st;   // selected texture
    float ms;   // player move speed
    float cms;  // custom move speed (high)
    float lms;  // custom move speed (low)
    uchar grav; // gravity on/off toggle
    uchar plock;// pitchlock on/off toggle
    uchar r1, r2;
    uint num_voxels;
    vec voxels[max_voxels]; // x,y,z,w (w = texture_id)
}
game_state;
game_state g;

int swapfunc(const void *a, const void *b)
{
    const float aw = ((vec*)a)->w;
    const float bw = ((vec*)b)->w;
    if(aw == bw){return 0;}
    else if(aw == -1.f){return 1;}
    return 0;
}
void optimState()
{
    qsort(g.voxels, g.num_voxels, sizeof(vec), swapfunc);
    for(uint i = 0; i < g.num_voxels; i++)
    {
        if(g.voxels[i].w == -1.f)
        {
            g.num_voxels = i;
            break;
        }
    }
}

void defaultState(const uint type)
{
    g.sens = 0.003f;
    g.xrot = 0.f;
    g.yrot = 1.5f;
    g.pp = (vec){0.f, 4.f, 0.f};
    if(type == 0)
    {
        g.ms = 9.3f;
    }
    g.st = 10.f;
    g.pb = (vec){0.f, 0.f, 0.f, -1.f};
    if(type == 0)
    {
        g.lms = g.ms;
        g.cms = g.ms*2.f;
    }
    g.grav = 0;
    if(g.num_voxels == 0){g.num_voxels = 1;}
}

uint loadUncompressedState()
{
#ifdef __linux__
    setlocale(LC_NUMERIC, "");
    const uint64_t st = microtime();
#endif
    char file[256];
    sprintf(file, "%sworld.db2", appdir);
    FILE* f = fopen(file, "rb");
    if(f != NULL)
    {
        fseek(f, 0, SEEK_END);
        long rs = ftell(f);
        fseek(f, 0, SEEK_SET);
        if(fread(&g, 1, rs, f) != rs)
        {
            char tmp[16];
            timestamp(tmp);
            printf("[%s] fread() was of an unexpected size.\n", tmp);
        }
        fclose(f);
        fks = (g.ms == g.cms); // update F-Key State
        char tmp[16];
        timestamp(tmp);
#ifndef __linux__
        printf("[%s] Loaded %u voxels.\n", tmp, g.num_voxels);
#else
        printf("[%s] Loaded %'u voxels. (%'lu μs)\n", tmp, g.num_voxels, microtime()-st);
#endif
        return 1;
    }
    return 0;
}

#ifdef NO_COMPRESSION
    void saveState()
    {
        optimState();
    #ifdef __linux__
        setlocale(LC_NUMERIC, "");
        const uint64_t st = microtime();
    #endif
        char file[256];
        sprintf(file, "%sworld.db2", appdir);
        FILE* f = fopen(file, "wb");
        if(f != NULL)
        {
            const size_t ws = header_size + (g.num_voxels*sizeof(vec));
            if(fwrite(&g, 1, ws, f) != ws)
            {
                char tmp[16];
                timestamp(tmp);
                printf("[%s] Save corrupted.\n", tmp);
            }
            fclose(f);
            char tmp[16];
            timestamp(tmp);
    #ifndef __linux__
            printf("[%s] Saved %u voxels.\n", tmp, g.num_voxels);
    #else
            printf("[%s] Saved %'u voxels.  (%'lu μs)\n", tmp, g.num_voxels, microtime()-st);
    #endif
        }
    }

    uint loadState(){return loadUncompressedState();}
#else
    void saveState()
    {
        optimState();
    #ifdef __linux__
        setlocale(LC_NUMERIC, "");
        const uint64_t st = microtime();
    #endif
        char file[256];
        sprintf(file, "%sworld.gz", appdir);
        gzFile f = gzopen(file, "wb1h");
        if(f != Z_NULL)
        {
            const size_t ws = header_size + (g.num_voxels*sizeof(vec));
            if(gzwrite(f, &g, ws) != ws)
            {
                char tmp[16];
                timestamp(tmp);
                printf("[%s] Save corrupted.\n", tmp);
            }
            gzclose(f);
            char tmp[16];
            timestamp(tmp);
    #ifndef __linux__
            printf("[%s] Saved %'u voxels.\n", tmp, g.num_voxels);
    #else
            printf("[%s] Saved %'u voxels.  (%'lu μs)\n", tmp, g.num_voxels, microtime()-st);
    #endif
        }
    }

    uint loadState()
    {
    #ifdef __linux__
        setlocale(LC_NUMERIC, "");
        const uint64_t st = microtime();
    #endif
        char file[256];
        sprintf(file, "%sworld.gz", appdir);
        gzFile f = gzopen(file, "rb");
        if(f != Z_NULL)
        {
            int gr = gzread(f, &g, max_data_size);
            gzclose(f);
            fks = (g.ms == g.cms); // update F-Key State
            char tmp[16];
            timestamp(tmp);
    #ifndef __linux__
            printf("[%s] Loaded %u voxels\n", tmp, g.num_voxels);
    #else
            printf("[%s] Loaded %'u voxels. (%'lu μs)\n", tmp, g.num_voxels, microtime()-st);
    #endif
            return 1;
        }
        return 0;
    }
#endif

//*************************************
// ship state
//*************************************

#define uSS float
typedef struct
{
    // this is how much force the ship can thrust in each axis (drain rate per tick?)
    // thats for each face so 6 axis plus and minus
    // electric ion thrust
    uSS pxtf, mxtf;
    uSS pytf, mytf;
    uSS pztf, mztf;
    // fuel thrust
    uSS pxftf, mxftf;
    uSS pyftf, myftf;
    uSS pzftf, mzftf;
    // xenon thrust
    uSS pxxf, mxxf;
    uSS pyxf, myxf;
    uSS pzxf, mzxf;

    // this is how much force the ship can fire in each axis (damage rate per tick?)
    // this is also the fuel burn rate in each axis
    // railgun
    uSS pxrff, mxrff;
    uSS pyrff, myrff;
    uSS pzrff, mzrff;
    // laser
    uSS pxlff, mxlff;
    uSS pylff, mylff;
    uSS pzlff, mzlff;

    // this is how much suction rate/force the ship has in each axis
    uSS pxsf, mxsf;
    uSS pysf, mysf;
    uSS pzsf, mzsf;
    
    // rates / per-tickers
    uSS power;  // electric regeneration rate per tick
    uSS cool;   // heat dissipation rate per tick
    uSS crypto; // crypto currency generation rate per tick
    uSS fgen;   // fuel refine rate per tick
    uSS xgen;   // xenon refine rate per tick
    uSS ngen;   // nuclear refine rate per tick

    // maximum capacities
    uSS store;  // storage capacity
    uSS fstore; // fuel storage capacity
    uSS estore; // electric storage capacity
    uSS xstore; // xenon storage capacity
    uSS hstore; // heat/thermal storage capacity
    uSS nstore; // nuclear storage capacity

    uSS drain;   // power usage rate per tick
    uSS heat;    // ambient heat generation rate per tick
    uSS maxheat; // maximum heat the ship can produce at once
    uSS maxelec; // maximum electric the ship can use at once
    uSS maxfuel; // maximum fuel the ship can use at once
    uSS maxxenon;// maximum xenon the ship can use at once

    // current/active state tracking
    // uSS temp; // ships current temperature
    // uSS scrap_ore; // mined resource/asteroid ["scraps" stored]
    // uSS uranium_ore; // mined resource/asteroid ["scraps" stored]
    // uSS fuel_ore; // mined resource/asteroid ["scraps" stored]
    // uSS fuel; // liquid gas fuel [refined and stored]
    // uSS rods; // uranium fuel [refined and stored]
    // uSS elec; // current amount of stored electricity
}
ship_state;
ship_state s;

// tells you which nodes by texture id are connected to which faces of vi
void getFaces(float* px, float* mx, float* py, float* my, float* pz, float* mz, const uint vi)
{
    *px = -1.f, *mx = -1.f, *py = -1.f, *my = -1.f, *pz = -1.f, *mz = -1.f;
    for(uint i = 0; i < g.num_voxels; i++)
    {
        if(g.voxels[i].w < 0.f){continue;}
        if(      g.voxels[i].x == g.voxels[vi].x-1 &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z )  {*mx = g.voxels[vi].w;}
        else if( g.voxels[i].x == g.voxels[vi].x+1 &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z )  {*px = g.voxels[vi].w;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y-1 &&
                 g.voxels[i].z == g.voxels[vi].z )  {*my = g.voxels[vi].w;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y+1 &&
                 g.voxels[i].z == g.voxels[vi].z )  {*py = g.voxels[vi].w;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z-1 ){*mz = g.voxels[vi].w;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z+1 ){*pz = g.voxels[vi].w;}
    }
    //
}
// tells you which face axis is free (not facing another voxel)
void freeFaces(Uint8* px, Uint8* mx, Uint8* py, Uint8* my, Uint8* pz, Uint8* mz, const uint vi)
{
    *px = 1, *mx = 1, *py = 1, *my = 1, *pz = 1, *mz = 1;
    for(uint i = 0; i < g.num_voxels; i++)
    {
        if(g.voxels[i].w < 0.f){continue;}
        if(      g.voxels[i].x == g.voxels[vi].x-1 &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z )  {*mx = 0;}
        else if( g.voxels[i].x == g.voxels[vi].x+1 &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z )  {*px = 0;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y-1 &&
                 g.voxels[i].z == g.voxels[vi].z )  {*my = 0;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y+1 &&
                 g.voxels[i].z == g.voxels[vi].z )  {*py = 0;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z-1 ){*mz = 0;}
        else if( g.voxels[i].x == g.voxels[vi].x   &&
                 g.voxels[i].y == g.voxels[vi].y   &&
                 g.voxels[i].z == g.voxels[vi].z+1 ){*pz = 0;}
    }
    //
}
// plus x for each free face
Uint8 incrementFreeFaces(uSS* px, uSS* mx, uSS* py, uSS* my, uSS* pz, uSS* mz, const uSS x, const uint vi)
{
    Uint8 tpx, tmx, tpy, tmy, tpz, tmz;
    freeFaces(&tpx, &tmx, &tpy, &tmy, &tpz, &tmz, vi);
    if(tpx == 1){*px += x;}
    if(tmx == 1){*mx += x;}
    if(tpy == 1){*py += x;}
    if(tmy == 1){*my += x;}
    if(tpz == 1){*pz += x;}
    if(tmz == 1){*mz += x;}
    return tpx+tmx+tpy+tmy+tpz+tmz; // return number of free faces
}
// calc ship state
void calcShipState()
{
#if defined(__linux__) && defined(VERBOSE)
    setlocale(LC_NUMERIC, "");
    const uint64_t st = microtime();
#endif

    // generate new state
    memset(&s, 0x00, sizeof(ship_state));
    for(uint i = 0; i < g.num_voxels; i++)
    {
        if(g.voxels[i].w < 0.f){continue;} // skip deleted
        
        if(g.voxels[i].w == 0.f) // copper heat block
        {
            s.hstore += 1.f;
        }
        else if(g.voxels[i].w == 1.f) // heat-to-electric copper heat block
        {
            s.hstore += 1.f;
            s.power += 0.05f;
        }
        else if(g.voxels[i].w == 2.f) // copper heat radiator
        {
            Uint8 tpx, tmx, tpy, tmy, tpz, tmz;
            freeFaces(&tpx, &tmx, &tpy, &tmy, &tpz, &tmz, i);
            s.cool += 0.16f * ((float)(tpx+tmx+tpy+tmy+tpz+tmz));
        }
        else if(g.voxels[i].w == 3.f) // gold heat radiator
        {
            Uint8 tpx, tmx, tpy, tmy, tpz, tmz;
            freeFaces(&tpx, &tmx, &tpy, &tmy, &tpz, &tmz, i);
            s.cool += 0.48f * ((float)(tpx+tmx+tpy+tmy+tpz+tmz));
        }
        else if(g.voxels[i].w == 4.f) // mining ore suction
        {
            incrementFreeFaces(&s.pxsf, &s.mxsf, &s.pysf, &s.mysf, &s.pzsf, &s.mzsf, 1.f, i);
            s.maxelec += 1.f;
        }
        else if(g.voxels[i].w == 5.f) // storage
        { 
            s.store += 1.f;
        }
        else if(g.voxels[i].w == 6.f) // solar panel
        { 
            Uint8 tpx, tmx, tpy, tmy, tpz, tmz;
            freeFaces(&tpx, &tmx, &tpy, &tmy, &tpz, &tmz, i);
            s.power += 0.1f * ((float)(tpx+tmx+tpy+tmy+tpz+tmz));
        }
        else if(g.voxels[i].w == 7.f) // battery
        { 
            s.estore += 1.f;
            s.heat += 0.03f;
            s.maxheat += 0.03f;
        }
        else if(g.voxels[i].w == 8.f) // ion truster
        { 
            const Uint8 f = incrementFreeFaces(&s.pxtf, &s.mxtf, &s.pytf, &s.mytf, &s.pztf, &s.mztf, 1.f, i);
            s.maxheat += 0.1f * f;
            s.maxelec += 1.f * f;
        }
        else if(g.voxels[i].w == 9.f) // fuel refinary
        { 
            s.fgen += 1.f;
            s.maxheat += 1.f;
            s.maxelec += 1.f;
        }
        else if(g.voxels[i].w == 10.f) // fuel tank
        { 
            s.fstore += 1.f;
        }
        else if(g.voxels[i].w == 11.f) // fuel thruster
        { 
            const Uint8 f = incrementFreeFaces(&s.pxftf, &s.mxftf, &s.pyftf, &s.myftf, &s.pzftf, &s.mzftf, 1.f, i);
            s.maxheat += 1.f * f;
            s.maxfuel += 1.f * f;
        }
        else if(g.voxels[i].w == 12.f) // fuel thruster with electrical re-capture generator
        { 
            const Uint8 f = incrementFreeFaces(&s.pxftf, &s.mxftf, &s.pyftf, &s.myftf, &s.pzftf, &s.mzftf, 1.f, i);
            s.power += 0.3f * f;
            s.maxheat += 1.f * f;
            s.maxfuel += 1.f * f;
        }
        else if(g.voxels[i].w == 13.f) // laser
        { 
            const Uint8 f = incrementFreeFaces(&s.pxlff, &s.mxlff, &s.pylff, &s.mylff, &s.pzlff, &s.mzlff, 1.f, i);
            s.maxheat += 4.f * f;
            s.maxelec += 1.f * f;
        }
        else if(g.voxels[i].w == 14.f) // railgun
        { 
            const Uint8 f = incrementFreeFaces(&s.pxrff, &s.mxrff, &s.pyrff, &s.myrff, &s.pzrff, &s.mzrff, 1.f, i); // !!
            s.maxheat += 1.f * f;
            s.maxelec += 1.f * f;
        }
        else if(g.voxels[i].w == 15.f) // railgun accelerator
        { 
            const Uint8 f = incrementFreeFaces(&s.pxrff, &s.mxrff, &s.pyrff, &s.myrff, &s.pzrff, &s.mzrff, 1.f, i); // !!
            s.maxheat += 1.f * f;
            s.maxelec += 1.f * f;
        }
        else if(g.voxels[i].w == 16.f) // nuclear refinary
        { 
            s.ngen += 1.f;
            s.maxheat += 1.f;
            s.maxelec += 1.f;
        }
        else if(g.voxels[i].w == 17.f) // nuclear reactor
        { 
            s.power += 10.f;
            s.nstore += 1.f;
            s.maxheat += 6.f;
        }
        else if(g.voxels[i].w == 18.f) // nuclear xenon refinary
        { 
            s.xgen += 0.5f;
            s.xstore += 0.5f;
            s.maxheat += 1.f;
            s.maxelec += 1.f;
        }
        else if(g.voxels[i].w == 19.f) // xenon thruster
        { 
            const Uint8 f = incrementFreeFaces(&s.pxxf, &s.mxxf, &s.pyxf, &s.myxf, &s.pzxf, &s.mzxf, 1.f, i);
            s.maxheat += 1.f * f;
            s.maxelec += 1.f * f;
            s.maxxenon += 1.f * f;
        }
        else if(g.voxels[i].w == 20.f) // crypto miner
        { 
            s.drain += 1.f;
            s.crypto += 1.f;
            s.heat += 1.f;
            s.maxheat += 1.f;
            s.maxelec += 1.f;
        }
        else if(g.voxels[i].w == 30.f) // gold heat block
        { 
            s.hstore += 4.f;
        }
        else if(g.voxels[i].w == 31.f) // aluminium heat block
        { 
            s.hstore += 0.5f;
        }
    }

    // update hud
    drawHud();

    // timing
#if defined(__linux__) && defined(VERBOSE)
    char tmp[16];
    timestamp(tmp);
    printf("[%s] Ship state took %'lu μs\n", tmp, microtime()-st);
#endif
}

//*************************************
// ray & render functions
//*************************************

// render state id's
GLint projection_id;
GLint view_id;
GLint voxel_id;
GLint position_id;
GLint normal_id;
GLint texcoord_id;
GLint sampler_id;

// render state matrices
mat projection;
mat view;
mat model;

// hit_vec will be untouched by this if there's no collision (function by hax/test_user)
#define RAY_DEPTH 70
int ray(vec *hit_vec, const uint depth, const vec vec_start_pos)
{
	int hit = -1;
	float bestdist;
	float start_pos[] = {vec_start_pos.x, vec_start_pos.y, vec_start_pos.z};
	float lookdir[] = {look_dir.x, look_dir.y, look_dir.z};
	for (uint i = 0; i < g.num_voxels; i++)
    {
		if(g.voxels[i].w < 0.f){continue;}
		float offset[] = {g.voxels[i].x - start_pos[0], g.voxels[i].y - start_pos[1], g.voxels[i].z - start_pos[2]};
		int j = 0;
		// hmmmmmm... is there some decent way to get around this...
		float max = fabsf(offset[0]);
		float tmp = fabsf(offset[1]);
		if(tmp > max){max = tmp; j = 1;}
		tmp = fabsf(offset[2]);
		if(tmp > max){max = tmp; j = 2;}
		if (max <= 0.5f) { // unlikely but better than breaking... costs some cycles though
			*hit_vec = vec_start_pos;
			return i;
		}
		float dist_to_start = offset[j];
		if(dist_to_start > 0.f){dist_to_start -= 0.5f;}else{dist_to_start += 0.5f;}
		const float multiplier = dist_to_start / lookdir[j];
        // too far out (or not in the right direction), don't bother
		if(multiplier > depth || (hit != -1 && multiplier > bestdist) || multiplier < 0.f){continue;}
		// Might end up taking all 222 of those cycles below... :/
		// At least better than what it was originally though
		// And with enough out of range there's still a chance average comes out under
		{
			float pos[] = {start_pos[0] + (lookdir[0] * multiplier), start_pos[1] + (lookdir[1] * multiplier), start_pos[2] + (lookdir[2] * multiplier)};
			if (pos[0] > g.voxels[i].x + 1.5f || pos[0] < g.voxels[i].x - 1.5f ||
				pos[1] > g.voxels[i].y + 1.5f || pos[1] < g.voxels[i].y - 1.5f ||
				pos[2] > g.voxels[i].z + 1.5f || pos[2] < g.voxels[i].z - 1.5f) {continue;} // not even a chance of hitting
            ///
			if ((j == 0 || (pos[0] >= g.voxels[i].x - 0.5f && pos[0] <= g.voxels[i].x + 0.5f)) && // j == n so float inaccuracies won't proceed to tell me that it's out after I just put it on the edge
				(j == 1 || (pos[1] >= g.voxels[i].y - 0.5f && pos[1] <= g.voxels[i].y + 0.5f)) &&
				(j == 2 || (pos[2] >= g.voxels[i].z - 0.5f && pos[2] <= g.voxels[i].z + 0.5f))) { // hit
				hit = i;
				float sign;
				if(offset[j] > 0.f){sign = -1.f;}else{sign = 1.f;}
				if(j == 0){hit_vec->x = sign;}else{hit_vec->x = 0.f;}
				if(j == 1){hit_vec->y = sign;}else{hit_vec->y = 0.f;}
				if(j == 2){hit_vec->z = sign;}else{hit_vec->z = 0.f;}
				bestdist = multiplier;
				continue; // no need to check side cases :P
			}
		}
        ///
		float first_dist;
		char first_dir;
		float second_dist;
		char second_dir;
		for (int n = 0, y = 0; n <= 2; n++)
        {
			if (n != j)
            {
				float dist;
				if(offset[n] > 0){dist = offset[n] - 0.5f;}else{dist = offset[n] + 0.5f;}
				dist = dist / lookdir[n];
				if (y == 0) {
					first_dist = dist;
					first_dir = n;
				} else {
					if (dist < first_dist) {
						second_dist = first_dist;
						second_dir = first_dir;
						first_dist = dist;
						first_dir = n;
					} else {
						second_dist = dist;
						second_dir = n;
					}
				}
				y++;
			}
		}
        ///
		{
			if(first_dist > depth || (hit != -1 && first_dist > bestdist)){continue;}
			float pos[] = {start_pos[0] + (lookdir[0] * first_dist), start_pos[1] + (lookdir[1] * first_dist), start_pos[2] + (lookdir[2] * first_dist)};
			if ((first_dir == 0 || (pos[0] >= g.voxels[i].x - 0.5f && pos[0] <= g.voxels[i].x + 0.5f)) &&
				(first_dir == 1 || (pos[1] >= g.voxels[i].y - 0.5f && pos[1] <= g.voxels[i].y + 0.5f)) &&
				(first_dir == 2 || (pos[2] >= g.voxels[i].z - 0.5f && pos[2] <= g.voxels[i].z + 0.5f)))
            {
				hit = i;
				float sign;
				if(offset[first_dir] > 0.f){sign = -1.f;}else{sign = 1.f;}
				if(first_dir == 0){hit_vec->x = sign;}else{hit_vec->x = 0.f;}
				if(first_dir == 1){hit_vec->y = sign;}else{hit_vec->y = 0.f;}
				if(first_dir == 2){hit_vec->z = sign;}else{hit_vec->z = 0.f;}
				bestdist = first_dist;
				continue;
			}
		}
        ///
		{
			if (second_dist > depth || (hit != -1 && second_dist > bestdist)){continue;}
			float pos[] = {start_pos[0] + (lookdir[0] * second_dist), start_pos[1] + (lookdir[1] * second_dist), start_pos[2] + (lookdir[2] * second_dist)};
			if ((second_dir == 0 || (pos[0] >= g.voxels[i].x - 0.5f && pos[0] <= g.voxels[i].x + 0.5f)) &&
				(second_dir == 1 || (pos[1] >= g.voxels[i].y - 0.5f && pos[1] <= g.voxels[i].y + 0.5f)) &&
				(second_dir == 2 || (pos[2] >= g.voxels[i].z - 0.5f && pos[2] <= g.voxels[i].z + 0.5f)))
            {
				hit = i;
				float sign;
				if(offset[second_dir] > 0.f){sign = -1.f;}else{sign = 1.f;}
				if(second_dir == 0){hit_vec->x = sign;}else{hit_vec->x = 0.f;}
				if(second_dir == 1){hit_vec->y = sign;}else{hit_vec->y = 0.f;}
				if(second_dir == 2){hit_vec->z = sign;}else{hit_vec->z = 0.f;}
				bestdist = second_dist;
				continue;
			}
		}
	}
	return hit;
}
void traceViewPath(const uint face)
{
    g.pb.w = -1.f;
    vec rp = g.pb;
    lray = ray(&rp, RAY_DEPTH, ipp);
    if(lray > -1 && face == 1)
    {
        rp.x += g.voxels[lray].x;
        rp.y += g.voxels[lray].y;
        rp.z += g.voxels[lray].z;
        uint rpif = 1;
        for(uint i = 0; i < g.num_voxels; i++)
        {
            if(g.voxels[i].w < 0.f){continue;}
            if(rp.x == g.voxels[i].x && rp.y == g.voxels[i].y && rp.z == g.voxels[i].z)
            {
                rpif = 0;
                break;
            }
        }
        if(rpif == 1)
        {
            g.pb   = rp;
            g.pb.w = 1.f;
        }
    }
}
int placeVoxel(const float repeat_delay)
{
    ptt = t+repeat_delay;

    if(g.pb.w == -1){return 0;}

    for(uint i = 0; i < g.num_voxels; i++)
    {
        if(g.voxels[i].w < 0.f)
        {
            g.voxels[i] = g.pb;
            g.voxels[i].w = g.st;
            calcShipState();
            return 1;
        }
    }
    if(g.num_voxels < max_voxels)
    {
        g.voxels[g.num_voxels] = g.pb;
        g.voxels[g.num_voxels].w = g.st;
        g.num_voxels++;
        calcShipState();
        return 1;
    }
    return 0;
}

//*************************************
// more utility functions
//*************************************
void printAttrib(SDL_GLattr attr, char* name)
{
    int i;
    SDL_GL_GetAttribute(attr, &i);
    printf("%s: %i\n", name, i);
}
SDL_Surface* SDL_RGBA32Surface(Uint32 w, Uint32 h)
{
    return SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
}
void doPerspective()
{
    glViewport(0, 0, winw, winh);
    if(winw > 527 && winh > 620)
    {
        SDL_FreeSurface(sHud);
        sHud = SDL_RGBA32Surface(winw, winh);
        drawHud();
        hudmap = esLoadTextureA(winw, winh, sHud->pixels, 0);
    }
    ww = (float)winw;
    wh = (float)winh;
    mIdent(&projection);
    mPerspective(&projection, 60.0f, ww / wh, 0.01f, ddist);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
}
forceinline uint insideFrustum(const float x, const float y, const float z)
{
    const float xm = x+g.pp.x, ym = y+g.pp.y, zm = z+g.pp.z;
    return (xm*look_dir.x) + (ym*look_dir.y) + (zm*look_dir.z) > 0.f; // check the angle
}
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    memcpy(s->pixels, data, s->pitch*h);
    return s;
}
forceinline Uint32 getpixel(const SDL_Surface *surface, Uint32 x, Uint32 y)
{
    const Uint8 *p = (Uint8*)surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
    return *(Uint32*)p;
}
forceinline void setpixel(SDL_Surface *surface, Uint32 x, Uint32 y, Uint32 pix)
{
    const Uint8* pixel = (Uint8*)surface->pixels + (y * surface->pitch) + (x * surface->format->BytesPerPixel);
    *((Uint32*)pixel) = pix;
}
void replaceColour(SDL_Surface* surf, SDL_Rect r, Uint32 old_color, Uint32 new_color)
{
    const Uint32 max_y = r.y+r.h;
    const Uint32 max_x = r.x+r.w;
    for(Uint32 y = r.y; y < max_y; ++y)
        for(Uint32 x = r.x; x < max_x; ++x)
            if(getpixel(surf, x, y) == old_color){setpixel(surf, x, y, new_color);}
}
void line(SDL_Surface *surface, Uint32 x0, Uint32 y0, Uint32 x1, Uint32 y1, Uint32 colour)
{
    const int dx = abs((Sint32)x1 - (Sint32)x0), sx = x0 < x1 ? 1 : -1;
    const int dy = abs((Sint32)y1 - (Sint32)y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    while(1)
    {
        setpixel(surface, x0, y0, colour);
        if(x0 == x1 && y0 == y1){break;}
        e2 = err;
        if(e2 > -dx){err -= dy; x0 += sx;}
        if(e2 < dy){err += dx; y0 += sy;}
    }
}

//*************************************
// Simple Font
//*************************************
int drawText(SDL_Surface* o, const char* s, Uint32 x, Uint32 y, Uint8 colour)
{
    static const Uint8 font_map[] = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,0,0,0,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,255,255,0,0,0,0,0,255,255,0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,0,255,255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,255,255,255,0,0,0,0,255,255,255,0,255,0,0,0,0,0,255,0,0,0,0,0,255,255,0,0,0,0,0,255,0,0,0,0,0,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,0,0,0,0,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,0,0,255,255,255,255,0,0,255,0,0,0,0,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,255,255,0,255,255,0,0,0,0,0,255,0,0,0,0,255,0,0,0,0,0,0,255,0,0,0,0,255,255,0,0,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,0,0,255,0,0,255,255,255,0,0,255,255,255,0,0,255,255,255,255,255,0,0,255,255,255,0,0,255,0,0,255,255,255,255,0,0,0,0,255,0,0,255,0,0,255,255,255,0,0,0,255,255,255,0,0,0,0,0,0,255,255,0,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,0,0,255,255,0,0,255,0,0,255,255,255,255,255,255,0,0,255,255,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,255,255,0,0,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,0,0,0,0,0,255,0,0,255,255,0,0,0,0,255,255,0,0,255,255,255,0,0,255,255,0,0,255,255,255,0,0,255,255,255,255,255,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,255,255,255,255,255,0,255,255,255,255,255,255,255,255,0,0,255,0,0,255,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,255,0,0,0,0,255,255,255,0,0,255,255,255,0,0,255,255,255,255,255,0,0,255,255,255,0,0,255,0,0,255,255,255,255,0,0,0,0,0,0,255,255,0,0,255,255,255,0,0,0,0,255,0,0,0,0,0,0,0,0,255,0,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,0,0,255,255,0,0,255,0,0,255,255,255,255,255,255,0,0,255,255,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,255,0,0,0,0,255,255,0,0,0,0,255,255,255,255,0,0,0,255,0,0,0,0,255,0,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,0,255,0,0,0,0,255,0,0,0,255,255,0,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0,0,0,255,255,0,0,0,0,0,0,0,0,0,255,0,0,0,255,0,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,0,255,255,0,0,0,0,0,0,0,255,0,255,0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,0,0,0,0,0,255,255,0,0,255,0,0,255,255,255,255,255,0,0,255,255,255,255,0,0,255,255,0,0,0,255,255,0,0,255,255,255,0,0,255,255,255,255,255,255,255,0,0,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,255,255,0,255,255,255,255,255,255,255,255,0,0,255,0,0,255,0,0,0,0,0,255,0,0,255,255,255,255,0,0,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,0,0,0,0,0,0,0,255,0,0,255,255,255,255,0,0,0,0,0,255,255,255,0,0,255,255,255,0,255,0,0,0,0,255,0,0,0,255,0,0,0,0,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,0,0,255,255,0,0,255,0,0,0,0,0,255,255,255,0,0,255,255,0,0,255,255,255,0,0,255,0,0,0,0,255,255,0,0,255,0,0,255,0,0,255,255,255,0,0,255,255,255,0,0,0,0,255,255,255,0,0,0,255,255,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,255,0,0,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,0,0,0,0,255,255,255,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,255,0,0,0,0,255,255,0,0,255,0,0,255,255,255,255,0,0,255,255,255,0,0,0,255,255,0,255,0,0,255,255,0,0,0,0,255,0,0,0,0,0,255,255,255,255,0,0,255,255,0,0,0,0,255,0,0,255,255,0,0,0,0,255,255,255,255,255,0,255,255,255,255,255,255,255,255,0,0,255,0,0,255,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,255,0,0,0,0,255,255,255,0,0,255,255,255,0,0,255,255,0,0,0,0,0,255,255,255,0,0,255,0,0,255,255,255,255,0,0,0,0,0,255,255,255,0,0,255,255,255,0,255,255,0,0,255,255,0,0,0,255,255,0,0,0,0,0,255,255,255,0,0,0,0,0,0,0,255,0,0,255,255,255,0,0,0,0,0,0,0,255,255,255,0,0,0,0,0,255,255,0,0,255,255,0,0,255,255,255,0,0,255,0,0,0,0,255,255,0,0,0,0,0,0,0,0,255,255,255,0,0,255,255,255,255,0,0,255,255,255,0,0,0,255,255,255,0,0,0,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,0,0,0,0,0,0,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,255,255,0,0,255,255,0,0,255,0,0,0,0,255,0,0,255,0,0,255,0,0,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,0,0,255,255,0,0,255,0,0,255,255,255,0,0,255,255,255,255,255,255,0,0,0,255,255,0,0,255,255,255,255,255,0,0,0,0,255,255,0,0,255,255,255,0,0,255,0,0,255,255,0,0,255,0,0,0,0,0,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,255,0,0,0,0,255,255,255,0,0,255,255,255,0,0,255,255,255,0,0,0,0,255,255,255,0,0,255,0,0,255,255,255,255,0,0,0,0,0,0,255,255,0,0,255,255,255,0,255,255,255,255,255,255,0,0,0,255,255,255,0,0,0,0,255,255,255,0,0,0,0,255,255,255,255,0,0,255,255,255,0,0,0,0,255,0,0,255,255,255,255,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,0,0,255,0,0,0,0,255,255,0,0,0,255,255,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,0,0,0,255,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,0,0,0,0,255,255,0,0,255,255,0,0,255,0,0,0,0,255,0,0,0,0,0,0,0,0,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,0,0,255,255,0,0,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,255,255,255,255,0,0,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,255,0,0,255,255,255,255,255,255,255,0,255,255,255,255,255,255,255,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,0,0,255,0,0,255,255,255,0,0,255,255,255,0,0,255,255,255,0,0,0,0,255,255,255,0,0,255,0,0,255,255,255,255,0,0,0,0,255,0,0,255,0,0,255,255,255,0,255,255,255,255,255,255,0,0,0,255,255,255,255,0,0,0,255,255,255,0,0,0,0,255,255,255,255,0,0,255,255,255,0,0,0,0,255,255,0,0,255,255,255,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,0,0,255,255,0,0,255,255,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,0,0,0,0,255,255,255,255,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,255,0,0,255,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,255,255,0,0,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,0,0,255,255,0,0,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,0,0,255,255,0,0,255,0,0,255,0,0,255,255,255,255,0,0,255,255,0,0,255,255,255,0,0,255,0,0,255,255,0,0,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,255,255,0,0,0,0,0,0,255,255,255,0,255,255,255,255,255,255,255,0,0,255,255,255,0,0,0,0,0,0,0,255,255,0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0,0,0,0,255,255,255,255,0,0,0,0,0,0,0,0,255,255,255,0,0,0,0,0,0,0,0,0,0,255,0,0,255,255,0,0,0,0,0,0,0,0,255,255,255,255,255,255,0,0,0,255,255,255,255,0,255,0,0,0,0,0,255,0,0,255,255,255,255,255,0,0,0,0,0,255,0,0,255,255,255,0,0,0,0,0,0,0,255,255,255,0,0,255,255,255,0,0,0,0,0,255,255,255,0,0,255,255,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,0,0,0,0,255,0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,0,255,0,0,0,0,0,0,0,255,255,255,0,0,0,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,255,255,0,0,0,0,0,0,255,255,0,0,255,255,0,0,0,0,255,255,0,0,255,0,0,0,0,255,0,0,0,0,0,255,255,0,0,0,0,0,0,0,255,255,0,0,0,0,255,255,0,0,0,255,0,0,0,0,0,255,255,0,0,255,255,255,0,0,255,255,0,0,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,0,0,0,255,0,0,0,0,255,0,0,0,0,0,0,0,0,0,0,255,0,0,0,0,255,255,255,255,0,0,255,255,0,0,0,0,255,255,0,0,0,0,255,255,255,0,0,255,255,255,0,0,0,0,255,255,0,0,0,0,255,0,0,0,0,255,255,255,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,0,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
    static const Uint32 m = 1;
    static SDL_Surface* font_black = NULL;
    static SDL_Surface* font_white = NULL;
    static SDL_Surface* font_aqua = NULL;
    static SDL_Surface* font_gold = NULL;
    static SDL_Surface* font_cia = NULL;
    if(font_black == NULL)
    {
        font_black = SDL_RGBA32Surface(380, 11);
        for(int y = 0; y < font_black->h; y++)
        {
            for(int x = 0; x < font_black->w; x++)
            {
                const Uint8 c = font_map[(y*font_black->w)+x];
                setpixel(font_black, x, y, SDL_MapRGBA(font_black->format, c, c, c, 255-c));
            }
        }
        font_white = SDL_RGBA32Surface(380, 11);
        SDL_BlitSurface(font_black, &font_black->clip_rect, font_white, &font_white->clip_rect);
        replaceColour(font_white, font_white->clip_rect, 0xFF000000, 0xFFFFFFFF);
        font_aqua = SDL_RGBA32Surface(380, 11);
        SDL_BlitSurface(font_black, &font_black->clip_rect, font_aqua, &font_aqua->clip_rect);
        replaceColour(font_aqua, font_aqua->clip_rect, 0xFF000000, 0xFFFFFF00);
        font_gold = SDL_RGBA32Surface(380, 11);
        SDL_BlitSurface(font_black, &font_black->clip_rect, font_gold, &font_gold->clip_rect);
        replaceColour(font_gold, font_gold->clip_rect, 0xFF000000, 0xFF00BFFF);
        font_cia = SDL_RGBA32Surface(380, 11);
        SDL_BlitSurface(font_black, &font_black->clip_rect, font_cia, &font_cia->clip_rect);
        replaceColour(font_cia, font_cia->clip_rect, 0xFF000000, 0xFF97C920);
        // #20c997(0xFF97C920) #00FF41(0xFF41FF00) #61d97c(0xFF7CD961) #51d4e9(0xFFE9D451)
    }
    if(s[0] == '*' && s[1] == 'K') // signal cleanup
    {
        SDL_FreeSurface(font_black);
        SDL_FreeSurface(font_white);
        SDL_FreeSurface(font_aqua);
        SDL_FreeSurface(font_gold);
        SDL_FreeSurface(font_cia);
        font_black = NULL;
        return 0;
    }
    SDL_Surface* font = font_black;
    if(     colour == 1){font = font_white;}
    else if(colour == 2){font = font_aqua;}
    else if(colour == 3){font = font_gold;}
    else if(colour == 4){font = font_cia;}
    SDL_Rect dr = {x, y, 0, 0};
    const Uint32 len = strlen(s);
    for(Uint32 i = 0; i < len; i++)
    {
        if(s[i] == 'A'){     SDL_BlitSurface(font, &(SDL_Rect){0,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'B'){SDL_BlitSurface(font, &(SDL_Rect){7,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'C'){SDL_BlitSurface(font, &(SDL_Rect){13,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'D'){SDL_BlitSurface(font, &(SDL_Rect){19,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'E'){SDL_BlitSurface(font, &(SDL_Rect){26,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == 'F'){SDL_BlitSurface(font, &(SDL_Rect){31,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == 'G'){SDL_BlitSurface(font, &(SDL_Rect){36,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'H'){SDL_BlitSurface(font, &(SDL_Rect){43,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'I'){SDL_BlitSurface(font, &(SDL_Rect){50,0,4,9}, o, &dr); dr.x += 4+m;}
        else if(s[i] == 'J'){SDL_BlitSurface(font, &(SDL_Rect){54,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == 'K'){SDL_BlitSurface(font, &(SDL_Rect){59,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'L'){SDL_BlitSurface(font, &(SDL_Rect){65,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == 'M'){SDL_BlitSurface(font, &(SDL_Rect){70,0,9,9}, o, &dr); dr.x += 9+m;}
        else if(s[i] == 'N'){SDL_BlitSurface(font, &(SDL_Rect){79,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'O'){SDL_BlitSurface(font, &(SDL_Rect){85,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'P'){SDL_BlitSurface(font, &(SDL_Rect){92,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'Q'){SDL_BlitSurface(font, &(SDL_Rect){98,0,7,11}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'R'){SDL_BlitSurface(font, &(SDL_Rect){105,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'S'){SDL_BlitSurface(font, &(SDL_Rect){112,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'T'){SDL_BlitSurface(font, &(SDL_Rect){118,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'U'){SDL_BlitSurface(font, &(SDL_Rect){124,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == 'V'){SDL_BlitSurface(font, &(SDL_Rect){131,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'W'){SDL_BlitSurface(font, &(SDL_Rect){137,0,10,9}, o, &dr); dr.x += 10+m;}
        else if(s[i] == 'X'){SDL_BlitSurface(font, &(SDL_Rect){147,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'Y'){SDL_BlitSurface(font, &(SDL_Rect){153,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'Z'){SDL_BlitSurface(font, &(SDL_Rect){159,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'a'){SDL_BlitSurface(font, &(SDL_Rect){165,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'b'){SDL_BlitSurface(font, &(SDL_Rect){171,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'c'){SDL_BlitSurface(font, &(SDL_Rect){177,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == 'd'){SDL_BlitSurface(font, &(SDL_Rect){182,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'e'){SDL_BlitSurface(font, &(SDL_Rect){188,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'f'){SDL_BlitSurface(font, &(SDL_Rect){194,0,4,9}, o, &dr); dr.x += 3+m;}
        else if(s[i] == 'g'){SDL_BlitSurface(font, &(SDL_Rect){198,0,6,11}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'h'){SDL_BlitSurface(font, &(SDL_Rect){204,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'i'){SDL_BlitSurface(font, &(SDL_Rect){210,0,2,9}, o, &dr); dr.x += 2+m;}
        else if(s[i] == 'j'){SDL_BlitSurface(font, &(SDL_Rect){212,0,3,11}, o, &dr); dr.x += 3+m;}
        else if(s[i] == 'k'){SDL_BlitSurface(font, &(SDL_Rect){215,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'l'){SDL_BlitSurface(font, &(SDL_Rect){221,0,2,9}, o, &dr); dr.x += 2+m;}
        else if(s[i] == 'm'){SDL_BlitSurface(font, &(SDL_Rect){223,0,10,9}, o, &dr); dr.x += 10+m;}
        else if(s[i] == 'n'){SDL_BlitSurface(font, &(SDL_Rect){233,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'o'){SDL_BlitSurface(font, &(SDL_Rect){239,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'p'){SDL_BlitSurface(font, &(SDL_Rect){245,0,6,11}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'q'){SDL_BlitSurface(font, &(SDL_Rect){251,0,6,11}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'r'){SDL_BlitSurface(font, &(SDL_Rect){257,0,4,9}, o, &dr); dr.x += 4+m;}
        else if(s[i] == 's'){SDL_BlitSurface(font, &(SDL_Rect){261,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == 't'){SDL_BlitSurface(font, &(SDL_Rect){266,0,4,9}, o, &dr); dr.x += 4+m;}
        else if(s[i] == 'u'){SDL_BlitSurface(font, &(SDL_Rect){270,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'v'){SDL_BlitSurface(font, &(SDL_Rect){276,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'w'){SDL_BlitSurface(font, &(SDL_Rect){282,0,8,9}, o, &dr); dr.x += 8+m;}
        else if(s[i] == 'x'){SDL_BlitSurface(font, &(SDL_Rect){290,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'y'){SDL_BlitSurface(font, &(SDL_Rect){296,0,6,11}, o, &dr); dr.x += 6+m;}
        else if(s[i] == 'z'){SDL_BlitSurface(font, &(SDL_Rect){302,0,5,9}, o, &dr); dr.x += 5+m;}
        else if(s[i] == '0'){SDL_BlitSurface(font, &(SDL_Rect){307,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '1'){SDL_BlitSurface(font, &(SDL_Rect){313,0,4,9}, o, &dr); dr.x += 4+m;}
        else if(s[i] == '2'){SDL_BlitSurface(font, &(SDL_Rect){317,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '3'){SDL_BlitSurface(font, &(SDL_Rect){323,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '4'){SDL_BlitSurface(font, &(SDL_Rect){329,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '5'){SDL_BlitSurface(font, &(SDL_Rect){335,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '6'){SDL_BlitSurface(font, &(SDL_Rect){341,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '7'){SDL_BlitSurface(font, &(SDL_Rect){347,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '8'){SDL_BlitSurface(font, &(SDL_Rect){353,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == '9'){SDL_BlitSurface(font, &(SDL_Rect){359,0,6,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == ':'){SDL_BlitSurface(font, &(SDL_Rect){365,0,2,9}, o, &dr); dr.x += 2+m;}
        else if(s[i] == '.'){SDL_BlitSurface(font, &(SDL_Rect){367,0,2,9}, o, &dr); dr.x += 2+m;}
        else if(s[i] == '+'){SDL_BlitSurface(font, &(SDL_Rect){369,0,7,9}, o, &dr); dr.x += 7+m;}
        else if(s[i] == '-'){dr.x++; SDL_BlitSurface(font, &(SDL_Rect){376,0,4,9}, o, &dr); dr.x += 6+m;}
        else if(s[i] == ' '){dr.x += 2+m;}
    }
    return dr.x;
}
int lenText(const char* s)
{
    int x = 0;
    static const int m = 1;
    const Uint32 len = strlen(s);
    for(Uint32 i = 0; i < len; i++)
    {
        if(s[i] == 'A'){     x += 7+m;}
        else if(s[i] == 'B'){x += 6+m;}
        else if(s[i] == 'C'){x += 6+m;}
        else if(s[i] == 'D'){x += 7+m;}
        else if(s[i] == 'E'){x += 5+m;}
        else if(s[i] == 'F'){x += 5+m;}
        else if(s[i] == 'G'){x += 7+m;}
        else if(s[i] == 'H'){x += 7+m;}
        else if(s[i] == 'I'){x += 4+m;}
        else if(s[i] == 'J'){x += 5+m;}
        else if(s[i] == 'K'){x += 6+m;}
        else if(s[i] == 'L'){x += 5+m;}
        else if(s[i] == 'M'){x += 9+m;}
        else if(s[i] == 'N'){x += 6+m;}
        else if(s[i] == 'O'){x += 7+m;}
        else if(s[i] == 'P'){x += 6+m;}
        else if(s[i] == 'Q'){x += 7+m;}
        else if(s[i] == 'R'){x += 7+m;}
        else if(s[i] == 'S'){x += 6+m;}
        else if(s[i] == 'T'){x += 6+m;}
        else if(s[i] == 'U'){x += 7+m;}
        else if(s[i] == 'V'){x += 6+m;}
        else if(s[i] == 'W'){x += 10+m;}
        else if(s[i] == 'X'){x += 6+m;}
        else if(s[i] == 'Y'){x += 6+m;}
        else if(s[i] == 'Z'){x += 6+m;}
        else if(s[i] == 'a'){x += 6+m;}
        else if(s[i] == 'b'){x += 6+m;}
        else if(s[i] == 'c'){x += 5+m;}
        else if(s[i] == 'd'){x += 6+m;}
        else if(s[i] == 'e'){x += 6+m;}
        else if(s[i] == 'f'){x += 3+m;}
        else if(s[i] == 'g'){x += 6+m;}
        else if(s[i] == 'h'){x += 6+m;}
        else if(s[i] == 'i'){x += 2+m;}
        else if(s[i] == 'j'){x += 3+m;}
        else if(s[i] == 'k'){x += 6+m;}
        else if(s[i] == 'l'){x += 2+m;}
        else if(s[i] == 'm'){x += 10+m;}
        else if(s[i] == 'n'){x += 6+m;}
        else if(s[i] == 'o'){x += 6+m;}
        else if(s[i] == 'p'){x += 6+m;}
        else if(s[i] == 'q'){x += 6+m;}
        else if(s[i] == 'r'){x += 4+m;}
        else if(s[i] == 's'){x += 5+m;}
        else if(s[i] == 't'){x += 4+m;}
        else if(s[i] == 'u'){x += 6+m;}
        else if(s[i] == 'v'){x += 6+m;}
        else if(s[i] == 'w'){x += 8+m;}
        else if(s[i] == 'x'){x += 6+m;}
        else if(s[i] == 'y'){x += 6+m;}
        else if(s[i] == 'z'){x += 5+m;}
        else if(s[i] == '0'){x += 6+m;}
        else if(s[i] == '1'){x += 4+m;}
        else if(s[i] == '2'){x += 6+m;}
        else if(s[i] == '3'){x += 6+m;}
        else if(s[i] == '4'){x += 6+m;}
        else if(s[i] == '5'){x += 6+m;}
        else if(s[i] == '6'){x += 6+m;}
        else if(s[i] == '7'){x += 6+m;}
        else if(s[i] == '8'){x += 6+m;}
        else if(s[i] == '9'){x += 6+m;}
        else if(s[i] == ':'){x += 2+m;}
        else if(s[i] == '.'){x += 2+m;}
        else if(s[i] == '+'){x += 7+m;}
        else if(s[i] == '-'){x += 7+m;}
        else if(s[i] == ' '){x += 2+m;}
    }
    return x;
}

//*************************************
// update & render
//*************************************
void drawHud()
{
    SDL_FillRect(sHud, &sHud->clip_rect, 0x00000000);
    setpixel(sHud, winw2, winh2, 0xFFFFFF00);
    //
    //drawText(sHud, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789:.+-", 300, 10, 4);
    //
    setpixel(sHud, winw2+1, winh2, 0xFFFFFF00);
    setpixel(sHud, winw2-1, winh2, 0xFFFFFF00);
    setpixel(sHud, winw2, winh2+1, 0xFFFFFF00);
    setpixel(sHud, winw2, winh2-1, 0xFFFFFF00);
    //
    setpixel(sHud, winw2+2, winh2, 0xFFFFFF00);
    setpixel(sHud, winw2-2, winh2, 0xFFFFFF00);
    setpixel(sHud, winw2, winh2+2, 0xFFFFFF00);
    setpixel(sHud, winw2, winh2-2, 0xFFFFFF00);
    //
    setpixel(sHud, winw2+3, winh2, 0xFFFFFF00);
    setpixel(sHud, winw2-3, winh2, 0xFFFFFF00);
    setpixel(sHud, winw2, winh2+3, 0xFFFFFF00);
    setpixel(sHud, winw2, winh2-3, 0xFFFFFF00);
    //
    int ih = 75;
    if(g.st == 0.f){ih = (11*4);}
    else if(g.st == 1.f){ih = (11*5);}
    else if(g.st == 2.f){ih = (11*4);}
    else if(g.st == 3.f){ih = (11*4);}
    else if(g.st == 4.f){ih = (11*4);}
    else if(g.st == 5.f){ih = (11*4);}
    else if(g.st == 6.f){ih = (11*4);}
    else if(g.st == 7.f){ih = (11*5);}
    else if(g.st == 8.f){ih = (11*5);}
    else if(g.st == 9.f){ih = (11*6);}
    else if(g.st == 10.f){ih = (11*4);}
    else if(g.st == 11.f){ih = (11*5);}
    else if(g.st == 12.f){ih = (11*6);}
    else if(g.st == 13.f){ih = (11*6);}
    else if(g.st == 14.f){ih = (11*6);}
    else if(g.st == 15.f){ih = (11*7);}
    if(g.st == 16.f){ih = (11*6);}
    if(g.st == 17.f){ih = (11*6);}
    if(g.st == 18.f){ih = (11*6);}
    if(g.st == 19.f){ih = (11*6);}
    if(g.st == 20.f){ih = (11*6);}
    if(g.st == 21.f){ih = (11*4);}
    if(g.st == 22.f){ih = (11*4);}
    if(g.st == 23.f){ih = (11*4);}
    if(g.st == 24.f){ih = (11*4);}
    if(g.st == 25.f){ih = (11*4);}
    if(g.st == 26.f){ih = (11*4);}
    if(g.st == 27.f){ih = (11*4);}
    if(g.st == 28.f){ih = (11*4);}
    if(g.st == 29.f){ih = (11*4);}
    if(g.st == 30.f){ih = (11*4);}
    if(g.st == 31.f){ih = (11*4);}
    //
    SDL_FillRect(sHud, &(SDL_Rect){0, 0, 265, 623}, 0x99000000);
    SDL_FillRect(sHud, &(SDL_Rect){winw-265, 0, 265, ih}, 0x99000000);
    //
    char pn[256];
    if(g.st == 0.f)
    {
        drawText(sHud, "Copper Heat Block", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Storage Capacity: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
    }
    else if(g.st == 1.f)
    {
        drawText(sHud, "Electric Generating Copper Heat Block", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Storage Capacity: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Electric Generation: ", winw-255, 35, 1);
        drawText(sHud, "0.05", ofs, 35, 2);
    }
    else if(g.st == 2.f)
    {
        drawText(sHud, "Copper Heat Radiator", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Dissipation per face: ", winw-255, 24, 1);
        drawText(sHud, "0.16", ofs, 24, 2);
    }
    else if(g.st == 3.f)
    {
        drawText(sHud, "Gold Heat Radiator", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Dissipation per face: ", winw-255, 24, 1);
        drawText(sHud, "0.48", ofs, 24, 2);
    }
    else if(g.st == 4.f)
    {
        drawText(sHud, "Mining Vacuum", winw-255, 10, 2);
        int ofs = drawText(sHud, "Suction Power: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
    }
    else if(g.st == 5.f)
    {
        drawText(sHud, "Storage", winw-255, 10, 2);
        int ofs = drawText(sHud, "Storage Capacity: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
    }
    else if(g.st == 6.f)
    {
        drawText(sHud, "Solar Panel", winw-255, 10, 2);
        int ofs = drawText(sHud, "Electric Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "0.1", ofs, 24, 2);
    }
    else if(g.st == 7.f)
    {
        drawText(sHud, "Battery", winw-255, 10, 2);
        int ofs = drawText(sHud, "Electric Storage: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Heat Generation: ", winw-255, 35, 1);
        drawText(sHud, "0.03", ofs, 35, 2);
    }
    else if(g.st == 8.f)
    {
        drawText(sHud, "Ion Thruster", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Electric Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
    }
    else if(g.st == 9.f)
    {
        drawText(sHud, "Fuel Refinery", winw-255, 10, 2);
        int ofs = drawText(sHud, "Fuel Generation: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Heat Generation: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Electric Use: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    else if(g.st == 10.f)
    {
        drawText(sHud, "Fuel Tank", winw-255, 10, 2);
        int ofs = drawText(sHud, "Fuel Capacity: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
    }
    else if(g.st == 11.f)
    {
        drawText(sHud, "Fuel Thruster", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Fuel Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
    }
    else if(g.st == 12.f)
    {
        drawText(sHud, "Fuel Thruster with Electrical Generator", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Fuel Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Electric Generation per face: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    else if(g.st == 13.f)
    {
        drawText(sHud, "Laser", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "4", ofs, 24, 2);
        ofs = drawText(sHud, "Electric Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Damage per face: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    else if(g.st == 14.f)
    {
        drawText(sHud, "Railgun", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Electric Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Damage per face: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    else if(g.st == 15.f)
    {
        drawText(sHud, "Railgun Accelerator", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Electric Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        drawText(sHud, "Connect to Railgun to accelerate", winw-255, 46, 3);
        drawText(sHud, "the projectile. Doubles damage.", winw-255, 57, 3);
    }
    if(g.st == 16.f)
    {
        drawText(sHud, "Nuclear Refinery", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Nuclear Generation: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Electric Use: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    if(g.st == 17.f)
    {
        drawText(sHud, "Nuclear Reactor", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation: ", winw-255, 24, 1);
        drawText(sHud, "6", ofs, 24, 2);
        ofs = drawText(sHud, "Nuclear Storage: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Electric Generation: ", winw-255, 46, 1);
        drawText(sHud, "10", ofs, 46, 2);
    }
    if(g.st == 18.f)
    {
        drawText(sHud, "Nuclear Xenon Refinery", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Xenon Generation: ", winw-255, 35, 1);
        drawText(sHud, "0.5", ofs, 35, 2);
        ofs = drawText(sHud, "Electric Use: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    if(g.st == 19.f)
    {
        drawText(sHud, "Xenon Ion Thruster", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation per face: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Xenon Use per face: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Electric Use per face: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    if(g.st == 20.f)
    {
        drawText(sHud, "CryptoCurrency Mining Rig", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Generation: ", winw-255, 24, 1);
        drawText(sHud, "1", ofs, 24, 2);
        ofs = drawText(sHud, "Electric Use: ", winw-255, 35, 1);
        drawText(sHud, "1", ofs, 35, 2);
        ofs = drawText(sHud, "Crypto Coins per Second: ", winw-255, 46, 1);
        drawText(sHud, "1", ofs, 46, 2);
    }
    if(g.st == 21.f)
    {
        drawText(sHud, "Green Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 22.f)
    {
        drawText(sHud, "Aqua Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 23.f)
    {
        drawText(sHud, "Light Blue Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 24.f)
    {
        drawText(sHud, "Blue Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 25.f)
    {
        drawText(sHud, "Purple Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 26.f)
    {
        drawText(sHud, "Pink Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 27.f)
    {
        drawText(sHud, "Red Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 28.f)
    {
        drawText(sHud, "Orange Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 29.f)
    {
        drawText(sHud, "Yellow Armor", winw-255, 10, 2);
        drawText(sHud, "A self-healing armor.", winw-255, 24, 3);
    }
    if(g.st == 30.f)
    {
        drawText(sHud, "Gold Heat Block", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Storage Capacity: ", winw-255, 24, 1);
        drawText(sHud, "4", ofs, 24, 2);
    }
    if(g.st == 31.f)
    {
        drawText(sHud, "Aluminium Heat Block", winw-255, 10, 2);
        int ofs = drawText(sHud, "Heat Storage Capacity: ", winw-255, 24, 1);
        drawText(sHud, "0.5", ofs, 24, 2);
    }
    //
    char name[128];
    char final[512];
    const uint len = sizeof(ship_state) / sizeof(uSS);
    for(uint i = 0; i < len; i++)
    {
        if(i == 0){      sprintf(name, "+X Electric Ion Thruster");}
        else if(i == 1){ sprintf(name, "-X Electric Ion Thruster");}
        else if(i == 2){ sprintf(name, "+Y Electric Ion Thruster");}
        else if(i == 3){ sprintf(name, "-Y Electric Ion Thruster");}
        else if(i == 4){ sprintf(name, "+Z Electric Ion Thruster");}
        else if(i == 5){ sprintf(name, "-Z Electric Ion Thruster");}
        else if(i == 6){ sprintf(name, "+X Fuel Thruster");}
        else if(i == 7){ sprintf(name, "-X Fuel Thruster");}
        else if(i == 8){ sprintf(name, "+Y Fuel Thruster");}
        else if(i == 9){ sprintf(name, "-Y Fuel Thruster");}
        else if(i == 10){sprintf(name, "+Z Fuel Thruster");}
        else if(i == 11){sprintf(name, "-Z Fuel Thruster");}
        else if(i == 12){sprintf(name, "+X Xenon Thruster");}
        else if(i == 13){sprintf(name, "-X Xenon Thruster");}
        else if(i == 14){sprintf(name, "+Y Xenon Thruster");}
        else if(i == 15){sprintf(name, "-Y Xenon Thruster");}
        else if(i == 16){sprintf(name, "+Z Xenon Thruster");}
        else if(i == 17){sprintf(name, "-Z Xenon Thruster");}
        else if(i == 18){sprintf(name, "+X Railgun Force");}
        else if(i == 19){sprintf(name, "-X Railgun Force");}
        else if(i == 20){sprintf(name, "+Y Railgun Force");}
        else if(i == 21){sprintf(name, "-Y Railgun Force");}
        else if(i == 22){sprintf(name, "+Z Railgun Force");}
        else if(i == 23){sprintf(name, "-Z Railgun Force");}
        else if(i == 24){sprintf(name, "+X Laser Force");}
        else if(i == 25){sprintf(name, "-X Laser Force");}
        else if(i == 26){sprintf(name, "+Y Laser Force");}
        else if(i == 27){sprintf(name, "-Y Laser Force");}
        else if(i == 28){sprintf(name, "+Z Laser Force");}
        else if(i == 29){sprintf(name, "-Z Laser Force");}
        else if(i == 30){sprintf(name, "+X Mining Vacuum Suction Force");}
        else if(i == 31){sprintf(name, "-X Mining Vacuum Suction Force");}
        else if(i == 32){sprintf(name, "+Y Mining Vacuum Suction Force");}
        else if(i == 33){sprintf(name, "-Y Mining Vacuum Suction Force");}
        else if(i == 34){sprintf(name, "+Z Mining Vacuum Suction Force");}
        else if(i == 35){sprintf(name, "-Z Mining Vacuum Suction Force");}
        else if(i == 36){sprintf(name, "Electric Regeneration Rate");}
        else if(i == 37){sprintf(name, "Heat Dissipation Rate");}
        else if(i == 38){sprintf(name, "Crypto Currency Mining Rate");}
        else if(i == 39){sprintf(name, "Fuel Refinery Rate");}
        else if(i == 40){sprintf(name, "Xenon Refinery Rate");}
        else if(i == 41){sprintf(name, "Nuclear Refinery Rate");}
        else if(i == 42){sprintf(name, "Mining Storage Capacity");}
        else if(i == 43){sprintf(name, "Fuel Storage Capacity");}
        else if(i == 44){sprintf(name, "Electric Storage Capacity");}
        else if(i == 45){sprintf(name, "Xenon Storage Capacity");}
        else if(i == 46){sprintf(name, "Heat Storage Capacity");}
        else if(i == 47){sprintf(name, "Nuclear Storage Capacity");}
        else if(i == 48){sprintf(name, "Idle Power Usage Rate");}
        else if(i == 49){sprintf(name, "Idle Heat Generation Rate");}
        else if(i == 50){sprintf(name, "Maximum Heat Ship Could Produce");}
        else if(i == 51){sprintf(name, "Maximum Electric Ship Could Use");}
        else if(i == 52){sprintf(name, "Maximum Fuel Ship Could Use");}
        else if(i == 53){sprintf(name, "Maximum Xenon Ship Could Use");}
        else{sprintf(name, "unknown");}
        sprintf(final, "%s:\n", name);
        const int app = i > 35 ? 11 : 0;
        int tlen = drawText(sHud, final, 10, 10+(i*11)+app, 1);
        sprintf(final, " %g\n", ((float*)&s)[i]);
        drawText(sHud, final, tlen, 10+(i*11)+app, 2);
    }
    //
    flipHud();
}
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static float lt = 0;
    t = fTime();
    const float dt = t-lt;
    lt = t;

//*************************************
// input handling
//*************************************
    static int mx=0, my=0, lx=0, ly=0, md=0;
    static float idle = 0.f;

    // if user is idle for 3 minutes, save.
    if(idle != 0.f && t-idle > 180.f)
    {
        saveState();
        idle = 0.f; // so we only save once
        // on input a new idle is set, and a
        // count-down for a new save begins.
    }
    
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
            {
                if(focus_mouse == 0){break;}
                if(event.key.keysym.sym == SDLK_w){ks[0] = 1;}
                else if(event.key.keysym.sym == SDLK_a){ks[1] = 1;}
                else if(event.key.keysym.sym == SDLK_s){ks[2] = 1;}
                else if(event.key.keysym.sym == SDLK_d){ks[3] = 1;}
                else if(event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_LCTRL){ks[4] = 1;} // move down Z
                else if(event.key.keysym.sym == SDLK_LEFT){ks[5] = 1;}
                else if(event.key.keysym.sym == SDLK_RIGHT){ks[6] = 1;}
                else if(event.key.keysym.sym == SDLK_UP){ks[7] = 1;}
                else if(event.key.keysym.sym == SDLK_DOWN){ks[8] = 1;}
                else if(event.key.keysym.sym == SDLK_SPACE){ks[9] = 1;} // move up Z
                else if(event.key.keysym.sym == SDLK_ESCAPE) // unlock mouse focus
                {
                    focus_mouse = 0;
                    SDL_ShowCursor(1);
                }
                else if(event.key.keysym.sym == SDLK_q) // clone pointed voxel texture
                {
                    traceViewPath(0);
                    if(lray > -1){g.st = g.voxels[lray].w;}
                }
                else if(event.key.keysym.sym == SDLK_SLASH || event.key.keysym.sym == SDLK_x) // - change selected node
                {
                    g.st -= 1.f;
                    if(g.st < 0.f){g.st = 31.f;}
                    drawHud();
                }
                else if(event.key.keysym.sym == SDLK_QUOTE || event.key.keysym.sym == SDLK_c) // + change selected node
                {
                    g.st += 1.f;
                    if(g.st > 31.f){g.st = 0.f;}
                    drawHud();
                }
                else if(event.key.keysym.sym == SDLK_RCTRL) // remove pointed voxel
                {
                    dtt = t+0.3f;
                    traceViewPath(0);
                    if(lray > 0)
                    {
                        g.voxels[lray].w = -1.f;
                        calcShipState();
                    }
                }
                else if(event.key.keysym.sym == SDLK_RSHIFT) // place a voxel
                {
                    traceViewPath(1);
                    placeVoxel(0.3f);
                }
                else if(event.key.keysym.sym == SDLK_r || event.key.keysym.sym == SDLK_g) // gravity toggle
                {
                    g.grav = 1 - g.grav;
                }
                else if(event.key.keysym.sym == SDLK_e || event.key.keysym.sym == SDLK_f) // change movement speeds
                {
                    fks = 1 - fks;
                    if(fks){g.ms = g.cms;}
                       else{g.ms = g.lms;}
                }
                else if(event.key.keysym.sym == SDLK_1) // change movement speeds
                {
                    g.ms = 9.3f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_2) // change movement speeds
                {
                    g.ms = 18.6f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_3) // change movement speeds
                {
                    g.ms = 37.2f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_4) // change movement speeds
                {
                    g.ms = 74.4f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_5) // change movement speeds
                {
                    g.ms = 148.8f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_6) // change movement speeds
                {
                    g.ms = 297.6f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_7) // change movement speeds
                {
                    g.ms = 595.2f;
                    if(fks){g.cms=g.ms;}else{g.lms=g.ms;}
                }
                else if(event.key.keysym.sym == SDLK_t)
                {
                    defaultState(1);
                }
                else if(event.key.keysym.sym == SDLK_F3)
                {
                    saveState();
                }
                else if(event.key.keysym.sym == SDLK_F8)
                {
                    loadState();
                }
// #ifdef VERBOSE
//                 else if(event.key.keysym.sym == SDLK_p)
//                 {
//                     for(uint i = 0; i < 100000; i++)
//                     {   // none of these voxels are grid-aligned without the roundf()
//                         if(g.num_voxels >= max_voxels){break;}
//                         g.voxels[g.num_voxels] = (vec){roundf(randfc()*ddist), roundf(randfc()*ddist), roundf(randfc()*ddist), esRand(0, 16)};
//                         g.num_voxels++;
//                     }
//                 }
// #endif
                else if(event.key.keysym.sym == SDLK_p)
                {
                    g.plock = 1 - g.plock;
                }
                idle = t;
            }
            break;

            case SDL_KEYUP:
            {
                if(focus_mouse == 0){break;}
                if(event.key.keysym.sym == SDLK_w){ks[0] = 0;}
                else if(event.key.keysym.sym == SDLK_a){ks[1] = 0;}
                else if(event.key.keysym.sym == SDLK_s){ks[2] = 0;}
                else if(event.key.keysym.sym == SDLK_d){ks[3] = 0;}
                else if(event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_LCTRL){ks[4] = 0;}
                else if(event.key.keysym.sym == SDLK_LEFT){ks[5] = 0;}
                else if(event.key.keysym.sym == SDLK_RIGHT){ks[6] = 0;}
                else if(event.key.keysym.sym == SDLK_UP){ks[7] = 0;}
                else if(event.key.keysym.sym == SDLK_DOWN){ks[8] = 0;}
                else if(event.key.keysym.sym == SDLK_SPACE){ks[9] = 0;}
                else if(event.key.keysym.sym == SDLK_RSHIFT){ptt = 0.f;}
                else if(event.key.keysym.sym == SDLK_RCTRL){dtt = 0.f;}
                idle = t;
            }
            break;

            case SDL_MOUSEWHEEL: // change selected node
            {
                if(focus_mouse == 0){break;}

                if(event.wheel.y > 0)
                {
                    g.st += 1.f;
                    if(g.st > 31.f){g.st = 0.f;}
                }
                else if(event.wheel.y < 0)
                {
                    g.st -= 1.f;
                    if(g.st < 0.f){g.st = 31.f;}
                }
                drawHud();
            }
            break;

            case SDL_MOUSEMOTION:
            {
                if(focus_mouse == 0){break;}
                mx = event.motion.x;
                my = event.motion.y;
                idle = t;
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT){ptt = 0.f;}
                else if(event.button.button == SDL_BUTTON_RIGHT){dtt = 0.f;}
                md = 0;
                idle = t;
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                lx = event.button.x;
                ly = event.button.y;
                mx = event.button.x;
                my = event.button.y;

                if(focus_mouse == 0) // lock mouse focus on every mouse input to the window
                {
                    SDL_ShowCursor(0);
                    focus_mouse = 1;
                    break;
                }

                if(event.button.button == SDL_BUTTON_LEFT) // place a voxel
                {
                    traceViewPath(1);
                    placeVoxel(0.3f);
                }
                else if(event.button.button == SDL_BUTTON_RIGHT) // remove pointed voxel
                {
                    dtt = t+0.3f;
                    traceViewPath(0);
                    if(lray > 0)
                    {
                        g.voxels[lray].w = -1.f;
                        calcShipState();
                    }
                }
                else if(event.button.button == SDL_BUTTON_MIDDLE) // clone pointed voxel
                {
                    traceViewPath(0);
                    if(lray > -1){g.st = g.voxels[lray].w;}
                }
                else if(event.button.button == SDL_BUTTON_X1) // change movement speeds
                {
                    fks = 1 - fks;
                    if(fks){g.ms = g.cms;}
                       else{g.ms = g.lms;}
                }
                idle = t;
            }
            break;

            case SDL_WINDOWEVENT:
            {
                if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    winw = event.window.data1;
                    winh = event.window.data2;
                    winw2 = winw/2;
                    winh2 = winh/2;
                    doPerspective();
                }
            }
            break;

            case SDL_QUIT:
            {
                SDL_HideWindow(wnd);
                drawText(NULL, "*K", 0, 0, 0);
                saveState();
                SDL_FreeSurface(sHud);
                SDL_FreeSurface(s_icon);
                SDL_GL_DeleteContext(glc);
                SDL_DestroyWindow(wnd);
                SDL_Quit();
                exit(0);
            }
            break;
        }
    }

//*************************************
// keystates
//*************************************
    if(focus_mouse == 1)
    {
        mGetViewZ(&look_dir, view); // <---- SETTING GLOBAL LOOK DIRECTION

        if(g.grav == 1 || g.plock == 1)
        {
            look_dir.z = -0.001f;
            vNorm(&look_dir);
        }

        if(ptt != 0.f && t > ptt) // place trigger
        {
            traceViewPath(1);
            placeVoxel(0.1f);
        }
        
        if(dtt != 0.f && t > dtt) // delete trigger
        {
            traceViewPath(0);
            if(lray > 0)
            {
                g.voxels[lray].w = -1.f;
                calcShipState();
            }
            dtt = t+0.1f;
        }

        if(ks[0] == 1) // W
        {
            vec m;
            vMulS(&m, look_dir, g.ms * dt);
            vSub(&g.pp, g.pp, m);
        }
        else if(ks[2] == 1) // S
        {
            vec m;
            vMulS(&m, look_dir, g.ms * dt);
            vAdd(&g.pp, g.pp, m);
        }

        if(ks[1] == 1) // A
        {
            vec vdc;
            mGetViewX(&vdc, view);
            vec m;
            vMulS(&m, vdc, g.ms * dt);
            vSub(&g.pp, g.pp, m);
        }
        else if(ks[3] == 1) // D
        {
            vec vdc;
            mGetViewX(&vdc, view);
            vec m;
            vMulS(&m, vdc, g.ms * dt);
            vAdd(&g.pp, g.pp, m);
        }

        if(ks[4] == 1) // LSHIFT (down)
        {
            vec vdc;
            if(g.plock == 1)
                vdc = (vec){0.f,0.f,-1.f};
            else
                mGetViewY(&vdc, view);
            vec m;
            vMulS(&m, vdc, g.ms * dt);
            vSub(&g.pp, g.pp, m);
        }
        else if(ks[9] == 1) // SPACE (up)
        {
            vec vdc;
            if(g.plock == 1)
                vdc = (vec){0.f,0.f,-1.f};
            else
                mGetViewY(&vdc, view);
            vec m;
            vMulS(&m, vdc, g.ms * dt);
            vAdd(&g.pp, g.pp, m);
        }

        if(ks[5] == 1) // LEFT
            g.xrot += 1.f*dt;
        else if(ks[6] == 1) // RIGHT
            g.xrot -= 1.f*dt;

        if(ks[7] == 1) // UP
            g.yrot += 1.f*dt;
        else if(ks[8] == 1) // DOWN
            g.yrot -= 1.f*dt;

        // player gravity
        if(g.grav == 1)
        {
            static uint falling = 0;

            vec vipp = g.pp; // inverse player position (setting global 'ipp' here is perfect)
            vInv(&vipp); // <--
            vipp.x = roundf(vipp.x);
            vipp.y = roundf(vipp.y);
            vipp.z = roundf(vipp.z); // now vipp is voxel aligned

            falling = 1;
            for(int j = 0; j < g.num_voxels; j++)
            {
                if(g.voxels[j].x == vipp.x && g.voxels[j].y == vipp.y && g.voxels[j].z == vipp.z-1.f)
                {
                    falling = 0;
                    break;
                }

                if(g.voxels[j].x == vipp.x && g.voxels[j].y == vipp.y && g.voxels[j].z == vipp.z-2.f)
                {
                    falling = 2;
                    break;
                }
            }
            // this extra loop makes you auto climb ALL nodes 
            for(int j = 0; j < g.num_voxels; j++)
            {
                if(g.voxels[j].x == vipp.x && g.voxels[j].y == vipp.y && g.voxels[j].z == vipp.z-1.f)
                {
                    falling = 0;
                    break;
                }
            }
            if(falling == 1)        {g.pp.z += (g.ms*0.5f)*dt;}
            else if(falling == 0)   {g.pp.z -= g.ms*dt;}

            mGetViewZ(&look_dir, view);
        }

    //*************************************
    // camera/mouse control
    //*************************************
        const float xd = lx-mx;
        const float yd = ly-my;
        if(xd != 0 || yd != 0)
        {
            g.xrot += xd*g.sens;
            g.yrot += yd*g.sens;
        
            if(g.plock == 1)
            {
                if(g.yrot > 3.11f)
                    g.yrot = 3.11f;
                if(g.yrot < 0.03f)
                    g.yrot = 0.03f;
            }
            else
            {
                if(g.yrot > 3.14f)
                    g.yrot = 3.14f;
                if(g.yrot < 0.1f)
                    g.yrot = 0.1f;
            }
            
            lx = winw2, ly = winh2;
            SDL_WarpMouseInWindow(wnd, lx, ly);
        }
    }

    mIdent(&view);
    mRotate(&view, g.yrot, 1.f, 0.f, 0.f);
    mRotate(&view, g.xrot, 0.f, 0.f, 1.f);
    mTranslate(&view, g.pp.x, g.pp.y, g.pp.z);

//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

    // shade for voxels
    shadeVoxel(&position_id, &projection_id, &view_id, &voxel_id, &normal_id, &texcoord_id, &sampler_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, mdlVoxel.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlVoxel.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlVoxel.tid);
    glVertexAttribPointer(texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(texcoord_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texmap);
    glUniform1i(sampler_id, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlVoxel.iid);

    // voxels
    ipp = g.pp; // inverse player position (setting global 'ipp' here is perfect)
    vInv(&ipp); // <--
    glUniformMatrix4fv(view_id, 1, GL_FALSE, (float*)&view.m[0][0]);
    mGetViewZ(&look_dir, view); // <---- SETTING GLOBAL LOOK DIRECTION
    for(int j = 0; j < g.num_voxels; j++)
    {
        if(g.voxels[j].w < 0.f || 
            vDistSq(ipp, g.voxels[j]) >= ddist2 ||
            insideFrustum(g.voxels[j].x, g.voxels[j].y, g.voxels[j].z) == 0){continue;}

        glUniform4f(voxel_id, g.voxels[j].x, g.voxels[j].y, g.voxels[j].z, g.voxels[j].w);
        glDrawElements(GL_TRIANGLES, voxel_numind, GL_UNSIGNED_BYTE, 0);
    }

    // targeting voxel
    vec rp = g.pb;
    traceViewPath(1); // !!!!!!!! <--- RAY CAST
    if(lray > -1)
    {
        // to halve the voxel size invert the texture id
        if(g.st == 0.f)
            glUniform4f(voxel_id, g.pb.x, g.pb.y, g.pb.z, -0.00001f); // but you cant invert 0 so use an epsilon
        else
            glUniform4f(voxel_id, g.pb.x, g.pb.y, g.pb.z, -g.st);
        
        glDrawElements(GL_TRIANGLES, voxel_numind, GL_UNSIGNED_BYTE, 0);
    }

    // hud
    shadeHud(&position_id, &texcoord_id, &sampler_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hudmap);
    glUniform1i(sampler_id, 0);
    glBindBuffer(GL_ARRAY_BUFFER, mdlPlane.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);
    glBindBuffer(GL_ARRAY_BUFFER, mdlPlane.tid);
    glVertexAttribPointer(texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(texcoord_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPlane.iid);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
        glDrawElements(GL_TRIANGLES, hud_numind, GL_UNSIGNED_BYTE, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

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
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) < 0)
    {
        printf("ERROR: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(wnd == NULL)
    {
        printf("ERROR: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    glc = SDL_GL_CreateContext(wnd);
    if(glc == NULL)
    {
        printf("ERROR: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    // set icon
    s_icon = surfaceFromData((Uint32*)&icon, 16, 16);
    SDL_SetWindowIcon(wnd, s_icon);

    // seed random
    srand(time(0));
    srandf(time(0));

    // get paths
    basedir = SDL_GetBasePath();
    appdir = SDL_GetPrefPath("voxdsp", "omnivoxel");

    // print info
    printf("----\n");
    printAttrib(SDL_GL_DOUBLEBUFFER, "GL_DOUBLEBUFFER");
    printAttrib(SDL_GL_DEPTH_SIZE, "GL_DEPTH_SIZE");
    printAttrib(SDL_GL_RED_SIZE, "GL_RED_SIZE");
    printAttrib(SDL_GL_GREEN_SIZE, "GL_GREEN_SIZE");
    printAttrib(SDL_GL_BLUE_SIZE, "GL_BLUE_SIZE");
    printAttrib(SDL_GL_ALPHA_SIZE, "GL_ALPHA_SIZE");
    printAttrib(SDL_GL_BUFFER_SIZE, "GL_BUFFER_SIZE");
    printAttrib(SDL_GL_DOUBLEBUFFER, "GL_DOUBLEBUFFER");
    printAttrib(SDL_GL_DEPTH_SIZE, "GL_DEPTH_SIZE");
    printAttrib(SDL_GL_STENCIL_SIZE, "GL_STENCIL_SIZE");
    printAttrib(SDL_GL_ACCUM_RED_SIZE, "GL_ACCUM_RED_SIZE");
    printAttrib(SDL_GL_ACCUM_GREEN_SIZE, "GL_ACCUM_GREEN_SIZE");
    printAttrib(SDL_GL_ACCUM_BLUE_SIZE, "GL_ACCUM_BLUE_SIZE");
    printAttrib(SDL_GL_ACCUM_ALPHA_SIZE, "GL_ACCUM_ALPHA_SIZE");
    printAttrib(SDL_GL_STEREO, "GL_STEREO");
    printAttrib(SDL_GL_MULTISAMPLEBUFFERS, "GL_MULTISAMPLEBUFFERS");
    printAttrib(SDL_GL_MULTISAMPLESAMPLES, "GL_MULTISAMPLESAMPLES");
    printAttrib(SDL_GL_ACCELERATED_VISUAL, "GL_ACCELERATED_VISUAL");
    printAttrib(SDL_GL_RETAINED_BACKING, "GL_RETAINED_BACKING");
    printAttrib(SDL_GL_CONTEXT_MAJOR_VERSION, "GL_CONTEXT_MAJOR_VERSION");
    printAttrib(SDL_GL_CONTEXT_MINOR_VERSION, "GL_CONTEXT_MINOR_VERSION");
    printAttrib(SDL_GL_CONTEXT_FLAGS, "GL_CONTEXT_FLAGS");
    printAttrib(SDL_GL_CONTEXT_PROFILE_MASK, "GL_CONTEXT_PROFILE_MASK");
    printAttrib(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, "GL_SHARE_WITH_CURRENT_CONTEXT");
    printAttrib(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, "GL_FRAMEBUFFER_SRGB_CAPABLE");
    printAttrib(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, "GL_CONTEXT_RELEASE_BEHAVIOR");
    printAttrib(SDL_GL_CONTEXT_EGL, "GL_CONTEXT_EGL");
    printf("----\n");
    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf("Compiled against SDL version %u.%u.%u.\n", compiled.major, compiled.minor, compiled.patch);
    printf("Linked against SDL version %u.%u.%u.\n", linked.major, linked.minor, linked.patch);
    printf("----\n");
    printf("currentPath: %s\n", basedir);
    printf("dataPath:    %s\n", appdir);
    printf("----\n");
    printf(">>> OmniVoxel <<<\n\n");
    printf("James William Fletcher (github.com/mrbid)\n\n");
    printf("Mouse locks when you click on the game window, press ESCAPE to unlock the mouse.\n\n");
    printf("W,A,S,D = Move around based on relative orientation to X and Y.\n");
    printf("SPACE + L-SHIFT = Move up and down relative Z.\n");
    printf("Left Click / R-SHIFT = Place node.\n");
    printf("Right Click / R-CTRL = Delete node.\n");
    printf("Arrow Keys = Move the view around.\n");
    printf("E / F / Mouse4 = Toggle player fast speed on and off.\n");
    printf("1-7 = Change move speed for selected fast state.\n");
    printf("Q / Middle Click = Clone texture of pointed node.\n");
    printf("Mouse Scroll / Slash + Quote / X + C = Change selected node type.\n");
    printf("T = Resets player state to start.\n");
    printf("P = Toggle pitch lock.\n");
    printf("R / G = Gravity on/off.\n");
    printf("F3 = Save. (auto saves on exit or idle for more than 3 minutes)\n");
    printf("F8 = Load. (will erase what you have done since the last save)\n");
    printf("----\n");

//*************************************
// projection
//*************************************
    doPerspective();

//*************************************
// compile & link shader program
//*************************************
    makeVoxel();
    makeHud();

//*************************************
// configure render options
//*************************************
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);

//*************************************
// init stuff
//*************************************

    // center voxel
    memset(g.voxels, 0x00, sizeof(vec)*max_voxels);
    g.voxels[0] = (vec){0.f, 0.f, 0.f, 0.f};
    g.voxels[1] = (vec){1.f, 1.f, 0.f, 0.f};
    g.voxels[2] = (vec){1.f, -1.f, 0.f, 0.f};
    g.voxels[3] = (vec){1.f, 0.f, 0.f, 0.f};
    g.num_voxels = 4;

    // load states
#ifdef NO_COMPRESSION
    if(loadState() == 0)
#else
    if(loadState() == 0 && loadUncompressedState() == 0)
#endif
    {
        // default state
        defaultState(0);

        // done
        char tmp[16];
        timestamp(tmp);
        printf("[%s] New world created.\n", tmp);
    }
    else
    {
        calcShipState();
    }
    if(g.num_voxels == 0){g.num_voxels = 1;}

    // argv mouse sensitivity
    if(argc >= 2){g.sens = atof(argv[1]);}

    // make sure voxel data is grid-aligned
    if(argc == 3)
    {
        for(uint i = 0; i < g.num_voxels; i++)
        {
            if(g.voxels[i].w < 0){continue;}
            g.voxels[i].x = roundf(g.voxels[i].x);
            g.voxels[i].y = roundf(g.voxels[i].y);
            g.voxels[i].z = roundf(g.voxels[i].z);
        }
    }
    
    // hud buffer
    sHud = SDL_RGBA32Surface(winw, winh);
    drawHud();
    hudmap = esLoadTextureA(winw, winh, sHud->pixels, 0);

//*************************************
// execute update / render loop
//*************************************
#ifdef VERBOSE
    t = fTime();
    uint fps = 0;
    float ft = t+3.f;
#endif
    while(1)
    {
#ifdef VERBOSE
        if(t > ft)
        {
            char tmp[16];
            timestamp(tmp);
            printf("[%s] %u fps, %u voxels\n", tmp, fps/3, g.num_voxels);
            fps = 0;
            ft = t+3.f;
        }
#endif
        main_loop();
#ifdef VERBOSE
        fps++;
#endif
    }
    return 0;
}
