/*
    James William Fletcher (github.com/mrbid)
        November 2022 - May 2023

    To reduce file size the icosphere could be
    generated on program execution by subdividing
    a icosahedron and then snapping the points to
    a unit sphere; and producing an index buffer
    for each triangle from each subdivision.
    
    Get current epoch: date +%s
    Start online game: ./fat <future epoch time> <msaa>

    !! I used to be keen on f32 but I tend to use float
    more now just because it's easier to type. I am still
    undecided.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>
#include <sys/time.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/mman.h>

#include <errno.h>

//#define VERBOSE
//#define IGNORE_OLD_PACKETS
//#define DEBUG_GL
//#define FAST_START
//#define FAKE_PLAYER
//#define SUN_GOD
#define ENABLE_PLAYER_PREDICTION

#pragma GCC diagnostic ignored "-Wunused-result"

#define uint unsigned int
#define sint int
#define f32 GLfloat

#include "inc/gl.h"
#define GLFW_INCLUDE_NONE
#include "inc/glfw3.h"

#ifndef __x86_64__
    #define NOSSE
#endif

#include "inc/esAux2.h"
#include "inc/protocol.h"

#include "inc/res.h"
#include "assets/models.h"
#include "assets/images.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024;
uint winh = 768;
uint center = 1;
int msaa = 16;
double t = 0;   // time
f32 dt = 0;     // delta time
double fc = 0;  // frame count
double lfct = 0;// last frame count time
f32 aspect;
double rww, ww, rwh, wh, ww2, wh2;
double uw, uh, uw2, uh2; // normalised pixel dpi
double x,y,lx,ly;

// render state id's
GLint projection_id = -1;
GLint modelview_id;
GLint normalmat_id = -1;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;
GLint texcoord_id;
GLint sampler_id;     // diffuse map
GLint specularmap_id; // specular map
GLint normalmap_id;   // normal map

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// models
ESModel mdlMenger;
ESModel mdlExo;
ESModel mdlRock[9];

ESModel mdlUfo;
ESModel mdlUfoBeam;
ESModel mdlUfoLights;
ESModel mdlUfoShield;
ESModel mdlUfoInterior;

// textures
GLuint tex[18];

// camera vars
#define FAR_DISTANCE 10000.f
vec lightpos = {0.f, 0.f, 0.f};
uint focus_cursor = 0;
f32 sens = 0.001f;
f32 rollsens = 0.001f;

// game vars
#define GFX_SCALE 0.01f
#define MOVE_SPEED 0.5f
#define MAX_PLAYERS 256
#define MAX_COMETS 65535
uint16_t NUM_COMETS = 64;
uint rcs = 0;
f32 rrcs = 0.f;
uint keystate[8] = {0};
vec pp = {0.f, 0.f, 0.f};   // velocity
vec ppr = {0.f, 0.f, -2.3f};// actual position
uint brake = 0;
uint hits = 0;
uint popped = 0;
f32 damage = 0;
f32 sun_pos = 0.f;
time_t sepoch = 0;
int lgdonce = 0;
uint te[2] = {0};
uint8_t killgame = 0;
pthread_t tid[2];
uint autoroll = 1;
f32 sunsht = 0.f;
uint controls = 0;

typedef struct
{
    vec vel, newvel, pos, newpos;
    f32 rot, newrot, scale, newscale, is_boom;
} comet;

f32 players[MAX_PLAYERS*3] = {0};
f32 pvel[MAX_PLAYERS*3] = {0};
f32 pfront[MAX_PLAYERS*3] = {0};
f32 pup[MAX_PLAYERS*3] = {0};
comet comets[MAX_COMETS] = {0};
f32 exohit[655362] = {0}; // 0.65 MB * 4 = 2.6 MB

// networking
#define UDP_PORT 8086
#define RECV_BUFF_SIZE 512
char server_host[256];
uint32_t sip = 0; // server ip
const uint64_t pid = 0x75F6073677E10C44; // protocol id
int ssock = -1;
struct sockaddr_in server;
uint8_t myid = 0;
uint64_t latest_packet = 0;
f32 rncr = 0.f; // reciprocal number comet render
_Atomic uint64_t last_render_time = 0;

//*************************************
// utility functions
//*************************************
#define isequal(str, len, compared_to_str) len == sizeof(compared_to_str) - 1 && memcmp(str, compared_to_str, len) == 0
void loadConfig()
{
    const char* home = getenv("HOME");
    int fd;
    if(home == NULL)
    {
        fd = open(".config/fat.cfg", O_RDONLY);
    }
    else
    {
        char fp[strlen(home)+32];
        sprintf(fp, "%s/.config/fat.cfg", home);
        fd = open(fp, O_RDONLY);
    }
    if(fd < 0)
    {
        fd = open("fat.cfg", O_RDONLY);
        if(fd < 0){return;}
    }
    puts("!! fat.cfg detected (argv parameters will override the config)");
    off_t len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char data[len+1];
    size_t len_read = read(fd, data, len);
    data[len] = 0;
    int offset = 0;
    int wasroll = 0;
    while(offset < len){
        int paramlen;
        int linelen;
        int val;
        if(sscanf(data+offset, "%*s%n %i%n", &paramlen, &val, &linelen) != 1){break;}
             if(isequal(data+offset, paramlen, "NUM_ASTEROIDS")){NUM_COMETS = val;}
        else if(isequal(data+offset, paramlen, "MSAA")){msaa = val;}
        else if(isequal(data+offset, paramlen, "AUTOROLL")){autoroll = val;}
        else if(isequal(data+offset, paramlen, "CENTER_WINDOW")){center = val;}
        else if(isequal(data+offset, paramlen, "CONTROLS")){controls = val;}
        else if(isequal(data+offset, paramlen, "LOOKSENS")){sens = 0.0001f*(f32)val;}
        else if(isequal(data+offset, paramlen, "ROLLSENS")){rollsens = 0.0001f*(f32)val; wasroll=1;}
        offset += linelen;
        while(data[offset] && data[offset] != '\n'){offset++;}
        offset++;
    }
    close(fd);
    if(wasroll == 0){rollsens = sens;}
}

#ifdef VERBOSE
void dumpComet(const uint id)
{
    printf("%g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g\n",
        comets[id].pos.x,
        comets[id].pos.y,
        comets[id].pos.z,
        comets[id].vel.x,
        comets[id].vel.y,
        comets[id].vel.z,
        comets[id].newpos.x,
        comets[id].newpos.y,
        comets[id].newpos.z,
        comets[id].newvel.x,
        comets[id].newvel.y,
        comets[id].newvel.z,
        comets[id].rot,
        comets[id].newrot,
        comets[id].scale,
        comets[id].newscale);
}

void dumpZeroComets()
{
    for(uint16_t i = 0; i < NUM_COMETS; i++)
    {
        if(comets[i].pos.x == 0 && comets[i].pos.y == 0 && comets[i].pos.z == 0)
        {
            printf("[%u] %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g\n",
                i,
                comets[i].pos.x,
                comets[i].pos.y,
                comets[i].pos.z,
                comets[i].vel.x,
                comets[i].vel.y,
                comets[i].vel.z,
                comets[i].newpos.x,
                comets[i].newpos.y,
                comets[i].newpos.z,
                comets[i].newvel.x,
                comets[i].newvel.y,
                comets[i].newvel.z,
                comets[i].rot,
                comets[i].newrot,
                comets[i].scale,
                comets[i].newscale);
        }
    }
    printf("----\n");
}

void dumpComets()
{
    for(uint16_t i = 0; i < NUM_COMETS; i++)
    {
        printf("[%u] %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
            i,
            comets[i].pos.x,
            comets[i].pos.y,
            comets[i].pos.z,
            comets[i].vel.x,
            comets[i].vel.y,
            comets[i].vel.z,
            comets[i].newpos.x,
            comets[i].newpos.y,
            comets[i].newpos.z,
            comets[i].newvel.x,
            comets[i].newvel.y,
            comets[i].newvel.z,
            comets[i].rot,
            comets[i].newrot,
            comets[i].scale,
            comets[i].newscale);
    }
    printf("----\n");
}
#endif

void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
uint64_t microtime()
{
    struct timeval tv;
    struct timezone tz;
    memset(&tz, 0, sizeof(struct timezone));
    gettimeofday(&tv, &tz);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}
int urand_int()
{
    int f = open("/dev/urandom", O_RDONLY | O_CLOEXEC), s;
    read(f, &s, sizeof(int));
    close(f);
    return s;
}
f32 microtimeToScalar(const uint64_t timestamp)
{
    if(last_render_time == 0){last_render_time = microtime();}
    return (f32)(int64_t)(last_render_time-timestamp) * 0.000001f;
}
void scaleBuffer(f32* b, GLsizeiptr s)
{
    for(GLsizeiptr i = 0; i < s; i++)
        b[i] *= GFX_SCALE;
}
void scaleBufferC(f32* b, GLsizeiptr s, const f32 scale)
{
    for(GLsizeiptr i = 0; i < s; i++)
    {
        b[i] *= scale;
        b[i] *= -1.f;
    }
}
void flipUV(f32* b, GLsizeiptr s)
{
    for(GLsizeiptr i = 1; i < s; i+=2)
        b[i] *= -1.f;
}
void killGame()
{
    damage = 1337.f; // this trips the threads into exiting and flipping `killgame = 1`
}
void doExoImpact(vec p, f32 f)
{
    for(GLsizeiptr h = 0; h < exo_numvert; h++)
    {
        if(exohit[h] == 0.03f){continue;}
        const GLsizeiptr i = h*3;
        vec v = {exo_vertices[i], exo_vertices[i+1], exo_vertices[i+2]};
        f32 ds = vDistSq(v, p);
        if(ds < f*f)
        {
            ds = vDist(v, p);
            vNorm(&v);
            f32 sr = f-ds;
            if(exohit[h]+sr > 0.03f){sr -= (exohit[h]+sr)-0.03f;}
            vMulS(&v, v, sr);
            exo_vertices[i]   -= v.x;
            exo_vertices[i+1] -= v.y;
            exo_vertices[i+2] -= v.z;
            exohit[h] += sr;
            if(exohit[h] == 0.03f)
            {
                exo_colors[i]   = inner_colors[i];
                exo_colors[i+1] = inner_colors[i+1];
                exo_colors[i+2] = inner_colors[i+2];
            }
            else
            {
                exo_colors[i]   -= 0.2f;
                exo_colors[i+1] -= 0.2f;
                exo_colors[i+2] -= 0.2f;
            }
        }
    }
    esRebind(GL_ARRAY_BUFFER, &mdlExo.vid, exo_vertices, exo_vertices_size, GL_STATIC_DRAW);
    esRebind(GL_ARRAY_BUFFER, &mdlExo.cid, exo_colors, exo_colors_size, GL_STATIC_DRAW);
}
void incrementHits()
{
    hits++;
    char title[256];
    sprintf(title, "AstroImpact | %u/%u | %.2f%% | %.2f mins", hits, popped, 100.f*damage, (time(0)-sepoch)/60.0);
    glfwSetWindowTitle(window, title);
    if(damage > 10.f)
    {
        killgame = 1;
        sprintf(title, "AstroImpact | %u/%u | 100%% | %.2f mins | GAME END", hits, popped, (time(0)-sepoch)/60.0);
        glfwSetWindowTitle(window, title);
    }
}


//*************************************
// networking utility functions
//*************************************
// void dumpPacket(const char* buff, const size_t rs)
// {
//     FILE* f = fopen("packet.dat", "wb");
//     if(f != NULL)
//     {
//         fwrite(buff, rs, 1, f);
//         fclose(f);
//     }
//     exit(EXIT_SUCCESS);
// }
uint32_t HOSTtoIPv4(const char* hostname)
{
    struct hostent* host = gethostbyname(hostname);
    if(host == NULL){return -1;}
    struct in_addr** addr = (struct in_addr**)host->h_addr_list;
    for(int i = 0; addr[i] != NULL; i++){return addr[i]->s_addr;}
    return 0;
}

int csend(const unsigned char* data, const size_t len)
{
    uint retry = 0;
    while(sendto(ssock, data, len, 0, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        retry++;
        if(retry > 3)
        {
            printf("csend() failed.\n");
            printf("ERROR: %s\n", strerror(errno));
            break;
        }
        usleep(100000); // 100ms
    }
    return 0;
}

//*************************************
// network send thread
//*************************************
void *sendThread(void *arg)
{
    // dont send until 1 second before game start
    //sleep(time(0)-sepoch-1);

    // send position on interval until end game
    const size_t packet_len = 65;
    unsigned char packet[packet_len];
    memcpy(&packet, &pid, sizeof(uint64_t));
    memcpy(&packet[8], &(uint8_t){MSG_TYPE_PLAYER_POS}, sizeof(uint8_t));
    const size_t poslen = sizeof(f32)*3;
    const useconds_t st = 50000;
    useconds_t cst = st;
    const uint retries = 3;
    const useconds_t rst = st / retries;
    te[0] = 1;
    while(1)
    {
        // kill (game over)
        if(damage == 1337.f){break;}
        else if(damage > 10.f)
        {
            killgame = 1;
            char title[256];
            sprintf(title, "AstroImpact | %u/%u | 100%% | %.2f mins | GAME END", hits, popped, (time(0)-sepoch)/60.0);
            glfwSetWindowTitle(window, title);
            printf("sendThread: exiting, game over.\n");
            break;
        }

        // every 10ms
        usleep(cst);

        // send position
        cst = 0;
        const uint64_t mt = HTOLE64(microtime());
        memcpy(&packet[9], &mt, sizeof(uint64_t));
        memcpy(&packet[17], &(uint32_t[]){HTOLE32F(ppr.x), HTOLE32F(ppr.y), HTOLE32F(ppr.z)}, poslen);
        memcpy(&packet[29], &(uint32_t[]){HTOLE32F(pp.x), HTOLE32F(pp.y), HTOLE32F(pp.z)}, poslen);
        memcpy(&packet[41], &(uint32_t[]){HTOLE32F(-view.m[0][2]), HTOLE32F(-view.m[1][2]), HTOLE32F(-view.m[2][2])}, poslen);
        memcpy(&packet[53], &(uint32_t[]){HTOLE32F(-view.m[0][1]), HTOLE32F(-view.m[1][1]), HTOLE32F(-view.m[2][1])}, poslen);
        uint retry = 0;
        while(sendto(ssock, packet, packet_len, 0, (struct sockaddr*)&server, sizeof(server)) < 0)
        {
            usleep(rst);
            retry++;
            cst += rst;
            if(retry >= retries)
            {
                printf("sendThread: sendto() failed after %u retries.\n", retry);
                printf("ERROR: %s\n", strerror(errno));
                break;
            }
        }
        if(cst == 0 || cst > st){cst = st;}else{cst = st - (rst*retry);}
    }
}

//*************************************
// network recv thread
//*************************************
void window_size_callback(GLFWwindow* window, int width, int height);
void *recvThread(void *arg)
{
    uint slen = sizeof(server);
    unsigned char buff[RECV_BUFF_SIZE];
    te[1] = 1;
    while(1)
    {
//*************************************
// INIT / CONTROL
//*************************************
        // kill (game over)
        if(damage == 1337.f){break;}
        else if(damage > 10.f)
        {
            killgame = 1;
            char title[256];
            sprintf(title, "AstroImpact | %u/%u | 100%% | %.2f mins | GAME END", hits, popped, (time(0)-sepoch)/60.0);
            glfwSetWindowTitle(window, title);
            printf("recvThread: exiting, game over.\n");
            break;
        }

        // loading game
        static time_t lgnt = 0;
        if(lgdonce == 0 && time(0) < sepoch && lgnt != sepoch-time(0))
        {
            char title[256];
            uint wp = 0;
            for(uint i = 0; i < MAX_PLAYERS; i++)
            {
                const uint j = i*3;
                if(players[j] != 0.f || players[j+1] != 0.f || players[j+2] != 0.f)
                    wp++;
            }
            const time_t tleft = sepoch-time(0);
            sprintf(title, "Please wait... %lu seconds. Total players waiting %u.", tleft, wp+1);
            glfwSetWindowTitle(window, title);

            // allow early exit
            glfwPollEvents();
            if(glfwWindowShouldClose(window))
            {
                // send disconnect message
                for(int i = 0; i < 3; i++)
                {
                    unsigned char packet[9];
                    memcpy(&packet, &pid, sizeof(uint64_t));
                    memcpy(&packet[8], &(uint8_t){MSG_TYPE_PLAYER_DISCONNECTED}, sizeof(uint8_t));
                    csend(packet, 9);
                    usleep(100000);
                }
                glfwDestroyWindow(window);
                glfwTerminate();
                exit(EXIT_SUCCESS);
            }

            lgnt = tleft;
        }
        else if(lgdonce == 0 && time(0) >= sepoch)
        {
            glfwSetWindowTitle(window, "AstroImpact");
            glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
            lgdonce = 1;
        }

        // wait for packet
        memset(buff, 0x00, sizeof(buff));
        const ssize_t rs = recvfrom(ssock, buff, RECV_BUFF_SIZE, 0, (struct sockaddr *)&server, &slen);
        if(rs < 9){continue;} // must have pid + id minimum
        if(memcmp(&buff[0], &pid, sizeof(uint64_t)) != 0){continue;} // invalid protocol id?

#ifdef VERBOSE
        // debug protocol
        uint64_t rpid = 0;
        uint8_t rpacid = 0;
        memcpy(&rpid, &buff[0], sizeof(uint64_t));
        memcpy(&rpacid, &buff[8], sizeof(uint8_t));
        printf("PACKET: (PID) 0x%lX / (ID) 0x%.02X\n", rpid, rpacid);
#endif

//*************************************
// MSG_TYPE_ASTEROID_POS
//*************************************
        if(buff[8] == MSG_TYPE_ASTEROID_POS) // asteroid pos
        {
            static const size_t ps = sizeof(uint16_t)+(sizeof(uint32_t)*8);
            size_t ofs = sizeof(uint64_t)+sizeof(uint8_t);
            uint64_t timestamp = 0;
            if(rs > sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint64_t))
            {
                memcpy(&timestamp, &buff[ofs], sizeof(uint64_t));
                timestamp = LE64TOH(timestamp);
                ofs += sizeof(uint64_t);
#ifdef IGNORE_OLD_PACKETS
                if(timestamp >= latest_packet) // skip this packet if it is not new
                    latest_packet = timestamp;
                else
                    continue;
#endif
            }
            while(ofs+ps <= rs)
            {
                uint16_t id;
                memcpy(&id, &buff[ofs], sizeof(uint16_t));
                id = LE16TOH(id);

                if(id >= MAX_COMETS){continue;}
                ofs += sizeof(uint16_t);

                uint32_t tmp[3];
                if(comets[id].is_boom > 0.f) // nonono ! im animating!
                {
                    memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                    comets[id].newvel.x = LE32FTOH(tmp[0]);
                    comets[id].newvel.y = LE32FTOH(tmp[1]);
                    comets[id].newvel.z = LE32FTOH(tmp[2]);
                    ofs += sizeof(uint32_t)*3;
                    
                    memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                    comets[id].newpos.x = LE32FTOH(tmp[0]);
                    comets[id].newpos.y = LE32FTOH(tmp[1]);
                    comets[id].newpos.z = LE32FTOH(tmp[2]);
                    ofs += sizeof(uint32_t)*3;

                    memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                    comets[id].newrot = LE32FTOH(tmp[0]);
                    ofs += sizeof(uint32_t);

                    memcpy(&comets[id].newscale, &buff[ofs], sizeof(uint32_t));
                    comets[id].newscale = LE32FTOH(tmp[0]);
                    ofs += sizeof(uint32_t);

                    continue;
                }

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                comets[id].vel.x = LE32FTOH(tmp[0]);
                comets[id].vel.y = LE32FTOH(tmp[1]);
                comets[id].vel.z = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                comets[id].pos.x = LE32FTOH(tmp[0]);
                comets[id].pos.y = LE32FTOH(tmp[1]);
                comets[id].pos.z = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                // --
                    // timestamp correction
                    const f32 s = microtimeToScalar(timestamp);
                    vec c = comets[id].vel;
                    vMulS(&c, c, s);
                    vAdd(&comets[id].pos, comets[id].pos, c);
                // --

                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                comets[id].rot = LE32FTOH(tmp[0]);
                ofs += sizeof(uint32_t);

                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                comets[id].scale = LE32FTOH(tmp[0]);
                ofs += sizeof(uint32_t);

#ifdef VERBOSE
                //printf("A[%u]: %+.2f %+.2f %+.2f, %+.2f %+.2f %+.2f, %+.2f %+.2f %+.2f, %+.2f %+.2f %+.2f, %+.2f, %+.2f, %+.2f, %+.2f\n", id, comets[id].pos.x, comets[id].pos.y, comets[id].pos.z, comets[id].vel.x, comets[id].vel.y, comets[id].vel.z, comets[id].newpos.x, comets[id].newpos.y, comets[id].newpos.z, comets[id].newvel.x, comets[id].newvel.y, comets[id].newvel.z, comets[id].rot, comets[id].newrot, comets[id].scale, comets[id].newscale);
                printf("A[%u]: %+.2f, %+.2f, %+.2f\n", id, comets[id].pos.x, comets[id].pos.y, comets[id].pos.z);
#endif
            }
        }
//*************************************
// MSG_TYPE_PLAYER_POS
//*************************************
        else if(buff[8] == MSG_TYPE_PLAYER_POS) // player pos
        {
            static const size_t ps = sizeof(uint8_t)+(sizeof(uint32_t)*12);
            size_t ofs = sizeof(uint64_t)+sizeof(uint8_t);
            uint64_t timestamp = 0;
            if(rs > sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint64_t))
            {
                memcpy(&timestamp, &buff[ofs], sizeof(uint64_t));
                timestamp = LE64TOH(timestamp);
                ofs += sizeof(uint64_t);
#ifdef IGNORE_OLD_PACKETS
                if(timestamp >= latest_packet) // skip this packet if it is not new
                    latest_packet = timestamp;
                else
                    continue;
#endif
            }
            while(ofs+ps <= rs)
            {
                uint8_t id;
                memcpy(&id, &buff[ofs], sizeof(uint8_t));
                if(id == myid || id >= MAX_PLAYERS) // nonono ! that's me!
                {
                    ofs += ps;
                    continue;
                }
                ofs += sizeof(uint8_t);
                uint32_t tmp[3];
                uint16_t ido = id*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                players[ido]   = LE32FTOH(tmp[0]);
                players[ido+1] = LE32FTOH(tmp[1]);
                players[ido+2] = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                pvel[ido]   = LE32FTOH(tmp[0]);
                pvel[ido+1] = LE32FTOH(tmp[1]);
                pvel[ido+2] = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                pfront[ido]   = LE32FTOH(tmp[0]);
                pfront[ido+1] = LE32FTOH(tmp[1]);
                pfront[ido+2] = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                pup[ido]   = LE32FTOH(tmp[0]);
                pup[ido+1] = LE32FTOH(tmp[1]);
                pup[ido+2] = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

#ifdef ENABLE_PLAYER_PREDICTION
                // timestamp correction
                const f32 s = microtimeToScalar(timestamp);
                vec p = (vec){players[ido], players[ido+1], players[ido+2]};
                vec v = (vec){pvel[ido], pvel[ido+1], pvel[ido+2]};
                vMulS(&v, v, s);
                vAdd(&p, p, v);
                players[ido]   = p.x;
                players[ido+1] = p.y;
                players[ido+2] = p.z;
#endif
#ifdef VERBOSE
                printf("P[%u]: %+.2f, %+.2f, %+.2f - %+.2f, %+.2f, %+.2f\n", id, players[ido], players[ido+1], players[ido+2], pvel[ido], pvel[ido+1], pvel[ido+2]);
#endif
            }
        }
//*************************************
// MSG_TYPE_SUN_POS
//*************************************
        else if(buff[8] == MSG_TYPE_SUN_POS) // sun pos
        {
            static const size_t ps = sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint64_t)+sizeof(uint32_t);
            if(rs == ps)
            {
                uint64_t timestamp = 0;
                memcpy(&timestamp, &buff[sizeof(uint64_t)+sizeof(uint8_t)], sizeof(uint64_t));
                timestamp = LE64TOH(timestamp);
#ifdef IGNORE_OLD_PACKETS
                if(timestamp >= latest_packet) // skip this packet if it is not new
                    latest_packet = timestamp;
                else
                    continue;
#endif
                uint32_t tmp;
                memcpy(&tmp, &buff[sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint64_t)], sizeof(uint32_t));
                sun_pos = LE32FTOH(tmp);

                // timestamp correction
                const f32 s = microtimeToScalar(timestamp);
                sun_pos += 0.03f*s;
            }
#ifdef VERBOSE
            printf("S: %+.2f\n", sun_pos);
#endif
        }
//*************************************
// MSG_TYPE_ASTEROID_DESTROYED
//*************************************
        else if(buff[8] == MSG_TYPE_ASTEROID_DESTROYED) // asteroid destroyed **RELIABLE**
        {
            static const size_t ps = sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint64_t)+sizeof(uint16_t)+(sizeof(uint32_t)*8);
            size_t ofs = sizeof(uint64_t)+sizeof(uint8_t);
            if(rs == ps)
            {
                // read packet
                uint64_t timestamp = 0;
                memcpy(&timestamp, &buff[ofs], sizeof(uint64_t));
                timestamp = LE64TOH(timestamp);
#ifdef IGNORE_OLD_PACKETS
                if(timestamp > latest_packet) // skip this packet if it is not new
                    latest_packet = timestamp;
                else
                    continue;
#endif
                ofs += sizeof(uint64_t);
                uint16_t id;
                memcpy(&id, &buff[ofs], sizeof(uint16_t));
                id = LE16TOH(id);
                ofs += sizeof(uint16_t);

                uint32_t tmp[3];
                
                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                comets[id].newvel.x = LE32FTOH(tmp[0]);
                comets[id].newvel.y = LE32FTOH(tmp[1]);
                comets[id].newvel.z = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                comets[id].newpos.x = LE32FTOH(tmp[0]);
                comets[id].newpos.y = LE32FTOH(tmp[1]);
                comets[id].newpos.z = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                // --
                    // timestamp correction
                    const f32 s = microtimeToScalar(timestamp);
                    vec c = comets[id].newvel;
                    vMulS(&c, c, s);
                    vAdd(&comets[id].newpos, comets[id].newpos, c);
                // --

                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                comets[id].newrot = LE32FTOH(tmp[0]);
                ofs += sizeof(uint32_t);

                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                comets[id].newscale = LE32FTOH(tmp[0]);

                // set asteroid to boom state
#ifdef VERBOSE
                //dumpComet(id);
                printf("[%u] asteroid_destroyed received.\n", id);
#endif
                if(comets[id].is_boom == 0.f)
                {
                    popped++;
                    char title[256];
                    sprintf(title, "AstroImpact | %u/%u | %.2f%% | %.2f mins", hits, popped, 100.f*damage, (time(0)-sepoch)/60.0);
                    glfwSetWindowTitle(window, title);
                }
                comets[id].is_boom = 1.f;

                // send confirmation
                unsigned char packet[11];
                memcpy(&packet, &pid, sizeof(uint64_t));
                memcpy(&packet[8], &(uint8_t){MSG_TYPE_ASTEROID_DESTROYED_RECVD}, sizeof(uint8_t));
                id = HTOLE16(id);
                memcpy(&packet[9], &id, sizeof(uint16_t));
                csend(packet, 11);
            }
        }
//*************************************
// MSG_TYPE_EXO_HIT
//*************************************
        else if(buff[8] == MSG_TYPE_EXO_HIT) // exo hit **RELIABLE**
        {
            static const size_t ps = sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint64_t)+sizeof(uint16_t)+sizeof(uint16_t)+(sizeof(uint32_t)*16)+sizeof(uint32_t);
            size_t ofs = sizeof(uint64_t)+sizeof(uint8_t);
            if(rs == ps)
            {
                // read packet
                uint64_t timestamp = 0;
                memcpy(&timestamp, &buff[ofs], sizeof(uint64_t));
                timestamp = LE64TOH(timestamp);
                ofs += sizeof(uint64_t);
#ifdef IGNORE_OLD_PACKETS
                if(timestamp >= latest_packet) // skip this packet if it is not new
                    latest_packet = timestamp;
                else
                    continue;
#endif
                
                uint16_t hit_id, asteroid_id;
                memcpy(&hit_id, &buff[ofs], sizeof(uint16_t));
                hit_id = LE16TOH(hit_id);
                ofs += sizeof(uint16_t);
                memcpy(&asteroid_id, &buff[ofs], sizeof(uint16_t));
                asteroid_id = LE16TOH(asteroid_id);
                ofs += sizeof(uint16_t);
                
                uint32_t tmp[3];

                if(comets[asteroid_id].is_boom <= 0.f) // animating
                {
                    memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                    comets[asteroid_id].vel.x = LE32FTOH(tmp[0]);
                    comets[asteroid_id].vel.y = LE32FTOH(tmp[1]);
                    comets[asteroid_id].vel.z = LE32FTOH(tmp[2]);
                }
                ofs += sizeof(uint32_t)*3;

#ifdef VERBOSE // prediction accuracy test
                const vec op = comets[asteroid_id].pos;
#endif
                
                if(comets[asteroid_id].is_boom <= 0.f) // animating
                {
                    memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                    comets[asteroid_id].pos.x = LE32FTOH(tmp[0]);
                    comets[asteroid_id].pos.y = LE32FTOH(tmp[1]);
                    comets[asteroid_id].pos.z = LE32FTOH(tmp[2]);
                }
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                comets[asteroid_id].newvel.x = LE32FTOH(tmp[0]);
                comets[asteroid_id].newvel.y = LE32FTOH(tmp[1]);
                comets[asteroid_id].newvel.z = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                memcpy(&tmp, &buff[ofs], sizeof(uint32_t)*3);
                comets[asteroid_id].newpos.x = LE32FTOH(tmp[0]);
                comets[asteroid_id].newpos.y = LE32FTOH(tmp[1]);
                comets[asteroid_id].newpos.z = LE32FTOH(tmp[2]);
                ofs += sizeof(uint32_t)*3;

                // --
                    // timestamp correction
                    const f32 s = microtimeToScalar(timestamp);
                    vec c = comets[asteroid_id].newvel;
                    vMulS(&c, c, s);
                    vAdd(&comets[asteroid_id].newpos, comets[asteroid_id].newpos, c);
                // --

                if(comets[asteroid_id].is_boom <= 0.f) // animating
                {
                    memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                    comets[asteroid_id].rot = LE32FTOH(tmp[0]);
                }
                ofs += sizeof(uint32_t);

                if(comets[asteroid_id].is_boom <= 0.f) // animating
                {
                    memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                    comets[asteroid_id].scale = LE32FTOH(tmp[0]);
                }
                ofs += sizeof(uint32_t);

                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                comets[asteroid_id].newrot = LE32FTOH(tmp[0]);
                ofs += sizeof(uint32_t);

                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                comets[asteroid_id].newscale = LE32FTOH(tmp[0]);
                ofs += sizeof(uint32_t);

#ifdef VERBOSE // prediction accuracy test
                const vec np = comets[asteroid_id].pos;
                const f32 d = vDist(op, np);
                if(d > 0.1f){printf("EXO_HIT_DIST: %g\n", vDist(op, np));printf("%g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g\n", op.x, op.y, op.z, comets[asteroid_id].pos.x, comets[asteroid_id].pos.y, comets[asteroid_id].pos.z, comets[asteroid_id].vel.x, comets[asteroid_id].vel.y, comets[asteroid_id].vel.z, comets[asteroid_id].newpos.x, comets[asteroid_id].newpos.y, comets[asteroid_id].newpos.z, comets[asteroid_id].newvel.x, comets[asteroid_id].newvel.y, comets[asteroid_id].newvel.z, comets[asteroid_id].rot, comets[asteroid_id].newrot, comets[asteroid_id].scale, comets[asteroid_id].newscale);}
#endif

                // kill game?
                memcpy(&tmp[0], &buff[ofs], sizeof(uint32_t));
                damage = LE32FTOH(tmp[0]);

#ifdef VERBOSE
                //dumpComet(asteroid_id);

                if(damage > 10.f)
                {
                    printf("[%u] exo_hit [END GAME]\n", asteroid_id);
                }
                else
                {
                    printf("[%u] exo_hit\n", asteroid_id);
                }
#endif

                if(comets[asteroid_id].is_boom <= 0.f) // animating
                {
                    // do impact
                    incrementHits();

                    // set asteroid to boom state
                    vec dir = comets[asteroid_id].vel;
                    vNorm(&dir);
                    vec fwd;
                    vMulS(&fwd, dir, 0.03f);
                    vAdd(&comets[asteroid_id].pos, comets[asteroid_id].pos, fwd);
                    comets[asteroid_id].is_boom = 1.f;
                }

                // send confirmation
                unsigned char packet[11];
                memcpy(&packet, &pid, sizeof(uint64_t));
                memcpy(&packet[8], &(uint8_t){MSG_TYPE_EXO_HIT_RECVD}, sizeof(uint8_t));
                hit_id = HTOLE16(hit_id);
                memcpy(&packet[9], &hit_id, sizeof(uint16_t));
                csend(packet, 11);
            }
        }
//*************************************
// MSG_TYPE_PLAYER_DISCONNECTED
//*************************************
        else if(buff[8] == MSG_TYPE_PLAYER_DISCONNECTED) // player disconnect
        {
            static const size_t ps = sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint8_t);
            if(rs == ps)
            {
                // read packet
                uint8_t id;
                memcpy(&id, &buff[sizeof(uint64_t)+sizeof(uint8_t)], sizeof(uint8_t));
                uint16_t ido = id*3;
                players[ido]   = 0.f;
                players[ido+1] = 0.f;
                players[ido+2] = 0.f;

//#ifdef VERBOSE
                printf("[%u] player_disconnected\n", id);
//#endif

                // send confirmation
                unsigned char packet[10];
                memcpy(&packet, &pid, sizeof(uint64_t));
                memcpy(&packet[8], &(uint8_t){MSG_TYPE_PLAYER_DISCONNECTED_RECVD}, sizeof(uint8_t));
                memcpy(&packet[9], &id, sizeof(uint8_t));
                csend(packet, 10);
                
                // if we are the player disconnecting, let's quit!
                if(id == myid){killGame();}
            }
        }
//*************************************
// MSG_TYPE_REGISTER_ACCEPTED
//*************************************
        else if(buff[8] == MSG_TYPE_REGISTER_ACCEPTED) // registration accepted
        {
            static uint accepted = 0;
            if(accepted == 0)
            {
                static const size_t ps = sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t);
                if(rs == ps)
                {
                    memcpy(&myid, &buff[sizeof(uint64_t)+sizeof(uint8_t)], sizeof(uint8_t));
                    memcpy(&NUM_COMETS, &buff[sizeof(uint64_t)+sizeof(uint8_t)+sizeof(uint8_t)], sizeof(uint16_t));
                    NUM_COMETS = LE16TOH(NUM_COMETS);
                    if(NUM_COMETS > MAX_COMETS){NUM_COMETS = MAX_COMETS;}
                    else if(NUM_COMETS < 16){NUM_COMETS = 16;}
                    rcs = NUM_COMETS / 8;
                    rrcs = 1.f / (f32)rcs;
                    rncr = 11.f / NUM_COMETS;
                    printf("recvThread: Registration was accepted; id(%u), num_comets(%u).\n", myid, NUM_COMETS);
                }
                else
                    printf("recvThread: Registration was accepted; id(%u).\n", myid);
                accepted = 1;
            }
        }
//*************************************
// MSG_TYPE_BAD_REGISTER_VALUE
//*************************************
        else if(buff[8] == MSG_TYPE_BAD_REGISTER_VALUE) // epoch too old
        {
            printf("recvThread: Your epoch already expired, it's too old slow poke!\n");
            exit(EXIT_FAILURE);
        }
    }
}

//*************************************
// init networking
//*************************************
int initNet()
{
    // resolve host
    sip = HOSTtoIPv4(server_host);
    if(sip <= 0)
    {
        printf("HOSTtoIPv4() failed.\n");
        return -1;
    }

    // create socket to send to server on
    if((ssock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("socket() failed.");
        printf("ERROR: %s\n", strerror(errno));
        return -2;
    }

    // create server sockaddr_in
    memset((char*)&server, 0, sizeof(server));
    server.sin_family = AF_INET; // IPv4
    server.sin_addr.s_addr = sip;
    server.sin_port = htons(UDP_PORT); // remote port

    // connect to server
    if(connect(ssock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        printf("connect() failed.\n");
        printf("ERROR: %s\n", strerror(errno));
        return -3;
    }

    // register game (send 33 times in ~3 seconds)
    const size_t packet_len = 19;
    unsigned char packet[packet_len];
    memcpy(&packet, &pid, sizeof(uint64_t));
    memcpy(&packet[8], &(uint8_t){MSG_TYPE_REGISTER}, sizeof(uint8_t));
    memcpy(&packet[9], &sepoch, sizeof(uint64_t)); // 8 bytes
    memcpy(&packet[17], &NUM_COMETS, sizeof(uint16_t));
    uint rrg = 0, rrgf = 0;
    while(1)
    {
        if(sendto(ssock, packet, packet_len, 0, (struct sockaddr*)&server, sizeof(server)) < 0)
            rrgf++;
        else
            rrg++;
        if(rrg+rrgf >= 33){break;}
        usleep(100000); // 100ms
    }
    if(rrg == 0)
    {
        printf("After %u attempts to register with the server, none dispatched.\n", rrgf);
        printf("ERROR: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // create recv thread
    if(pthread_create(&tid[0], NULL, recvThread, NULL) != 0)
    {
        pthread_detach(tid[0]);
        printf("pthread_create(recvThread) failed.\n");
        return -4;
    }

    // create send thread
    if(pthread_create(&tid[1], NULL, sendThread, NULL) != 0)
    {
        pthread_detach(tid[1]);
        printf("pthread_create(sendThread) failed.\n");
        return -5;
    }

    // wait for launch confirmation
    while(te[0] == 0 || te[1] == 0)
        sleep(1);

    // done
    printf("Threads launched and game registration sent.\n");
    return 0;
}


//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for frame interpolation
//*************************************
    static double lt = 0;
    if(lt == 0){lt = t;}
    dt = t-lt;
    lt = t;

//*************************************
// keystates
//*************************************

    static f32 zrot = 0.f;

    if(keystate[2] == 1) // W
    {
        vec vdc = (vec){view.m[0][2], view.m[1][2], view.m[2][2]};
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vAdd(&pp, pp, m);
    }
    else if(keystate[3] == 1) // S
    {
        vec vdc = (vec){view.m[0][2], view.m[1][2], view.m[2][2]};
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vSub(&pp, pp, m);
    }

    if(keystate[0] == 1) // A
    {
        vec vdc = (vec){view.m[0][0], view.m[1][0], view.m[2][0]};
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vAdd(&pp, pp, m);
    }
    else if(keystate[1] == 1) // D
    {
        vec vdc = (vec){view.m[0][0], view.m[1][0], view.m[2][0]};
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vSub(&pp, pp, m);
    }

    if(keystate[4] == 1) // SPACE
    {
        vec vdc = (vec){view.m[0][1], view.m[1][1], view.m[2][1]};
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vSub(&pp, pp, m);
    }
    else if(keystate[5] == 1) // SHIFT
    {
        vec vdc = (vec){view.m[0][1], view.m[1][1], view.m[2][1]};
        vec m;
        vMulS(&m, vdc, MOVE_SPEED * dt);
        vAdd(&pp, pp, m);
    }

    if(brake == 1)
        vMulS(&pp, pp, 0.99f*(1.f-dt));

    vec ppi = pp;
    vMulS(&ppi, ppi, dt);
    vAdd(&ppr, ppr, ppi);

    const f32 pmod = vMod(ppr);
    if(autoroll == 1 && pmod < 2.f) // self-righting
    {
        vec dve = (vec){view.m[0][0], view.m[1][0], view.m[2][0]};
        vec dvp = ppr;
        vInv(&dvp);
        vNorm(&dvp);
        const f32 ta = vDot(dve, dvp);
        if(ta < -0.03f || ta > 0.03f)
        {
            // const f32 ia = ta*0.01f;
            const f32 ia = (ta*0.03f) * ( 1.f - ((pmod - 1.14f) * 1.162790656f) ); // 0.86f
            zrot -= ia*dt;
            //printf("%f %f %f\n", zrot, ta, pmod);
        }
        else
            zrot = 0.f;
    }
    else // roll inputs
    {
        if(brake == 1)
            zrot *= 0.99f*(1.f-dt);

        if(keystate[6] && !keystate[7]) {
            if(zrot > 0.f) {
                zrot += dt*rollsens*10.f;
            } else {
                zrot *= 1.f-(dt*0.02f);
                zrot += dt*rollsens*10.f;
            }
        } else if(keystate[7] && !keystate[6]) {
            if(zrot < 0.f) {
                zrot -= dt*rollsens*10.f;
            } else {
                zrot *= 1.f-(dt*0.02f);
                zrot -= dt*rollsens*10.f;
            }
        } else if(keystate[6] && keystate[7]) {
            zrot = 0.f;
        } else {
            if (zrot > 0.09f || zrot < -0.09f)
                zrot *= 1.f-(dt*0.91f);
            else if (zrot > 0.001f)
                zrot -= 0.001f*dt;
            else if (zrot < -0.001f)
                zrot += 0.001f*dt;
            else
                zrot = 0.f;
        }
    }
    if(pmod < 1.13f) // exo collision
    //if(pmod < 1.28f) // exo collision
    {
        vec n = ppr;
        vNorm(&n);
         vReflect(&pp, pp, (vec){-n.x, -n.y, -n.z}); // better if I don't normalise pp
         vMulS(&pp, pp, 0.3f);
        vMulS(&n, n, 1.13f - pmod);
        vAdd(&ppr, ppr, n);
    }

    // sun collision
    f32 ud = vDistSq(lightpos, (vec){-ppr.x, -ppr.y, -ppr.z});
    if(ud < 0.16f)
    {
        vec rn = (vec){-ppr.x, -ppr.y, -ppr.z};
        vSub(&rn, rn, lightpos);
        vNorm(&rn);

        vec n = pp;
        vNorm(&n);
        vInv(&n);

        if(vDot(rn, n) < 0.f)
        {
            vReflect(&pp, pp, (vec){rn.x, rn.y, rn.z});
            vMulS(&pp, pp, 0.3f);
        }
        else
        {
            vCopy(&pp, rn);
            vMulS(&pp, pp, 1.3f); // give a kick back for shield harassment
        }

        ud = vDist(lightpos, (vec){-ppr.x, -ppr.y, -ppr.z});
        vMulS(&n, n, 0.4f - ud);
        if(isnormal(n.x) == 1 && isnormal(n.y) == 1 && isnormal(n.z) == 1)
            vAdd(&ppr, ppr, n);

        sunsht = t;
    }

//*************************************
// camera
//*************************************

    // mouse delta to rot
    f32 xrot = 0.f, yrot = 0.f;
    if(focus_cursor == 1)
    {
        glfwGetCursorPos(window, &x, &y);
        if(controls == 0)
            xrot = (lx-x)*sens;
        else
            zrot = (lx-x)*rollsens;
        yrot = (ly-y)*sens;
        lx = x, ly = y;
    }

    // gimbal free rotation in three axis
    mAngleAxisRotate(&view, view, xrot, yrot, zrot);

    // translate
    mTranslate(&view, ppr.x, ppr.y, ppr.z);

    // sun pos
#ifdef SUN_GOD
    vec lookdir;
    mGetViewZ(&lookdir, view);
    vMulS(&lookdir, lookdir, 1.3f);
    vec np = (vec){-ppr.x, -ppr.y, -ppr.z};
    vAdd(&np, np, lookdir);
    lightpos = np;
#else
    sun_pos += 0.03f*dt;
    lightpos.x = sinf(sun_pos) * 6.3f;
    lightpos.y = cosf(sun_pos) * 6.3f;
    lightpos.z = sinf(sun_pos) * 6.3f;
#endif

#ifdef VERBOSE
    dumpZeroComets();
    //dumpComets();
#endif

//*************************************
// render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ///
    
    shadeLambert2(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.f);
    
    ///

    glBindBuffer(GL_ARRAY_BUFFER, mdlExo.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlExo.cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlExo.iid);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    glDrawElements(GL_TRIANGLES, exo_numind, GL_UNSIGNED_INT, 0);

    ///

    // lambert
    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, 0.f, 0.f, 0.f);
    glUniform1f(opacity_id, 1.f);
    glUniform3f(color_id, 1.f, 1.f, 0.f);

    // bind menger
    glBindBuffer(GL_ARRAY_BUFFER, mdlMenger.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMenger.iid);

    // "light source" dummy object
    mIdent(&model);
    mSetPos(&model, lightpos);
    mScale(&model, 3.4f, 3.4f, 3.4f);
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glDrawElements(GL_TRIANGLES, ncube_numind, GL_UNSIGNED_INT, 0);

    ///

    // players
#ifdef FAKE_PLAYER
    players[0] = 0.f;
    players[1] = 3.f;
    players[2] = 0.f;
    pfront[0] = -view.m[0][2];
    pfront[1] = -view.m[1][2];
    pfront[2] = -view.m[2][2];
    pup[0] = -view.m[0][1];
    pup[1] = -view.m[1][1];
    pup[2] = -view.m[2][1];
#endif
    
    for(uint i = 0; i < MAX_PLAYERS; i++)
    {
        const uint j = i*3;
        if(players[j] != 0.f || players[j+1] != 0.f || players[j+2] != 0.f)
        {
#ifdef ENABLE_PLAYER_PREDICTION
            if(killgame == 0)
            {
                players[j]   += pvel[j]  *dt;
                players[j+1] += pvel[j+1]*dt;
                players[j+2] += pvel[j+2]*dt;
            }
#endif
            const f32 cpmod = vMod((vec){players[j], players[j+1], players[j+2]});

            // render ufo
            shadePhong4(&position_id, &projection_id, &modelview_id, &normalmat_id, &lightpos_id, &normal_id, &texcoord_id, &sampler_id, &specularmap_id, &normalmap_id, &opacity_id);
            glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
            glUniform3f(lightpos_id, 0.f, 0.f, 0.f);
            glUniform1f(opacity_id, 1.f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex[14]);
            glUniform1i(sampler_id, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, tex[15]);
            glUniform1i(specularmap_id, 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, tex[16]);
            glUniform1i(normalmap_id, 2);

            glBindBuffer(GL_ARRAY_BUFFER, mdlUfo.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlUfo.nid);
            glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(normal_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlUfo.tid);
            glVertexAttribPointer(texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(texcoord_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlUfo.iid);

            mIdent(&model);
            vec ep = (vec){-players[j], -players[j+1], -players[j+2]};
            vec en = ep;
            if(cpmod < 1.29f)
            {
                vNorm(&en);
                mLookAt(&model, ep, en);
            }
            else
            {
                mSetPos(&model, (vec){-players[j], -players[j+1], -players[j+2]});
                vec front = (vec){pfront[j], pfront[j+1], pfront[j+2]};
                vec up = (vec){pup[j], pup[j+1], pup[j+2]};
                vec left;
                vCross(&left, front, up);
                model.m[0][0] = left.x;
                model.m[0][1] = left.y;
                model.m[0][2] = left.z;
                model.m[1][0] = front.x;
                model.m[1][1] = front.y;
                model.m[1][2] = front.z;
                model.m[2][0] = up.x;
                model.m[2][1] = up.y;
                model.m[2][2] = up.z;
            }
            mMul(&modelview, &model, &view);

            mat inverted, normalmat;
            mInvert(&inverted.m[0][0], &modelview.m[0][0]);
            mTranspose(&normalmat, &inverted);

            glUniformMatrix4fv(normalmat_id, 1, GL_FALSE, (f32*) &normalmat.m[0][0]);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glDrawElements(GL_TRIANGLES, ufo_numind, GL_UNSIGNED_SHORT, 0);

            // render lights
            shadeFullbright(&position_id, &projection_id, &modelview_id, &color_id, &opacity_id);
            glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
            glUniform3f(color_id, 0.f, 1.f, 1.f);

            glBindBuffer(GL_ARRAY_BUFFER, mdlUfoLights.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlUfoLights.iid);

            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
            glDrawElements(GL_TRIANGLES, ufo_lights_numind, GL_UNSIGNED_BYTE, 0);

            // render interior
            shadeFullbright1(&position_id, &projection_id, &modelview_id, &color_id, &opacity_id);
            glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
            glUniform1f(opacity_id, 1.f);

            glBindBuffer(GL_ARRAY_BUFFER, mdlUfoInterior.vid);
            glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(position_id);

            glBindBuffer(GL_ARRAY_BUFFER, mdlUfoInterior.cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlUfoInterior.iid);

            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
            glDrawElements(GL_TRIANGLES, ufo_interior_numind, GL_UNSIGNED_BYTE, 0);

            // render beam
            if(cpmod < 1.27f)
            {
                shadeLambert4(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &texcoord_id, &sampler_id, &opacity_id);
                glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
                glUniform3f(lightpos_id, 0.f, 0.f, 0.f);
                glUniform1f(opacity_id, 0.3f);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex[13]);
                glUniform1i(sampler_id, 0);

                glBindBuffer(GL_ARRAY_BUFFER, mdlUfoBeam.vid);
                glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(position_id);

                glBindBuffer(GL_ARRAY_BUFFER, mdlUfoBeam.nid);
                glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(normal_id);

                glBindBuffer(GL_ARRAY_BUFFER, mdlUfoBeam.tid);
                glVertexAttribPointer(texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(texcoord_id);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlUfoBeam.iid);

                mIdent(&model);
                mLookAt(&model, ep, en);
                mRotZ(&model, t*0.3f);
                mMul(&modelview, &model, &view);

                glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
                glEnable(GL_BLEND);
                glDrawElements(GL_TRIANGLES, beam_numind, GL_UNSIGNED_BYTE, 0);
                glDisable(GL_BLEND);
            }
        }
    }

    // lambert4
    shadeLambert4(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &texcoord_id, &sampler_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, 0.f, 0.f, 0.f);
    glUniform1f(opacity_id, 1.f);

    // comets
    int bindstate = -1;
    GLushort indexsize = rock8_numind;
    for(uint h = 0; h < 2; h++)
    {
        if(h == 1)
        {
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex[12]);
            glUniform1i(sampler_id, 0);
        }

        for(uint16_t i = 0; i < NUM_COMETS; i++)
        {
            // simulation
            if(h == 1 && comets[i].is_boom > 0.f) // explode
            {
                if(killgame == 0)
                {
                    // while is_boom stepping for prediction offset
                    vec dtv = comets[i].newvel;
                    vMulS(&dtv, dtv, dt);
                    vAdd(&comets[i].newpos, comets[i].newpos, dtv);

                    if(comets[i].is_boom == 1.f)
                    {
                        doExoImpact((vec){comets[i].pos.x, comets[i].pos.y, comets[i].pos.z}, (comets[i].scale+(vMod(comets[i].vel)*0.1f))*1.2f);
                        comets[i].scale *= 2.0f;
                    }
                
                    comets[i].is_boom -= 0.3f*dt;
                    comets[i].scale -= 0.03f*dt;
                }

                if(comets[i].is_boom <= 0.f || comets[i].scale <= 0.f)
                {
                    comets[i].pos = comets[i].newpos;
                    comets[i].vel = comets[i].newvel;
                    comets[i].rot = comets[i].newrot;
                    comets[i].scale = comets[i].newscale;
                    
                    comets[i].is_boom = 0.f;
                    continue;
                }
                glUniform1f(opacity_id, comets[i].is_boom);
            }
            else if(h == 0 && comets[i].is_boom <= 0.f) // detect impacts
            {
                if(killgame == 0)
                {
                    // increment position
                    vec dtv = comets[i].vel;
                    vMulS(&dtv, dtv, dt);
                    vAdd(&comets[i].pos, comets[i].pos, dtv);

                    // player impact
                    const f32 cd = vDistSq((vec){-ppr.x, -ppr.y, -ppr.z}, comets[i].pos);
                    const f32 cs = comets[i].scale+0.18f;
                    if(cd < cs*cs)
                    {
                        static f32 lt = 0;
                        if(t > lt)
                        {
                            // tell server of collision
                            const uint64_t mt = microtime();
                            const size_t packet_len = 19;
                            unsigned char packet[packet_len];
                            memcpy(&packet, &pid, sizeof(uint64_t));
                            memcpy(&packet[8], &(uint8_t){MSG_TYPE_ASTEROID_DESTROYED}, sizeof(uint8_t));
                            memcpy(&packet[9], &mt, sizeof(uint64_t));
                            memcpy(&packet[17], &(uint16_t){i}, sizeof(uint16_t));
                            csend(packet, 19);
                            lt = t + 0.1f;
                        }
    #ifdef VERBOSE
                        printf("[%u] asteroid_destroyed sent.\n", i);
    #endif
                    }

                    // sun impact
                    const f32 cld = 0.4f+comets[i].scale;
                    if(vDistSq(comets[i].pos, lightpos) < cld*cld)
                        sunsht = t;
                }
            }

            // this is zuper efficient now
            if(h == 0 && comets[i].is_boom <= 0.f)
            {
                // painters algo
                static int lt = -1;
                const uint ti = (rncr * (f32)i) + 0.5f;
                if(ti != lt)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tex[ti]);
                    glUniform1i(sampler_id, 0);
                    lt = ti;
                }
            }
            else if(h == 1 && comets[i].is_boom <= 0.f)
                continue;
            else if(h == 0 && comets[i].is_boom > 0.f)
                continue;

            // translate comet
            mIdent(&model);
            mSetPos(&model, comets[i].pos);

            // rotate comet
            f32 mag = 0.f;
            if(killgame == 0){mag = comets[i].rot*0.01f*t;}
            if(comets[i].rot < 100.f)
                mRotY(&model, mag);
            if(comets[i].rot < 200.f)
                mRotZ(&model, mag);
            if(comets[i].rot < 300.f)
                mRotX(&model, mag);
            
            // scale comet
            mScale(&model, comets[i].scale, comets[i].scale, comets[i].scale);

            // make modelview
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

            // bind one of the 8 rock models
            uint nbs = i * rrcs;
            if(nbs > 7){nbs = 7;}
            if(nbs != bindstate)
            {
                glBindBuffer(GL_ARRAY_BUFFER, mdlRock[nbs].vid);
                glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(position_id);

                glBindBuffer(GL_ARRAY_BUFFER, mdlRock[nbs].nid);
                glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(normal_id);

                glBindBuffer(GL_ARRAY_BUFFER, mdlRock[nbs].tid);
                glVertexAttribPointer(texcoord_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(texcoord_id);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRock[nbs].iid);
                bindstate = nbs;
            }

            if(nbs == 0){indexsize = rock1_numind;}
            else if(nbs == 1){indexsize = rock2_numind;}
            else if(nbs == 2){indexsize = rock3_numind;}
            else if(nbs == 3){indexsize = rock4_numind;}
            else if(nbs == 4){indexsize = rock5_numind;}
            else if(nbs == 5){indexsize = rock6_numind;}
            else if(nbs == 6){indexsize = rock7_numind;}
            else if(nbs == 7){indexsize = rock8_numind;}

            // draw it
            if(comets[i].is_boom > 0.f)
                glDrawElements(GL_TRIANGLES, indexsize, GL_UNSIGNED_SHORT, 0);
            else
                glDrawElements(GL_TRIANGLES, indexsize, GL_UNSIGNED_SHORT, 0);
        }

        if(h == 1){glDisable(GL_BLEND);glEnable(GL_CULL_FACE);}
    }

    // render shield
    const f32 dsht = (t-sunsht)*0.3f;
    if(dsht <= 1.f)
    {
        shadeLambert2(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
        glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
        glUniform3f(lightpos_id, 0.f, 0.f, 0.f);
        glUniform1f(opacity_id, 1.f-dsht);

        glBindBuffer(GL_ARRAY_BUFFER, mdlUfoShield.vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlUfoShield.cid);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlUfoShield.iid);

        mIdent(&model);
        mSetPos(&model, lightpos);
        mMul(&modelview, &model, &view);

        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glEnable(GL_BLEND);
        glDrawElements(GL_TRIANGLES, ufo_shield_numind, GL_UNSIGNED_SHORT, 0);
        glDisable(GL_BLEND);
    }

    // swap
    glfwSwapBuffers(window);
    last_render_time = microtime();
}

//*************************************
// Input Handelling
//*************************************
void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = (double)winw;
    wh = (double)winh;
    rww = 1.0/ww;
    rwh = 1.0/wh;
    ww2 = ww/2.0;
    wh2 = wh/2.0;
    uw = (double)aspect/ww;
    uh = 1.0/wh;
    uw2 = (double)aspect/ww2;
    uh2 = 1.0/wh2;

    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 0.01f, FAR_DISTANCE);
    if(projection_id != -1){glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(lgdonce == 0){return;} // nothing until game start
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_A){ keystate[0] = 1; }
        else if(key == GLFW_KEY_D){ keystate[1] = 1; }
        else if(key == GLFW_KEY_W){ keystate[2] = 1; }
        else if(key == GLFW_KEY_S){ keystate[3] = 1; }
        else if(key == GLFW_KEY_SPACE){ keystate[4] = 1; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[5] = 1; }
        else if(key == GLFW_KEY_Q){ keystate[6] = 1; }
        else if(key == GLFW_KEY_E){ keystate[7] = 1; }
        else if(key == GLFW_KEY_LEFT_CONTROL){ brake = 1; }
        else if(key == GLFW_KEY_LEFT_ALT){ pp = (vec){0.f, 0.f, 0.f}; }
        else if(key == GLFW_KEY_F)
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
                lfct = t;
                fc = 0;
            }
        }
        else if(key == GLFW_KEY_R)
        {
            autoroll = 1 - autoroll;
            printf("autoroll: %u\n", autoroll);
        }
        else if(key == GLFW_KEY_T)
        {
            controls = 1 - controls;
            printf("controls: %u\n", controls);
        }
        else if(key == GLFW_KEY_ESCAPE)
        {
            focus_cursor = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            window_size_callback(window, winw, winh);
        }
        else if(key == GLFW_KEY_V)
        {
            // view inversion
            printf(":: %+.2f\n", view.m[1][1]);

            // dump view matrix
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[0][0], view.m[0][1], view.m[0][2], view.m[0][3]);
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[1][0], view.m[1][1], view.m[1][2], view.m[1][3]);
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[2][0], view.m[2][1], view.m[2][2], view.m[2][3]);
            printf("%+.2f %+.2f %+.2f %+.2f\n", view.m[3][0], view.m[3][1], view.m[3][2], view.m[3][3]);
            printf("---\n");
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(key == GLFW_KEY_A){ keystate[0] = 0; }
        else if(key == GLFW_KEY_D){ keystate[1] = 0; }
        else if(key == GLFW_KEY_W){ keystate[2] = 0; }
        else if(key == GLFW_KEY_S){ keystate[3] = 0; }
        else if(key == GLFW_KEY_SPACE){ keystate[4] = 0; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[5] = 0; }
        else if(key == GLFW_KEY_Q){ keystate[6] = 0; }
        else if(key == GLFW_KEY_E){ keystate[7] = 0; }
        else if(key == GLFW_KEY_LEFT_CONTROL){ brake = 0; }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(lgdonce == 0){return;} // nothing until game start
    if(action == GLFW_PRESS)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if(focus_cursor == 0 && lgdonce != 0)
            {
                focus_cursor = 1;
                glfwGetCursorPos(window, &lx, &ly);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                window_size_callback(window, winw, winh);
            }
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
            brake = 1;
        else if(button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_4)
            pp = (vec){0.f, 0.f, 0.f};
    }
    else if(action == GLFW_RELEASE)
        brake = 0;
}

//*************************************
// Process Entry Point
//*************************************
#ifdef DEBUG_GL
void GLAPIENTRY
DebugCallback(  GLenum source, GLenum type, GLuint id,
                GLenum severity, GLsizei length,
                const GLchar *msg, const void *data)
{
    // https://gist.github.com/liam-middlebrook/c52b069e4be2d87a6d2f
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDebugMessageControl.xhtml
    // https://registry.khronos.org/OpenGL-Refpages/es3/html/glDebugMessageControl.xhtml
    char* _source;
    char* _type;
    char* _severity;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
        _source = "API";
        break;

        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        _source = "WINDOW SYSTEM";
        break;

        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        _source = "SHADER COMPILER";
        break;

        case GL_DEBUG_SOURCE_THIRD_PARTY:
        _source = "THIRD PARTY";
        break;

        case GL_DEBUG_SOURCE_APPLICATION:
        _source = "APPLICATION";
        break;

        case GL_DEBUG_SOURCE_OTHER:
        _source = "UNKNOWN";
        break;

        default:
        _source = "UNKNOWN";
        break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
        _type = "ERROR";
        break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        _type = "DEPRECATED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        _type = "UDEFINED BEHAVIOR";
        break;

        case GL_DEBUG_TYPE_PORTABILITY:
        _type = "PORTABILITY";
        break;

        case GL_DEBUG_TYPE_PERFORMANCE:
        _type = "PERFORMANCE";
        break;

        case GL_DEBUG_TYPE_OTHER:
        _type = "OTHER";
        break;

        case GL_DEBUG_TYPE_MARKER:
        _type = "MARKER";
        break;

        default:
        _type = "UNKNOWN";
        break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        _severity = "HIGH";
        break;

        case GL_DEBUG_SEVERITY_MEDIUM:
        _severity = "MEDIUM";
        break;

        case GL_DEBUG_SEVERITY_LOW:
        _severity = "LOW";
        break;

        case GL_DEBUG_SEVERITY_NOTIFICATION:
        _severity = "NOTIFICATION";
        break;

        default:
        _severity = "UNKNOWN";
        break;
    }

    printf("%d: %s of %s severity, raised from %s: %s\n",
            id, _type, _severity, _source, msg);
}
#endif

int main(int argc, char** argv)
{
    sepoch = time(0);
    sepoch = ((((time_t)(((double)sepoch / 60.0) / 3.0)+1)*3)*60); // next 20th of an hour

    // load config
    loadConfig();

    // help
    printf("----\n");
    printf("%s\n", glfwGetVersionString());
    printf("----\n");
    printf("AstroImpact\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("https://AstroImpact.net\n");
    printf("https://notabug.org/AstroImpact/AstroImpact\n");
    printf("----\n");
    printf("Argv: <start epoch> <server host/ip> <num asteroids> <msaa 0-16> <autoroll 0-1> <center window 0-1> <control-type 0-1> <mouse sensitivity (0.001+)> <roll sensitivity (0.001+)>\n");
    printf("e.g: %s %lu vfcash.co.uk 64 16 1 1 0 0.003 0.003\n", argv[0], time(0)+33);
    printf("----\n");
    printf("- Config File (~/.config/fat.cfg) or relative to cwd: (.config/fat.cfg) or (fat.cfg)\n");
    const char* home = getenv("HOME");
    if(home != NULL){printf("Create or edit this file: %s/.config/fat.cfg\nusing the following template:\n----\n", home);}
    printf("NUM_ASTEROIDS %u\n", NUM_COMETS);
    printf("MSAA %u\n", msaa);
    printf("AUTOROLL %u\n", autoroll);
    printf("CENTER_WINDOW %u\n", center);
    printf("CONTROLS %u\n", controls);
    printf("LOOKSENS %g\n", sens*1000.f);
    printf("ROLLSENS %g\n", rollsens*1000.f);
    printf("----\n");
    printf("!! The CONTROLS setting in the config file allows you to swap between roll and yaw for mouse x:l=>r.\n");
    printf("----\n");
    printf("F = FPS to console.\n");
    printf("R = Toggle auto-tilt/roll around planet.\n");
    printf("T = Toggle between mouse look yaw/roll.\n");
    printf("W, A, S, D, Q, E, SPACE, LEFT SHIFT.\n");
    printf("Q+E to stop rolling.\n");
    printf("L-CTRL / Right Click to Brake.\n");
    printf("L-ALT / Mouse 3/4 Click to Instant Brake.\n");
    printf("Escape to free mouse lock.\n");
    printf("----\n");
    printf("current epoch: %lu\n", time(0));

    // start at epoch
    if(argc >= 2)
    {
        sepoch = atoll(argv[1]);
        if(sepoch != 0 && sepoch-time(0) < 3)
        {
            printf("suggested epoch: %lu\n----\nYour epoch should be at minimum 3 seconds into the future.\n", time(0)+26);
            return 0;
        }
    }

    printf("start epoch:   %lu\n", sepoch);
    printf("----\n");

    // allow custom host
    //sprintf(server_host, "127.0.0.1");
    sprintf(server_host, "vfcash.co.uk");
    if(argc >= 3){sprintf(server_host, "%s", argv[2]);}

    // custom number of asteroids (for epoch creator only)
    if(argc >= 4){NUM_COMETS = atoi(argv[3]);}
    if(NUM_COMETS > MAX_COMETS){NUM_COMETS = MAX_COMETS;}
    else if(NUM_COMETS < 16){NUM_COMETS = 16;}
    rcs = NUM_COMETS / 8;
    rrcs = 1.f / (f32)rcs;
    rncr = 11.f / NUM_COMETS;

    // allow custom msaa level
    if(argc >= 5){msaa = atoi(argv[4]);}

    // is auto-roll enabled
    if(argc >= 6){autoroll = atoi(argv[5]);}

    // is centered
    if(argc >= 7){center = atoi(argv[6]);}

    // controls type
    if(argc >= 8){controls = atoi(argv[7]);}

    // mouse sens
    if(argc >= 9){sens = atof(argv[8]);}

    // roll sens
    if(argc >= 10){rollsens = atof(argv[9]);}
    
    printf("Asteroids: %hu\n", NUM_COMETS);
    printf("MSAA: %u\n", msaa);
    printf("Auto-Roll: %u\n", autoroll);
    printf("Center-Window: %u\n", center);
    printf("Control-Type: %u\n", controls);
    printf("Mouse Sensitivity: %.0f (%g)\n", sens*10000.f, sens);
    printf("Roll Sensitivity: %.0f (%g)\n", rollsens*10000.f, rollsens);
    printf("----\n");
    printf("Server Host/IP: %s\n", server_host);
    printf("----\n");

    // init glfw
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, "AstroImpact", NULL, NULL);
    if(!window)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if(center == 1){glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2));} // center window on desktop
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

    // debug
#ifdef DEBUG_GL
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND MENGER *****
    scaleBuffer(ncube_vertices, ncube_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlMenger.vid, ncube_vertices, ncube_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlMenger.iid, ncube_indices, ncube_indices_size, GL_STATIC_DRAW);

    // ***** BIND EXO *****
    GLsizeiptr s = exo_numvert*3;
    for(GLsizeiptr i = 0; i < s; i+=3)
    {
        const f32 g = (exo_colors[i] + exo_colors[i+1] + exo_colors[i+2]) * 0.3333333433f;
        const f32 h = (1.f-g)*0.01f;
        vec v = {exo_vertices[i], exo_vertices[i+1], exo_vertices[i+2]};
        vNorm(&v);
        vMulS(&v, v, h);
        exo_vertices[i]   *= GFX_SCALE;
        exo_vertices[i+1] *= GFX_SCALE;
        exo_vertices[i+2] *= GFX_SCALE;
        exo_vertices[i]   -= v.x;
        exo_vertices[i+1] -= v.y;
        exo_vertices[i+2] -= v.z;
        exo_vertices[i]   *= 1.03f;
        exo_vertices[i+1] *= 1.03f;
        exo_vertices[i+2] *= 1.03f;
    }
    esBind(GL_ARRAY_BUFFER, &mdlExo.vid, exo_vertices, exo_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlExo.cid, exo_colors, exo_colors_size, GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlExo.iid, exo_indices, exo_indices_size, GL_STATIC_DRAW);

    // ***** BIND ROCK1 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].vid, rock1_vertices, rock1_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].nid, rock1_normals, rock1_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].iid, rock1_indices, rock1_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].tid, rock1_uvmap, rock1_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK2 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].vid, rock2_vertices, rock2_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].nid, rock2_normals, rock2_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].iid, rock2_indices, rock2_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].tid, rock2_uvmap, rock2_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK3 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].vid, rock3_vertices, rock3_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].nid, rock3_normals, rock3_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].iid, rock3_indices, rock3_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].tid, rock3_uvmap, rock3_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK4 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].vid, rock4_vertices, rock4_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].nid, rock4_normals, rock4_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].iid, rock4_indices, rock4_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].tid, rock4_uvmap, rock4_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK5 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].vid, rock5_vertices, rock5_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].nid, rock5_normals, rock5_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].iid, rock5_indices, rock5_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].tid, rock5_uvmap, rock5_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK6 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].vid, rock6_vertices, rock6_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].nid, rock6_normals, rock6_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].iid, rock6_indices, rock6_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].tid, rock6_uvmap, rock6_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK7 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].vid, rock7_vertices, rock7_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].nid, rock7_normals, rock7_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].iid, rock7_indices, rock7_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].tid, rock7_uvmap, rock7_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND ROCK8 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].vid, rock8_vertices, rock8_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].nid, rock8_normals, rock8_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].iid, rock8_indices, rock8_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].tid, rock8_uvmap, rock8_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND UFO *****
    scaleBuffer(ufo_vertices, ufo_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlUfo.vid, ufo_vertices, ufo_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfo.nid, ufo_normals, ufo_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfo.iid, ufo_indices, ufo_indices_size, GL_STATIC_DRAW);
    flipUV(ufo_uvmap, ufo_numvert*2);
    esBind(GL_ARRAY_BUFFER, &mdlUfo.tid, ufo_uvmap, ufo_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND UFO BEAM *****
    scaleBuffer(beam_vertices, beam_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlUfoBeam.vid, beam_vertices, beam_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoBeam.nid, beam_normals, beam_normals_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoBeam.iid, beam_indices, beam_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoBeam.tid, beam_uvmap, beam_uvmap_size, GL_STATIC_DRAW);

    // ***** BIND UFO_LIGHTS *****
    scaleBuffer(ufo_lights_vertices, ufo_lights_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlUfoLights.vid, ufo_lights_vertices, ufo_lights_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlUfoLights.iid, ufo_lights_indices, ufo_lights_indices_size, GL_STATIC_DRAW);

    // ***** BIND UFO SHIELD *****
    scaleBufferC(ufo_shield_vertices, ufo_shield_numvert*3, GFX_SCALE+0.007f);
    esBind(GL_ARRAY_BUFFER, &mdlUfoShield.vid, ufo_shield_vertices, ufo_shield_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoShield.iid, ufo_shield_indices, ufo_shield_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoShield.cid, ufo_shield_colors, ufo_shield_colors_size, GL_STATIC_DRAW);

    // ***** BIND UFO SHIELD *****
    scaleBuffer(ufo_interior_vertices, ufo_interior_numvert*3);
    esBind(GL_ARRAY_BUFFER, &mdlUfoInterior.vid, ufo_interior_vertices, ufo_interior_vertices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoInterior.iid, ufo_interior_indices, ufo_interior_indices_size, GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlUfoInterior.cid, ufo_interior_colors, ufo_interior_colors_size, GL_STATIC_DRAW);

    // ***** LOAD TEXTURES *****
    tex[0] = esLoadTexture(512, 512, tex_rock1);
    tex[1] = esLoadTexture(512, 512, tex_rock2);
    tex[2] = esLoadTexture(512, 512, tex_rock3);
    tex[3] = esLoadTexture(512, 512, tex_rock4);
    tex[4] = esLoadTexture(512, 512, tex_rock5);
    tex[5] = esLoadTexture(512, 512, tex_rock6);
    tex[6] = esLoadTexture(512, 512, tex_rock7);
    tex[7] = esLoadTexture(512, 512, tex_rock8);
    tex[8] = esLoadTexture(512, 512, tex_rock9);
    tex[9] = esLoadTexture(512, 512, tex_rock10);
    tex[10] = esLoadTexture(512, 512, tex_rock11);
    tex[11] = esLoadTexture(512, 512, tex_rock12);
    tex[12] = esLoadTexture(512, 512, tex_flames);
    tex[13] = esLoadTextureWrapped(256, 256, tex_plasma);
    tex[14] = esLoadTexture(4096, 4096, tex_ufodiff);
    tex[15] = esLoadTexture(4096, 4096, tex_ufospec);
    tex[16] = esLoadTexture(4096, 4096, tex_ufonorm);

//*************************************
// compile & link shader programs
//*************************************

    makeLambert();
    makeLambert2();
    makeLambert4();
    makeFullbright();
    makeFullbright1();
    makePhong4();

//*************************************
// configure render options
//*************************************

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);

//*************************************
// execute update / render loop
//*************************************

    // render loading screen
    glfwSetWindowTitle(window, "Please wait...");
    window_size_callback(window, winw, winh);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, 0.f, 0.f, 0.f);
    glUniform1f(opacity_id, 1.f);
    glUniform3f(color_id, 1.f, 1.f, 0.f);
    glBindBuffer(GL_ARRAY_BUFFER, mdlMenger.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlMenger.iid);
    mIdent(&view);
    mSetPos(&view, (vec){0.f, 0.f, -0.19f});
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    glDrawElements(GL_TRIANGLES, ncube_numind, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);

    // random start position
    srandf(urand_int());
    vRuvBT(&ppr);
    vMulS(&ppr, ppr, 2.3f);

    vec vd = ppr;
    vInv(&vd);
    vNorm(&vd);
    mSetViewDir(&view, vd, (vec){0.f,1.f,0.f});

#ifndef FAST_START
    // create net
    initNet();

    // wait until epoch
    uint64_t ttt = sepoch - 1;
    for(uint64_t t1 = time(0); t1 < ttt; t1 = time(0)) // for loop abuse
        sleep(ttt - t1);
    ttt = sepoch * 1000000;
    for(uint64_t t1 = microtime(); t1 < ttt; t1 = microtime())
        usleep(ttt - t1);
#endif

    // init
    window_size_callback(window, winw, winh);
    t = glfwGetTime();
    lfct = t;
    
    // event loop
    while(!glfwWindowShouldClose(window))
    {
        t = glfwGetTime();
        glfwPollEvents();
        main_loop();
        fc++;
    }

    // send disconnect message
    for(int i = 0; i < 3; i++)
    {
        unsigned char packet[9];
        memcpy(&packet, &pid, sizeof(uint64_t));
        memcpy(&packet[8], &(uint8_t){MSG_TYPE_PLAYER_DISCONNECTED}, sizeof(uint8_t));
        csend(packet, 9);
        usleep(100000);
    }

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
