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

static uint8 adcBuffer[ADC_BUFFER_SIZE];

/***********************************************************************************
* LOCAL FUNCTIONS
*/

// 
static float dft180(int* buffer, uint16 buffer_size);
static void startADCConv(void);
// static uint8 calculateAverage(uint8* buffer, uint16 buffer_size);

static int getMinValue(int* buffer, uint16 buffer_size);
static int getMaxValue(int* buffer, uint16 buffer_size);
static int getRiseTime(int* buffer, uint16 buffer_size, uint16 sample_freq, unint16 freq);
static float dft300(int* buffer, uint16 buffer_size);
static void initializeDatabase(void);
static void determineWaveform(int* waveform_parameters);
static void UartOutout(void);
static void appConfigTimer(uint16 rate);
static void appTimerISR(void);

static volatile uint8 appUartRxIdle;

// Input Buffer
int input_buffer[INPUT_BUFFER_LENGTH];
    
// Max Value, Min Value, Rise Time, 180 Hertz, 300 Hertz initialized
int maximum_voltage = 0;
int minimum_voltage = 0;
int peak_to_peak = 0;
int rise = 0;
int FFT_180 = 0;
int FFT_300 = 0;

// Intervals
int intervals[NUMBER_DEVICES][NUMBER_PARAMETERS][2];
int arrayOfParameters[NUMBER_PARAMETERS];

// Variables
uint8 averageValue;





void main(void)
{
    // Initialise board peripherals
    halBoardInit();    
    int averageValueprecomma = 0;
    int averageValuepastcomma = 0;
    // Indicate that device is powered
    halLedSet(1);    
    halLcdClear();
    initializeDatabase();
     
   
    // Wait for user to press S1 to enter menu
    while(1)
    {
      // Initialize
      maximum_voltage = 0;
      minimum_voltage = 0;
      peak_to_peak = 0;
      rise = 0;
      FFT_180 = 0;
      FFT_300 = 0;
      for(int i =0; i<NUMBER_PARAMETERS; i++)
      {
        arrayOfParameters[i] = 0;
      }
      
      // Outputs on the Screen for User to read
      halLcdWriteLines("Press Button 1", "to Read the", "Load Signaturee"); 
      while (halButtonPushed()!=HAL_BUTTON_1);
      halMcuWaitMs(350);
      halLcdClear();
  
      // Read input
      for(int i=0; i<INPUT_BUFFER_LENGTH; i++)
      {
        // Reads the input
        startADCConv();
        input_buffer[i] = adcBuffer[0];
        
        // Delay to Match the Desired Sampling Frequency
        halMcuWaitUs(200);
      }
      for(int i=0; i<INPUT_BUFFER_LENGTH; i++)
      {
        averageValueprecomma = input_buffer[i] / 39;
        averageValuepastcomma = ((input_buffer[i] * 100) / 39) - (input_buffer[i] * 100);
        input_buffer[i] = averageValueprecomma*100 + averageValuepastcomma - OFFSET;
      }
      
         
         
      halLcdWriteLines("Stored Device's", "Load Signature!", " ");
      while (halButtonPushed()!=HAL_BUTTON_1);
      
      
      // Algorithm: finds all the "characteristics"
      halLcdClear();
      halLcdWriteLines(" ", "Calculating", "Please wait");
      maximum_voltage = getMaxValue(input_buffer, INPUT_BUFFER_LENGTH);
      minimum_voltage = getMinValue(input_buffer, INPUT_BUFFER_LENGTH);
      rise = getRiseTime(input_buffer, INPUT_BUFFER_LENGTH, SAMPLING_FREQUENCY, FREQUENCY );
      peak_to_peak = maximum_voltage - minimum_voltage;
      FFT_180 = (int) 1000 * dft180(input_buffer, INPUT_BUFFER_LENGTH);
      FFT_300 = (int) 1000 * dft300(input_buffer, INPUT_BUFFER_LENGTH);
      
      // 0 = Rise Time, 1 = 180Hz response, 2 = 300Hz response, 3 = peak-to-peak
      arrayOfParameters[0] = rise;
      arrayOfParameters[1] = FFT_180;
      arrayOfParameters[2] = FFT_300;
      arrayOfParameters[3] = peak_to_peak;
      
      // Print Max and Min Voltage
      
      halLcdClear();
      halLcdWriteLines(" ", "Max, Min", " ");
      utilLcdDisplayCounters(1, maximum_voltage, ',',  minimum_voltage, 'V');
      while (halButtonPushed()!=HAL_BUTTON_1);
 
      // Print the Rise Time     
      halLcdClear();
      halLcdWriteLines(" ", "Rise Time (us)", " ");
      utilLcdDisplayCounters(1, 0, ',',  rise, 's');
      while (halButtonPushed()!=HAL_BUTTON_1);

      // Print the 180Hz response
      halLcdClear();
      halLcdWriteLines(" ", "180Hz response", " ");
      utilLcdDisplayCounters(1, 0, ',',  FFT_180, ' '); 
      while (halButtonPushed()!=HAL_BUTTON_1);
      
      // Print the 300 response
      halLcdClear();
      halLcdWriteLines(" ", "300Hz response", " ");
      utilLcdDisplayCounters(1, 0, ',',  FFT_300, ' '); 
      while (halButtonPushed()!=HAL_BUTTON_1);
       
      
      
      // Output what the waveform is

      determineWaveform(arrayOfParameters);
      while (halButtonPushed()!=HAL_BUTTON_1);
    }
}