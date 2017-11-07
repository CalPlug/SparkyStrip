#include <stdio.h>
#include <math.h>
#include <conio.h>
#include <windows.h>
using namespace std; 


typedef struct Complex_tag {float real,imag;} COMPLEX;
typedef struct Special_tag {float value; int position;} SPECIAL;
typedef struct Threshold_tag {float min; float max;} THRESHOLD;

// Sampling Frequency, ours is set at 5000 samples per second
#define SAMPLING_FREQ 5000
#define WAVEFORM_FREQ 60

#define BUFFER_LENGTH 512
#define PI 3.14159265358979

// Channels are the number of channels being analyzed
#define NUMBER_OF_CHANNELS 2
// Number of parameters are the number of parameters that are being analyzed
#define NUMBER_OF_PARAMETERS 5
// Number of devices is the number of recognizable devices stored
#define NUMBER_OF_DEVICES 4

// MatLab Input File pointer
FILE *fp[NUMBER_OF_CHANNELS];
FILE *ofp;
int colors[NUMBER_OF_CHANNELS];

// Number of Samples per Period
int period_samples = SAMPLING_FREQ/WAVEFORM_FREQ;

// buffers and input buffers
float input[BUFFER_LENGTH][NUMBER_OF_CHANNELS];
COMPLEX complex_buffer[BUFFER_LENGTH][NUMBER_OF_CHANNELS];

// Used to Scan the input from file into
char line[80];

// Counters
int counter = 0;
int CH_counter = 0;
int i = 0;
int j = 0;

// booleans
int booleans[NUMBER_OF_CHANNELS];

// Used to analyze each of the inputs
int rise_times[NUMBER_OF_CHANNELS];
int fall_times[NUMBER_OF_CHANNELS];
float max_value[NUMBER_OF_CHANNELS];
float normalized_FFT_180[NUMBER_OF_CHANNELS];
float normalized_FFT_300[NUMBER_OF_CHANNELS];
float power_factor[NUMBER_OF_CHANNELS];

// Threshold values
float FFT180Thres = 0.5;
float FFT300Thres = 0.5;
float MaxVoltThres = 0.4;
int RiseSamplesThres = 5;
int PhaseShiftThres;

// Set Threshold Values
THRESHOLD interval[NUMBER_OF_DEVICES][NUMBER_OF_PARAMETERS];

// Maximum values
SPECIAL max_value_2nd_wave[NUMBER_OF_CHANNELS];
float max_dft_value[NUMBER_OF_CHANNELS];

//for debugging
int sample_no = 0;
float dft_value = 0.0;

//Initallizes all buffers, input, and other arrays
void initialize()
{
	//Initializes all the values in the input and buffer arrays
	for(i=0; i<BUFFER_LENGTH; i++)
	{
		for(j=0; j<NUMBER_OF_CHANNELS; j++)
		{
			complex_buffer[i][j].real = 0.0;
			complex_buffer[i][j].imag = 0.0;
			input[i][j] = 0.0;
		}
	}
	
	//Initallizes all the analysis values, used for determining the waveforms, to 0.0
	for(i=0; i<NUMBER_OF_CHANNELS; i++)
	{
		colors[i] = 0;
		max_value[i] = 0.0;
		power_factor[i] = 0.0;
		normalized_FFT_180[i] = 0.0;
		normalized_FFT_300[i] = 0.0;
		rise_times[i] = 0;
		fall_times[i] = 0;
		booleans[i] = 0;
		max_value_2nd_wave[i].value = 0.0;
		max_value_2nd_wave[i].position = 0;
		max_dft_value[i];
	}
}

