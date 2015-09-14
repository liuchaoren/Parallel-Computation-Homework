#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "bmp.h"
#include "omp.h"


extern "C" void LoadBMPFile(uchar3 **img, BMPHeader *hdr, BMPInfoHeader *infoHdr, const char *name);

extern "C" void WriteBMPFile(uchar3 **img, BMPHeader hdr, BMPInfoHeader infoHdr, const char *name);

#define idx(A,B) ((A) * infoHdr.width + (B))

typedef struct pixel {
	double x, y, z;
} Pixel;

#ifdef OLD
// filter routine
void filter(pixel  *myimg, pixel *oimg, BMPInfoHeader infoHdr)
{
  int x, y;
  
  for (y = 1; y < infoHdr.height-1; y++)
    for (x =1; x < infoHdr.width-1; x++)
      {
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
#endif

/*
 * The Prewitt kernels can be applied after a blur to help highlight edges
 * The input image must be gray scale/intensities:
 *     double intensity = (in[in_offset].red + in[in_offset].green + in[in_offset].blue)/3.0;
 * Each kernel must be applied to the blured images separately and then composed:
 *     blurred[i] with prewittX -> Xedges[i]
 *     blurred[i] with prewittY -> Yedges[i]
 *     outIntensity[i] = sqrt(Xedges[i]*Xedges[i] + Yedges[i]*Yedges[i])
 * To turn the out intensity to an out color set each color to the intensity
 *     out[i].red = outIntensity[i]
 *     out[i].green = outIntensity[i]
 *     out[i].blue = outIntensity[i]
 *
 * For more on the Prewitt kernels and edge detection:
 *     http://en.wikipedia.org/wiki/Prewitt_operator
 */
void prewittX_kernel(const int rows, const int cols, double * const kernel) {
	if(rows != 3 || cols !=3) {
		printf("Bad Prewitt kernel matrix\n");
		return;
	}
	for(int i=0;i<3;i++) {
		kernel[0 + (i*rows)] = -1.0;
		kernel[1 + (i*rows)] = 0.0;
		kernel[2 + (i*rows)] = 1.0;
	}
}

void prewittY_kernel(const int rows, const int cols, double * const kernel) {
        if(rows != 3 || cols !=3) {
                //std::cerr << "Bad Prewitt kernel matrix\n";
	//	printf("Bad Prewitt kernel matrix\n");
                return;
        }
        for(int i=0;i<3;i++) {
                kernel[i + (0*rows)] = 1.0;
                kernel[i + (1*rows)] = 0.0;
                kernel[i + (2*rows)] = -1.0;
        }
}

/*
 * The gaussian kernel provides a stencil for blurring images based on a 
 * normal distribution
 */
void gaussian_kernel(const int rows, const int cols, const double stddev, double * const kernel) {
	const double denom = 2.0 * stddev * stddev;
	const double g_denom = M_PI * denom;
	const double g_denom_recip = (1.0/g_denom);
	double sum = 0.0;
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			const double row_dist = i - (rows/2);
			const double col_dist = j - (cols/2);
			const double dist_sq = (row_dist * row_dist) + (col_dist * col_dist);
			const double value = g_denom_recip * exp((-dist_sq)/denom);
			kernel[i + (j*rows)] = value;
			sum += value;
		}
	}
	// Normalize
	const double recip_sum = 1.0 / sum;
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			kernel[i + (j*rows)] *= recip_sum;
		}		
	}
}

void do_stencil(const int dim, const int radius, const double stddev, const int rows, const int cols, pixel * const in, pixel * const out, double *kernel) {
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			const int out_offset = i + (j*rows);
			// For each pixel, do the stencil
			for(int x = i - radius, kx = 0; x <= i + radius; ++x, ++kx) {
				for(int y = j - radius, ky = 0; y <= j + radius; ++y, ++ky) {
					if(x >= 0 && x < rows && y >= 0 && y < cols) {
						const int in_offset = x + (y*rows);
						const int k_offset = kx + (ky*dim);
						out[out_offset].x   += kernel[k_offset] * in[in_offset].x;
						out[out_offset].y += kernel[k_offset] * in[in_offset].y;
						out[out_offset].z  += kernel[k_offset] * in[in_offset].z;
					}
				}
			}
		}
	}
}

double  apply_stencil(const int radius, const double stddev, const int rows, const int cols, pixel * const in, pixel * const out) {
	const int dim = radius*2+1;
	double *kernel;
	kernel = (double *) malloc(dim*dim*sizeof(double));
	gaussian_kernel(dim, dim, stddev, kernel);

	double tstart, tend;
      	tstart = omp_get_wtime();
        do_stencil(dim,radius, stddev, rows, cols, in, out, kernel);
        tend = omp_get_wtime();
	return(tend-tstart);
}
// main read, call filter, write new image
int main()
{

  BMPHeader hdr;
  BMPInfoHeader infoHdr;
  uchar3 *bimg;
  Pixel *img,*oimg;
  int x,y,img_size;
  double start, end;
  
  LoadBMPFile(&bimg, &hdr, &infoHdr, "./img.bmp");
  printf("Data init done: size = %d, width = %d, height = %d.\n",
	hdr.size, infoHdr.width, infoHdr.height);

  img_size = infoHdr.width * infoHdr.height * sizeof(Pixel);
  img = (Pixel *) malloc(img_size);
  memset(img,0,img_size);
  oimg = (Pixel *) malloc(img_size);
  memset(oimg,0,img_size);
  // convert to doubles for processing
  for (y=0; y<infoHdr.height; y++)
    for (x=0; x<infoHdr.width; x++)
    {
	 img[idx(y,x)].x = bimg[idx(y,x)].x/255.0;   
	 img[idx(y,x)].y = bimg[idx(y,x)].y/255.0;   
	 img[idx(y,x)].z = bimg[idx(y,x)].z/255.0;   
    }   

    double runtime;
    runtime = apply_stencil(3, 32.0, infoHdr.width, infoHdr.height, img, oimg);
    printf("time for stencil = %f seconds\n",runtime);

  //start = omp_get_wtime();
  //filter(img, oimg, infoHdr);

  //end = omp_get_wtime();
 // printf("Time for filter %f seconds\n",end-start);

  // clear bitmap array
  memset(bimg,0,infoHdr.height*infoHdr.width*3);
  // convert to uchar3 for output
  for (y=0; y<infoHdr.height; y++)
    for (x=0; x<infoHdr.width; x++)
    {
	 bimg[idx(y,x)].x = oimg[idx(y,x)].x*255;   
	 bimg[idx(y,x)].y = oimg[idx(y,x)].y*255;   
	 bimg[idx(y,x)].z = oimg[idx(y,x)].z*255;   
    }   

  // write the output file
  WriteBMPFile(&bimg, hdr,infoHdr, "./img-new.bmp");
  
}
