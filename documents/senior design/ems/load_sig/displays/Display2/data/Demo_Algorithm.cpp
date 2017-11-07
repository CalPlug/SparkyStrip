/* EMS Team: Demo Algorythm 											*/
/* Purpose: To determine what waveform the appliance is, if any, based  */
/* 		on the current waveform database.								*/
/* Instructions: Use the "InputWaveformChangeToTXT.m" to set the		*/
/* 		'Input1.txt' to the one that you want.  Then compile this		*/
/* 		C program, and run it.  It will then both print out the 		*/
/* 		waveform identity, and output another text file. The output 	*/
/*      text file was used to display on java which file it was via 	*/
/* 		a program called "Processing" but for our intents and purposes	*/
/*		do not worry about this program. 								*/

/* 	List of Libraries that Code uses */
#include <stdio.h>
#include <math.h>
#include <conio.h>
#include <windows.h>

// 	It's C++ code used to output the Results to a text file.
using namespace std; 
// slkjfweofhwogw

// 	Typedef: Used to make the code simpler.  For example, a variable
// 		of type COMPLEX would have two parts: a real and imaginary,
//		both are of type "float".
//
// 	EX: Declaration of "COMPLEX" type.
//		COMPLEX example;
// 	EX: The real and imaginary parts are being initialized.
//		example.real = 0;
//		example.imag = 0;

typedef struct Complex_tag {float real,imag;} COMPLEX;
typedef struct Special_tag {float value; int position;} SPECIAL;
typedef struct Threshold_tag {float min; float max;} THRESHOLD;

/* 																	*/
/* A Compiled List of Definitions for Terms.  Used For Simplicity 	*/
/* EX: PI would equal 3.14159......, BUFFER_LENGTH would equal 512	*/
/* 		i = PI would set i = 3.14159..								*/

// Sampling Frequency, ours is set at 5000 samples per second  In other words, the sampling
// 		Frequency is 5kHz.  Waveform frequency is the frequency of the appliances, all of them
//  	are roughly 60 Hz.
#define SAMPLING_FREQ 5000
#define WAVEFORM_FREQ 60

// Buffer Length: used to set the length of the input array, that holds the
// 		sampled waveform.
#define BUFFER_LENGTH 512

// PI: value of PI.  Self-explanatory
#define PI 3.14159265358979

// Channels are the number of channels being analyzed
#define NUMBER_OF_CHANNELS 1

// Number of parameters are the number of parameters that are being analyzed. Parameters are FFT at 180, rise, etc.
#define NUMBER_OF_PARAMETERS 5

// Number of devices is the number of recognizable devices stored.  Examples are the Vornado Fan, Sharp Tv, etc.
#define NUMBER_OF_DEVICES 13

// MatLab Input File pointer,  Used to read the file, from MatLab.
FILE *fp[NUMBER_OF_CHANNELS];
FILE *ofp;

// Number of Samples per Period.  Given a 5kHz wave, and a 60Hz signal, it should roughly be 83.3 samples per period
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

// Booleans is a variable corresponding to each waveform.  If Channel 1 is a fan, HP-Laptop, or
// any other device, stored in the database, then booleans[1] is true (or 1).  Otherwise, we want it
// to be false (or 0).
int booleans[NUMBER_OF_CHANNELS];

// Used to analyze each of the inputs, corresponding to each of the "input channels".  There should be only one input channel.
int rise_times[NUMBER_OF_CHANNELS];
int fall_times[NUMBER_OF_CHANNELS];
float rise_times_calculated[NUMBER_OF_CHANNELS];
float max_value[NUMBER_OF_CHANNELS];
float normalized_FFT_180[NUMBER_OF_CHANNELS];
float normalized_FFT_300[NUMBER_OF_CHANNELS];
float power_factor[NUMBER_OF_CHANNELS];