// Finds the Rise times of each.
void rise_fall_times(int CH)
{
	rise_times[CH] = 0;
	fall_times[CH] = 0;
	booleans[CH] = 1;
	counter = max_value_2nd_wave[CH].position;
	//Find rise time algorythm
	while(booleans[CH] == 1)
	{
		if( input[counter][CH] < 0.9 * max_value_2nd_wave[CH].value)
		{
			booleans[CH] = 0;
		}
		else
		{
			counter--;
		}
	}
	booleans[CH] = 1;
	while(booleans[CH] == 1)
	{
		if( input[counter][CH] < 0.1 * max_value_2nd_wave[CH].value)
		{
			booleans[CH] = 0;
		}
		counter--;
		rise_times[CH]++;
	}
	
	counter = max_value_2nd_wave[CH].position;
	//Find fall time algorythm
	while(booleans[CH] == 1)
	{
		if( input[counter][CH] < 0.9 * max_value_2nd_wave[CH].value)
		{
			booleans[CH] = 0;
		}
		else
		{
			counter++;
		}
	}
	booleans[CH] = 1;
	while(booleans[CH] == 1)
	{
		if( input[counter][CH] < 0.1 * max_value_2nd_wave[CH].value)
		{
			booleans[CH] = 0;
		}
		counter++;
		fall_times[CH]++;
	}

}

void dft(int CH)
{	
// Initializes the initial maximum dft value
	max_dft_value[CH] = 0.0;
	
	// Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
	for(i=0;i<BUFFER_LENGTH;i++)
	{
		// Initializes the complex buffer
		complex_buffer[i][CH].real = 0.0;
		complex_buffer[i][CH].imag = 0.0;
		for(j=0;j<BUFFER_LENGTH;j++)
		{
			complex_buffer[i][CH].real += input[j][CH]*((float) cos(2*PI*i*j/BUFFER_LENGTH));
			complex_buffer[i][CH].imag -= input[j][CH]*((float) sin(2*PI*i*j/BUFFER_LENGTH));
		}
		// Records the Highest DFT value
		if(max_dft_value[CH] < sqrt(complex_buffer[i][CH].real*complex_buffer[i][CH].real + complex_buffer[i][CH].imag*complex_buffer[i][CH].imag))
		{
			max_dft_value[CH] = sqrt(complex_buffer[i][CH].real*complex_buffer[i][CH].real + complex_buffer[i][CH].imag*complex_buffer[i][CH].imag);
		}	
	}
		
	// Records the Normalized Harmonic values of each Channel at 180 and 300 Hz
	normalized_FFT_180[CH] = sqrt(complex_buffer[18][CH].real*complex_buffer[18][CH].real + complex_buffer[18][CH].imag*complex_buffer[18][CH].imag)/max_dft_value[CH];
	normalized_FFT_300[CH] = sqrt(complex_buffer[31][CH].real*complex_buffer[31][CH].real + complex_buffer[31][CH].imag*complex_buffer[31][CH].imag)/max_dft_value[CH];
	
}

// Gets Phase Shift at the Channel
void get_phase(int CH)
{
	power_factor[CH] = 2 * PI *(max_value_2nd_wave[CH].position - max_value_2nd_wave[0].position )/ period_samples;
	power_factor[CH] = fabsf(cos(power_factor[CH]));

}

// Reads input file from Matlab
void read_input(int CH)
{
	// Channel 0 is assumed to be set to the reference voltage (the AC power source)
	// Algorythm uses this Reference Voltage to determine the phase of different inputs
	if(CH == 0)
		fp[CH] = fopen("Input0.txt", "r");
	else if(CH == 1)
		fp[CH] = fopen("Input1.txt", "r");
	else if(CH == 2)
		fp[CH] = fopen("Input2.txt", "r");
	
	counter = 0;
	
	
	/* get a line, up to 80 chars from fr.  done if NULL */
	while( (fgets(line, 80, fp[CH]) != NULL) && (counter<BUFFER_LENGTH) )
	{
	/* get a line, up to 80 chars from fr.  done if NULL */
		sscanf (line, "%f", &input[counter][CH]);
		if(input[counter][CH] > max_value[CH])
		{
		// Records the maximum value
			max_value[CH] = input[counter][CH];
		}	
		// Records the maximum value within the second period	
		if((counter > period_samples) && (counter < 2*period_samples))
		{
			if(input[counter][CH] > max_value_2nd_wave[CH].value)
			{
				max_value_2nd_wave[CH].value = input[counter][CH];
				max_value_2nd_wave[CH].position = counter;
			}
		}
		counter++;
		
	}
	// close file
	if(CH == 0)
		fclose(fp[CH]);
	else if(CH == 1)
		fclose(fp[CH]);
	else if(CH == 2)
		fclose(fp[CH]);
	
	dft(CH);
	rise_fall_times(CH);
	get_phase(CH);
	
}

