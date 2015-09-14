#ifndef _BMP_H
#define _BMP_H
#include <stdio.h>

typedef struct{
    short type;
    int size;
    short reserved1;
    short reserved2;
    int offset;
} BMPHeader;

typedef struct{
    int size;
    int width;
    int height;
    short planes;
    short bitsPerPixel;
    unsigned compression;
    unsigned imageSize;
    int xPelsPerMeter;
    int yPelsPerMeter;
    int clrUsed;
    int clrImportant;
} BMPInfoHeader;

#ifndef __NVCC__
#pragma pack(1)
//Isolated definition
typedef struct{
  unsigned char x, y, z;
} uchar3;
#endif

#endif
