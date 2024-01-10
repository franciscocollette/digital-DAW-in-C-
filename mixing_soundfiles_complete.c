




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

  printf("Hello! This is a mixing program that can take any amount of audio files, adjust the gain, choose the start time of each one, and adjust the panning in the mono files, amazing innit?!\n");
  while (1) {
   printf("Now, how many audio files would you like to mix this time? ");
   scanf("%d", &NumberFiles);
   if (NumberFiles < 2) { printf("error, at least it should be 2 audio files, try again!\n");
     continue;
    }
   break;
  }

  //declaring and initializing variables 
  const char *FilesList[NumberFiles];
  float GainList[NumberFiles];
  SF_INFO sfinfoList[NumberFiles];
  SNDFILE *infileList[NumberFiles], *outfile;
  float silenceSecondsList[NumberFiles];
  int silenceSamplesList[NumberFiles];
  float panList[NumberFiles];
  float panLList[NumberFiles];
  float panRList[NumberFiles];

  // going trough all the files opening and setting gain, starting time, panning:
  for (int i = 0; i < NumberFiles; i++) {
   char filename[100]; // Assuming file names are at most 100 characters
   float gain;
   while(1) { 

    printf("Enter the name of audio file %d: ", i + 1);
    scanf("%s", filename);

    FilesList[i] = strdup(filename); // Use strdup to dynamically allocate memory for each file name
    if (!FilesList[i]) {
      printf("Error: Memory allocation failed.\n");
      continue;
    }
    //open input files: 
    infileList[i] = sf_open(FilesList[i], SFM_READ, &sfinfoList[i]);
    if(!infileList[i]) { 
        printf("Couldn't open input file %s, error: %s\n", FilesList[i], sf_strerror(NULL));
        for (int j = 0; j <= i; j++) {
          free((void *)FilesList[j]);
          sf_close(infileList[j]);
      }
      continue;
    }
    //checking if samplerate is ok
    if (sfinfoList[0].samplerate != sfinfoList[i].samplerate) {
      printf("\nThe sample rate doesnt match, the mix is not possible :( try again! \n");
      continue;
    }
    break;
   }
    // checking if the file is mono and setting the panning:
   if (sfinfoList[i].channels==1) { 
      while(1) { 
        printf("file %d is a mono file! Conversion to stereo will be done!\n", i+1);
        printf("Enter the panning please (0 is fully left, 1 is fully right): ");
        scanf("%f",&panList[i]);
        // I did an improved version of the panning we learned, the older version reduces the amplitude when in the middle because each sample is * 0.5 when it should be *1
        // I recommend this website as a tool to understand functions, graphs and to create functions (maybe for future lectures) https://www.desmos.com/calculator?lang=es
        if (panList[i] < 0 || panList[i] > 1 ) {
          printf("Error in panning, you should enter a float value between 0 and 1\n");
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
   // now entering the gain for each file:
    while(1) { 
      printf("Gain: ");
      scanf("%f", &gain);
      if (gain >1 || gain <0) { printf("the gain should be between 0 and 1! try again...");}
      else break;
    }
    GainList[i] = gain;         

    // entering the starting time and calculate the silenced samples:
    printf("Starting time in seconds: ");
    scanf("%f", &silenceSecondsList[i]);
    silenceSamplesList[i] = silenceSecondsList[i] * sfinfoList[i].samplerate * 2; // not sure why have to multiply it by 2, something with the stereo writing for sure

  }  
  


  // Now we have an array of const char * where each element points to a dynamically allocated file name, a list of gains, panning and starting times

  //printf("Sample rate of the input file 1: %d\n", sfinfoList[0].samplerate);
  //printf("Sample rate of the input file 2: %d\n", sfinfoList[1].samplerate);   //just debugging
 /*for (int i = 0; i < NumberFiles; i++) { 
      printf("\nName of the %d audio file: %s\n", i+1, FilesList[i]);
      printf("gain for the %d audio file: %f \n", i+1, GainList[i]);
  } */   //just debugging

//preparing the output file:
  char outputFileName[100];
  printf("\nEnter the name of the output file:");
  scanf("%s", outputFileName);
  SF_INFO sfinfoOut = {0}; // Initialize to 0 to avoid issues
  sfinfoOut = sfinfoList[0];
  sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  sfinfoOut.channels = 2; 
  //printf("name of the outputfile is:: %s \n",outputFileName );

  // Open the output file
  outfile = sf_open(outputFileName, SFM_WRITE, &sfinfoOut);
  if (!outfile) {
        printf("Couldn't open the output file, error: %s\n", sf_strerror(NULL));
            for (int i = 0; i < NumberFiles ; i++) {
            free((void *)FilesList[i]);
            sf_close(infileList[i]);
        return 1; }
  }


  // declaring and initializing more variables and lists for the reading and writing process...
  float bufferList[NumberFiles][BUFFER_SIZE];
  sf_count_t bytesReadList[NumberFiles];
  sf_count_t maxBytesRead = 0;
  float bufferMix[BUFFER_SIZE];
  float mixedSample;
  float bufferMonoList[NumberFiles][BUFFER_SIZE_2];
  int allBytesReadZero = 0; // Assume all bytes read are initially not zero

 

  while (1)
  {
    memset(bufferMix, 0, sizeof(float) * BUFFER_SIZE); // clear the bufferMix each iteration, very importante, otherwise it acumulates all samles 

    for (int i =0; i<NumberFiles; i++) {    
        if (silenceSamplesList[i] <= 0 ) {     // if there is any silence in [i] we dont read the file [i]
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
      else { 
        bytesReadList[i] = BUFFER_SIZE;    // if there is any silence the size of the bytes read are same as buffer 
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

    //populate the bufferMix 
    for (int i = 0; i < NumberFiles; i++) { 
        for (sf_count_t j = 0; j < maxBytesRead; j++ ) { 
          if (silenceSamplesList[i] <= 0 ) {
           bufferMix[j] += bufferList[i][j] * GainList[i];   // if no silence we 
           }
           else { 
           bufferMix[j] += 0;
           silenceSamplesList[i] -= 1;  
          }
        }
    }
    //write the output file 
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