// Returns the waveform at the channel
void determine_waveform(int CH)
{	
	// booleans returns 0 if it doesnt find the WaveForm type, and 1 if it does
	booleans[CH] = 0;
	
	if(CH == 0)
	{
		printf("\nChannel #%d is the Reference Probe\n\n", CH);
		booleans[CH] = 1;
	}
	else
	{
		for(i = 0; i <NUMBER_OF_DEVICES; i++)
		{
			if((max_value[CH] > interval[i][0].min) && (max_value[CH] < interval[i][0].max))
			{
				if((normalized_FFT_180[CH] > interval[i][1].min) && (normalized_FFT_180[CH] < interval[i][1].max))
				{
					if((normalized_FFT_300[CH] > interval[i][2].min) && (normalized_FFT_300[CH] < interval[i][2].max))
					{
						if((rise_times[CH] > interval[i][3].min) && (rise_times[CH] < interval[i][3].max))
						{
							if((power_factor[CH] > interval[i][4].min) && (power_factor[CH] < interval[i][4].max))
							{
								booleans[CH] = 1;
								if(i == 0)
								{
									printf("\nChannel #%d is an Fan\n\n", CH);
									colors[CH] = 0;
									ofp = fopen("Output.txt", "w");
									fprintf(ofp, "0");
								}
								if(i == 1)
								{
									printf("\nChannel #%d is a TV\n\n", CH);
									colors[CH] = 1;
									ofp = fopen("Output.txt", "w");
									fprintf(ofp, "1");
								}
								if(i == 2)
								{
									printf("\nChannel #%d is a Set-Top-Box\n\n", CH);
									colors[CH] = 2;
									ofp = fopen("Output.txt", "w");
									fprintf(ofp, "2");
								}
								if(i == 3)
								{
									printf("\nChannel #%d is a HP Laptop\n\n", CH);
									colors[CH] = 3;
									ofp = fopen("Output.txt", "w");
									fprintf(ofp, "3");
								}
							}
						}
					}
				}
			}
		}
	}
	
	if( booleans[CH] == 0)
	{
		printf("\nUnrecognized signal in Channel #%d\n\n", CH);
		colors[CH] = 4;
		ofp = fopen("Output.txt", "w");
		fprintf(ofp, "4");
	}
	// For Debugging purposes
	printf("The Max Reading is %f\nThe Normalized FFT at 180 Hz is %f\nThe Normalized FFT at 300 Hz is %f\n", max_value[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
	printf("The number of rising samples is %d\n", rise_times[CH]);
	printf("The number of falling samples is %d\n", fall_times[CH]);
//	printf("The max at 2nd waveform is at %d\n", max_value_2nd_wave[CH].position);
//	printf("The Phase is %f\n", 2 * PI *(max_value_2nd_wave[CH].position - max_value_2nd_wave[0].position )/ period_samples);
//	printf("The Power Factor is %f\n", power_factor[CH]);
	
}

// Initalizes all the stored parameters in the array, intervals.  Stores the max and mins of each waveform for each parameter.
void set_parameters()
{
		// Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop
		// Parameters are as follows: 0 = peak value, 1 = Normalized FFT at 180, 2 = Normalized FFT at 300, 3 = Rise Time (in number of samples), 4 = Power Factor
		interval[0][0].min = 0.6;
		interval[0][0].max = 0.8;
		interval[0][1].min = 0.0;
		interval[0][1].max = 0.3;
		interval[0][2].min = 0.0;
		interval[0][2].max = 0.1;
		interval[0][3].min = 10.0;
		interval[0][3].max = 15.0;
		interval[0][4].min = 0.0;
		interval[0][4].max = 1.0;
		
		interval[1][0].min = 0.2;
		interval[1][0].max = 0.35;
		interval[1][1].min = 0.08;
		interval[1][1].max = 0.2;
		interval[1][2].min = 0.08;
		interval[1][2].max = 0.2;
		interval[1][3].min = 10.0;
		interval[1][3].max = 15.0;
		interval[1][4].min = 0.0;
		interval[1][4].max = 1.0;
		
		interval[2][0].min = 0.1;
		interval[2][0].max = 0.22;
		interval[2][1].min = 0.6;
		interval[2][1].max = 0.7;
		interval[2][2].min = 0.5;
		interval[2][2].max = 0.6;
		interval[2][3].min = 0.0;
		interval[2][3].max = 5.0;
		interval[2][4].min = 0.0;
		interval[2][4].max = 1.0;
		
		interval[3][0].min = 0.4;
		interval[3][0].max = 1.0;
		interval[3][1].min = 0.58;
		interval[3][1].max = 0.7;
		interval[3][2].min = 0.54;
		interval[3][2].max = 0.68;
		interval[3][3].min = 0.0;
		interval[3][3].max = 5.0;
		interval[3][4].min = 0.0;
		interval[3][4].max = 1.0;
		
}

void windowColors(int device1) {
     
    HANDLE h;
    h=GetStdHandle(STD_OUTPUT_HANDLE);
    COORD start1={0,0};
    COORD t;
    COORD end1 = {40, 40};
	switch( device1 ) 
	{
		case 0:
			SetConsoleTextAttribute(h,BACKGROUND_RED); // Fan
			break;
		case 1:
			SetConsoleTextAttribute(h,BACKGROUND_BLUE); // TV
			break;
		case 2:
			SetConsoleTextAttribute(h,BACKGROUND_GREEN); // STB
			break;
		case 3:
			SetConsoleTextAttribute(h, BACKGROUND_INTENSITY); // HP
			break;
		case 4:
			SetConsoleTextAttribute(h, 0); // unidentified
			break;
	}
     
    for(int i=start1.X;i<end1.X;++i)
        for(int j=start1.Y;j<end1.Y;++j)
        {
            t.X=i;
            t.Y=j;
            SetConsoleCursorPosition(h,t);
            printf(" ");
        }

/*	COORD start2={40,0};
    COORD end2 = {79, 40};

	switch( device2 ) 
	{
		case 0:
			SetConsoleTextAttribute(h,BACKGROUND_RED);
			break;
		case 1:
			SetConsoleTextAttribute(h,BACKGROUND_BLUE);
			break;
		case 2:
			SetConsoleTextAttribute(h,BACKGROUND_GREEN);
			break;
		case 3:
			SetConsoleTextAttribute(h, BACKGROUND_INTENSITY);
			break;
		case 4:
			SetConsoleTextAttribute(h, 0);
			break;
	}
	for(int i=start2.X;i<end2.X;++i)
        for(int j=start2.Y;j<end2.Y;++j)
        {
            t.X=i;
            t.Y=j;
            SetConsoleCursorPosition(h,t);
            printf(" ");
        }
		*/
	getch();
}

void main()
{
	//Initializes values to be used
	initialize();
	set_parameters();
	//a loop is needed in the main code when it is run in real time
	for(CH_counter = 0; CH_counter<NUMBER_OF_CHANNELS; CH_counter++)
	{
		read_input(CH_counter);
		determine_waveform(CH_counter);
	}
	getch();
//	windowColors(colors[1]);

}