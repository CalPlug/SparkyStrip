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

// Application parameters
#define RF_CHANNEL                25      // 2.4 GHz RF channel

// BasicRF address definitions
#define PAN_ID                0x2007

#define SWITCH_ADDR1          0x2520
#define SWITCH_ADDR2          0x2521
#define SWITCH_ADDR3          0x2522
#define SWITCH_ADDR4          0x2523
#define SWITCH_ADDR5          0x2524
#define SWITCH_ADDR6          0x2525
#define LIGHT_ADDR1           0xBEEA      // different Addresses                 //0xBEEF
#define LIGHT_ADDR2           0xBEEB
#define LIGHT_ADDR3           0xBEEC
#define LIGHT_ADDR4           0xBEED
#define LIGHT_ADDR5           0xBEEE
#define LIGHT_ADDR6           0xBEEF
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
#define INPUT_BUFFER_LENGTH     512
#define OFFSET                -5843      
#define SAMPLING_FREQUENCY     5000
#define FREQUENCY                60
#define PI                 3.141592

#define NUMBER_DEVICES            4
#define NUMBER_PARAMETERS         4
#define LOWER                     0
#define UPPER                     1

/**************  Function Declarations    ************************************
******************************************************************************/

static uint8 adcBuffer[ADC_BUFFER_SIZE];
static float dft180(int* buffer, uint16 buffer_size);
static void startADCConv(void);
static int getMinValue(int* buffer, uint16 buffer_size);
static int getMaxValue(int* buffer, uint16 buffer_size);
static int getRiseTime(int* buffer, uint16 buffer_size);
static float dft300(int* buffer, uint16 buffer_size);
static void initializeDatabase(void);
static void determineWaveform(int* waveform_parameters);
static void appTransmitter(int8 DeviceNumber);


/******************        Variables      ************************************
******************************************************************************/

// Input Buffer
int input_buffer[INPUT_BUFFER_LENGTH];

// Max Value, Min Value, Rise Time, 180 Hertz, 300 Hertz initialized
int maximum_voltage = 0;
int minimum_voltage = 0;
int peak_to_peak = 0;
int rise = 0;
int FFT_180 = 0;
int FFT_300 = 0;
int8 waveform_number = 0;

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
      
      
      // READING PHASE
      halLcdWriteLines("Transmitter", " Read ", "Load Signaturee"); // Outputs on the Screen for User to read
//      while (halButtonPushed()!=HAL_BUTTON_1);
      halMcuWaitMs(350);
      halLcdClear();
  
      // Read input
      for(int i=0; i<INPUT_BUFFER_LENGTH; i++)
      {
        // Reads the input
        startADCConv();
        input_buffer[i] = adcBuffer[0];
        halMcuWaitUs(200);
      }
     
      
      // Converts to Voltage
      for(int i=0; i<INPUT_BUFFER_LENGTH; i++)
      {
        averageValueprecomma = input_buffer[i] / 39;
        averageValuepastcomma = ((input_buffer[i] * 100) / 39) - (input_buffer[i] * 100);
        input_buffer[i] = averageValueprecomma*100 + averageValuepastcomma - OFFSET;
      }
      
         
      // DISPLAY CONFIRMATION OF STORED DEVICES
//      halLcdWriteLines("Stored Device's", "Load Signature!", " ");
//      while (halButtonPushed()!=HAL_BUTTON_1);
      
      // PERFORMS CALCULATIONS TO DETERMIN WAVEFORM
      halLcdClear();
      halLcdWriteLines(" ", "Calculating", "Please wait");
      maximum_voltage = getMaxValue(input_buffer, INPUT_BUFFER_LENGTH);
      minimum_voltage = getMinValue(input_buffer, INPUT_BUFFER_LENGTH);
      rise = getRiseTime(input_buffer, INPUT_BUFFER_LENGTH);
      peak_to_peak = maximum_voltage - minimum_voltage;
      FFT_180 = (int) 1000 * dft180(input_buffer, INPUT_BUFFER_LENGTH);
      FFT_300 = (int) 1000 * dft300(input_buffer, INPUT_BUFFER_LENGTH);
      
      /***** 0 = Rise Time, 1 = 180Hz response, 2 = 300Hz response, 3 = peak-to-peak*****/
      
      arrayOfParameters[0] = rise;
      arrayOfParameters[1] = FFT_180;
      arrayOfParameters[2] = FFT_300;
      arrayOfParameters[3] = peak_to_peak; 
      
      /************* Print Calculated Values **********************************************
      *************************************************************************************
      
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
// Returns the max of the buffer
static int getMaxValue(int* buffer, uint16 buffer_size)
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
// Returns the Min of the buffer
static int getMinValue(int* buffer, uint16 buffer_size)
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
// Returns the rise time
static int getRiseTime(int* buffer, uint16 buffer_size)
{
  int rise_time = 0;
  int samples_per_period = SAMPLING_FREQUENCY/FREQUENCY;
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
static float dft180(int* buffer, uint16 buffer_size)
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

static float dft300(int* buffer, uint16 buffer_size)
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
   return FFT_300Hz/FFT_60Hz;
}

// Initializes the database to be used
static void initializeDatabase(void)
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
        halLcdWriteLines("The waveform is", "HP Laptop + FAN", " "); 
        waveform_number = 5;
      }
      else if (number_of_devices == 2)
      {
        halLcdWriteLines("The waveform is", "HP Laptop + FAN", "Quantity 2"); 
        waveform_number = 6;
      }
    }
    else if(device_number == 3)
    {
      if (number_of_devices == 1)
        halLcdWriteLines("The waveform is", " ", "Quantity 1");
      else if (number_of_devices == 2)
        halLcdWriteLines("The waveform is", " ", "Quantity 2");
    }
  }
}

static void appTransmitter(int8 DeviceNumber)
{
   
#ifdef ASSY_EXP4618_CC2420
    halLcdClearLine(1);
    halLcdWriteSymbol(HAL_LCD_SYMBOL_TX, 1);
#endif

    pTxData[0] = DeviceNumber;
       // Initialize BasicRF
    basicRfConfig.myAddr = SWITCH_ADDR;
    if(basicRfInit(&basicRfConfig)==FAILED) 
    {HAL_ASSERT(FALSE);}

            basicRfReceiveOff();
            basicRfSendPacket(LIGHT_ADDR1, pTxData, APP_PAYLOAD_LENGTH);
            // Put MCU to sleep. It will wake up on joystick interrupt
}