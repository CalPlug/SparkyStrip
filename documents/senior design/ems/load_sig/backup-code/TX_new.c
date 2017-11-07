// TRANSMITTER
/***********************************************************************************
* INCLUDES
*/
#include <hal_lcd.h>
#include <hal_led.h>
#include <hal_joystick.h>
#include <hal_assert.h>
#include <hal_board.h>
#include <hal_int.h>
#include <stdio.h>
#include <math.h>
#include "hal_mcu.h"
#include "hal_button.h"
#include "hal_rf.h"
#include "util_lcd.h"
#include "basic_rf.h"
#include "adc.h"
#include "hal_defs.h"
#include "hal_uart.h"
#include "hal_timer_32k.h"

#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_int.h"
#include "hal_joystick.h"
#include "hal_board.h"
#include "hal_assert.h"


/***********************************************************************************
******************************   CONSTANTS   ***************************************
***********************************************************************************/

#define TX_ID_NUMBER             102      // Change this to send a different "ID"
                                          // bit, identifying the meter attached
                                          // Start from 101, 102, 103, 104, etc

// Application parameters
#define RF_CHANNEL                25      // 2.4 GHz RF channel

// BasicRF address definitions
#define PAN_ID                0x2007

#define SWITCH_ADDR           0x2525	 // Addresses
#define LIGHT_ADDR            0xBEEF

#define APP_PAYLOAD_LENGTH        1
#define LIGHT_TOGGLE_CMD          0

// Application states
#define IDLE                      0
#define SEND_CMD                  1

// Application role
#define NONE                      0
#define SWITCH                    1
#define LIGHT                     2
#define APP_MODES                 2

#define UART_RX_IDLE_TIME       100     // 100 ms
#define N_RETRIES                 5     // Number of transmission retries
#define ADC_BUFFER_SIZE           1
#define DELAY_TIME              200
#define INPUT_BUFFER_LENGTH     600
#define CONVERTED_BUFFER_LENGTH 512

#define OFFSET                -5843 

#define SAMPLING_FREQUENCY     5000
#define FREQUENCY                60
#define PI                 3.141592

#define NUMBER_DEVICES            4
#define NUMBER_PARAMETERS        12
#define LOWER                     0
#define UPPER                     1

typedef struct Complex_tag {float real,imag;} COMPLEX;
typedef struct Threshold_tag {float min; float max;} THRESHOLD;
typedef struct Results_tag {float input[CONVERTED_BUFFER_LENGTH]; COMPLEX fft[CONVERTED_BUFFER_LENGTH]; float rise; float fall; float peak_to_peak; float norm180; float norm300;
							float real180; float real300; float imag180; float imag300; float avg_value; float area; float half_area_time;} RESULTS;


/**************  Function Declarations    ************************************
******************************************************************************/

static uint8 adcBuffer[ADC_BUFFER_SIZE];
static void startADCConv(void);
static float dft300(int* buffer, uint16 buffer_size);

static void initializeDatabase(void);
static void initialize();
static void determineWaveform(int* waveform_parameters);
static void appTransmitter(int8 DeviceNumber);

// Finds Parameters
static void rise_fall_times(float input[CONVERTED_BUFFER_LENGTH]);
static void pk_to_pk(float input[CONVERTED_BUFFER_LENGTH]);
static void dft(float input[CONVERTED_BUFFER_LENGTH]);
static void area(float input[CONVERTED_BUFFER_LENGTH]);



/******************        Variables      ************************************
******************************************************************************/

// Input Buffer
int input_buffer[INPUT_BUFFER_LENGTH];
float converted_buffer[CONVERTED_BUFFER_LENGTH];
RESULTS results;

/********************************/
/*	Characteristics 	*/
/********************************/

float rise_times;		// Attribute #0: Rise Time
float fall_times;		// Attribute #1: Fall Time
float peak_to_peak;		// Attribute #2: Peak-to-Peak Value
float normalized_FFT_180;	// Attribute #3: 180 Hz (Magnitude) Response
float real_FFT_180;		// Attribute #4: 180 Hz (Real) Response
float imaginary_FFT_180;	// Attribute #5: 180 Hz (Imaginary) Response
float normalized_FFT_300;	// Attribute #6: 300 Hz (Magnitude) Response
float real_FFT_300;		// Attribute #7: 300 Hz (Real) Response
float imaginary_FFT_300;	// Attribute #8: 300 Hz (Imaginary) Response
float average_wave_value;	// Attribute #9: Average Value of Half a pariod
float area_period;		// Attribute #10: Area of a Period
float half_area_time;		// Attribute #11: Time it takes from start of a period, to the "half" period mark 


