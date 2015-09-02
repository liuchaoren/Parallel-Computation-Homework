#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/md5.h>
#include <tbb/tick_count.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

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

#define NUM_WORKER 32
#define TILE_SIZE 16*NUM_WORKER
int main(int argc, char** argv) {
   if(argc <  2) {
    printf("Usage: %s <password hash>\n",argv[0]);
    return 1;
  }
  int type = 1;
  if (argc == 3)
    type = atoi(argv[2]);


  tick_count tstart = tick_count::now();
  char final_passmatch[9];

  /* 
     First version: Processing a batch within one iteration
     Process TILE_SIZE subtasks parallelly in the while loop 
     Time wasted at end of parallel section because of untied run time
  */
  if (type == 1)
    {
      char thread_passmatch[NUM_WORKER][9];
      long currpass = 0;
      int notfound = 1;

      while(notfound)
	{
	  cilk_for(int i = 0; i < TILE_SIZE; i++)
	    {
	      // Get id for current thread
	      int thread_id = __cilkrts_get_worker_number();
	      // Process currpass+i and store it in thread_passmatch[thread_id]
	      genpass(currpass+i, thread_passmatch[thread_id]);
	      // If the test passed, store current result
	      // Note: only one correct result for this case, there is no race condition here
	      if ( !test(argv[1], thread_passmatch[thread_id]) )
		{
		  notfound = 0;
		  strcpy(final_passmatch, thread_passmatch[thread_id]);
		}
	    }
	  currpass += TILE_SIZE;  // Update currpass by TILE_SIZE
	}
    }
  /*
    Second version: Each thread works on its own subtask group as a while loop
    Time wasted for untied run time for different subtask groups
  */
  else if (type == 2)
    {
      char thread_passmatch[NUM_WORKER][9];
      long thread_currpass[NUM_WORKER];
      int notfound = 1;

      cilk_for(int i = 0; i < NUM_WORKER; i++)
	{
	  // Initialize currpass for each thread
	  thread_currpass[i] = i;
	  while(notfound)
	    {
	      // Process currpass and store it to passmach for this thread
	      genpass(thread_currpass[i], thread_passmatch[i]);
	      // If the test passed, store current result
	      // Note: only one correct result for this case, there is no race condition here
	      if ( !test(argv[1], thread_passmatch[i]) )
		{
		  notfound = 0;
		  strcpy(final_passmatch, thread_passmatch[i]);
		}
	      // Update currpass for this thread
	      thread_currpass[i] += NUM_WORKER;
	    }
	}
    }

  tick_count tend = tick_count::now();
  printf("time for recovery = %g seconds\n",(tend-tstart).seconds());
  printf("found: %s\n",final_passmatch);
  return 0;
}
