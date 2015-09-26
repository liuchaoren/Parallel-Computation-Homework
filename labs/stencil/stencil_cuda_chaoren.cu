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
#define new_idx(A, B, C) (A * strip_width * rows + B * strip_width + C)
#define new_idx_last(A, B, C) (A * strip_width * rows + B * strip_width_last + C)

#define thrds 256
#define strip_width 16

typedef struct pixel {
	float x, y, z;
} Pixel;


__global__ void filter(Pixel *myimg, Pixel *oimg, int rows, int strip_width, int strip_width_last, int strip_num)
{
  __shared__ Piexel temp[(strip_width + 2)^2];
  int gindex = threadIdx.x + blockIdx.x * blockDim.x;
  int x = threadIdx.x 
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

double  apply_stencil(const int rows, const int cols, uint64_t img_size, int strip_width, int strip_width_last, int strip_num, Pixel * const in, Pixel * const out, Pixel * const in_d, Pixel * const out_d) {
    	cudaMemcpy(in_d, in, img_size, cudaMemcpyHostToDevice);
    	cudaMemcpy(out_d, out, img_size, cudaMemcpyHostToDevice);
	double tstart, tend;
      	tstart = omp_get_wtime();
	filter<<<(cols*rows + thrds - 1) / thrds, thrds>>>(in_d, out_d, rows, strip_width, strip_width_last, strip_num);
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
  Pixel *img_d, *oimg_d;
  uint64_t x,y;
  uint64_t new_x, new_y, new_z;
  uint64_t img_size;
  double start, end;
//  int strip_width;

  if(argc != 2) {
    printf("Usage: %s imageName\n", argv[0]);
    return 1;
  }

  
  LoadBMPFile(&bimg, &hdr, &infoHdr, argv[1]);
  printf("Data init done: size = %d, width = %d, height = %d.\n",
	hdr.size, infoHdr.width, infoHdr.height);

  img_size = infoHdr.width * infoHdr.height * sizeof(Pixel);
  img = (Pixel *) malloc(img_size);
  cudaMalloc((void **) &img_d, img_size);
  if (img == NULL) {
    printf("Error Cant alloc image space\n");
    exit(-1);
  }
  memset(img,0,img_size);
  oimg = (Pixel *) malloc(img_size);
  cudaMalloc((void **) &oimg_d, img_size);
  if (oimg == NULL) {
    printf("Error Cant alloc output image space\n");
    exit(-1);
  }
  memset(oimg,0,img_size);
  printf("Convert image\n");
  // convert to floats for processing and data reorganization 
  int rows = infoHdr.height;
  int cols = infoHdr.width;
  if (cols % strip_width != 0) { // the width of last strip is smaller than strip_width
	int strip_num = cols / strip_width + 1
	int strip_width_last = cols - (strip_num - 1) * strip_width
  } else {
	int strip_num = cols / strip_width
	int strip_width_last = strip_width
  }
	
  for (y=0; y<rows; y++)
    for (x=0; x<cols; x++)
    {
	 new_z = x/step_width;
     	 new_y = y;
	 new_x = x % step_width;
//	 img[idx(y,x)].x = bimg[idx(y,x)].x/255.0;   
//	 img[idx(y,x)].y = bimg[idx(y,x)].y/255.0;   
//	 img[idx(y,x)].z = bimg[idx(y,x)].z/255.0;   
	 if (new_z < strip_num - 1) {
	 	img[new_idx(new_z, new_y, new_x)].x = bimg[idx(y,x)].x/255.0;   
	 	img[new_idx(new_z, new_y, new_x)].y = bimg[idx(y,x)].y/255.0;   
	 	img[new_idx(new_z, new_y, new_x)].z = bimg[idx(y,x)].z/255.0;   
	} else { 
	 	img[new_idx_last(new_z, new_y, new_x)].x = bimg[idx(y,x)].x/255.0;   
	 	img[new_idx_last(new_z, new_y, new_x)].y = bimg[idx(y,x)].y/255.0;   
	 	img[new_idx_last(new_z, new_y, new_x)].z = bimg[idx(y,x)].z/255.0;   
	}

    }   
    
    double runtime;
    runtime = apply_stencil(infoHdr.height, infoHdr.width, img_size, strip_width, strip_width_last, strip_num, img, oimg, img_d, oimg_d);
    printf("time for stencil = %f seconds\n",runtime);

  // clear bitmap array
  memset(bimg,0,infoHdr.height*infoHdr.width*3);
  double err = 0.0;
  // convert to uchar3 for output
printf("rows %d cols %d\n",rows, cols);
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
