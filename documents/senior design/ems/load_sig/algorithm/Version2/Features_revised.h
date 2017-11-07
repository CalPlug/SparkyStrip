/* 						EMS Team: Demo Algorythm 						*/
/*																		*/
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

/************************/
/*  Pre-Defined Values	*/
/************************/

// Sampling Frequency, ours is set at 5000 samples per second  In other words, the sampling
// 		Frequency is 5kHz.  Waveform frequency is the frequency of the appliances, all of them
//  	are roughly 60 Hz.
#define SAMPLING_FREQ 				5000
#define WAVEFORM_FREQ 				60
#define UNMODIFIED_BUFFER_LENGTH	600
#define BUFFER_LENGTH 				512
#define PI 							3.14159265358979
#define NUMBER_OF_PARAMETERS 		12
#define NUMBER_OF_DEVICES 			4

// # of samples in a period
int samples_per_period = SAMPLING_FREQ/WAVEFORM_FREQ;

// Time in between the number of samples
float time_per_sample = 1/SAMPLING_FREQ;

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
typedef struct Threshold_tag {float min; float max;} THRESHOLD;
typedef struct Results_tag {float input[BUFFER_LENGTH]; COMPLEX fft[BUFFER_LENGTH]; float rise; float fall; float peak_to_peak; float norm180; float norm300;
							float real180; float real300; float imag180; float imag300; float avg_value; float area; float half_area_time;} RESULTS;

/************************************/
/*  Variables used to read input	*/
/************************************/

// Input File pointer.  Used to read the text file, converted from Excel to Txt in MatLab.
// Output File pointer.  Writes into output text file.
FILE *fp;
FILE *ofp;
// Used to Scan the input from file into
char line[80];

/************/
/*  Buffers	*/
/************/


float input_buffer[BUFFER_LENGTH];
RESULTS results;


/********************/
/*	Characteristics	*/
/********************/

float rise_times;			// Attribute #0: Rise Time
float fall_times;			// Attribute #1: Fall Time
float peak_to_peak;			// Attribute #2: Peak-to-Peak Value
float normalized_FFT_180;	// Attribute #3: 180 Hz (Magnitude) Response
float real_FFT_180;			// Attribute #4: 180 Hz (Real) Response
float imaginary_FFT_180;	// Attribute #5: 180 Hz (Imaginary) Response
float normalized_FFT_300;	// Attribute #6: 300 Hz (Magnitude) Response
float real_FFT_300;			// Attribute #7: 300 Hz (Real) Response
float imaginary_FFT_300;	// Attribute #8: 300 Hz (Imaginary) Response
float average_wave_value;	// Attribute #9: Average Value of Half a pariod
float area_period;			// Attribute #10: Area of a Period
float half_area_time;		// Attribute #11: Time it takes from start of a period, to the "half" period mark 

float attribute_values[NUMBER_OF_PARAMETERS];	// All the attributes are stored here, each numbered according to the comments above




/************************/
/*	Threshold Values	*/
/************************/

// Set Threshold Values.  Each device has a corresponding number  (ex: 0 corresponds to a fan.)
//		Each parameter also corresponds to a waveform attribute (ex: 1 corresponds to Normalized FFT at 180. )
// 		Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop, 4 = One Fan, 5 = Two Fans, 6 = Three Fans
//		Parameters are as follows: 0 = peak value, 1 = Normalized FFT at 180, 2 = Normalized FFT at 300, 3 = Rise Time (in number of samples), 4 = Power Factor
// 		
// 		For example, interval[0][3].min corresponds to a Fan's minimum rise time value,
//			while interval[0][3].max would correspond to its maximum.

THRESHOLD interval[NUMBER_OF_DEVICES][NUMBER_OF_PARAMETERS];

/****************************/
/*	Function Declarations	*/
/****************************/

void initialize();
void set_parameters(THRESHOLD interval[NUMBER_OF_DEVICES][NUMBER_OF_PARAMETERS]);
void read_input(float input[BUFFER_LENGTH]);
void print_all_attribute_values();

