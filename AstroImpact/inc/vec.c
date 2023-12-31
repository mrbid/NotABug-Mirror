/*
    James William Fletcher (github.com/mrbid)
        September 2021

    Portable floating-point Vec3 lib with basic SSE support
*/

#if !defined(__linux__) || defined(NOSSE)
    #define SEIR_RAND
#endif

#ifdef SEIR_RAND
int srandfq = 74235;
#else
__int64_t srandfq = 74235;
#endif
