// While not particulary secure... it turns out this strategy also isn't too 
// bad (as long as the keys are kept secret and have different lengths and 
// aren't reused together)... or at least that is what a member of the security
// group that works in crypto told me.

#include "key.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <omp.h>
#include <tbb/tick_count.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_opxor.h>

using namespace tbb;

// utility function: given a list of keys, a list of files to pull them from, 
// and the number of keys -> pull the keys out of the files, allocating memory 
// as needed
void getKeys(xorKey* keyList, char** fileList, int numKeys)
{
  int keyLoop=0;
  for(keyLoop=0;keyLoop<numKeys;keyLoop++)
  {
     readKey(&(keyList[keyLoop]), fileList[keyLoop]);
  }
}
//Given text, a list of keys, the length of the text, and the number of keys, encodes the text
void encode(char* plainText, char* cypherText, xorKey* keyList, int ptextlen, int numKeys) {
  tick_count tstart = tick_count::now();

  // Outer loop: process each component parallelly using map
  cilk_for(int charLoop=0;charLoop<ptextlen;charLoop++) {
    char cipherChar=plainText[charLoop]; 
    // Inner loop: process XOR of plain text and keys parallelly using reduce
    // However, seems reduction does not boost the performance, actually mush slower than serialize inner loop
    for(int keyLoop=0;keyLoop<numKeys;keyLoop++) {
       cipherChar=cipherChar ^ getBit(&(keyList[keyLoop]),charLoop);
    }
    cypherText[charLoop]=cipherChar;
  }

  /*
  // Outer loop: process each component parallelly using map
  cilk_for(int charLoop=0;charLoop<ptextlen;charLoop++) {
    cilk::reducer< cilk::op_xor<char> > parallel_cipherChar(plainText[charLoop]);
    // Inner loop: process XOR of plain text and keys parallelly using reduce
    // However, seems reduction does not boost the performance, actually mush slower than serialize inner loop
    cilk_for(int keyLoop=0;keyLoop<numKeys;keyLoop++) {
       *parallel_cipherChar ^=  getBit(&(keyList[keyLoop]),charLoop);
    }
    cypherText[charLoop] = parallel_cipherChar.get_value();
  }
  */

  tick_count tend = tick_count::now();
  printf("time for encode = %g seconds\n",(tend-tstart).seconds());
}

void decode(char* cypherText, char* plainText, xorKey* keyList, int ptextlen, int numKeys) {
  encode(cypherText, plainText, keyList, ptextlen, numKeys); //isn't symmetric key cryptography awesome? 
}

int main(int argc, char* argv[]) {
  if(argc<=2)
  {
      printf("Usage: %s <fileToEncrypt> <key1> <key2> ... <key_n>\n",argv[0]);
      return 1;
  }

  // read in the keys
  int numKeys=argc-2;
  xorKey* keyList=(xorKey*)malloc(sizeof(xorKey)*numKeys); // allocate key list
  getKeys(keyList,&(argv[2]),numKeys);
  
  // read in the data to encrypt/decrypt
  off_t textLength=fsize(argv[1]); //length of our text
  FILE* rawFile=(FILE*)fopen(argv[1],"rb"); //The intel in plaintext
  char* rawData = (char*)malloc(sizeof(char)*textLength);
  fread(rawData,textLength,1,rawFile);
  fclose(rawFile);

  printf("textLength = %d, numKeys = %d \n", textLength, numKeys);

  // Encrypt
  char* cypherText = (char*)malloc(sizeof(char)*textLength);
  encode(rawData,cypherText,keyList,textLength,numKeys);

  // Decrypt
  char* plainText = (char*)malloc(sizeof(char)*textLength);
  decode(cypherText,plainText,keyList,textLength,numKeys);

  // write out
  FILE* encryptedFile=(FILE*)fopen("encryptedOut","wb");
  FILE* decryptedFile=(FILE*)fopen("decryptedOut","wb");

  fwrite(cypherText,textLength,1,encryptedFile);
  fwrite(plainText,textLength,1,decryptedFile);

  fclose(encryptedFile);
  fclose(decryptedFile);

  // Check
  int i;
  for(i=0;i<textLength;i++) {
    if(rawData[i]!=plainText[i]) {
      printf("Encryption/Decryption is non-deterministic\n");
      i=textLength;
    }
  }

  return 0;

}
