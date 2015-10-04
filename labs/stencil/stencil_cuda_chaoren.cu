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
//#define new_idx(A, B, C) (A * strip_width * rows + B * strip_width + C)
//#define new_idx_last(A, B, C) (A * strip_width * rows + B * strip_width_last + C)

#define thrds 256

typedef struct pixel {
	float x, y, z;
} Pixel;

__device__ int indexFinder(int y, int x, int rowLen) {
  return y * rowLen + x;
}


__global__ void filter(int rows, int cols, Pixel *myimg, Pixel *oimg) 
{
  unsigned int tempThreadsx = threadsPerBlock.x + 2;
  unsigned int tempThreadsy = threadsPerBlock.y + 2;

  __shared__ Piexel* temp = cudamalloc(tempThreadsx * tempThreadsy *sizeof(Pixel));

  int globalx = blockIdx.x * threadsPerBlock.x + threadIdx.x;
  int globaly = blockIdx.y * threadsPerBlock.y + threadIdx.y;
  if (globalx < cols && globaly < rows) {
    usigned int gindex = indexFinder(globaly, globalx,  cols);
    usigned int tempx = threadIdx.x + 1;
    usigned int tempy = threadIdx.y + 1;
    usigned int bindex = indexFinder(tempy, tempx, tempThreadsx);
    temp[bindex] = myimg[gindex];   // load itself
    if (threadIdx.x == 0 && globalx != 0) {
      int leftbindex = bindex - 1;
      int leftgindex = gindex - 1;
      temp[leftbindex]  = myimg[leftgindex];
    }
    if (threadIdx.x == threadsPerBlock.x - 1 && globalx != cols - 1) {
 		int rightbindex = bindex + 1;
 		int rightgindex = gindex + 1;
 		temp[rightbindex] = myimg[rightgindex]
    }
    if(threadIdx.y == 0 && globaly != 0) {
    	int abovebindex = bindex - tempThreadsx;
    	int abovegindex = gindex - cols;
    	temp[abovebindex] = myimg[abovegindex];
    }
    if(threadIdx.y == threadsPerBlock.y - 1 && globaly ! = rows - 1) {
    	int underbindex = bindex + tempThreadsx;
    	int undergindex = gindex + cols;
    	temp[underbindex] = myimg[undergindex];
    }
    int cornerbindex, cornergindey;
    if (threadIdx.x == 0 and threadIdx.y == 0) { 
    	cornerbindex = bindex - tempThreadsx - 1;
    	cornergindey = gindex - cols - 1;
    	temp[cornerbindex] = myimg[cornergindex];
    }
     if (threadIdx.x == 0 and threadIdx.y == threadsPerBlock.y - 1) { 
    	cornerbindex = bindex + tempThreadsx - 1;
    	cornergindey = gindex + cols - 1;
    	temp[cornerbindex] = myimg[cornergindex];
    }
    if (threadIdx.x == threadsPerBlock.x - 1 and threadIdx.y == 0) { 
    	cornerbindex = bindex - tempThreadsx + 1;
    	cornergindey = gindex - cols + 1;
    	temp[cornerbindex] = myimg[cornergindex];
    }
    if (threadIdx.x == threadsPerBlock.x - 1 and threadIdx.y == threadsPerBlock.y - 1) { 
    	cornerbindex = bindex + tempThreadsx + 1;
    	cornergindey = gindex + cols + 1;
    	temp[cornerbindex] = myimg[cornergindex];
    }

    __syncthreads();
    if (globalx > 0 && globalx < cols - 1 && globaly > 0 and globaly < rows - 1) {
    	oimg[gindex].z = (temp[indexFinder(tempy, tempx, threadsPerBlock.x)].z 
    					+ temp[indexFinder(tempy, tempx-1, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy, tempx+1, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy-1, tempx, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy-1, tempx-1, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy-1, tempx+1, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy+1, tempx, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy+1, tempx-1, threadsPerBlock.x)].z
    					+ temp[indexFinder(tempy+1, tempx+1, threadsPerBlock.x)].z) / 9;

    	oimg[gindex].y = (temp[indexFinder(tempy, tempx, threadsPerBlock.x)].y 
    					+ temp[indexFinder(tempy, tempx-1, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy, tempx+1, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy-1, tempx, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy-1, tempx-1, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy-1, tempx+1, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy+1, tempx, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy+1, tempx-1, threadsPerBlock.x)].y
    					+ temp[indexFinder(tempy+1, tempx+1, threadsPerBlock.x)].y) / 9;

    	oimg[gindex].x = (temp[indexFinder(tempy, tempx, threadsPerBlock.x)].x 
    					+ temp[indexFinder(tempy, tempx-1, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy, tempx+1, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy-1, tempx, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy-1, tempx-1, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy-1, tempx+1, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy+1, tempx, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy+1, tempx-1, threadsPerBlock.x)].x
    					+ temp[indexFinder(tempy+1, tempx+1, threadsPerBlock.x)].x) / 9;
    }
  }
}

double  apply_stencil(const int rows, const int cols, Pixel * const in_d, Pixel * const out_d, Piexl * const out, uint64_t img_size) {
	dim3 threadsPerBlock(16, 16);
	blockx = cols % threadsPerBlock.x = 0 ? cols/threadsPerBlock.x : cols/threadsPerBlock.x + 1
	blocky = rows % threadsPerBlock.y = 0 ? rows/threadsPerBlock.y : rows/threadsPerBlock.y + 1
	dim3 numBlocks(blockx, blocky);
	double tstart, tend;
    tstart = omp_get_wtime();
	filter<<<numBlocks, threadsPerBlock>>>(rows, cols, in_d, out_d);
    tend = omp_get_wtime();
    cudaMemcpy(out, out_d, img_size, cudaMemcpyDeviceToHost);
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
//  uint64_t new_x, new_y, new_z;
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
//  if (cols % strip_width != 0) { // the width of last strip is smaller than strip_width
//	int strip_num = cols / strip_width + 1
//	int strip_width_last = cols - (strip_num - 1) * strip_width
//  } else {
//	int strip_num = cols / strip_width
//	int strip_width_last = strip_width
//  }
	
  for (y=0; y<rows; y++)
    for (x=0; x<cols; x++)
    {
//	 new_z = x/step_width;
//	 new_y = y;
//	 new_x = x % step_width;
//	 img[idx(y,x)].x = bimg[idx(y,x)].x/255.0;   
//	 img[idx(y,x)].y = bimg[idx(y,x)].y/255.0;   
//	 img[idx(y,x)].z = bimg[idx(y,x)].z/255.0;   
	 img[idx(y,x)].x = bimg[idx(y,x)].x/255.0;   
	 img[idx(y,x)].y = bimg[idx(y,x)].y/255.0;   
	 img[idx(y,x)].z = bimg[idx(y,x)].z/255.0;   
    }   

// copy to cuda memory
  cudaMemcpy(img_d, img, img_size, cudaMemcpyHostToDevice);
  // cudaMemcpy(oimg_d, oimg, img_size, cudaMemcpyHostToDevice);
    
    double runtime;
    runtime = apply_stencil(infoHdr.height, infoHdr.width, img_d, oimg_d, oimg, img_size);
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
  free(img); free(oimg); free(bimg); 
  cudaFree(img_d); cudaFree(oimg_d);
  
}