// Finds Parameters
void rise_fall_times(float input[BUFFER_LENGTH]);
void pk_to_pk(float input[BUFFER_LENGTH]);
void dft(float input[BUFFER_LENGTH]);
void area(float input[BUFFER_LENGTH]);



/********************/
/*	Main Function	*/
/********************/

void main()
{

	//	Initializes values to be used.  This is more as a safe programming practice, to ensure we don't use variables with undefined variables.
	initialize();
	
	//	Initializes the intervals that are going to be used.  These intervals have a min and a max value, that are used to determine if
	//	the test waveform has values within the interval for specified device.
	set_parameters(interval);

	// Reads the input.  In our code, it would read the "input" from the text file, but in real time, it would read the waveform
	// and store it in a buffer.
	read_input(input_buffer);
	
	//	Calculates Characteristics
	rise_fall_times(input_buffer);
	pk_to_pk(input_buffer);
	dft(input_buffer);
	area(input_buffer);
	
	// Determines what the waveform is, based on the stored values in "input[BUFFER_LENGTH][NUMBER_OF_CHANNELS]"
//	determine_waveform();
	
	// Prints out all the attribute values
	print_all_attribute_values();


}

/************************/
/*	Initialize Function	*/
/************************/
// 	Initallizes all buffers and characteristics.
void initialize()
{
	// Counters
	int i = 0, j = 0;
	
	// Initializes all the values in the input and buffer arrays
	for(i=0; i<BUFFER_LENGTH; i++)
	{
		
		results.fft[i].real = 0.0;
		results.fft[i].imag = 0.0;
		results.input[i] = 0.0;
	}
	
	// Characteristics
	rise_times = 0.0;			// Rise Time
	results.rise = 0.0;
	fall_times = 0.0;			// Fall Time
	results.fall = 0.0;
	peak_to_peak = 0.0;			// Peak-to-Peak Value
	results.peak_to_peak = 0.0;
	normalized_FFT_180 = 0.0;	// 180 Hz (Magnitude) Response
	results.norm180 = 0.0;
	real_FFT_180 = 0.0;			// 180 Hz (Real) Response
	results.real180 = 0.0;
	imaginary_FFT_180 = 0.0;	// 180 Hz (Imaginary) Response
	results.imag180 = 0.0;
	normalized_FFT_300 = 0.0;	// 300 Hz (Magnitude) Response
	results.real300 = 0.0;
	real_FFT_300 = 0.0;			// 300 Hz (Real) Response
	results.real300 = 0.0;
	imaginary_FFT_300 = 0.0;	// 300 Hz (Imaginary) Response
	results.imag300 = 0.0;
	average_wave_value = 0.0;	// Average Value of Half a pariod
	results.avg_value = 0.0;
	area_period = 0.0;			// Area of a Period
	results.area = 0.0;
	half_area_time = 0.0;		// Time it takes from start of a period, to the "half" period mark 
	results.half_area_time = 0.0;
	
	
}

/************************/
/*	Parameters Function	*/
/************************/

// Initalizes all the stored parameters in the array, intervals.  Stores the max and mins of each waveform for each parameter.
void set_parameters(THRESHOLD interval[NUMBER_OF_DEVICES][NUMBER_OF_PARAMETERS])
{
		// Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop, 4 = One Fan, 5 = Two Fans, 6 = Three Fan, 7 = HP+FAN, 8 = HP+TV, 9 = STB+FAN, 10 = STB+HP, 11 = STB+TV, 12 = TV+FAN
		// Parameters are as follows: 0 = peak value, 1 = Normalized FFT at 180, 2 = Normalized FFT at 300, 3 = Rise Time (in number of samples), 4 = Power Factor

		// Vornado Fan
		interval[0][0].min = 0.6;
		interval[0][0].max = 1.1;
		interval[0][0].max = 0.8;
		interval[0][1].min = 0.0;
		interval[0][1].max = 0.03;
		interval[0][2].min = 0.0;
		interval[0][2].max = 0.05;
		interval[0][3].min = 10.0;
		interval[0][3].max = 25.0;
		interval[0][3].max = 15.0;
		interval[0][4].min = 0.0;
		interval[0][4].max = 1.0;
		
		
}

