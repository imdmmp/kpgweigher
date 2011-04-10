/******************************************************************/
// CS5532 driver           
#include "define.h"
#include "spi.h"
#include "cs5532.h"
#include <MEGA16.h> 
#include "uart.h"
#include "drveeprom.h" 
/******************************************************************/

/******************************************************************/
//                      CS5532 Reset      
// Set RS bit in configuration reg to "1" to trigger reset, then 
// change it back to "0" 
// Return 0x10 if reset is successful. Return 0x0 if reset
// failed.
/******************************************************************/ 
u32 CS5532_Reset(void)
{ 
  // Set RS in configuration register to "1" to reset CS5532. 
  // First to write configuration register command
  SPI_MasterTransmit_c(CMD_WRITE_CFG_REG);     
  
  // write data to configuration register, 32bits (4 bytes)
  SPI_MasterTransmit_l(0x20000000); 
  
  sleepms(1);
  
  //change RS in configuration register back to "0"
  // First to write configuration register command
  SPI_MasterTransmit_c(CMD_WRITE_CFG_REG);
  // write data to configuration register, 32bits (4 bytes)
  SPI_MasterTransmit_l(0x00000000);  // Set RS to 0
  
  //check if reset is successful(RV "1") or not.
  //read configuration register, send command first
  SPI_MasterTransmit_c(CMD_READ_CFG_REG);
  // read 4-bytes from configuration register. MSB first.
  return SPI_MasterReceive_l();
}                        

/******************************************************************/
//                      CS5532 Initilization      
// At least 15 SYNC1 followed by SYNC0
// Call subroutine CS5532_Reset() to reset CS5532
/******************************************************************/
u32 CS5532_Init(void)
{
  u8 i;
  for (i=18; i>0; i--)
        SPI_MasterTransmit_c(SYNC1);
  SPI_MasterTransmit_c(SYNC0);
  // reset CS5532
  return CS5532_Reset();
}   

/******************************************************************/
//                      CS5532 Configuration      
// Configuration register is 32-bit long and only D31~D19 are used
// RS(D29): 0, normal operation; 1, reset cycle
// RV(D28): 0, normal operation; 1, reset valid
// IS(D27): 0, normal input; 1, input pairs are short internally
// OGS(D20): 0, cal Regs are used based on CS1-CS0; 1, OG1-OG0
// Filter Rate Select(D19): 0, 60Hz filter; 1, 50Hz filter
// Keep all other bits as default "0"
// Argument examples:
// cfgh: 0x20  --- Reset
// cfgl: 0x8   --- 50 HZ filter
// cfgl: 0x0   --- 60 HZ filter
// cfgh: 0x8,  --- Input short
// For CS5530, Bit D14~D11 are word rate config bits. These bits are
// reserved bits and should be set to "0" if AD is CS5532.
// D10 is the unipolar or bipolar input option bit, only for CS5530
// should be clear to zero if CS5532 is used.
/******************************************************************/
void CS5532_Config(u16 cfg_hw, u16 cfg_lw)
{ 
  // First to write configuration register command
  SPI_MasterTransmit_c(CMD_WRITE_CFG_REG);
  SPI_MasterTransmit_w(cfg_hw); 
  SPI_MasterTransmit_w(cfg_lw);
}
 
/******************************************************************/
//            CS5532 continous conversion
// Initialize CS5532 to continous conversion mode. 
// SDO of CS5532 falls to indicate completion of a conversion cycle.
// To make code work well on CS5530 which only have setup1, we force
// setup setting to SETUP1.
/******************************************************************/
void CS5532_Cont_Conversion(void)
{ 
  SPI_MasterTransmit_c(CMD_CONTINUE_CONV_SETUP1);  
}

/******************************************************************/
//            CS5532 single conversion
// Initialize CS5532 to single conversion mode. 
// SDO of CS5532 falls to indicate completion of a conversion cycle.
/******************************************************************
void CS5532_Single_Conversion(void)
{
  SPI_MasterTransmit_c(CMD_SINGLE_CONV_SETUP1); 
}        

/******************************************************************/
//            CS5532 conversion result readout
// ADC conversion result readout
// SDO of CS5532 falls to indicate completion of a conversion cycle
// read conversion result when is low.
// return SUCCESSFUL(0x0) when data is read. return TIMEOUTERR(0xff)
// if timeout error occurs. 
// Conversion result is returned via pointer parameter.
/******************************************************************/
u8 CS5532_ReadADC(u8 *ConvDataBuf)
{ 
  //long int timeout = CS5532_READ_DATA_TIMEOUT;
  unsigned char i;
  unsigned char *pConvDataBuf;
  pConvDataBuf = ConvDataBuf; 
   
  // return busy flag: data not available
  if(PORTB.P_MISO == 1)
      return(0xff);
  
  // first 8 clock to clear SDO flag;
   SPI_MasterTransmit_c(NULL);

  // read conversion result
  for(i=4;i>0;i--)
    *pConvDataBuf++ = SPI_MasterReceive_c();
    
  return(0x0);
}

