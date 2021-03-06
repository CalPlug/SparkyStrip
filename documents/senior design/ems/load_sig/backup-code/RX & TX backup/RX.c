//RECEIVER
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


static void UartOutout(void);
static void appConfigTimer(uint16 rate);
static void appTimerISR(void);
static void appReceiver();
static volatile uint8 appUartRxIdle;
static void UartOutout(void);
static void appConfigTimer(uint16 rate);
static void appTimerISR(void);
static uint8 pTxData[APP_PAYLOAD_LENGTH] ;
static uint8 pRxData[APP_PAYLOAD_LENGTH]; 
static basicRfCfg_t basicRfConfig;
static uint8 array[1] = {0};


// MAIN FUNCTION: CODE STARTS HERE!
void main(void)
{
    // Initialise board peripherals
    halBoardInit();    
    // Config basicRF
    basicRfConfig.panId = PAN_ID;
    basicRfConfig.channel = RF_CHANNEL;
    basicRfConfig.ackRequest = TRUE;
   
      
    // Wait for user to press S1 to enter menu
    while(1)
    {
      
      halLcdWriteLines("Ready ", " ", " ");  
      appReceiver();      
      halUartInit(HAL_UART_BAUDRATE_38400, 0);
      appConfigTimer(1000/UART_RX_IDLE_TIME); // 100 ms RX idle timeout
      UartOutout();
      halLcdWriteLines("Received", " ", " ");
      halMcuWaitMs(1000);
     
      
    }
}
/**************Definition of the functions************************************
******************************************************************************/



static void UartOutout(void)
{
  for(int i = 0; i < 20; i++)   
  {
    
  halUartWrite(pRxData, 1);
  halMcuWaitMs(100);
  }
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

static void appReceiver()
{
    halLcdWriteLine(HAL_LCD_LINE_1, "Light");
    halLcdWriteLine(HAL_LCD_LINE_2, "Ready");
#ifdef ASSY_EXP4618_CC2420
    halLcdClearLine(1);
    halLcdWriteSymbol(HAL_LCD_SYMBOL_RX, 1);
#endif

    // Initialize BasicRF
    basicRfConfig.myAddr = LIGHT_ADDR1;
    if(basicRfInit(&basicRfConfig)==FAILED) {
      HAL_ASSERT(FALSE);
    }
    basicRfReceiveOn();

    // Main loop
    
        while(!basicRfPacketIsReady());

        if(basicRfReceive(pRxData, APP_PAYLOAD_LENGTH, NULL)>0) {
            if(pRxData[0] == LIGHT_TOGGLE_CMD) {
                halLedToggle(1);
            }
	    if(pRxData[0] == 1) 
            {halLedToggle(2);}
        }
        array[0] = pRxData[0];
    
    
}
