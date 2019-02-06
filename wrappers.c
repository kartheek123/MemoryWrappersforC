/*
Implementation of Wrapper functions
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <math.h>


//Buffer for when malloc is not allocated yet but it is called
char tmpbuff[1024*1024];
// Padding variable to hold size content of memory
static const size_t memoryPadding = 16;
//To keep track of the amount of size allocated in tmpbuff
static size_t tmpbuffAllocatedSize = 0;

/*
  Wrapper Functions
*/
static void* (*mallocWrapperFunc)(size_t) = NULL;
static void* (*callocWrapperFunc)(size_t, size_t) = NULL;
static void* (*reallocWrapperFunc)(void *ptr, size_t size);
static void (*freeWrapperFunc)(void *) = NULL;

/*
Clock variables to figure out if 5 seconds have pased to print
*/
static clock_t begin;
static clock_t clockForPrint;

/*
  Statistics variables for printing summary of allocations
*/
static int timeToPrint;
static int ageAllocationCounter = 0;
static int overallAllocationsSinceStart = 0;
static float totalAllocatedSize = 0;

/*
  Allocations by size variables
*/
static int bytes0to4 = 0;
static int bytes4to8 = 0;
static int bytes8to16 = 0;
static int bytes16to32 = 0;
static int bytes32to64 = 0;
static int bytes64to128 = 0;
static int bytes128to256 = 0;
static int bytes256to512 = 0;
static int bytes512to1024 = 0;
static int bytes1024to2048 = 0;
static int bytes2048to4096 = 0;
static int bytesOver4096 = 0;

/*
  Allocations by age variables
*/
static int lessthan1sec = 0;
static int lessthan10sec = 0;
static int lessthan100sec = 0;
static int lessthan1000sec = 0;
static int morethan1000sec = 0;

/*
  Initializing all wrapper functions by getting its appropriate handle
*/
static void init()
{
  begin = clock();
  clockForPrint = clock();

  mallocWrapperFunc  = dlsym (RTLD_NEXT, "malloc");
  callocWrapperFunc  = dlsym (RTLD_NEXT, "calloc");
  reallocWrapperFunc = dlsym (RTLD_NEXT, "realloc");
  freeWrapperFunc    = dlsym (RTLD_NEXT, "free");

  if (!mallocWrapperFunc || !callocWrapperFunc || !reallocWrapperFunc || !freeWrapperFunc)
  {
      fprintf(stderr, "Error in DLSYM acquiring handle: %s\n", dlerror());
      exit(1);
  }
}

static void incrementSizeCount(size_t byteSize)
{
  totalAllocatedSize += byteSize;

  if(byteSize > 0 && byteSize <= 4) { bytes0to4++; }
  else if(byteSize > 4 && byteSize <= 8) { bytes4to8++; }
  else if(byteSize > 8  && byteSize <= 16) { bytes8to16++; }
  else if(byteSize > 16 && byteSize <= 32) { bytes16to32++; }
  else if(byteSize > 32 && byteSize <= 64) { bytes32to64++; }
  else if(byteSize > 64 && byteSize <= 128) { bytes64to128++; }
  else if(byteSize > 128 && byteSize <= 256) { bytes128to256++; }
  else if(byteSize > 256 && byteSize <= 512) { bytes256to512++; }
  else if(byteSize > 512 && byteSize <= 1024) { bytes512to1024++; }
  else if(byteSize > 1024 && byteSize <= 2048) { bytes1024to2048++; }
  else if(byteSize > 2048 && byteSize <= 4096) { bytes2048to4096++; }
  else if(byteSize > 4096) { bytesOver4096++; }

  ageAllocationCounter = ((double)(clock() - begin)/CLOCKS_PER_SEC);  
  if(timeToPrint < 1) { lessthan1sec++; }
  else if(timeToPrint < 10){ lessthan10sec++; }
  else if(timeToPrint < 100) { lessthan100sec++; }
  else if(timeToPrint < 1000) { lessthan1000sec++; }
  else { morethan1000sec++; }
}

static void decrementSizeCount(size_t byteSize)
{
  totalAllocatedSize -= byteSize;

  if(byteSize > 0 && byteSize <= 4) { bytes0to4--; }
  else if(byteSize > 4 && byteSize <= 8) { bytes4to8--; }
  else if(byteSize > 8  && byteSize <= 16) { bytes8to16--; }
  else if(byteSize > 16 && byteSize <= 32) { bytes16to32--; }
  else if(byteSize > 32 && byteSize <= 64) { bytes32to64--; }
  else if(byteSize > 64 && byteSize <= 128) { bytes64to128--; }
  else if(byteSize > 128 && byteSize <= 256) { bytes128to256--; }
  else if(byteSize > 256 && byteSize <= 512) { bytes256to512--; }
  else if(byteSize > 512 && byteSize <= 1024) { bytes512to1024--; }
  else if(byteSize > 1024 && byteSize <= 2048) { bytes1024to2048--; }
  else if(byteSize > 2048 && byteSize <= 4096) { bytes2048to4096--; }

  ageAllocationCounter = ((double)(clock() - begin)/CLOCKS_PER_SEC);  
  if(timeToPrint < 1) { lessthan1sec--; }
  else if(timeToPrint < 10){ lessthan10sec--; }
  else if(timeToPrint < 100) { lessthan100sec--; }
  else if(timeToPrint < 1000) { lessthan1000sec--; }
  else { morethan1000sec--; }
}

