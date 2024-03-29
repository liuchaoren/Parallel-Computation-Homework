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
#include <tbb/tbb.h>
#include <tbb/task_group.h>

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

#define NUM_THREADS 32
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
      task_group g;
      vector<int> temp(stepsremaining,0);
      /*
      int * temp = new int[stepsremaining];
      int len = stepsremaining;
      for (int i = 0; i < len; i++)
	temp[i]=0;
      */
      for( ; stepsremaining>0 ; stepsremaining-- ) {
        
        // Does the Carter particle split? If so, start the walk for the new one
	if(doesSplit(&seedbuf, splitProb, x, y, radius)) {
	  //printf("spliting!\n");
	  long int newseed;
	  lrand48_r(&seedbuf, &newseed);
	  g.run( [=,&temp]{ temp[stepsremaining-1] = walker(seed + newseed, x, y, stepsremaining-1);} );
        }
        
        // Make the particle walk?
        updateLocation(&seedbuf, area, &x, &y, radius);
      }

      g.wait();
      // record the final location
      outArea[toOffset(x,y,radius)] += 1;

      //      for (int i = 0; i < len; i++)
      //	particles += temp[i];
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
    

  for (int i = 0; i < coordPairs*2; i+=2) {
    totParticles += parallel_reduce(blocked_range<int>(0,amount),int(0),
				    [&](blocked_range<int> t, int reducetot)
				    {
				      for (int j = t.begin(); j < t.end(); j++)
					reducetot += walker(i+j, coords[i], coords[i+1], sim_steps);
				      return reducetot;
				    },
				    [](int x, int y) -> int {return x + y;}
				    );
  }
        
  /*
    for(int i=0;i<coordPairs*2;i+=2) {
        for(int j=0;j<amount;j++) {
	  totParticles += walker(i+j, coords[i], coords[i+1], sim_steps);
        }
    }
*/

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