// ID Number: identifies which meter is the signal's. The meters will have a different
// ID number
int8 waveform_number = 0;
int local_max = 0;
int local_max_location = 0;


// Intervals
int intervals[NUMBER_DEVICES][NUMBER_PARAMETERS][2];
int arrayOfParameters[NUMBER_PARAMETERS];

// Variables
uint8 averageValue;

// Light Switch
static uint8 pTxData[APP_PAYLOAD_LENGTH];
static uint8 pRxData[APP_PAYLOAD_LENGTH];
static basicRfCfg_t basicRfConfig;

/******************        MAIN FUNCTION      ********************************
******************************************************************************/

void main(void)
{
    // Initialise board peripherals
    basicRfConfig.panId = PAN_ID;
    basicRfConfig.channel = RF_CHANNEL;
    basicRfConfig.ackRequest = TRUE;
    halBoardInit();
    
    if(halRfInit()==FAILED) {
      HAL_ASSERT(FALSE);
    }
    
//    halUartInit(HAL_UART_BAUDRATE_38400, 0);
//    appConfigTimer(1000/UART_RX_IDLE_TIME); // 100 ms RX idle timeout
//    halTimer32kIntEnable();

    int averageValueprecomma = 0;
    int averageValuepastcomma = 0;
    halLedSet(1);    // Indicate that device is powered
    halLcdClear();
    initializeDatabase();
      
    // Wait for user to press S1 to enter menu
    while(1)
    {

      for(int i =0; i<NUMBER_PARAMETERS; i++)
      {
          arrayOfParameters[i] = 0;
      }
      
      
      // READING PHASE
      halLcdWriteLines("Transmitter", " Read ", "Load Signaturee"); // Outputs on the Screen for User to read
      while (halButtonPushed()!=HAL_BUTTON_1);
      halMcuWaitMs(350);
      halLcdClear();
  
      // Read input
      for(int i=0; i<INPUT_BUFFER_LENGTH; i++)
      {
        // Reads the input
        startADCConv();
        input_buffer[i] = adcBuffer[0];
        halMcuWaitUs(160); // Leads to correct sampling frequency of 5 kHz, at minimum resolution
      }
      
      // Converts to Voltage
      for(int i=0; i<INPUT_BUFFER_LENGTH; i++)
      {
        averageValueprecomma = input_buffer[i] / 39;
        averageValuepastcomma = ((input_buffer[i] * 100) / 39) - (input_buffer[i] * 100);
        input_buffer[i] = averageValueprecomma*100 + averageValuepastcomma - OFFSET;
      }
      
      
      // Shift the values over until starts at the local max
      // first, find local max in first period.
      local_max = 0;
      for(int i=0; i<SAMPLING_FREQUENCY/FREQUENCY; i++)
      {
          if(local_max < input_buffer[i])
          {
            local_max = input_buffer[i];
            local_max_location = i;
          }
      }
      
      //Then use a new buffer, starting at the maximum value.
     for(int i=0; i<CONVERTED_BUFFER_LENGTH; i++)
      {
          converted_buffer[i] = input_buffer[i + local_max_location];
      }
      
      // PERFORMS CALCULATIONS TO DETERMINE WAVEFORM
      halLcdClear();
    
      // Printout: Transmitter Meter Number  
      if( TX_ID_NUMBER == 101 )
        halLcdWriteLines("  Transmitter", "  Meter #: 1", " ");
      else if( TX_ID_NUMBER == 102 )
        halLcdWriteLines("  Transmitter", "  Meter #: 2", " ");
      else if( TX_ID_NUMBER == 103 )
        halLcdWriteLines("  Transmitter", "  Meter #: 3", " ");
      else if( TX_ID_NUMBER == 104 )
        halLcdWriteLines("  Transmitter", "  Meter #: 4", " ");
      else if( TX_ID_NUMBER == 105 )
        halLcdWriteLines("  Transmitter", "  Meter #: 5", " ");
      else
        halLcdWriteLines("  Transmitter", "  Meter #: 6", " ");
     
      /**************************/
      /*        Calculations    */
      /**************************/
      
      rise_fall_times(converted_buffer);
      pk_to_pk(converted_buffer);
      dft(converted_buffer);
      area(converted_buffer);
      
      /***** 0 = Rise Time, 1 = 180Hz response, 2 = 300Hz response, 3 = peak-to-peak*****/
      
      arrayOfParameters[0] = rise_times;			// Attribute #0: Rise Time
      arrayOfParameters[1] = fall_times;			// Attribute #1: Fall Time
      arrayOfParameters[2] = peak_to_peak;			// Attribute #2: Peak-to-Peak Value
      arrayOfParameters[3] = normalized_FFT_180;	// Attribute #3: 180 Hz (Magnitude) Response
      arrayOfParameters[4] = real_FFT_180;			// Attribute #4: 180 Hz (Real) Response
      arrayOfParameters[5] = imaginary_FFT_180;	// Attribute #5: 180 Hz (Imaginary) Response
      arrayOfParameters[6] = normalized_FFT_300;	// Attribute #6: 300 Hz (Magnitude) Response
      arrayOfParameters[7] = real_FFT_300;			// Attribute #7: 300 Hz (Real) Response
      arrayOfParameters[8] = imaginary_FFT_300;	// Attribute #8: 300 Hz (Imaginary) Response
      arrayOfParameters[9] = average_wave_value;	// Attribute #9: Average Value of Half a pariod
      arrayOfParameters[10] = area_period;			// Attribute #10: Area of a Period
      arrayOfParameters[11] = half_area_time;		// Attribute #11: Time it takes from start of a period, to the "half" period mark
      
      /************* Print Calculated Values **********************************************
      *************************************************************************************
      
      // Print the local max
      halLcdClear();
      halLcdWriteLines(" ", "Local Max", " ");
      utilLcdDisplayCounters(1, 0, ',',  local_max, 's');
      while (halButtonPushed()!=HAL_BUTTON_1);
 
      
      // Print the max and min
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
      **********************************************************************
      *********************************************************************/
     
      determineWaveform(arrayOfParameters);
//      while (halButtonPushed()!=HAL_BUTTON_1);
      appTransmitter(TX_ID_NUMBER);
      
      halMcuWaitMs(1);// 10 second Delay from ID bit to real bit.
      
      appTransmitter(waveform_number);
//      halLcdWriteLines("Sent data to", "RX!!", " ");
//      while (halButtonPushed()!=HAL_BUTTON_1);
    }
}

