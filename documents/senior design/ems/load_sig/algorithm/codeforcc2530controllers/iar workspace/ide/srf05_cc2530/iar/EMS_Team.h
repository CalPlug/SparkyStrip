/***********************************************************************************
  Filename:     EMS_Team.h

  Description:  EMS-Team library header file

***********************************************************************************/

#ifndef EMS_TEAM_H
#define EMS_TEAM_H

#ifdef __cplusplus
extern "C" {
#endif
 
/***********************************************************************************
 * GLOBAL FUNCTIONS
 ************************************/

void initializeDatabase(void);
void startADCConv(void);  
int getMaxValue(int* buffer, uint16 buffer_size);
int getMinValue(int* buffer, uint16 buffer_size);
int getRiseTime(int* buffer, uint16 buffer_size, uint16 sample_freq, unint16 freq);  
float dft300(int* buffer, uint16 buffer_size);
float dft180(int* buffer, uint16 buffer_size);
  
  
  
  
  
  
#ifdef  __cplusplus
}
#endif

/**********************************************************************************/
#endif