void printStatistics()
{
  time_t now;
  time(&now);
 
  fprintf(stderr, ">>>>>>>>>>>> %s <<<<<<<<<<<<<\n", ctime(&now));
  fprintf(stderr, "%u Overall allocations since start\n", overallAllocationsSinceStart);
  fprintf(stderr, "%0.1fMiB Current total allocated size \n \n", (totalAllocatedSize/pow(2,20)));
  fprintf(stderr, "Current Allocations by size:\n");
  fprintf(stderr, "0 - 4 bytes: %u\t \n"
  "4 - 8 bytes: %u\t \n"
  "8 - 16 bytes: %u\t \n"
  "16 - 32 bytes: %u\t \n"
  "32 - 64 bytes: %u\t \n"
  "64 - 128 bytes: %u\t \n"
  "128 - 256 bytes: %u\t \n"
  "256 - 512 bytes: %u\t \n"
  "512 - 1024 bytes: %u\t \n"
  "1024 - 2048 bytes: %u\t \n"
  "2048 - 4096 bytes: %u\t \n"
  "4096+ bytes: %u\t \n \n",
  bytes0to4,
  bytes4to8,
  bytes8to16,
  bytes16to32,
  bytes32to64,
  bytes64to128,
  bytes128to256,
  bytes256to512,
  bytes512to1024,
  bytes1024to2048,
  bytes2048to4096,
  bytesOver4096);

  fprintf(stderr, "Current Allocations by age:\n");
  fprintf(stderr, "<1 sec: %u\t \n"
  "<10 sec: %u\t \n"
  "<100 sec: %u\t \n"
  "<1000 sec: %u\t \n"
  ">1000 sec: %u\t \n \n",
  lessthan1sec, lessthan10sec, lessthan100sec, lessthan1000sec, morethan1000sec);
}

void *malloc(size_t size)
{
  void * result = NULL;
  static int isInitialized = 0;
  incrementSizeCount(size);

  // Initial call when malloc hasn't gotten malloc handle
  if(mallocWrapperFunc == NULL)
  {
    // Initializes all wrapper functions to get appropriate handles
    if(isInitialized == 0)
    {
       isInitialized = 1;
       init();
       isInitialized = 0;
    }
    else
    {
      if(size <sizeof(tmpbuff))
      {
        //Using tempbuffer and adding the requested size into the memory
        //which will be used to retrieve size in free
        result = tmpbuff + tmpbuffAllocatedSize;
        tmpbuffAllocatedSize += memoryPadding + size;
        *(size_t*)result = size;
        overallAllocationsSinceStart++;
        return (char*)result + memoryPadding;
      }
      else
      {
        fprintf(stderr, "Requesting too much memory!!\n");
      }
    }
  }
 
  result = (*mallocWrapperFunc)(size + memoryPadding);
  *(size_t*)result = size;
 
  timeToPrint = ((double)(clock() - begin)/CLOCKS_PER_SEC);  
  if(timeToPrint >= 5)
  {
    begin = clock();
    printStatistics();
  }

  overallAllocationsSinceStart++;
  return (char*)result + memoryPadding;
}

void *calloc(size_t nitems, size_t size)
{
  void *result = NULL;
  int newCallocByteSize = nitems*size;
 
  incrementSizeCount(size);

  result = malloc(newCallocByteSize);
  if(result != NULL)
  {
    memset(result, 0, newCallocByteSize);
  }

  timeToPrint = ((double)(clock() - begin)/CLOCKS_PER_SEC);  
  if(timeToPrint >= 5)
  {
    begin = clock();
    printStatistics();
  }
   
  overallAllocationsSinceStart++;
  return result;
}

void *realloc(void *ptr, size_t size)
{
  void *result = NULL;
  size_t sizeToBeFreed;

  if (reallocWrapperFunc == NULL)
  {
    result = malloc(size);
    if(result != NULL)
    {
      memcpy(result, ptr, size);
      free(ptr);
    }

    overallAllocationsSinceStart++;
    return result;
  }

  if (ptr == NULL)
  {
    return malloc(size);
  }

  if (size == 0)
  {
    free(ptr);
    return NULL;
  }

  ptr = (char*)ptr - memoryPadding;
  sizeToBeFreed = *(size_t*)ptr;

  decrementSizeCount(sizeToBeFreed);
  incrementSizeCount(size);

  result = (*reallocWrapperFunc)(ptr, memoryPadding + size);
  *(size_t*)result = size;

  timeToPrint = ((double)(clock() - begin)/CLOCKS_PER_SEC);  
  if(timeToPrint >= 5)
  {
    begin = clock();
    printStatistics();
  }

  overallAllocationsSinceStart++;
  return (char*)result + memoryPadding;
}

void free(void *ptr)
{
  size_t sizeToBeFreed;
 
  if (!ptr){ return; }

  timeToPrint = ((double)(clock() - begin)/CLOCKS_PER_SEC);  
  if(timeToPrint >= 5)
  {
    begin = clock();
    printStatistics();
  }

  ptr = (char*)ptr - memoryPadding;
  sizeToBeFreed = *(size_t*)ptr;

  overallAllocationsSinceStart--;
  decrementSizeCount(sizeToBeFreed);

  (*freeWrapperFunc)(ptr);
}