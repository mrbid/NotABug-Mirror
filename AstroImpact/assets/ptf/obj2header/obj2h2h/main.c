/*
    James William Fletcher (github.com/mrbid)
        December 2022
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "ufo.h"

int main(int argc, char** argv)
{
    FILE* f = fopen("out.txt", "w");
    for(size_t i = 0; i < 45346; i++)
    {
        //fprintf(f, "%g,%g,%g,", modelVertices[i].position[0], modelVertices[i].position[1], modelVertices[i].position[2]);
        //fprintf(f, "%g,%g,%g,", modelVertices[i].normal[0], modelVertices[i].normal[1], modelVertices[i].normal[2]);
        fprintf(f, "%g,%g,", modelVertices[i].uv[0], modelVertices[i].uv[1]);
    }
    fclose(f);
}