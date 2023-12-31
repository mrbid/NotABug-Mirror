#ifndef RES_H
#define RES_H

#pragma GCC diagnostic ignored "-Wtrigraphs"

static const struct {
  unsigned int  width;
  unsigned int  height;
  unsigned int  bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char pixel_data[16 * 16 * 4 + 1];
} icon_image = {
  16, 16, 4,
  "\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377"
  "\377\377\001\377\377\377\032\346\346\346k\346\346\346y\377\377\377%\377\377"
  "\377\001\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377"
  "\000\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\002"
  "\377\377\377%\336\336\336\215fff\356RRR\367\326\326\326\237\377\377\377,"
  "\377\377\377\002\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377"
  "\377\377\000\377\377\377\000\377\377\377\000\377\377\377\003\377\377\377-\326\326"
  "\326\240RRR\367\000JJ\377\000RR\377JJJ\371\314\314\314\247\377\377\377.\377\377"
  "\377\003\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377"
  "\000\377\377\377\004\377\377\377\070\314\314\314\255BBB\372\000JJ\377\000\214\214"
  "\377\000\231\231\377\000RR\377<AA\372\314\314\314\255\377\377\377\066\377\377"
  "\377\004\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\005\377\377\377"
  "A\275\275\275\273:::\374\000\063\063\377\000\231\231\377\000\314\314\377\000\326\326"
  "\377\000\231\231\377\000BB\377:::\374\275\275\275\267\377\377\377=\377\377\377"
  "\004\377\377\377\000\377\377\377\000\377\377\377#\275\275\275\267\040..\376\000\031"
  "\031\377\000JJ\377\000\326\326\377\000\367\367\377\000\367\367\377\000\336\336\377\000"
  "ff\377\000\031\031\377(\061\061\375\275\275\275\263\377\377\377%\377\377\377\000"
  "\377\377\377\001\377\377\377M{{{\353\000!!\377\000))\377\000\265\265\377\000\367\367"
  "\377\000\377\377\377\000\377\377\377\000\377\377\377\000\275\275\377\000\063\063\377"
  "\000!!\377sss\354\377\377\377Q\377\377\377\002\377\377\377\007\357\357\357v<AA"
  "\372\000JJ\377\000\255\255\377\000\357\357\377\000\377\377\377\000\377\377\377\000\377"
  "\377\377\000\377\377\377\000\367\367\377\000\255\255\377\000JJ\377:::\373\346\346"
  "\346{\377\377\377\007\377\377\377\020\326\326\326\234\026\060\060\377\000\204\204"
  "\377\000\326\326\377\000\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377"
  "\000\377\377\377\000\377\377\377\000\326\326\377\000\214\214\377\017++\377\326\326"
  "\326\236\377\377\377\021\377\377\377%\255\255\255\306\000JJ\377\000\245\245\377"
  "\000\336\336\377\000\357\357\377\000\377\377\377\000\377\377\377\000\377\377\377\000"
  "\377\377\377\000\357\357\377\000\336\336\377\000\255\255\377\000JJ\377\255\255\255"
  "\311\377\377\377'\377\377\377A\204\204\204\345\000ss\377\000\245\245\377\000\275"
  "\275\377\000\314\314\377\000\346\346\377\000\265\265\377\000\245\245\377\000\346\346"
  "\377\000\326\326\377\000\275\275\377\000\245\245\377\000ss\377\204\204\204\346\377"
  "\377\377D\377\377\377D{{{\345\000ZZ\377\000ff\377\000\214\214\377\000\245\245\377"
  "\000\214\214\377\000JJ\377\000JJ\377\000\214\214\377\000\245\245\377\000\214\214\377"
  "\000ff\377\000ZZ\377sss\353\377\377\377J\377\377\377\026\357\357\357p\245\245"
  "\245\317JJJ\372\000JJ\377\000RR\377\000ff\377\000ZZ\377\000ZZ\377\000ff\377\000RR\377\000"
  "JJ\377<AA\374\231\231\231\330\346\346\346\203\377\377\377\035\377\377\377"
  "\000\377\377\377\012\377\377\377\067\346\346\346\210\231\231\231\331\063\063\063"
  "\374\000!!\377\000!!\377\000!!\377\000!!\377\063\063\063\375\231\231\231\335\336\336"
  "\336\223\377\377\377A\377\377\377\020\377\377\377\001\377\377\377\000\377\377"
  "\377\000\377\377\377\001\377\377\377\017\377\377\377A\336\336\336\227\214\214"
  "\214\343\040((\376\040..\375\214\214\214\342\336\336\336\231\377\377\377E\377"
  "\377\377\022\377\377\377\001\377\377\377\000\377\377\377\000\377\377\377\000\377\377"
  "\377\000\377\377\377\000\377\377\377\000\377\377\377\002\377\377\377\024\377\377\377"
  "O\314\314\314\232\326\326\326\230\377\377\377M\377\377\377\025\377\377\377"
  "\002\377\377\377\000\377\377\377\000\377\377\377\000\377\377\377\000",
};

#endif