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
#include <tbb/tbb.h>

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


class XORsum {
    xorKey* my_keyList;
    int charLoop;
    public:
        char cipherChar;
        void operator() (const blocked_range<int>& r) {
            xorKey* keyList = my_keyList;
            for(int keyLoop=r.begin(); keyLoop != r.end(); keyLoop++) {
                cipherChar=cipherChar ^ getBit(&(keyList[keyLoop]),charLoop);
            }
        }
        
        XORsum(XORsum& x, split) : my_keyList(x.my_keyList), charLoop(x.charLoop), cipherChar(char(0)) {}
        void join(const XORsum& y) {cipherChar=cipherChar ^ y.cipherChar;}
        XORsum(xorKey* keyList, int charLoop) : my_keyList(keyList), charLoop(charLoop), cipherChar(char(0)) {}
};

//Given text, a list of keys, the length of the text, and the number of keys, encodes the text
void encode(char* plainText, char* cypherText, xorKey* keyList, int ptextlen, int numKeys) {
  int keyLoop=0;
  int charLoop=0;
  tick_count tstart = tick_count::now();

  // Change code here and re-compile it to run someone else's implementation
  // 1: Mengke Lian; 2: Kai Fan; 3: Chaoren Liu
  int who = 1;

  if (who == 1)
    {
      // Outer loop: process each component parallelly using map
      parallel_for(blocked_range<int>(0, ptextlen),
		   [&](blocked_range<int> r)
		   {
		     for (int i = r.begin(); i < r.end(); i++)
		       {
			 // Inner loop: process XOR of plain text and keys parallelly using reduce
			 // However, seems reduction does not boost the performance
			 cypherText[i] = parallel_reduce(blocked_range<int>(0,numKeys), 
							 char(0),
							 [&](blocked_range<int> rr, char cipherChar) -> char
							 {
							   for (int j = rr.begin(); j < rr.end(); j++)
							     cipherChar ^= getBit(&(keyList[j]),i);
							   return cipherChar;
							 },
							 [](char x, char y) -> char
							 {
							   return x ^ y;
							 }
							 ) ^ plainText[i];
			 /* char cipherChar = plainText[i]; 
			 for(int keyLoop = 0; keyLoop < numKeys; keyLoop++) {
			   cipherChar=cipherChar ^ getBit(&(keyList[keyLoop]),i);
			 }
			 cypherText[i]=cipherChar; */
		       }
		   }
		   );
    }
  else if (who == 2)
    {
      // Kai Fan's code here
      parallel_for(blocked_range<int>(0,ptextlen),
		   [&](blocked_range<int> r) 
		   {
		     for (int charLoop = r.begin(); charLoop < r.end(); charLoop++) 
		       {
			 char cipherChar =
			   plainText[charLoop] ^ parallel_reduce(blocked_range<int> (0,numKeys), char(0),
								 [&](blocked_range<int> t, char reducecipherChar)
								 {
								   for (int keyLoop = t.begin(); keyLoop < t.end(); keyLoop++)
								     reducecipherChar ^= getBit(&(keyList[keyLoop]),charLoop);
								   return reducecipherChar;
								 },
								 [](char x, char y) -> char {return x^y;}
								 );
			 cypherText[charLoop] = cipherChar;
		       }
		   }
		   );

    }
  else if (who == 3)
    {
      // Chaoren Liu's code here
// TBB part
  tbb::parallel_for(
    tbb::blocked_range<int>(0, ptextlen),
    [&](tbb::blocked_range<int> r) {
        for (int i=r.begin(); i<r.end(); i++) {
            XORsum xorchar(keyList, i);
            parallel_reduce(tbb::blocked_range<int>(0, numKeys), xorchar); 
//            for(keyLoop=0;keyLoop<numKeys;keyLoop++) {
//                cipherChar=cipherChar ^ getBit(&(keyList[keyLoop]),i);
//            }
//            cypherText[i]=cipherChar;
            cypherText[i]=plainText[i] ^ xorchar.cipherChar; 
        }
    }
  );
    }

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
  printf("the default number of threads is %d\n",tbb::task_scheduler_init::default_num_threads());

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