/******************************************************************/
//            CS5532 Manual Offset Calibration
// mannually write a data to offset register to do offset cal. 
// This function calculates data to be written into offset reg based
// on input arg (CS5532 output) so that CS5532 output can be initialized
// to zero. 
// Offset data ~= CS5532_Output * (1/4 + 1/64 + 1/128)
// 1 LSB represents 1.83007 of (-24) order of 2. 
// 1/(1.8300*2) ~ = 1/4 + 1/64 + 1/128
/******************************************************************/
void CS5532_Manual_Offset_Gain_Cal(u8 *AD_Output, u8 gain)
{
  u8 *ptr;
  u32 offset, temp1;   // temp2, temp3;
  
  //convert array data(CS5532 output) into a u32 type data
  ptr = AD_Output;
  offset = 0;
  temp1 = *ptr++;
  offset = temp1 << 24;
  temp1 = *ptr++;
  offset += temp1 << 16; 
  temp1 = *ptr++;
  offset += temp1 << 8; 
  offset += *ptr;

  //write offset data to offset register
  SPI_MasterTransmit_c(CMD_WRITE_OFFSET_REG1);
  SPI_MasterTransmit_l(offset); 
  
  //write gain settings to gain register
  SPI_MasterTransmit_c(CMD_WRITE_GAIN_REG1);
  SPI_MasterTransmit_c(1<<gain); 
  SPI_MasterTransmit_c(0); 
  SPI_MasterTransmit_c(0); 
  SPI_MasterTransmit_c(0); 
  
}

/******************************************************************/
// stop CS5532 continous conversion mode so that it can receive
// new command.
/******************************************************************/
u8 CS5532_Cont_Conv_Stop()
{ 
   if(PORTB.P_MISO == 1)
      return(0xff);
   //stop continous converson mode
   SPI_MasterTransmit_c(CMD_STOP_CONT_CONV);
   //clock out the 32 bits before exit
   SPI_MasterReceive_l();      
   return(0x0);   
}

/******************************************************************/
//            CS5532 system offset calibration
// Offset calibration on CS5532.
// return TIMEOUTERR (0xff) if timeout error occurs.
// return SUCCESSFUL (0x0) if operation is successful.
/******************************************************************/ 
/*u8 CS5532_SysOffsetCal(void)
{
  u16 timeout;

#ifdef _CS5532_DISPLAY_OFFSET_
  u8 data[4];
#endif  
  
 // perform offset calibration
 SPI_MasterTransmit_c(CMD_SYSTEM_OFFSET_CAL_SETUP1);
 // wait till calibration completes or timeout
 timeout = CAL_TIMEOUT_LIMIT;
 while(PORTB.P_MISO == 1)
 {
    if (timeout-- == 0)
      return(TIMEOUTERR);
 }
 
#ifdef _CS5532_DISPLAY_OFFSET_
  //read back offset register 
  SPI_MasterTransmit_c(CMD_READ_OFFSET_REG1);
  data[0] = SPI_MasterReceive_c(); 
  data[1] = SPI_MasterReceive_c(); 
  data[2] = SPI_MasterReceive_c(); 
  data[3] = SPI_MasterReceive_c();   
  
  //display through RS485 port
  sleepms(UART_DELAY);
  putchar(data[0]);   
  sleepms(UART_DELAY);
  putchar(data[1]);   
  sleepms(UART_DELAY);
  putchar(data[2]);   
  sleepms(UART_DELAY);
  putchar(data[3]);
  sleepms(UART_DELAY);  
#endif   
 
  return(SUCCESSFUL);   
} //*/

