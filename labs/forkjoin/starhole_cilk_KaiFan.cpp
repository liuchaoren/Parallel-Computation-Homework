#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <tbb/tick_count.h>
#include <vector>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

using namespace cv;
using namespace tbb;

#include "starhole_common.cpp"

// Represents the area in which the particles move
static DirUpdate*   area;

// Holds the split probabilities for Carter particles in cells
static double*      splitProb;

// Holds final counts of particles in cells
static int* outArea;

// Configuration parameters
static int radius;
static int sim_steps;

#define NUM_THREAD 32
#define grainsize 2500

// Returns the total number of particles descending from this call
// and increments the count at the right location
int walker(long int seed, int x, int y, int stepsremaining) {
    struct drand48_data seedbuf;
    srand48_r(seed, &seedbuf);
    int particles = 1;
    
    if (stepsremaining <= grainsize) {
      for( ; stepsremaining>0 ; stepsremaining-- ) {
        
        // Does the Carter particle split? If so, start the walk for the new one
	if(doesSplit(&seedbuf, splitProb, x, y, radius)) {
	  //printf("spliting!\n");
	  long int newseed;
	  lrand48_r(&seedbuf, &newseed);
	  particles += walker(seed + newseed, x, y, stepsremaining-1);
        }
        
        // Make the particle walk?
        updateLocation(&seedbuf, area, &x, &y, radius);
      }
  
    // record the final location
    outArea[toOffset(x,y,radius)] += 1;
    }
    else { // divide and conquer
      vector<int> temp(stepsremaining,0);
      for( ; stepsremaining>0 ; stepsremaining-- ) {
        
        // Does the Carter particle split? If so, start the walk for the new one
	if(doesSplit(&seedbuf, splitProb, x, y, radius)) {
	  //printf("spliting!\n");
	  long int newseed;
	  lrand48_r(&seedbuf, &newseed);	
	  temp[stepsremaining-1] = cilk_spawn walker(seed + newseed, x, y, stepsremaining-1);
        }
	// Make the particle walk?
        updateLocation(&seedbuf, area, &x, &y, radius);
      }

      cilk_sync;
      // record the final location
      outArea[toOffset(x,y,radius)] += 1;

      for (vector<int>::iterator iter = temp.begin(); iter < temp.end(); iter++)
	particles += *iter;
    }

    return particles;
}

int main(int argc, char** argv) {
    if(argc<6 || ((argc-4)%2 != 0)) {
        printf("Usage: %s <steps> <radius> <amount> <x1> <y1> ... <xN> <yN>\n",argv[0]);
        return 1;
    }

    printf("Attempting to setup initial state...\n");
    // Initialize simulation Params
    int* coords;
    int coordPairs, amount;
    readArgs(argc, argv, &sim_steps, &radius, &amount, &coordPairs, &coords);
    
    // Initialize simulation lookups
    initialize(radius,&outArea,&splitProb,&area);
    
    
    // Start initial walks
    printf("Starting the walks...\n");
    int totParticles = 0;
    tick_count tstart = tick_count::now();
    cilk_for(int i=0;i<coordPairs*2;i+=2) {
        cilk_for(int j=0;j<amount;j++) {
            totParticles += walker(i+j, coords[i], coords[i+1], sim_steps);
        }
    }
    tick_count tend = tick_count::now();
    printf("Walks complete... finished with %d particles\n",totParticles);
    printf("time for all walks = %g seconds\n",(tend-tstart).seconds());
   
    // Generate the output
    writeOutput(radius, outArea);

    free(coords);
    free(outArea);
    free(splitProb);
    free(area);
}
