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
}

#define TILE_SIZE 32 // The size of each batch
int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Usage: %s <password hash>\n",argv[0]);
        return 1;
    }
    char passmatch[TILE_SIZE][9];  // Change passmatch string into a vector of string for the batch
    long currpass = 0;
    int notfound[TILE_SIZE];  // Change indicator scalar into indicator vector for the batch
    tick_count tstart = tick_count::now();

    // Initialize indicator vector so that nothing found at first
    for(int i = 0; i < TILE_SIZE; i++)
      notfound[i] = 1;
    int match_index = -1;  // Initialize match_index

    while(parallel_not_found(notfound,TILE_SIZE,match_index)) {
      // In each iteration, a batch of matching processes is done parallelly
#pragma omp parallel for
      for(int i = 0; i < TILE_SIZE; i++)
      {
        genpass(currpass+i,passmatch[i]);
        notfound[i] = test(argv[1], passmatch[i]);
	//printf("Processing %d at thread %d, passmatch = %s, notfound = %d \n", currpass+i, omp_get_thread_num(), passmatch[i], notfound[i]);
      }
      currpass += TILE_SIZE;
    }

    tick_count tend = tick_count::now();
    printf("time for recovery = %g seconds\n",(tend-tstart).seconds());
    printf("found: %s\n",passmatch[match_index]);
    return 0;
}