/******************************************************************/
//            CS5532 system Gain calibration
// Gain calibration on CS5532 
// return TIMEOUTERR (0xff) if timeout error occurs.
// return SUCCESSFUL (0x0) if operation is successful.
/******************************************************************/
/*u8 CS5532_SysGainCal(void)
{
 u8 timeout;

#ifdef _CS5532_DISPLAY_GAIN_
 u8 data[4];
#endif   

 // gain calibration
 SPI_MasterTransmit_c(CMD_SYSTEM_GAIN_CAL_SETUP1);
 // wait till calibration completes or timeout
 timeout = CAL_TIMEOUT_LIMIT;
 while(PORTB.P_MISO == 1)
  { 
    if (timeout-- == 0)
      return(TIMEOUTERR);
  } 
 
#ifdef _CS5532_DISPLAY_GAIN_
  //read back offset register 
  SPI_MasterTransmit_c(CMD_READ_GAIN_REG1);
  data[0] = SPI_MasterReceive_c(); 
  data[1] = SPI_MasterReceive_c(); 
  data[2] = SPI_MasterReceive_c(); 
  data[3] = SPI_MasterReceive_c(); 
  
  sleepms(UART_DELAY);
  putchar(data[0]);   
  sleepms(UART_DELAY);
  putchar(data[1]);   
  sleepms(UART_DELAY);
  putchar(data[2]);   
  sleepms(UART_DELAY);
  putchar(data[3]);
  sleepms(UART_DELAY);  
#endif 
  
 return(SUCCESSFUL);  
}  //*/

/******************************************************************/
//            Read CS5532 Offset/Gain calibration result
// Read CS5532 offset/gain calibration result and generate checksum.
// Result is read back into a buffer pointed by a pinter parametric.
// Buffer size is at least 11 bytes long. Result will be stored into
// EEPROM later.
/******************************************************************
 //void CS5532_ReadCal(unsigned char *CalBuf) 
 void CS5532_ReadCal(unsigned int CalBuf)
 {
   unsigned char i;
   unsigned char checksum;
   unsigned char *pCalBuf;
   pCalBuf = CalBuf;
   checksum = 0;
   
   //read gain calibration result and save data into buffer
   SPI_MasterTransmit(CMD_READ_GAIN_REG1);
   *pCalBuf++ = 'G';
   for(i=4;i>0;i--)
     *pCalBuf++=SPI_MasterTransmit(NULL);  

   //read offset calibration result and save data into buffer
   SPI_MasterTransmit(CMD_READ_OFFSET_REG1);
   *pCalBuf++= 'O';
   for(i=4;i>0;i--)
      *pCalBuf++=SPI_MasterTransmit(NULL);
  
   //generate checksum byte
   pCalBuf=pCalBuf-10;
   for(i=10;i>0;i--)
       checksum=checksum+*pCalBuf++;
   // checksum byte
   *pCalBuf = ~checksum + 1;      
  } //*/

/******************************************************************/
//             Set CS5532 OFFSET/GAIN Registers 
// This function allows users to set offset cal value manually
// Arg 1: offset_data,  points to a 4-byte data array
// Arg 2: 'O' to set OFFSET register, 'G' to set Gain register.
// return 0x0 if set offset register successfully
// return 0xff if fails to set offset register.
/******************************************************************/
/*u8 CS5532_Set_Offset_Gain(u8 *setting, char reg)
{
  u8 *ptr;
  u8 i, cFlag;

  //Write offset data to OFFSET register
  ptr = setting;
  if ((reg != 'O') && (reg != 'G'))
      return(0xff);  
  if(reg =='O')  
    SPI_MasterTransmit_c(CMD_WRITE_OFFSET_REG1);
  else 
    SPI_MasterTransmit_c(CMD_WRITE_GAIN_REG1); 
  for(i=4;i>0;i--)
     SPI_MasterTransmit_c(*ptr++);
      
  // Verify whether setting is successfully done.
  cFlag = 0x0;
  ptr = setting;
  if(reg =='O')
    SPI_MasterTransmit_c(CMD_READ_OFFSET_REG1);
  else
    SPI_MasterTransmit_c(CMD_READ_GAIN_REG1);
  for(i=4;i>0;i--)
  { if(SPI_MasterReceive_c() != *ptr++)
       cFlag = 0xFF;
  } 
  return(cFlag);           
} //*/

/********************************************************************************/ 
// fill data buffer and get average 
// return AD_BUSY if A/D conversion is not completed yet.
// return AD_OVER_FLOW if A/D conversion overflow happens.
// otherwise return average of multiple AD samples.
/********************************************************************************/ 
u16 read_ad_data()
{
   u16 raw_data, temp;
   u8 ConvTempbuf[4]; 
   
   if(CS5532_ReadADC(ConvTempbuf) != SUCCESSFUL)                      
      return(AD_BUSY);
      
   // LSB byte of the 32 bits is monitor byte, 
   if (ConvTempbuf[3] != 0) { 
       RS485._global.diag_status1 |= 0x20;         // set bit 5 error flag
       return(AD_OVER_FLOW);                       
   }
   else{  
       RS485._global.diag_status1 &= 0xDF;         // clear error flag 
       temp = ConvTempbuf[0];                      // xianghua                   
       raw_data = temp << 8; 
       raw_data += ConvTempbuf[1];                 
   }   
   return(raw_data);
} 

