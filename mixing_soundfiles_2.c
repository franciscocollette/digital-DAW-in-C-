




// just because aways i make typo mistakes, to build the program: cc -o "progname" "codename.c"-lsndfile

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>

#define BUFFER_SIZE 1024
#define BUFFER_SIZE_2 512

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main() {   
  int NumberFiles;

  printf("Hello! This is a mixing program that can take any amount of audio files, adjust the gain of each one, and adjust the panning in the mono files!\n");
  printf("Now, how many audio files would you like to mix this time? ");
  scanf("%d", &NumberFiles);
  if (NumberFiles < 2) { printf("error, at least it should be 2 audio files");
  return 1;}

  const char *FilesList[NumberFiles];
  float GainList[NumberFiles];
  SF_INFO sfinfoList[NumberFiles];
  SNDFILE *infileList[NumberFiles], *outfile;

  for (int i = 0; i < NumberFiles; i++) {
    char filename[100]; // Assuming file names are at most 100 characters
    float gain;
    printf("Enter the name of audio file %d: ", i + 1);
    scanf("%s", filename);

    while(1) { 
      printf("Gain: ");
      scanf("%f", &gain);
      if (gain >1 || gain <0) { printf("the gain should be between 0 and 1! try again...");}
        else break;
    }
    FilesList[i] = strdup(filename); // Use strdup to dynamically allocate memory for each file name
    if (!FilesList[i]) {
      printf("Error: Memory allocation failed.\n");
      // Handle error, free memory, close files, and return
      }
    GainList[i] = gain;         


    //open input files: 
    infileList[i] = sf_open(FilesList[i], SFM_READ, &sfinfoList[i]);
    if(!infileList[i]) { 
        printf("Couldn't open input file %s, error: %s\n", FilesList[i], sf_strerror(NULL));
        for (int j = 0; j < i; j++) {
          free((void *)FilesList[j]);
          sf_close(infileList[j]);
          return 1;
      }
    }
    
  }

    for (int i =0; i < NumberFiles; i++ ) { 
     if (sfinfoList[0].samplerate != sfinfoList[i].samplerate) {
      printf("\nThe sample rate doesnt match, the mix is not possible :( \n");
        return 1;
     }
        
    }
  


  // Now you have an array of const char * where each element points to a dynamically allocated file name. and a list of gains

  printf("Sample rate of the input file 1: %d\n", sfinfoList[0].samplerate);
  printf("Sample rate of the input file 2: %d\n", sfinfoList[1].samplerate);

  for (int i = 0; i < NumberFiles; i++) { 
      printf("\nName of the %d audio file: %s\n", i+1, FilesList[i]);
      printf("gain for the %d audio file: %f \n", i+1, GainList[i]);
  }

  char outputFileName[100];
  printf("\nEnter the name of the output file:");
  scanf("%s", outputFileName);
  SF_INFO sfinfoOut = {0}; // Initialize to 0 to avoid issues
  sfinfoOut = sfinfoList[0];
  sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  sfinfoOut.channels = 2; 
  printf("name of the outputfile is:: %s \n",outputFileName );




  // Open the output file
  outfile = sf_open(outputFileName, SFM_WRITE, &sfinfoOut);
  if (!outfile) {
        printf("Couldn't open the output file, error: %s\n", sf_strerror(NULL));
            for (int i = 0; i < NumberFiles ; i++) {
            free((void *)FilesList[i]);
            sf_close(infileList[i]);
        return 1; }
  }


    
  float bufferList[NumberFiles][BUFFER_SIZE];
  sf_count_t bytesReadList[NumberFiles];
  sf_count_t maxBytesRead = 0;
  float bufferMix[BUFFER_SIZE];
  float mixedSample;

  float bufferMonoList[NumberFiles][BUFFER_SIZE_2];
  float panList[NumberFiles];
  float panLList[NumberFiles];
  float panRList[NumberFiles];
  int allBytesReadZero = 0; // Assume all bytes read are initially not zero


  for (int i =0; i<NumberFiles; i++) { 
    if (sfinfoList[i].channels==1) { 
      while(1) { 
        printf("file %d is a mono file! Conversion to stereo will be done!\n", i+1);
        printf("Enter the panning please (0 is fully left, 1 is fully right): ");
        scanf("%f",&panList[i]);
        // I did an improved version of the panning we learned, the older version reduces the amplitude when in the middle because each sample is * 0.5 when it should be *1
        // I recommend this website as a tool to understand functions, graphs and to create functions (maybe for future lectures) https://www.desmos.com/calculator?lang=es
        if (panList[i] < 0 || panList[i] > 1 ) {
          printf("Error panning, you should enter a float value between 0 and 1\n");
        }
        else if (panList[i]<=0.5) { 
          panLList[i] = 1;
          panRList[i] = 2 * panList[i];
          break;
        }
        else if (panList[i] > 0.5) {
          panLList[i] = 2 * (1 - panList[i]);
          panRList[i] = 1;
          break;
        }
      }
    }
  }

   while (1)
    {
      memset(bufferMix, 0, sizeof(float) * BUFFER_SIZE); // clear the bufferMix each iteration, very importante 

      for (int i =0; i<NumberFiles; i++) {
         if (sfinfoList[i].channels==1) { 
          bytesReadList[i] = sf_read_float(infileList[i], bufferMonoList[i], BUFFER_SIZE_2);
          for (sf_count_t j = 0; j < bytesReadList[i]; j++) {   // this fills the buffer1 with two same samples from the original buffer
            bufferList[i][2*j] = bufferMonoList[i][j] * panLList[i];
            bufferList[i][2 * j + 1] = bufferMonoList[i][j] * panRList[i];
          }
         }
         else {
          bytesReadList[i] = sf_read_float(infileList[i], bufferList[i], BUFFER_SIZE);
         }
      }

      for (int i =0; i<NumberFiles; i++) { 
       maxBytesRead = MAX(maxBytesRead, bytesReadList[i]);
      }

       // this breaks the while when no more samples to read
     for (int i = 0; i < NumberFiles; i++) { 
      if (bytesReadList[i] > 0) {
        allBytesReadZero = 0;    // If at least one file has bytes read greater than 0, set the flag to 0
        break; // No need to check further
        }
        else {
          allBytesReadZero = 1;
        }
      }
      if (allBytesReadZero == 1) { 
        break;
      }



      for (int i = 0; i < NumberFiles; i++) { 
        for (sf_count_t j = 0; j < maxBytesRead; j++ ) { 
          bufferMix[j] += bufferList[i][j] * GainList[i];
        }
      }

      sf_write_float(outfile, bufferMix, maxBytesRead);

    }

  printf("\nFiles successfully mixed and gain adjusted to %s!\n", outputFileName);

  // Don't forget to free the allocated memory when you're done with it.
  for (int i = 0; i < NumberFiles; i++) {
     sf_close(infileList[i]);
     free((void *)FilesList[i]);
    }
  sf_close(outfile);

  return 0;
}