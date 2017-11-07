// Current

/***********************************************************************************
* INCLUDES
*/
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

#define UART_RX_IDLE_TIME       100     // 100 ms
#define N_RETRIES                 5     // Number of transmission retries
#define ADC_BUFFER_SIZE 1
#define DELAY_TIME 200
#define ARRAY_BUFFER_LENGTH 1
#define OFFSET -5843      
#define SAMPLING_FREQUENCY 5000
#define FREQUENCY 60
#define PI 3.141592

#define NUMBER_DEVICES 4
#define NUMBER_PARAMETERS 4
#define LOWER 0
#define UPPER 1

static uint8 adcBuffer[ADC_BUFFER_SIZE];

static float dft180(int* buffer, uint16 buffer_size);
static void startADCConv(void);
static int getMinValue(int* buffer, uint16 buffer_size);
static int getMaxValue(int* buffer, uint16 buffer_size);
static int getRiseTime(int* buffer, uint16 buffer_size);
static float dft300(int* buffer, uint16 buffer_size);
//static void initializeDatabase(void);
//static void determineWaveform(int* waveform_parameters);
static void UartOutout(void);
static void appConfigTimer(uint16 rate);
static void appTimerISR(void);

static volatile uint8 appUartRxIdle;

// Input Buffer
uint8 array[ARRAY_BUFFER_LENGTH];
    
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


// MAIN FUNCTION: CODE STARTS HERE!
void main(void)
{
    // Initialise board peripherals
    halBoardInit();    
    halUartInit(HAL_UART_BAUDRATE_38400, 0);
    appConfigTimer(1000/UART_RX_IDLE_TIME); // 100 ms RX idle timeout
    halTimer32kIntEnable();
    halLedSet(1);    // Indicate that device is powered
    halLcdClear();

    while(1)
    {
      
      halLcdWriteLines("Send Value 0", " ", " "); // Outputs on the Screen for User to read
      while (halButtonPushed()!=HAL_BUTTON_1);
      halMcuWaitMs(350);
      halLcdClear();
  
      
      array[0] = (uint8) 0;
      UartOutout();
      halLcdWriteLines("Sent!!", " ", " ");
      while (halButtonPushed()!=HAL_BUTTON_1);
     }
     
      while (halButtonPushed()!=HAL_BUTTON_1);
    
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

static void UartOutout(void)
{
     halUartWrite(array, ARRAY_BUFFER_LENGTH);
}

static void appConfigTimer(uint16 rate)
{
    halTimer32kInit(TIMER_32K_CLK_FREQ/rate);
    halTimer32kIntConnect(&appTimerISR);
}
static void appTimerISR(void)
{
    appUartRxIdle = TRUE;
}
