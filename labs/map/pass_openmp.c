#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/md5.h>
#include <tbb/tick_count.h>
#include <omp.h>

using namespace tbb;


const char* chars="0123456789";

// tests if a hash matches a candidate password
int test(const char* passhash, const char* passcandidate) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    
    MD5((unsigned char*)passcandidate, strlen(passcandidate), digest);
    
    char mdString[34];
    mdString[33]='\0';
    for(int i=0; i<16; i++) {
      sprintf(&mdString[i*2], "%02x", (unsigned char)digest[i]);    
    }

    return strncmp(passhash, mdString, strlen(passhash));
}

// maps a PIN to a string
void genpass(long passnum, char* passbuff) {
    passbuff[8]='\0';
    int charidx;
    int symcount=strlen(chars);
    for(int i=7; i>=0; i--) {
        charidx=passnum%symcount;
        passnum=passnum/symcount;
        passbuff[i]=chars[charidx];
    }
}

// check the whether found in this batch, if found then assign the match_index for the batch
bool parallel_not_found(int* notfound, int N, int& match_index)
{
  for(int i = 0; i < N; i++)
    if (!notfound[i]) {
      match_index = i;
      // printf("Finally found at %d \n",i);
      return false;
    }
  return true;
};

#define NUM_WORKER 32 // The number of workers
#define TILE_SIZE 32*NUM_WORKER  // The size of each batch
#define pwlimit 100000000
#define pwlen 8
int main(int argc, char** argv) {
  if(argc <  2) {
    printf("Usage: %s <password hash>\n",argv[0]);
    return 1;
  }
  // This variable indicates running whose code
  // 1: Mengke Lian; 2: Kai Fan; 3: Chaoren Liu
  int who = 1;
  if (argc >= 3)
    who = atoi(argv[2]);
  // This variable indicates running which method
  // Since it is possible that one person tries multiple approaches
  int type = 1;
  if (argc >= 4)
    type = atoi(argv[3]);

  tick_count tstart = tick_count::now();
  char final_passmatch[9];  // Final result of passmatch

  if (who == 1)
    { 
      /* 
	 First version: Processing a batch within one iteration
	 Process TILE_SIZE subtasks parallelly in the while loop 
	 Time wasted at end of parallel section because of untied run time
      */
      if (type == 1)
	{
	  long currpass = 0;
	  // Change passmatch string into a vector of string for the batch
	  char passmatch[TILE_SIZE][9];  
	  // Change indicator scalar into indicator vector for the batch  
	  int notfound[TILE_SIZE];  

	  // Initialize indicator vector so that nothing found at first
	  for(int i = 0; i < TILE_SIZE; i++)
	    notfound[i] = 1;
	  int match_index = -1;  // Initialize match_index

	  omp_set_num_threads(NUM_WORKER);  // Set OMP thread number
	  while(parallel_not_found(notfound,TILE_SIZE,match_index)) {
	    // In each iteration, a batch of matching processes is done parallelly
#pragma omp parallel for
	    for(int i = 0; i < TILE_SIZE; i++)
	      {
		genpass(currpass+i,passmatch[i]);
		notfound[i] = test(argv[1], passmatch[i]);
		// printf("Processing %d at thread %d, passmatch = %s, notfound = %d \n", currpass+i, omp_get_thread_num(), passmatch[i], notfound[i]);
	      }
	    currpass += TILE_SIZE;  // Update the base index for next batch
	  }
	  strcpy(final_passmatch, passmatch[match_index]);
	}
      /*
	Second version: Threads races in the while loop
	Threads keep working on the next subtask
	Time wasted at critical update of shared_currpass, not suitable for large thread number
      */
      else if (type == 2)
	{
	  int shared_currpass = -1;  // Shared loop counter
	  int shared_notfound = 1;  // Shared loop stop flag
	  int thread_id;  // Thread ID
	  int thread_currpass;  // Thread loop counter
	  int  thread_notfound;  // Thread loop stop flag
	  char shared_passmatch[NUM_WORKER/2][9];  // Shared passmatch strings
	  omp_set_num_threads(NUM_WORKER/2);
#pragma omp parallel private(thread_id, thread_currpass, thread_notfound)
	  {
#pragma omp single
	    printf("There are %d threads in total. \n", omp_get_num_threads());
	    thread_id = omp_get_thread_num();
	    while(shared_notfound != 0)
	      {
		// Put shared_counter in critical region so that only one thread write it at same time
#pragma omp critical
		{
		  shared_currpass++;
		  thread_currpass = shared_currpass;
		}
		// Do the matching process
		genpass(thread_currpass,shared_passmatch[thread_id]);
		thread_notfound = test(argv[1], shared_passmatch[thread_id]);
		// If there is one thread_notfound becomes 0, change shared_notfound to 0
		if (thread_notfound == 0)
		  {
		    shared_notfound = 0;
		    strcpy(final_passmatch, shared_passmatch[thread_id]);
#pragma omp flush(shared_notfound)
		  }
		// printf("currpass = %d, passmatch = %s, thread %d, notfound = %d \n", thread_currpass, shared_passmatch[thread_id], thread_id, thread_notfound);
	      }
	  }
	}
      /*
	Third version: Each thread works on its own subtask group as a while loop
	Time wasted for untied run time for different subtask groups
      */
      else if (type == 3)
	{
	  int shared_notfound = 1;  // Shared loop stop flag
	  int thread_id;  // Thread ID
	  int thread_currpass = 0;  // Thread loop counter
	  int  thread_notfound;  // Thread loop stop flag
	  char shared_passmatch[NUM_WORKER][9];  // Shared passmatch strings

	  omp_set_num_threads(NUM_WORKER);
#pragma omp parallel private(thread_id, thread_currpass, thread_notfound)
	  {
	    thread_id = omp_get_thread_num();
	    while(shared_notfound != 0)
	      {
		// Do the matching process
		genpass(thread_currpass,shared_passmatch[thread_id]);
		thread_notfound = test(argv[1], shared_passmatch[thread_id]);
		// If there is one thread_notfound becomes 0, change shared_notfound to 0
		if (thread_notfound == 0)
		  {
		    shared_notfound = 0;
		    strcpy(final_passmatch, shared_passmatch[thread_id]);
#pragma omp flush(shared_notfound)
		  }
		// Update the currpass for this thread
		thread_currpass += NUM_WORKER;
		// printf("currpass = %d, passmatch = %s, thread %d, notfound = %d \n", thread_currpass, shared_passmatch[thread_id], thread_id, thread_notfound);
	      }
	  }
	}
      else
	printf("The forth argument for Mengke Lian's code is for the type, please input 1,2,3");
    }	   
  else if (who == 2)
    {
      // Kai Fan's code here
      // First Version: While-Batch Model
      if (type == 1)
	{
	  omp_set_num_threads(NUM_WORKER);
	  long currpass = 0;
	  int notfound = 1;

	  int BatchSize = 2048;
	  char temppassmatch[BatchSize][9];
   
	  while(notfound) {
     
#pragma omp parallel for  shared(temppassmatch)
	    for (int i = 0; i < BatchSize; i++){
	      genpass(currpass+i,temppassmatch[i]);
	      if (test(argv[1], temppassmatch[i]) == 0) {
		notfound = 0;
		strcpy(final_passmatch,temppassmatch[i]);
	      }
	    }
	    currpass += BatchSize;
	  }
	}
      // Second Version: While-Coset Model
      else if (type == 2)
	{
	  omp_set_num_threads(NUM_WORKER);
	  long currpass = 0;
	  int notfound = 1;
    
	  int threadId;
	  int thread_notfound = 1;
	  char shared_passmatch[NUM_WORKER][9];

#pragma omp parallel private(currpass, threadId, thread_notfound) shared(notfound, shared_passmatch)
	  {
	    threadId = omp_get_thread_num();
    
	    while(notfound) {
      
	      genpass(currpass,shared_passmatch[threadId]);
	      thread_notfound = test(argv[1], shared_passmatch[threadId]);
	      if (thread_notfound ==0) {
		notfound = 0;
		strcpy(final_passmatch,shared_passmatch[threadId]);
#pragma omp flush(notfound)
	      }
	      currpass += NUM_WORKER;
	      //printf("%ld %d %d\n", currpass, threadId, omp_get_num_threads());
	    }
	  }
	}
    }
  else if (who == 3)
    {
      // Chaoren Liu's code here
      long currpass=0;
      int notfound = 1;
      char passmatch[NUM_WORKER][pwlen+1];
  
#pragma omp parallel for private(notfound)
      for(currpass=0; currpass<pwlimit; currpass++) {
	int threadid = omp_get_thread_num();
	genpass(currpass, passmatch[threadid]);
	notfound=test(argv[1], passmatch[threadid]);
	//          printf("the threadid is %d\n", threadid);
	//          if (currpass == 99999999) {
	//             printf("The thread id is %d\n", threadid);
	//             printf("The passmatch is %s\n", passmatch[threadid]);
	//             notfound=test(argv[1], passmatch[threadid]);
	//             printf("the threadid is %d\n", threadid);
	//             printf("The notfound is %d\n", notfound);
	//             printf("the input is %s\n", argv[1]);
	//          }
	if (notfound == 0) {
	  strcpy(final_passmatch, passmatch[threadid]);                  
	  //           strcopy(passmatch_real, passmatch, 9);
	  //              printf("found: %s\n",passmatch[threadid]);
       	}
      } 
    }
  else
    printf("The third argument is for choosing whose code, please input 1,2,3 \n");

  tick_count tend = tick_count::now();
  printf("time for recovery = %g seconds\n",(tend-tstart).seconds());
  printf("found: %s\n",final_passmatch);
  return 0;
}

