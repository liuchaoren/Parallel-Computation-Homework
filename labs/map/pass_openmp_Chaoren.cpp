#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>
#include <stddef.h>

#include <openssl/md5.h>
#include <tbb/tick_count.h>

#define pwlimit 100000000
#define pwlen 8

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
    passbuff[pwlen]='\0';
    int charidx;
    int symcount=strlen(chars);
    for(int i=pwlen-1; i>=0; i--) {
        charidx=passnum%symcount;
        passnum=passnum/symcount;
        passbuff[i]=chars[charidx];
    }
}

void strcopy(char* s1, char* s2, int strlen) {
// copy the string in s2, to s1
    for (int i=0; i<strlen; i++) {
        s1[i] = s2[i];
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Usage: %s <password hash>\n",argv[0]);
        return 1;
    }
    long currpass=0;
    int notfound = 1;
    char passmatch[32][pwlen+1];

    tick_count tstart = tick_count::now();
    #pragma omp parallel for private(notfound)
    for(currpass=0; currpass<pwlimit; currpass++) {
        int threadid = omp_get_thread_num();
        genpass(currpass, passmatch[threadid]);
        notfound=test(argv[1], passmatch[threadid]);
//        printf("the threadid is %d\n", threadid);
//        if (currpass == 99999999) {
//           printf("The thread id is %d\n", threadid);
//           printf("The passmatch is %s\n", passmatch[threadid]);
//           notfound=test(argv[1], passmatch[threadid]);
//           printf("the threadid is %d\n", threadid);
//           printf("The notfound is %d\n", notfound);
//           printf("the input is %s\n", argv[1]);
//        }
        if (notfound == 0) {
//           strcopy(passmatch_real, passmatch, 9);
            printf("found: %s\n",passmatch[threadid]);
            
        }
    }
    tick_count tend = tick_count::now();
    printf("time for recovery = %g seconds\n",(tend-tstart).seconds());
    return 0;
}
