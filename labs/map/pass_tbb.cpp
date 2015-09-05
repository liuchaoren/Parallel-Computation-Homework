#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/md5.h>
#include <tbb/tick_count.h>
#include <tbb/tbb.h>
#include <tbb/task_group.h>

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

int global_notfound = 1;
#define NUM_WORKER 32
#define TILE_SIZE 16*NUM_WORKER
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
	   // Since in TBB there is no good way to show thread ID, space has to sacrifice to avoid write conflict in thread_passmatch
	   char thread_passmatch[TILE_SIZE][9];
	   long currpass = 0;
	   int notfound = 1;

	   while(notfound)
	     {
	       parallel_for(blocked_range<int>(0,TILE_SIZE),
			    [&](blocked_range<int> r)
			    {
			      for (int i = r.begin(); i < r.end(); i++)
				{
				  // Process currpass+i and store it in thread_passmatch[i]
				  genpass(currpass+i, thread_passmatch[i]);
				  // If the test passed, store current result
				  // Note: only one correct result for this case, there is no race condition here
				  if ( !test(argv[1], thread_passmatch[i]) )
				    {
				      notfound = 0;
				      strcpy(final_passmatch, thread_passmatch[i]);
				    }
				}
			    }
			    );
	       currpass += TILE_SIZE;  // Update currpass by TILE_SIZE
	     }
	 }
       /*
	 Second version: Each thread works on its own subtask group as a while loop
	 Time wasted for untied run time for different subtask groups
       */
       else if (type == 2)
	 {
	   auto subtask = [] (int i, char* final, char* answer)
	     {
	       int currpass = i;
	       char passmatch[9];
	       while(global_notfound)
		 {
		   // Process currpass and store it to passmach for this thread
		   genpass(currpass, passmatch);
		   // If the test passed, store current result
		   // Note: only one correct result for this case, there is no race condition here
		   if ( !test(answer, passmatch) )
		     {
		       global_notfound = 0;
		       strcpy(final, passmatch);
		     }
		   // Update currpass for this thread
		   currpass += NUM_WORKER;
		   // printf("Task %d, currpass = %d \n", i, currpass);
		 }
	     };
      
	   // Build task group and put in subtask into it
	   task_group g;
	   for (int i = 0; i < NUM_WORKER; i++)
	     g.run([=,&final_passmatch] 
		   {
		     subtask(i, final_passmatch, argv[1]);
		   });
	   g.wait();
	 }
       else
	 printf("The forth argument for Mengke Lian's code is for the type, please input 1,2");
     }	   
   else if (who == 2)
     {
       // Kai Fan's code here
     }
   else if (who == 2)
     {
       // Chaoren Liu's code here
     }
   else
     printf("The third argument is for choosing whose code, please input 1,2,3 \n");

  tick_count tend = tick_count::now();
  printf("time for recovery = %g seconds\n",(tend-tstart).seconds());
  printf("found: %s\n",final_passmatch);
  return 0;
}