/************************/
/*	Reading Function	*/
/************************/

// Reads input file from Matlab
void read_input(float input[BUFFER_LENGTH])
{
	// Algorythm uses this Reference Voltage to determine the phase of different inputs
	fp = fopen("Input1.txt", "r");
	
	// Buffer length 600
	float unmodified_input[UNMODIFIED_BUFFER_LENGTH];
	
	int counter = 0;
	int counter_1 = 0;
	int max_value_location = 0;
	float max_value = 0.0;
	
	
	/* get a line, up to 80 chars from fr.  done if NULL */
	while( (fgets(line, 80, fp) != NULL) && (counter<UNMODIFIED_BUFFER_LENGTH) )
	{
	/* get a line, up to 80 chars from fr.  done if NULL */
		sscanf (line, "%f", &unmodified_input[counter]);
		counter++;
	}

	// close file
	fclose(fp);
	
	// Finds location of the peak within the first period of samples.
	for(counter = 0; counter<samples_per_period; counter++)
	{
		if(max_value < unmodified_input[counter])
		{
			max_value = unmodified_input[counter];
			max_value_location = counter;
		}
	}
	
	
	// Writes into the input buffer, with the start of the array at the peak of the first period.
	for(counter = max_value_location; counter<(max_value_location + BUFFER_LENGTH); counter++)
	{
		input[counter_1] = unmodified_input[counter];
		results.input[counter_1] = input[counter_1];
		counter_1++;
	}
	
	
	
}

/********************/
/*	Rise/Fall Times	*/
/********************/

// Finds the number of rising samples (given 5000 kHz) and falling samples of each waveform
void rise_fall_times(float input[BUFFER_LENGTH])
{

	int i = 0, j = 0;
	rise_times = 0.0;
	fall_times = 0.0;
	float relative_max = 0.0;
	int relative_max_position = 0, current_position = 0;
	
	for(i = 0; i<3; i++)
	{
		relative_max = -2000.0;
		relative_max_position = 0;
		current_position = 0;
		
		// First find Relative max and position
		for(j = (i+1.25)*SAMPLING_FREQ/WAVEFORM_FREQ; j < ((i+2.25)*SAMPLING_FREQ/WAVEFORM_FREQ); j++)
		{
			if(input[j] > relative_max)
			{
				relative_max = input[j];
				relative_max_position = j;
			}
		}
		
		current_position = relative_max_position;
		
		// find 90% position
		while(input[current_position]>(0.9*relative_max))
		{
			current_position--;
		}
		// Increment number of rise time samples
		while(input[current_position]>(0.1*relative_max))
		{
			rise_times++;
			current_position--;
		}
		
		
		
		
		
		current_position = relative_max_position;
		// Increment number of fall time samples
		while(input[current_position]>(0.9*relative_max))
		{
			current_position++;
		}
		while(input[current_position]>(0.1*relative_max))
		{
			fall_times++;
			current_position++;
		}
		
	}
	
	rise_times = rise_times/(3*SAMPLING_FREQ);
	fall_times = fall_times/(3*SAMPLING_FREQ);
	
	results.rise = rise_times;
	results.fall = fall_times;
	
}