/**************Definition of the functions************************************
******************************************************************************/

// ADC CONVERTER
static void startADCConv(void) 
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

// Initializes the database to be used
static void initializeDatabase(void)
{
  // Devices are as follows: 0 = Fan, 1 = TV, 2 = Set-Top-Box, 3 = HP Laptop
  // Parameters are as follows:
  //    Attribute #0: Rise Time
  //    Attribute #1: Fall Time
  //    Attribute #2: Peak-to-Peak Value
  //    Attribute #3: 180 Hz (Magnitude) Response
  //    Attribute #4: 180 Hz (Real) Response
  //    Attribute #5: 180 Hz (Imaginary) Response
  //    Attribute #6: 300 Hz (Magnitude) Response
  //    Attribute #7: 300 Hz (Real) Response
  //    Attribute #8: 300 Hz (Imaginary) Response
  //    Attribute #9: Average Value of Half a pariod
  //    Attribute #10: Area of a Period
  //    Attribute #11: Time it takes from start of a period, to the "half" period mark 
  
  // Vornado Fan (low)
  intervals[0][0][LOWER] = 2400;
  intervals[0][0][UPPER] = 3000;
  intervals[0][1][LOWER] = 0;
  intervals[0][1][UPPER] = 20;
  intervals[0][2][LOWER] = 0;
  intervals[0][2][UPPER] = 40;
  intervals[0][3][LOWER] = 4800;
  intervals[0][3][UPPER] = 5300;
  
  // LapTop
  intervals[1][0][LOWER] = 0;
  intervals[1][0][UPPER] = 800;
  intervals[1][1][LOWER] = 700;
  intervals[1][1][UPPER] = 900;
  intervals[1][2][LOWER] = 400;
  intervals[1][2][UPPER] = 700;
  intervals[1][3][LOWER] = 5000;
  intervals[1][3][UPPER] = 12500;
  
   // Light-Bulb
  intervals[2][0][LOWER] = 0;
  intervals[2][0][UPPER] = 500;
  intervals[2][1][LOWER] = 600;
  intervals[2][1][UPPER] = 700;
  intervals[2][2][LOWER] = 350;
  intervals[2][2][UPPER] = 425;
  intervals[2][3][LOWER] = 2000;
  intervals[2][3][UPPER] = 2500;
  
  // HP + FAN
  intervals[3][0][LOWER] = 100;
  intervals[3][0][UPPER] = 2000;
  intervals[3][1][LOWER] = 70;
  intervals[3][1][UPPER] = 150;
  intervals[3][2][LOWER] = 45;
  intervals[2][2][UPPER] = 120;
  intervals[2][3][LOWER] = 9000;
  intervals[2][3][UPPER] = 12000;
  
}


