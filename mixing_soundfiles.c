#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#define BUFFER_SIZE 1024
#define BUFFER_SIZE_2 512
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
// just because aways i make typo mistakes, to build the program: cc -o "progname" "codename.c"-lsndfile
// try if set the chanels of the output file to 6, I could do a surround 5.1 sound file?

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: %s <input_file1> <gain1> <input_file2> <gain2> <output_file> \n", argv[0]);
        return 1;
    }

    const char *inputFileName1 = argv[1];
    float gain = atof(argv[2]); // Convert the gain parameter to a float
    const char *inputFileName2 = argv[3];
    float gain2 = atof(argv[4]);
    const char *outputFileName = argv[5];

    SF_INFO sfinfo1, sfinfo2, sfinfoOut;
    SNDFILE *infile1, *infile2, *outfile;

    // Open the input file 1
    infile1 = sf_open(inputFileName1, SFM_READ, &sfinfo1);
    if (!infile1)
    {
        printf("Couldn't open the input file, error: %s\n", sf_strerror(NULL));
        return 1;
    }

    infile2 = sf_open(inputFileName2, SFM_READ, &sfinfo2);
    if (!infile2)
    {
        printf("Couldnt open the input file 2, error: %s\n", sf_strerror(NULL));
        return 1;
    }

    // Configure the output file parameters based on the input file
    sfinfoOut = sfinfo1;
    sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfoOut.channels = 2;

    printf("Sample rate of the input file 1: %d\n", sfinfo1.samplerate);
    printf("Sample rate of the input file 2: %d\n", sfinfo2.samplerate);

    if (sfinfo1.samplerate != sfinfo2.samplerate)
    {
        printf("The sample rate doesnt match! the mix is not possible :( \n");
        return 1;
    }

    // sfinfoOut.samplerate = 22050; //  just playing here

    // Open the output file
    outfile = sf_open(outputFileName, SFM_WRITE, &sfinfoOut);
    if (!outfile)
    {
        printf("Couldn't open the output file, error: %s\n", sf_strerror(NULL));
        sf_close(infile1);
        sf_close(infile2);
        return 1;
    }

    printf("Sample rate of the output file: %d\n", sfinfoOut.samplerate);

    float bufferMono1[BUFFER_SIZE_2]; // for the mono files
    float bufferMono2[BUFFER_SIZE_2];
    float buffer1[BUFFER_SIZE];
    float buffer2[BUFFER_SIZE];
    float bufferMix[BUFFER_SIZE];
    sf_count_t bytesRead1, bytesRead2, maxBytesRead;
    float mixedSample;

    /*
     while ((bytesRead1 = sf_read_float(infile1, buffer1, BUFFER_SIZE)) > 0 &&
               (bytesRead2 = sf_read_float(infile2, buffer2, BUFFER_SIZE)) > 0) {

            // Determine the minimum number of samples to process
            maxBytesRead = MAX(bytesRead1, bytesRead2);

            for (sf_count_t i = 0; i < maxBytesRead; i++) {
                bufferMix[i] = buffer1[i] * gain + buffer2[i] * gain;
            }

            // Write the mixed samples to the output file
            sf_write_float(outfile, bufferMix, maxBytesRead);

        } */
    // this is working fine aswell but the audio finishes whit the shortest file

    /*    while (1)
         {
             bytesRead1 = sf_read_float(infile1, buffer1, BUFFER_SIZE);
             bytesRead2 = sf_read_float(infile2, buffer2, BUFFER_SIZE);
             maxBytesRead = MAX(bytesRead1, bytesRead2);

             if (bytesRead1 <= 0 && bytesRead2 <= 0)
             {
                 break;
             }


             for (sf_count_t i = 0; i < maxBytesRead; i++)
             {
                 bufferMix[i] = buffer1[i] * gain + buffer2[i] * gain2;
             }
             sf_write_float(outfile, bufferMix, maxBytesRead);
         }
         */

    float pan1, panL1, panR1;
    float pan2, panL2, panR2;

    if (sfinfo1.channels == 1)
    {
        while (1)
        {
            printf("\nfile 1 is mono! Conversion to stereo will be done\n");
            printf("Enter the panning please (0 is fully left, 1 is fully right): ");
            scanf("%f", &pan1);
            // I did an improved version of the panning we learned, the older version reduces the amplitude when in the middle because each sample is * 0.5 when it should be *1
            // I recommend this website as a tool to understand functions, graphs and to create functions (maybe for future lectures) https://www.desmos.com/calculator?lang=es
            if (pan1 < 0 || pan1 > 1)
            {
                printf("Error panning, you should enter a float value between 0 and 1\n");
            }
            else if (pan1 <= 0.5)
            {
                panL1 = 1;
                panR1 = 2 * pan1;
                break;
            }
            else if (pan1 > 0.5)
            {
                panL1 = 2 * (1 - pan1);
                panR1 = 1;
                break;
            }
        }
    }

    if (sfinfo2.channels == 1)
    {
        while (1)
        {
            printf("\nfile 2 is mono! conversion to stereo will be done\n");
            printf("Enter the panning please (0 is fully left, 1 is fully right): ");
            scanf("%f", &pan2);
            if (pan2 < 0 || pan2 > 1)
            {
                printf("Error panning, you should enter a float value between 0 and 1\n");
            }
            else if (pan2 <= 0.5)
            {
                panL2 = 1;
                panR2 = 2 * pan2;
                break;
            }
            else if (pan2 > 0.5)
            {
                panL2 = 2 * (1 - pan2);
                panR2 = 1;
                break;
            }
        }
    }

    while (1)
    {
        if (sfinfo1.channels == 1)
        {
            bytesRead1 = sf_read_float(infile1, bufferMono1, BUFFER_SIZE_2);

            for (sf_count_t i = 0; i < bytesRead1; i++) // this fills the buffer1 with two same samples from the original buffer
            {
                buffer1[2 * i] = bufferMono1[i] * panL1;
                buffer1[2 * i + 1] = bufferMono1[i] * panR1;
            }
        }
        else
        {
            bytesRead1 = sf_read_float(infile1, buffer1, BUFFER_SIZE);
        }

                if (sfinfo2.channels == 1)
        {
            bytesRead2 = sf_read_float(infile2, bufferMono2, BUFFER_SIZE_2);

            for (sf_count_t i = 0; i < bytesRead2; i++) // this fills the buffer1 with two same samples from the original buffer
            {
                buffer2[2 * i] = bufferMono2[i] * panL2;
                buffer2[2 * i + 1] = bufferMono2[i] * panR2;
            }
        }
        else
        {
            bytesRead2 = sf_read_float(infile2, buffer2, BUFFER_SIZE);
        }

        maxBytesRead = MAX(bytesRead1, bytesRead2);

        if (bytesRead1 <= 0 && bytesRead2 <= 0)     // this breaks the while when no more samples to read
        {
            break;
        }

        for (sf_count_t i = 0; i < maxBytesRead; i++)    
        {
            bufferMix[i] = buffer1[i] * gain + buffer2[i] * gain2;    //and here we populate the final buffer
        }
        sf_write_float(outfile, bufferMix, maxBytesRead);      //and write the final buffer to the output file
    }

    // Clean up and close the files
    sf_close(infile1);
    sf_close(infile2);
    sf_close(outfile);

    printf("\nFiles successfully mixed and gain adjusted to %s!\n", outputFileName);

    return 0;
}
