#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "bmp.h"
#include "omp.h"


extern "C" void LoadBMPFile(uchar3 **img, BMPHeader *hdr, BMPInfoHeader *infoHdr, const char *name);

extern "C" void WriteBMPFile(uchar3 **img, BMPHeader hdr, BMPInfoHeader infoHdr, const char *name);

#define idx(A,B) ((A) * cols + (B))

typedef struct pixel {
  float x, y, z;
} Pixel;


void filter(Pixel *myimg, Pixel *oimg, int rows, int cols)
{
#pragma omp parallel for 
  for (uint64_t y = 1; y < rows-1; y++)
    for (uint64_t x = 1; x < cols-1; x++) {
      oimg[idx(y,x)].z = 
	(myimg[idx(y,x)].z
	 + myimg[idx(y,x-1)].z 
	 + myimg[idx(y,x+1)].z 
	 + myimg[idx(y-1,x)].z
	 + myimg[idx(y-1,x-1)].z 
	 + myimg[idx(y-1,x+1)].z 
	 + myimg[idx(y+1,x)].z
	 + myimg[idx(y+1,x-1)].z 
	 + myimg[idx(y+1,x+1)].z)/9;

      oimg[idx(y,x)].y = 
	(myimg[idx(y,x)].y 
	 + myimg[idx(y,x-1)].y 
	 + myimg[idx(y,x+1)].y 
	 + myimg[idx(y-1,x)].y 
	 + myimg[idx(y-1,x-1)].y 
	 + myimg[idx(y-1,x+1)].y 
	 + myimg[idx(y+1,x)].y 
	 + myimg[idx(y+1,x-1)].y 
	 + myimg[idx(y+1,x+1)].y)/9;

      oimg[idx(y,x)].x = 
	(myimg[idx(y,x)].x 
	 + myimg[idx(y,x-1)].x 
	 + myimg[idx(y,x+1)].x 
	 + myimg[idx(y-1,x)].x 
	 + myimg[idx(y-1,x-1)].x 
	 + myimg[idx(y-1,x+1)].x 
	 + myimg[idx(y+1,x)].x 
	 + myimg[idx(y+1,x-1)].x 
	 + myimg[idx(y+1,x+1)].x)/9;
    }
}

double  apply_stencil(const int rows, const int cols, Pixel * const in, Pixel * const out) {

  double tstart, tend;
  tstart = omp_get_wtime();
  filter(in, out, rows, cols);
  tend = omp_get_wtime();
  return(tend-tstart);
}

// main read, call filter, write new image
int main(int argc, char **argv)
{

  BMPHeader hdr;
  BMPInfoHeader infoHdr;
  uchar3 *bimg;
  Pixel *img,*oimg;
  uint64_t x,y;
  uint64_t img_size;
  double start, end;
  if(argc != 2) {
    printf("Usage: %s imageName\n", argv[0]);
    return 1;
  }

  
  LoadBMPFile(&bimg, &hdr, &infoHdr, argv[1]);
  printf("Data init done: size = %d, width = %d, height = %d.\n",
	 hdr.size, infoHdr.width, infoHdr.height);

  img_size = infoHdr.width * infoHdr.height * sizeof(Pixel);
  img = (Pixel *) malloc(img_size);
  if (img == NULL) {
    printf("Error Cant alloc image space\n");
    exit(-1);
  }
  memset(img,0,img_size);
  oimg = (Pixel *) malloc(img_size);
  if (oimg == NULL) {
    printf("Error Cant alloc output image space\n");
    exit(-1);
  }
  memset(oimg,0,img_size);
  printf("Convert image\n");
  // convert to floats for processing
  int rows = infoHdr.height;
  int cols = infoHdr.width;

#pragma omp parallel for private(x,y)
  for (y=0; y<rows; y++)
    for (x=0; x<cols; x++)
      {
	img[idx(y,x)].x = bimg[idx(y,x)].x/255.0;   
	img[idx(y,x)].y = bimg[idx(y,x)].y/255.0;   
	img[idx(y,x)].z = bimg[idx(y,x)].z/255.0;   
      }   

  double runtime;
  runtime = apply_stencil(infoHdr.height, infoHdr.width, img, oimg);
  printf("time for stencil = %f seconds\n",runtime);

  // clear bitmap array
  memset(bimg,0,infoHdr.height*infoHdr.width*3);
  double err = 0.0;
  // convert to uchar3 for output
  printf("rows %d cols %d\n",rows, cols);

#pragma omp parallel for private(x,y) reduction(+:err)
  for (y=0; y<rows; y++)
    for (x=0; x<cols; x++)
      {
	bimg[idx(y,x)].x = oimg[idx(y,x)].x*255;   
	bimg[idx(y,x)].y = oimg[idx(y,x)].y*255;   
	bimg[idx(y,x)].z = oimg[idx(y,x)].z*255;   
	err += (img[idx(y,x)].x - oimg[idx(y,x)].x);
	err += (img[idx(y,x)].y - oimg[idx(y,x)].y);
	err += (img[idx(y,x)].z - oimg[idx(y,x)].z);
      }   
  printf("Cummulative error between images %g\n",err);

  // write the output file
  WriteBMPFile(&bimg, hdr,infoHdr, "./img-new.bmp");
 
}