static void determineWaveform(int* waveform_parameters)
{
  // Returns 1 if found device
  int found_device = 0;
  int device_number = -1;
  
  int number_of_devices = 1;
  
  // number of "matching characteristics"
  int matches = 0;
  
  for(int i=0; i<NUMBER_DEVICES; i++)
  {
    matches = 0;
    for(int j=0; j<NUMBER_PARAMETERS; j++)
    {
      if(waveform_parameters[j] > intervals[i][j][LOWER] && waveform_parameters[j] < intervals[i][j][UPPER])
      { 
        matches++;
      }
      // if all other conditions are met, except for the peak to peak, this occurs when matches == number-1
      else if(matches == NUMBER_PARAMETERS - 1)
      {
        //  checks if peak-to-peak is twice of that same device.  If so, then there are two devices.
        //  In the future, we could use a loop to determine 3 devices, 4, and so forth as long
        //  as it fits range of ADC
        if(waveform_parameters[j] > 2*intervals[i][j][LOWER] && waveform_parameters[j] < 2*intervals[i][j][UPPER] )
        {
          matches++;
          number_of_devices = 2;
        }
          
      }
    }
    if(matches >= NUMBER_PARAMETERS)
    {
      device_number = i;
      found_device = 1;
      // THis is used, so that it will exit for loop, since if i=NUMBER_DEVICES, for loop ends
      i = NUMBER_DEVICES;
    }
  }
  
  if(found_device == 0)
  {
    halLcdWriteLines("The waveform is", "Unrecognizable", "Signal");
    waveform_number = 0;
  }
  else
  {
    if(device_number == 0)
    {
      if (number_of_devices == 1)
      {
        halLcdWriteLines("The waveform is", "A Fan", "Quantity 1");
        waveform_number = 1;
      }
      else if (number_of_devices == 2)
      {
        halLcdWriteLines("The waveform is", "A Fan", "Quantity 2"); 
        waveform_number = 2;
      }
    }
    else if(device_number == 1)
    {
      if (number_of_devices == 1)
      {
        halLcdWriteLines("The waveform is", "HP-Laptop", "Quantity 1");
        waveform_number = 3;
      }
      else if (number_of_devices == 2)
      {
        halLcdWriteLines("The waveform is", "HP-Laptop", "Quantity 2"); 
        waveform_number = 4;
      }
    }
    else if(device_number == 2)
    {
      if (number_of_devices == 1)
      {
        halLcdWriteLines("The waveform is", "Light Bulb", "Quantity 1"); 
        waveform_number = 5;
      }
      else if (number_of_devices == 2)
      {
        halLcdWriteLines("The waveform is", "Light Bulb", "Quantity 2"); 
        waveform_number = 8;
      }
    }
    else if(device_number == 3)
    {
      if (number_of_devices == 1)
      {
        halLcdWriteLines("The waveform is", "HP Laptop + FAN", "Quantity 1");
        waveform_number = 6;
      }
      else if (number_of_devices == 2)
      {
        halLcdWriteLines("The waveform is", "HP Laptop + FAN", "Quantity 2");
      }
    }
  }
}

