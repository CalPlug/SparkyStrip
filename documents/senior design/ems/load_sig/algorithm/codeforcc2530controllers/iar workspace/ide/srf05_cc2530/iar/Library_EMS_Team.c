/***********************************************************************************
    Filename:     Librarz EMS Team.c

    Description:

***********************************************************************************/

#include <EMS_Team.h>
#include <hal_lcd.h>
#include <hal_led.h>
#include <hal_joystick.h>
#include <hal_assert.h>
#include <hal_board.h>
#include <hal_int.h>
#include "hal_mcu.h"
#include "hal_button.h"
#include "hal_rf.h"
#include "util_lcd.h"
#include "basic_rf.h"
#include "adc.h"
#include <stdio.h>
#include <math.h>
#include "hal_defs.h"

#include "hal_uart.h"
#include "hal_timer_32k.h"



#define ADC_BUFFER_SIZE 1
#define DELAY_TIME 200
#define INPUT_BUFFER_LENGTH 512
#define OFFSET -5843      
#define SAMPLING_FREQUENCY 5000
#define FREQUENCY 60
#define PI 3.141592

#define NUMBER_DEVICES 4
#define NUMBER_PARAMETERS 4
#define LOWER 0
#define UPPER 1

void initializeDatabase(void)
{
  // Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop
  // Parameters are as follows: 0 = Rise Time, 1 = 180Hz response, 2 = 300Hz response, 3 = peak-to-peak
  
  // Vornado Fan (low)
  intervals[0][0][LOWER] = 1800;
  intervals[0][0][UPPER] = 2500;
  intervals[0][1][LOWER] = 10;
  intervals[0][1][UPPER] = 60;
  intervals[0][2][LOWER] = 0;
  intervals[0][2][UPPER] = 30;
  intervals[0][3][LOWER] = 4800;
  intervals[0][3][UPPER] = 5200;
  
  // LapTop
  intervals[1][0][LOWER] = 0;
  intervals[1][0][UPPER] = 400;
  intervals[1][1][LOWER] = 50;
  intervals[1][1][UPPER] = 600;
  intervals[1][2][LOWER] = 230;
  intervals[1][2][UPPER] = 600;
  intervals[1][3][LOWER] = 11000;
  intervals[1][3][UPPER] = 12500;
  
  // HP + FAN
  intervals[2][0][LOWER] = 100;
  intervals[2][0][UPPER] = 2000;
  intervals[2][1][LOWER] = 0;
  intervals[2][1][UPPER] = 100;
  intervals[2][2][LOWER] = 0;
  intervals[2][2][UPPER] = 229;
  intervals[2][3][LOWER] = 11000;
  intervals[2][3][UPPER] = 12500;
  
  // nothing
  intervals[3][0][LOWER] = 0;
  intervals[3][0][UPPER] = 0;
  intervals[3][1][LOWER] = 0;
  intervals[3][1][UPPER] = 0;
  intervals[3][2][LOWER] = 0;
  intervals[3][2][UPPER] = 0;
  intervals[3][3][LOWER] = 0;
  intervals[3][3][UPPER] = 0;
  
}

void startADCConv(void) 
{
    uint8 channel = HAL_BOARD_IO_JOYSTICK_ADC_PIN;
    uint8 reference = ADC_REF_AVDD;
    uint8 resolution = ADC_9_BIT;
    
    ADC_ENABLE_CHANNEL(channel);
    for (uint16 i = 0; i < ADC_BUFFER_SIZE; i++) { 
      halLedSet(1);
      ADCIF = 0;
      ADC_SINGLE_CONVERSION(reference | resolution | channel);
      while(!ADCIF);
      adcBuffer[i] = ADCH & 0x7F;     
      halLedClear(1);
    }
    ADC_DISABLE_CHANNEL(channel);

}


int getMaxValue(int* buffer, uint16 buffer_size)
{
  int max = -20000;
  
  for(uint16 count = 0; count < buffer_size; count++) {
    if (max < buffer[count])
    {
        max = buffer[count];
    }    
  }

  return max;
}

int getMinValue(int* buffer, uint16 buffer_size)
{
  int min = 20000;
  
  for(uint16 count = 0; count < buffer_size; count++) {
    if (min > buffer[count])
    {
        min = buffer[count];
    }    
  }

  return min;
}

static int getRiseTime(int* buffer, uint16 buffer_size, uint16 sample_freq, unint16 freq)
{
  int rise_time = 0;
  int samples_per_period = sample_freq/freq;
  int boolean = 1;
  int max_at_period = 0;
  int counter = 0;
  int number_of_periods = 3;
  
  // Finds the "rise time" (number of samples) for the number of periods desired, and finds the average
  for(uint16 i = 1; i<(number_of_periods + 1); i++)
  {
    max_at_period = 0;
  // 2nd, 3rd and 4th Period, depending on for_loop.  Finds the max at loop, and stores the max and buffer location
    for(uint16 count = (i)*samples_per_period; count < (i+1)*samples_per_period; count++) 
    {
      if (max_at_period < buffer[count])
      {
          max_at_period = buffer[count];
          counter = count;
      }       
    }
    
    boolean = 1;
    //Finds the "array index" at 90% value of max in second wave
    while(boolean == 1)
    {
      if( buffer[counter] < 0.9 * max_at_period)
      {
        boolean = 0;
      }
      else
      {
        counter--;
      }
    }
    boolean = 1;
    while(boolean == 1)
    {
      if( buffer[counter] < 0.1 * max_at_period)
      {
        boolean = 0;
      }
      counter--;
      rise_time++;
    }
  }
  
  return rise_time*200/3;
}

// FFT at 180 Hertz.  Assumes 5000Hz Sampling Frequency
float dft180(int* buffer, uint16 buffer_size)
{	
    // Initialize
    float real_value = 0.0;
    float imaginary_value = 0.0;
    float FFT_60Hz = 0.0;
    float FFT_180Hz = 0.0;
    float FFT_Val = 0.0;
	
    // Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
    // Because the 60 Hz FFT may be represented in a different buffer, we check values around it for accuracy
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(5+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(5+i)/buffer_size));
      }	
	
      // 60 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_60Hz)
        FFT_60Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
    // Finds the 180 Hertz
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(17+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(17+i)/buffer_size));
      }	
	
      // 180 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_180Hz)
        FFT_180Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
    //
     
    return FFT_180Hz/FFT_60Hz;
}

float dft300(int* buffer, uint16 buffer_size)
{	
    // Initialize
    float real_value = 0.0;
    float imaginary_value = 0.0;
    float FFT_60Hz = 0.0;
    float FFT_300Hz = 0.0;
    float FFT_Val = 0.0;
	
    // Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
    // Because the 60 Hz FFT may be represented in a different buffer, we check values around it for accuracy
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(5+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(5+i)/buffer_size));
      }	
	
      // 60 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_60Hz)
        FFT_60Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
    // Finds the 300 Hertz
    for(int i=0; i<3; i++)
    {
      for(int count=0; count<buffer_size; count++)
      {
        real_value += buffer[count]*((float) cos(2*PI*count*(30+i)/buffer_size));
        imaginary_value -= buffer[count]*((float) sin(2*PI*count*(30+i)/buffer_size));
      }	
	
      // 180 Hz
      FFT_Val = sqrt(real_value*real_value + imaginary_value*imaginary_value);
      
      if( FFT_Val > FFT_300Hz)
        FFT_300Hz = FFT_Val;
    
      real_value = 0.0;
      imaginary_value = 0.0;
    }
    //
     
    return FFT_300Hz/FFT_60Hz;
}