/****************************/
/*	Peak to Peak Amplitude	*/
/****************************/
// Find's the peak to peak value
void pk_to_pk(float input[BUFFER_LENGTH])
{
	peak_to_peak = 0.0;
	float local_min = 2000.0, local_max = -2000.0;
	int i = 0, j = 0;
	
	for(i = 0; i<4; i++)
	{
		local_min = 2000.0;
		local_max = -2000.0;
		
		for(j = i*SAMPLING_FREQ/WAVEFORM_FREQ; j < ((i+1)*SAMPLING_FREQ/WAVEFORM_FREQ); j++)
		{
			if(input[j] > local_max)
				local_max = input[j];
			if(input[j] < local_min)
				local_min = input[j];
		}
		
		peak_to_peak += (local_max - local_min);
	}
	
	peak_to_peak = peak_to_peak/4;
	results.peak_to_peak = peak_to_peak;
}

/************************/
/*	Frequency Harmonics	*/
/************************/
// Computes the DFT of the input waveform.
void dft(float input[BUFFER_LENGTH])
{	
	normalized_FFT_180 = 0.0;	// Attribute #3: 180 Hz (Magnitude) Response
	real_FFT_180 = 0.0;			// Attribute #4: 180 Hz (Real) Response
	imaginary_FFT_180 = 0.0;	// Attribute #5: 180 Hz (Imaginary) Response
	normalized_FFT_300 = 0.0;	// Attribute #6: 300 Hz (Magnitude) Response
	real_FFT_300 = 0.0;			// Attribute #7: 300 Hz (Real) Response
	imaginary_FFT_300 = 0.0;	// Attribute #8: 300 Hz (Imaginary) Response

	// Initializes the initial maximum dft value
	float max_dft_value = 0.0, max_real_value = 0.0, max_imag_value = 0.0;
	int i = 0, j = 0;
	
	// Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
	for(i=0;i<BUFFER_LENGTH;i++)
	{
		// Initializes the complex buffer
		results.fft[i].real = 0.0;
		results.fft[i].imag = 0.0;
		for(j=0;j<BUFFER_LENGTH;j++)
		{
			results.fft[i].real += input[j]*((float) cos(2*PI*i*j/BUFFER_LENGTH));
			results.fft[i].imag -= input[j]*((float) sin(2*PI*i*j/BUFFER_LENGTH));
		}
		// Records the Highest DFT value
		if(max_dft_value < sqrt(results.fft[i].real*results.fft[i].real + results.fft[i].imag*results.fft[i].imag))
		{
			max_dft_value = sqrt(results.fft[i].real*results.fft[i].real + results.fft[i].imag*results.fft[i].imag);
		}
		
		// Records the Highest Real DFT value
		if(max_real_value < results.fft[i].real)
		{
			max_real_value = results.fft[i].real;
		}
		
		// Records the Highest imaginary DFT value
		if(max_imag_value < results.fft[i].imag)
		{
			max_imag_value = results.fft[i].imag;
		}
	
	}
	
	// Records the specific FFT intervals
	normalized_FFT_180 = sqrt(results.fft[18].real*results.fft[18].real + results.fft[18].imag*results.fft[18].imag)/max_dft_value;
	real_FFT_180 = results.fft[18].real/max_real_value;
	imaginary_FFT_180 = results.fft[18].imag/max_imag_value;
	normalized_FFT_300 = sqrt(results.fft[31].real*results.fft[31].real + results.fft[31].imag*results.fft[31].imag)/max_dft_value;
	real_FFT_300 = results.fft[31].real/max_real_value;
	imaginary_FFT_300 = results.fft[31].imag/max_imag_value;
	
	// Records attributes
	results.norm180 = normalized_FFT_180;
	results.real180 = real_FFT_180;
	results.imag180 = imaginary_FFT_180;
	results.norm300 = normalized_FFT_300;
	results.real300  = real_FFT_300;
	results.imag300  = imaginary_FFT_300;
	
}