// Device Info is either the Device ID or the Waveform Number
static void appTransmitter(int8 DeviceInfo)
{
   
#ifdef ASSY_EXP4618_CC2420
    halLcdClearLine(1);
    halLcdWriteSymbol(HAL_LCD_SYMBOL_TX, 1);
#endif

    pTxData[0] = DeviceInfo;
       // Initialize BasicRF
    // Depending on the "ID", in other words, which transmitter board we are using,
    // has a different light address.
 
    basicRfConfig.myAddr = SWITCH_ADDR;
    
    
    if(basicRfInit(&basicRfConfig)==FAILED) 
    {HAL_ASSERT(FALSE);}

            basicRfReceiveOff();
            basicRfSendPacket(LIGHT_ADDR, pTxData, APP_PAYLOAD_LENGTH);
            // Put MCU to sleep. It will wake up on joystick interrupt
}

/********************************/
/*	Initialize Function	*/
/********************************/
// 	Initallizes all buffers and characteristics.
void initialize()
{
	// Counters
	int i = 0, j = 0;
	
	// Initializes all the values in the input and buffer arrays
	for(i=0; i<CONVERTED_BUFFER_LENGTH; i++)
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
/********************************/
/*	Rise/Fall Times 	*/
/********************************/

// Finds the number of rising samples (given 5000 kHz) and falling samples of each waveform
void rise_fall_times(float input[CONVERTED_BUFFER_LENGTH])
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
		for(j = (i+1.25)*SAMPLING_FREQUENCY/FREQUENCY; j < ((i+2.25)*SAMPLING_FREQUENCY/FREQUENCY); j++)
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
	
	rise_times = rise_times/(3*SAMPLING_FREQUENCY);
	fall_times = fall_times/(3*SAMPLING_FREQUENCY);
	
	results.rise = rise_times;
	results.fall = fall_times;
	
}


/********************************/
/*	Peak to Peak Amplitude	*/
/********************************/
// Find's the peak to peak value
void pk_to_pk(float input[CONVERTED_BUFFER_LENGTH])
{
	peak_to_peak = 0.0;
	float local_min = 2000.0, local_max = -2000.0;
	int i = 0, j = 0;
	
	for(i = 0; i<4; i++)
	{
		local_min = 2000.0;
		local_max = -2000.0;
		
		for(j = i*SAMPLING_FREQUENCY/FREQUENCY; j < ((i+1)*SAMPLING_FREQUENCY/FREQUENCY); j++)
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

/********************************/
/*	Frequency Harmonics	*/
/********************************/
// Computes the DFT of the input waveform.
void dft(float input[CONVERTED_BUFFER_LENGTH])
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
	for(i=0;i<CONVERTED_BUFFER_LENGTH;i++)
	{
		// Initializes the complex buffer
		results.fft[i].real = 0.0;
		results.fft[i].imag = 0.0;
		for(j=0;j<CONVERTED_BUFFER_LENGTH;j++)
		{
			results.fft[i].real += input[j]*((float) cos(2*PI*i*j/CONVERTED_BUFFER_LENGTH));
			results.fft[i].imag -= input[j]*((float) sin(2*PI*i*j/CONVERTED_BUFFER_LENGTH));
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

/********************************/
/*	Area Properties 	*/
/********************************/

//finds area in the "positive" period, aka all the values with positive values in input.
void area(float input[CONVERTED_BUFFER_LENGTH])
{
	average_wave_value = 0.0;
	area_period = 0.0;
	half_area_time = 0.0;
	
	int i = 0, j = 0, positive_input_counter = 0;
	
	// Find area under period and average positive value.  Averages 4 periods
	for(i = 0; i<4; i++)
	{	
		for(j = (i+0.25)*SAMPLING_FREQUENCY/FREQUENCY; j < ((i+1.25)*SAMPLING_FREQUENCY/FREQUENCY); j++)
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
		for(j = (i+0.25)*SAMPLING_FREQUENCY/FREQUENCY; j < ((i+1.25)*SAMPLING_FREQUENCY/FREQUENCY); j++)
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
	
	time = time/SAMPLING_FREQUENCY;
	
	half_area_time = time/4;
	
	results.avg_value = average_wave_value;
	results.area = area_period;
	results.half_area_time = half_area_time;
}