// Set Threshold Values.  Each device has a corresponding number  (ex: 0 corresponds to a fan.)
//		Each parameter also corresponds to a waveform attribute (ex: 1 corresponds to Normalized FFT at 180. )
// 		Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop, 4 = One Fan, 5 = Two Fans, 6 = Three Fans
//		Parameters are as follows: 0 = peak value, 1 = Normalized FFT at 180, 2 = Normalized FFT at 300, 3 = Rise Time (in number of samples), 4 = Power Factor
// 		
// 		For example, interval[0][3].min corresponds to a Fan's minimum rise time value,
//			while interval[0][3].max would correspond to its maximum.
THRESHOLD interval[NUMBER_OF_DEVICES][NUMBER_OF_PARAMETERS];

// Maximum values.  The Maxes are used in finding the rise and fall time, where it is used as a reference to the highest point,
// 		in the second "period", and used to find the difference between 10% and 90%.
// 	The Max dft value is used to normalize the computed Harmonics of the waveform, so the magnitude is 1 at its maximum.
SPECIAL max_value_2nd_wave[NUMBER_OF_CHANNELS];
float max_dft_value[NUMBER_OF_CHANNELS];


// 	Initallizes all buffers, input, and other arrays.  Initialization is used to ensure that
//		there are no undefined variables used.
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
		rise_times_calculated[i] = 0.0;
	}
}

// Finds the number of rising samples (given 5000 kHz) and falling samples of each waveform
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
	
	// Rise-Time = number of samples*1/frequency
	rise_times_calculated[CH] = ((float) rise_times[CH])*1/SAMPLING_FREQ;
	
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


// Computes the DFT of the input waveform.
void dft(int CH)
{	
// Debug
	float fft_value = 0.0;
	
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
	
// Debug
	for(i=0;i<100;i++)
	{
		fft_value = sqrt(complex_buffer[i][CH].real*complex_buffer[i][CH].real + complex_buffer[i][CH].imag*complex_buffer[i][CH].imag)/max_dft_value[CH];
		//printf("FFT %d is %f\n", i, fft_value);
	}
		
	// Records the Normalized Harmonic values of each Channel at 180 and 300 Hz
	normalized_FFT_180[CH] = sqrt(complex_buffer[18][CH].real*complex_buffer[18][CH].real + complex_buffer[18][CH].imag*complex_buffer[18][CH].imag)/max_dft_value[CH];
	normalized_FFT_300[CH] = sqrt(complex_buffer[31][CH].real*complex_buffer[31][CH].real + complex_buffer[31][CH].imag*complex_buffer[31][CH].imag)/max_dft_value[CH];
	
}

// Gets Phase Shift at the Channel
/*
void get_phase(int CH)
{
	power_factor[CH] = 2 * PI *(max_value_2nd_wave[CH].position - max_value_2nd_wave[0].position )/ period_samples;
	power_factor[CH] = fabsf(cos(power_factor[CH]));

}
*/

// Reads input file from Matlab
void read_input(int CH)
{
	// Channel 0 is assumed to be set to the reference voltage (the AC power source)
	// Algorythm uses this Reference Voltage to determine the phase of different inputs
	if(CH == 0)
		fp[CH] = fopen("Input1.txt", "r");
	
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
	
	dft(CH);
	rise_fall_times(CH);
//	get_phase(CH);
	
}

