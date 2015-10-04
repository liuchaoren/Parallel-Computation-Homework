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

const int tile_size = 32;

__global__
void filter(Pixel *myimg, Pixel *oimg, int rows, int cols)
{
  __shared__ Pixel temp[tile_size+2][tile_size+2];

  int x = threadIdx.x + blockDim.x*blockIdx.x;
  int y = threadIdx.y + blockDim.y*blockIdx.y;

  int xx = threadIdx.x+1;
  int yy = threadIdx.y+1;

  int Dx = x+blockDim.x < cols ? blockDim.x : cols-x-1;
  int Dy = y+blockDim.y < rows ? blockDim.y : rows-y-1;
      	 
  // Copy pixels to shared memory
  if (x < cols && y < rows)
    {
      // Interior pixels
      temp[yy][xx] = myimg[idx(y,x)];
      // Left & right side pixels
      if(threadIdx.x == 0)
      	{
	  temp[yy][0] = myimg[idx(y,x-1)];
      	  temp[yy][Dx+1] = myimg[idx(y,x+Dx)];
      	}
      // Top & bottom pixels
      if(threadIdx.y == 0)
      	{
	  temp[0][xx] = myimg[idx(y-1,x)];
      	  temp[Dy+1][xx] = myimg[idx(y+Dy,x)];
      	}
      // Corner pixels
      if(threadIdx.x == 0 && threadIdx.y == 0)
      	{
      	  temp[0][0] = myimg[idx(y-1,x-1)];
      	  temp[0][Dx+1] = myimg[idx(y-1,x+Dx)];
      	  temp[Dy+1][0] = myimg[idx(y+Dy,x-1)];
      	  temp[Dy+1][Dx+1] = myimg[idx(y+Dy,x+Dx)];
      	}
    }

  __syncthreads();

  // Compute stencil for the block
  if (x > 0 && x < cols-1 && y > 0 && y < rows-1)
    {
      Pixel result;
      result.x = 0;
      result.y = 0;
      result.z = 0;

      for(int dy = -1; dy <=1; dy++)
	{
	  for(int dx = -1; dx <=1; dx++)
	    {
	      result.x += temp[yy+dy][xx+dx].x;
	      result.y += temp[yy+dy][xx+dx].y;
	      result.z += temp[yy+dy][xx+dx].z;
	    }
	}

      oimg[idx(y,x)].x = result.x/9;
      oimg[idx(y,x)].y = result.y/9;
      oimg[idx(y,x)].z = result.z/9;
    }
}

double  apply_stencil(const int rows, const int cols, Pixel * const in, Pixel * const out) {
  Pixel *d_in, *d_out;
  cudaMalloc(&d_in, rows*cols*sizeof(Pixel));
  cudaMalloc(&d_out, rows*cols*sizeof(Pixel));
  cudaMemcpy(d_in, in, rows*cols*sizeof(Pixel), cudaMemcpyHostToDevice);

  const dim3 blockSize(tile_size,tile_size,1);
  const dim3 gridSize((cols+tile_size-1)/tile_size,(rows+tile_size-1)/tile_size,1);

  double tstart, tend;
  tstart = omp_get_wtime();
  filter<<<gridSize, blockSize>>>(d_in, d_out, rows, cols);
  cudaDeviceSynchronize();
  tend = omp_get_wtime();

  cudaMemcpy(out, d_out, rows*cols*sizeof(Pixel), cudaMemcpyDeviceToHost);
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
  //double start, end;
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