/********************/
/*	Area Properties	*/
/********************/
//finds area in the "positive" period, aka all the values with positive values in input.
void area(float input[BUFFER_LENGTH])
{
	average_wave_value = 0.0;
	area_period = 0.0;
	half_area_time = 0.0;
	
	int i = 0, j = 0, positive_input_counter = 0;
	
	// Find area under period and average positive value.  Averages 4 periods
	for(i = 0; i<4; i++)
	{	
		for(j = (i+0.25)*SAMPLING_FREQ/WAVEFORM_FREQ; j < ((i+1.25)*SAMPLING_FREQ/WAVEFORM_FREQ); j++)
		{
			if(input[j] > 0.0)
			{
				area_period += input[j];
				positive_input_counter++;
			}
		}
	}
	
	average_wave_value = area_period/positive_input_counter;
	area_period = area_period/4;
	
	
	
	float local_max = -2000.0;
	int local_max_position = 0, found_zero_position = 0, found_half_area = 0;
	float time = 0.0, half_area = 0.0, current_area = 0.0;

	half_area = area_period/2;
	
	// Finds time it takes to reach "half" area.  Averages 4 periods
	for(i = 0; i<4; i++)
	{	
		// initialize everytime in loop to 0
		local_max = -2000.0;
		local_max_position = 0;
		found_zero_position = 0;
		current_area = 0.0;
		
		// First finds local max and its position
		for(j = (i+0.25)*SAMPLING_FREQ/WAVEFORM_FREQ; j < ((i+1.25)*SAMPLING_FREQ/WAVEFORM_FREQ); j++)
		{
			if(input[j] > local_max)
			{
				local_max = input[j];
				local_max_position = j;
			}
		}
		
		// Find point from max that reaches 0.05 of the max
		while(!found_zero_position)
		{
			if(input[local_max_position] < 0.05*local_max)
				found_zero_position = 1;
			else
				local_max_position--;
		}
		
		// count time until it reaches max area
		while(!found_half_area)
		{
			if(current_area > half_area)
			{
				found_half_area = 1;
			}
			else
			{
				current_area += input[local_max_position];
				local_max_position++;
				time++;
			}
		}
	}
	
	time = time/SAMPLING_FREQ;
	
	half_area_time = time/4;
	
	results.avg_value = average_wave_value;
	results.area = area_period;
	results.half_area_time = half_area_time;
}


/*
// Returns the waveform at the channel
void determine_waveform()
{	
	// booleans returns 0 if it doesnt find the WaveForm type, and 1 if it does
	booleans = 0;
	
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
	printf("The 1st Reading is %f\n", max_value_1st_wave[CH].value);
	printf("The 2nd Reading is %f\n", max_value_2nd_wave[CH].value);
	printf("The 3rd Reading is %f\n", max_value_3rd_wave[CH].value);
	printf("The 4th Reading is %f\n", max_value_4th_wave[CH].value);
	printf("The 5th Reading is %f\n", max_value_5th_wave[CH].value);
	printf("The Average Height of Peak is %f\n",ave_value_wave);
	printf("The area for one period is %f\n",area_period);
	printf("The position for half the area of one period is %f\n",half_area_position);
	printf("The half area is %f\n",area_temp);
}

*/

void print_all_attribute_values()
{ 

	printf("The Rise time is \t\t\t\t %f Seconds.\n", results.rise);
	printf("The Fall time is \t\t\t\t %f Seconds.\n", results.fall);
	printf("The Peak to Peak Voltage is \t\t\t %f Volts.\n", results.peak_to_peak);
	printf("The Normalized 180 Hertz Value is \t\t %f.\n", results.norm180);
	printf("The Real 180 Hertz Value is \t\t\t %f.\n", results.real180);
	printf("The Imaginary 180 Hertz Value is \t\t %f.\n", results.imag180);
	printf("The Normalized 300 Hertz Value is \t\t %f.\n", results.norm300);
	printf("The Real 300 Hertz Value is \t\t\t %f.\n", results.real300);
	printf("The Imaginary 300 Hertz Value is \t\t %f.\n", results.imag300);
	printf("The Average Amplitude in half a period is \t %f Volts\n", results.avg_value);
	printf("The Area in half the period is \t\t\t %f.\n", results.area);
	printf("The time to reach half this total area is \t %f seconds.\n", results.half_area_time);

}