// Returns the waveform at the channel
void determine_waveform(int CH)
{	
	// booleans returns 0 if it doesnt find the WaveForm type, and 1 if it does
	booleans[CH] = 0;
	
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
						booleans[CH] = 1;
						if(i == 0)
						{
							printf("\nChannel #%d represents a Fan\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "0\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 1)
						{
							printf("\nChannel #%d represents a TV\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "1\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 2)
						{
							printf("\nChannel #%d represents a Set-Top-Box\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "2\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 3)
						{
							printf("\nChannel #%d represents a HP Laptop\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 4)
						{
							printf("\nChannel #%d represents One Fan\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 5)
						{
							printf("\nChannel #%d represents Two Fans \n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 6)
						{
							printf("\nChannel #%d represents Three Fans\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 7)
						{
							printf("\nChannel #%d represents HP+FAN\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 8)
						{
							printf("\nChannel #%d represents HP+TV\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 9)
						{
							printf("\nChannel #%d represents STB+FAN\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 10)
						{
							printf("\nChannel #%d represents STB+HP\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 11)
						{
							printf("\nChannel #%d represents STB+TV\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
						if(i == 12)
						{
							printf("\nChannel #%d represents TV+FAN\n\n", CH);
							ofp = fopen("Output.txt", "w");
							fprintf(ofp, "3\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
						}
					}
				}
			}
		}
	}

	
	
	if(booleans[CH] == 0)
	{
		printf("\nUnrecognized signal in Channel #%d\n\n", CH);
		ofp = fopen("Output.txt", "w");
		fprintf(ofp, "4\n%f\n%f\n%f\n", rise_times_calculated[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
	}
	
	// For Debugging purposes
	printf("The Max Reading is %f\nThe Normalized FFT at 180 Hz is %f\nThe Normalized FFT at 300 Hz is %f\n", max_value[CH], normalized_FFT_180[CH], normalized_FFT_300[CH]);
	printf("The number of rising samples is %d\n", rise_times[CH]);
	printf("The rise time is %f\n", rise_times_calculated[CH]);
	printf("The number of falling samples is %d\n", fall_times[CH]);
	
}

// Initalizes all the stored parameters in the array, intervals.  Stores the max and mins of each waveform for each parameter.
void set_parameters()
{
		// Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop, 4 = One Fan, 5 = Two Fans, 6 = Three Fan, 7 = HP+FAN, 8 = HP+TV, 9 = STB+FAN, 10 = STB+HP, 11 = STB+TV, 12 = TV+FAN
		// Parameters are as follows: 0 = peak value, 1 = Normalized FFT at 180, 2 = Normalized FFT at 300, 3 = Rise Time (in number of samples), 4 = Power Factor

		// Vornado Fan
		interval[0][0].min = 0.6;
//		interval[0][0].max = 1.1;
		interval[0][0].max = 0.8;
		interval[0][1].min = 0.0;
		interval[0][1].max = 0.03;
		interval[0][2].min = 0.0;
		interval[0][2].max = 0.05;
		interval[0][3].min = 10.0;
//		interval[0][3].max = 25.0;
		interval[0][3].max = 15.0;
		interval[0][4].min = 0.0;
		interval[0][4].max = 1.0;
		
		// TV
		interval[1][0].min = 0.2;
		interval[1][0].max = 0.35;
		interval[1][1].min = 0.11;
		interval[1][1].max = 0.14;
		interval[1][2].min = 0.11;
		interval[1][2].max = 0.14;
		interval[1][3].min = 10.0;
		interval[1][3].max = 20.0;
		interval[1][4].min = 0.0;
		interval[1][4].max = 1.0;
		
		//	Set-Top-Box
		interval[2][0].min = 0.1;
		interval[2][0].max = 0.22;
		interval[2][1].min = 0.62;
		interval[2][1].max = 0.68;
		interval[2][2].min = 0.57;
		interval[2][2].max = 0.62;
		interval[2][3].min = 0.0;
		interval[2][3].max = 5.0;
		interval[2][4].min = 0.0;
		interval[2][4].max = 1.0;
		
		// HP Laptop
		interval[3][0].min = 0.4;
		interval[3][0].max = 1.0;
		interval[3][1].min = 0.59;
		interval[3][1].max = 0.71;
		interval[3][2].min = 0.52;
		interval[3][2].max = 0.69;
		interval[3][3].min = 0.0;
		interval[3][3].max = 5.0;
		interval[3][4].min = 0.0;
		interval[3][4].max = 1.0;

		// One Fan
		interval[4][0].min = 0.9;
		interval[4][0].max = 1.1;
		interval[4][1].min = 0.001;
		interval[4][1].max = 0.032;
		interval[4][2].min = 0.03;
		interval[4][2].max = 0.04;
		interval[4][3].min = 5.0;
		interval[4][3].max = 25.0;
		interval[4][4].min = 0.0;
		interval[4][4].max = 1.0;

		// Two Fans
		interval[5][0].min = 1.9;
		interval[5][0].max = 2.1;
		interval[5][1].min = 0.001;
		interval[5][1].max = 0.03;
		interval[5][2].min = 0.026;
		interval[5][2].max = 0.037;
		interval[5][3].min = 10.0;
		interval[5][3].max = 20.0;
		interval[5][4].min = 0.0;
		interval[5][4].max = 1.0;

		// Three Fans
		interval[6][0].min = 2.9;
		interval[6][0].max = 3.1;
		interval[6][1].min = 0.006;
		interval[6][1].max = 0.026;
		interval[6][2].min = 0.028;
		interval[6][2].max = 0.04;
		interval[6][3].min = 10.0;
		interval[6][3].max = 20.0;
		interval[6][4].min = 0.0;
		interval[6][4].max = 1.0;

		// HP+FAN
		interval[7][0].min = 1.6;
		interval[7][0].max = 2.0;
		interval[7][1].min = 0.04;
		interval[7][1].max = 0.11;
		interval[7][2].min = 0.03;
		interval[7][2].max = 0.08;
		interval[7][3].min = 10.0;
		interval[7][3].max = 20.0;
		interval[7][4].min = 0.0;
		interval[7][4].max = 1.0;

		// HP+TV
		interval[8][0].min = 0.85;
		interval[8][0].max = 0.95;
		interval[8][1].min = 0.22;
		interval[8][1].max = 0.30;
		interval[8][2].min = 0.11;
		interval[8][2].max = 0.16;
		interval[8][3].min = 5.0;
		interval[8][3].max = 16.0;
		interval[8][4].min = 0.0;
		interval[8][4].max = 1.0;

		// STB+FAN
		interval[9][0].min = 1.18;
		interval[9][0].max = 1.30;
		interval[9][1].min = 0.025;
		interval[9][1].max = 0.075;
		interval[9][2].min = 0.004;
		interval[9][2].max = 0.026;
		interval[9][3].min = 10.0;
		interval[9][3].max = 20.0;
		interval[9][4].min = 0.0;
		interval[9][4].max = 1.0;

		// STB+HP
		interval[10][0].min = 0.82;
		interval[10][0].max = 0.92;
		interval[10][1].min = 0.60;
		interval[10][1].max = 0.70;
		interval[10][2].min = 0.60;
		interval[10][2].max = 0.70;
		interval[10][3].min = 0.0;
		interval[10][3].max = 8.0;
		interval[10][4].min = 0.0;
		interval[10][4].max = 1.0;

		// STB+TV
		interval[11][0].min = 0.54;
		interval[11][0].max = 0.62;
		interval[11][1].min = 0.20;
		interval[11][1].max = 0.28;
		interval[11][2].min = 0.015;
		interval[11][2].max = 0.065;
		interval[11][3].min = 0.0;
		interval[11][3].max = 20.0;
		interval[11][4].min = 0.0;
		interval[11][4].max = 1.0;

		// TV+FAN
		interval[12][0].min = 1.25;
		interval[12][0].max = 1.35;
		interval[12][1].min = 0.02;
		interval[12][1].max = 0.07;
		interval[12][2].min = 0.04;
		interval[12][2].max = 0.08;
		interval[12][3].min = 6.0;
		interval[12][3].max = 18.0;
		interval[12][4].min = 0.0;
		interval[12][4].max = 1.0;
}


/* 	This is the main function.  It will call upon the other functions seen before.  Program starts here.	*/
void main()
{
	//	Initializes values to be used.  This is more as a safe programming practice, to ensure we don't use variables with undefined variables.
	initialize();
	
	//	Initializes the intervals that are going to be used.  These intervals have a min and a max value, that are used to determine if
	//	the test waveform has values within the interval for specified device.
	set_parameters();
	
	//	A loop is needed in the main code when it is run in real time.  There is only one input, so loop only runs once.
	for(CH_counter = 0; CH_counter<NUMBER_OF_CHANNELS; CH_counter++)
	{
		// Reads the input.  In our code, it would read the "input" from the text file, but in real time, it would read the waveform
		// and store it in a buffer.
		read_input(CH_counter);
		
		// Determines what the waveform is, based on the stored values in "input[BUFFER_LENGTH][NUMBER_OF_CHANNELS]"
		determine_waveform(CH_counter);
	}


}