/********************************************************************************/     
// software filter on material weight  
/********************************************************************************/     
#define AVERAGE_NUM 64  //average buffer size, must be times of 8.  xianghua 64

void CS5532_PoiseWeight()
{       
   static u16 Raw_DataBuf[AVERAGE_NUM];        // data buffer contains raw data from A/D
   static u8 cur_data_index, data_counts;      // variables associated to Raw_DataBuf
   static u8 raw_data_index; 
   
   u16 SortBuf[AVERAGE_NUM];     
   u16 temp, temp_latest, delta; 
   u32 sum;
   u8 i, j, filter_size;
        
   /*************************************************************/
   // read a data sample
   /*************************************************************/    
   temp = read_ad_data();       
   //if((temp == AD_BUSY) || (temp == AD_OVER_FLOW)) 
   {
      RS485._global.cs_mtrl = temp;            // invalidate "cs_mtrl"
      return;                                  // return if data is invalid
   }
             
   /*************************************************************/
   // calculate sample size for medium filtering
   /*************************************************************/   
   filter_size = 6;
   filter_size = filter_size << 3;             // size of data: 8 ~ 64, must be times of 8
   /*************************************************************/
   // push data to queue, report current A/D reading directly if data 
   // size has not reached required data size for filtering. 
   /*************************************************************/ 
   Raw_DataBuf[cur_data_index++] = temp;       
   if(cur_data_index > filter_size-1)
     cur_data_index = 0;              
   if(data_counts < filter_size)               
   {  data_counts++;  
      RS485._global.cs_mtrl = temp;            // skip filter if data sample size
      return;                                  // is not big enough.  
   }   
   /*************************************************************/
   // Medium Value Filtering
   // First copy data to sort buffer and sort data (Min in Buf[0])
   /*************************************************************/
   for(j=0; j<filter_size; j++)
      SortBuf[j] = Raw_DataBuf[j];             
   for(i=0; i<(filter_size>>1); i++)            // sort data buffer. min at buf[0].
   {  for(j=0; j < filter_size-1-i; j++)
      {  
         if(SortBuf[j] > SortBuf[j+1])         // swap data, max to higher address
         {  temp = SortBuf[j+1];
            SortBuf[j+1] = SortBuf[j];
            SortBuf[j] = temp;            
         }                                       
      }      
   }      
   /*********************************************************/
   // Medium Filtering, excludes high end (1/8 of queue size)
   // and low end data (1/8 of queue size)
   // Calculate average over medium parts (3/4 of queue size).
   /*********************************************************/   
   sum = 0;
   i = filter_size >> 3; 
   for(j=i;j<filter_size-i; j++)
      sum+= SortBuf[j];
   i = filter_size - (filter_size >>2);
   temp = (sum/i) & 0xFFFF;
   /*************************************************************/
   // Calculate average of latest 8 data sampes as an indicator
   // of remarkable data change. 
   /*************************************************************/   
   sum = 0;
   for(i=0; i<8; i++)
   {  if(cur_data_index>i)                      
        j = cur_data_index-1- i;
      else
        j = filter_size - 1 - i + cur_data_index; 
      sum+= Raw_DataBuf[j];   
   }
   temp_latest = (sum >> 3) & 0xFFFF;
   /*************************************************************/
   // weight-based average
   // temp1 is the calculation over latest 16 data samples
   // temp is the calculation over latest 32 data samples
   // if the delta between temp1 and temp is greater than 2g(
   // 50 * 0.04g, assume delta caused by noise is less than 1g)
   // it is very likely that material feeding or releasing starts,
   // so we only use the latest 16 data samples. 
   /*************************************************************/   
   if(temp_latest>temp)
      delta = temp_latest - temp;
   else
      delta = temp - temp_latest; 
      
   if(delta<32)
      sum = temp;
   else if(delta>64) 
   {   sum = temp_latest; 
       data_counts = 0;                      // invalidate data queue. 
   }
   else
   {  sum = temp;
      sum+= temp_latest;  
      sum = sum >>1; 
   }
   
   RS485._global.cs_mtrl = sum & 0xFFFF;        
}                                                                   

/*****************************************************************************/
//              Convert CS5532 output to gram  
// Formula to transfer CS5532 measurement to actual weight��
//   Mtrl_Weight_gram      (cs_mtrl  - cs_zero)        temp1
// ------------------  = --------------------------- = ------
//   Poise_Weight_gram     (cs_poise - cs_zero)        temp2
// note: to minimize non-lineary of A/D, 5 poises can be used for calibriation
// The one which is most close to cs_Mtrl will be found and used. 
// If Bit 7 of test_mode_reg1 is cleared, multi poises are used.
// "old_cs_zero" is used to monitor change of caliration data.
// if "old_cs_zero" is not equal to "cs_zero", "cs_poise" will be adjusted
// accordingly. 
// There could be such a confliction as below:
// "cs_zero" is changed by user, then node board software detect the change
// and start to adjust "cs_poise" automatically while at the same time
// user starts to change "cs_poise" (calibration) at the same time. Confliction!!!
// However because MCU speed to update cs_poise is much faster than user's 
// operations on PC, this case will not happen.
/***************************************************************************/
void CS5532_Poise2Result()
{  
   u8 min_delta_index;     // index of poise with min delta from material weight.
   u16 temp2;
   u32 temp1,temp3, temp4;
   u16 delta, min_delta;
   u8 i;             

   // check if error happened in CS5532_PoiseWeight()
   if(RS485._global.cs_mtrl > MAX_VALID_DATA)
   { // result is invalid. 
     // pass fail code to variable Weight_gram, which will be read by master board.
     RS485._global.Mtrl_Weight_gram =  RS485._global.cs_mtrl;                
     return;      
   }       
  
   if(RS485._global.cs_mtrl < RS485._flash.cs_zero)
      temp1 = 0;
   else
      temp1 = RS485._global.cs_mtrl - RS485._flash.cs_zero;                       

   /***************************************************************/
   //adjust cs_poise based on comparison between cs_zero and old_cs_zero
   if(RS485._flash.cs_zero > RS485._global.old_cs_zero)
   {
      delta = RS485._flash.cs_zero - RS485._global.old_cs_zero; 
      for(i=0;i<5;i++)
         RS485._flash.cs_poise[i] += delta;
   }
   if(RS485._flash.cs_zero < RS485._global.old_cs_zero)
   {
      delta = RS485._global.old_cs_zero - RS485._flash.cs_zero; 
      for(i=0;i<5;i++)
         RS485._flash.cs_poise[i] -= delta;   
   }   
   RS485._global.old_cs_zero = RS485._flash.cs_zero;   
   
   /***************************************************************/
   // Search the poise which is most close to cs_mtrl
   if(!ENABLE_MULTI_POISES) 
      min_delta_index = 0;
   else
   {  min_delta = 0xffff;
      for(i=0;i<5;i++)
      { 
         if(RS485._global.cs_mtrl > RS485._flash.cs_poise[i])
            delta =  RS485._global.cs_mtrl -  RS485._flash.cs_poise[i];
         else
            delta =  RS485._flash.cs_poise[i] - RS485._global.cs_mtrl;      
         if (delta < min_delta)
         {   min_delta = delta ; min_delta_index = i; }      
      }
   }          

   /***************************************************************/        
   if(RS485._flash.cs_poise[min_delta_index] < RS485._flash.cs_zero)
      temp2 = 0;
   else
      temp2 = RS485._flash.cs_poise[min_delta_index] - RS485._flash.cs_zero;  
        
   // compiler default: u16*u16 = u16       
   // so we use u16 * u32 = u32 here
   temp3 = RS485._flash.Poise_Weight_gram[min_delta_index] * temp1;                                                         
   temp3 = temp3 << 6;   // X64. so unity is 1/64g      	
        
   if(temp2 != 0)
   {
      temp4 =  temp3/temp2;
      RS485._global.Mtrl_Weight_gram = (temp4 >> 6);
      if(RS485._global.Mtrl_Weight_gram==0xffff)
          RS485._global.Mtrl_Weight_gram= OVERWEIGHT;
      RS485._global.Mtrl_Weight_decimal = temp4 & 0x3F;
      RS485._global.diag_status1 &= 0xBF;          // clear bit 6 error flag      
   }   
   else
   {  RS485._global.Mtrl_Weight_gram =  DIV_ERROR; // indicate bad calibration
      RS485._global.diag_status1 |= 0x40;          // set bit 6 error flag
   }                           
   // master board won't find this node if Mtrl_Weight_gram is always 0xffff.
   // Bad calibartion data will make Mtrl_Weight_gram always bad.
   // so we set Mtrl_Weight_gram to a different data to indicate calibration error.
   /***************************************************************/                      
   // if bit2 of test_mode_reg1 is set, AD output is sent to master board / PC.
   if(DISPLAY_AD_RAW_DATA)
        RS485._global.Mtrl_Weight_gram =  RS485._global.cs_mtrl;
   return;
}
