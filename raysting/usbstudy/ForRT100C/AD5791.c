/********************************************************************************
 Author : CAC (China Applications Support Team) 

 Date :   January, 2012

 File name :   AD5791.c

 Description :	 Use the GPIO to simulate the SPI communication of AD5791

 Hardware plateform : 	AD5791 and AD5791
********************************************************************************/

//#include "AD5791.h"
//#include "AD5791Driver.h"
#include "AD5791.h"
#include "stdio.h"
extern void swiDelay(unsigned char dl1,unsigned char dl2);

xdata unsigned long int AD5791Registers[5]={0,0,0,0,0};		// only low 24 bits are used for each register

unsigned char *dacbuf;
unsigned char *clrbuf;

unsigned char ch_dummy;

#define AD5791OutputBit(A,B)	A=B;

#define AD5791DelayUs(i)	swiDelay(1,i);

#define AD5791InputBit(A)  (A) 

unsigned char AD5791SpiRead(unsigned char WriteData)
{
	 unsigned   char   ReadData;
     unsigned   char  j;

  

      ReadData = 0;

      for(j=0; j<8; j++)
      {
         AD5791OutputBit(SCLK,1);

         if(0x80 == (WriteData & 0x80))
         {
            AD5791OutputBit(SDI,1);     //Send one to SDI pin
         }
         else
         {
            AD5791OutputBit(SDI,0);     //Send zero to SDI pin
         }
         WriteData = WriteData << 1;
         ReadData = (ReadData<<1) | AD5791InputBit(SDO); 

         AD5791OutputBit(SCLK,0);   /////

      }
      return ReadData;
}
void AD5791SpiWrite(unsigned char WriteData)
{
     unsigned   char   j;


      for(j=0; j<8; j++)
      {
         AD5791OutputBit(SCLK,1);

         if(0x80 == (WriteData & 0x80))
         {
            AD5791OutputBit(SDI,1);     //Send one to SDI pin
         }
         else
         {
            AD5791OutputBit(SDI,0);     //Send zero to SDI pin
         }
         WriteData = WriteData << 1;

         AD5791OutputBit(SCLK,0);   /////
      }
}
   // clear code is set to 0x0007ffff; since we enabled buffer function
#define MYCLR_CODE 	0x0007ffff
#define CONFIG_MODE	RBUF_ON|OPGND_DIS|DACTRI_DIS|BIN_2SC_BIN|SDO_EN|LIN_COMP_10

void AD5791Init()
{
   /* initialize the GPIOs for controlling the AD5791*/
   AD5791OutputBit(LDAC, 1);
   AD5791OutputBit(SYNC, 1);
   AD5791OutputBit(CLR, 1);
   AD5791OutputBit(SCLK, 1);
   AD5791OutputBit(SDI, 1);
   ch_dummy = AD5791InputBit(SDO);


	dacbuf = (unsigned char*)(AD5791Registers + DAC_VALUE);
	clrbuf = (unsigned char*)(AD5791Registers + CLEARCODE);

	AD5791Initialization();
	swiDelay(0x0f,0xff);

	AD5791Registers[CONTROL] = CONFIG_MODE;
	WriteToAD5791ViaSPI(CONTROL, AD5791Registers);

	AD5791Registers[CLEARCODE] = MYCLR_CODE;	
	WriteToAD5791ViaSPI(CLEARCODE, AD5791Registers);

	AD5791Registers[DAC_VALUE] = MYCLR_CODE;	
	WriteToAD5791ViaSPI(DAC_VALUE, AD5791Registers);
	AD5791HardwareLoad();	
	AD5791Registers[CONTROL] = 0;
	AD5791Registers[CLEARCODE] = MYCLR_CODE;

}

void AD5791HardwareReset()
{
	AD5791OutputBit(RESET,0);
	AD5791OutputBit(RESET,1);
	AD5791DelayUs(10);
}

void AD5791HardwareClear()
{
	AD5791OutputBit(CLR,0);
	AD5791OutputBit(CLR,1);
}

void AD5791HardwareLoad()
{
	AD5791OutputBit(LDAC,0);
	AD5791OutputBit(LDAC,1);
}

void AD5791Initialization()
{
	AD5791HardwareReset();
	AD5791HardwareClear();
}

void WriteToAD5791ViaSPI(unsigned char RegisterStartAddress, unsigned long int *DataBuffer)
{
	unsigned char  WriteData;
	

   AD5791OutputBit(SYNC,0);
   AD5791OutputBit(SCLK,1); 

		WriteData = ((RegisterStartAddress)<<4) | (DataBuffer[RegisterStartAddress]>>16);
		AD5791SpiWrite(WriteData);
		WriteData = DataBuffer[RegisterStartAddress]>>8;
		AD5791SpiWrite(WriteData);
		WriteData = DataBuffer[RegisterStartAddress];
		AD5791SpiWrite(WriteData);

   AD5791OutputBit(SCLK,1);
   AD5791OutputBit(SYNC,1);
}

void ReadFromAD5791ViaSPI(const unsigned char RegisterStartAddress,unsigned char *DataBuffer)
{
	unsigned char  WriteData;

   	AD5791OutputBit(SYNC,0);
	AD5791OutputBit(SCLK,1); 

	WriteData = 0x80 | ((RegisterStartAddress)<<4) ;	
	AD5791SpiWrite(WriteData);
	AD5791SpiWrite(NOP);
	AD5791SpiWrite(NOP);
	AD5791OutputBit(SCLK,1);
   	AD5791OutputBit(SYNC,1);

	AD5791OutputBit(SCLK,0);
   	AD5791OutputBit(SYNC,0);

	DataBuffer[0] = 0x00;
	DataBuffer[1] = 0x0f & AD5791SpiRead(NOP);
	DataBuffer[2] = AD5791SpiRead(NOP);
	DataBuffer[3] = AD5791SpiRead(NOP);

   	AD5791OutputBit(SYNC,1);
	AD5791OutputBit(SCLK,1);

}

void cm_ad5791(unsigned char temp)
{
					dacbuf[0] = USBGetChar();		
					dacbuf[1] = USBGetChar();		
					dacbuf[0] += dacbuf[1];
					dacbuf[2] = USBGetChar();		
					dacbuf[0] += dacbuf[2];
					dacbuf[3] = USBGetChar();		
					dacbuf[0] += dacbuf[3];
					temp = USBGetChar();
					dacbuf[0] +=  temp;
					if(dacbuf[0] == 0x00)
					{
						if(temp == 0x01)
						{
							WriteToAD5791ViaSPI(DAC_VALUE, AD5791Registers);
							AD5791HardwareLoad();
							
							swiDelay(0x0f,0xff);
							AD5791Registers[CLEARCODE] = 0;
							ReadFromAD5791ViaSPI(DAC_VALUE, clrbuf);
							temp = 0;
							if(AD5791Registers[CLEARCODE] == AD5791Registers[DAC_VALUE])
								temp = 1;
						}
						else if(temp == 0x02)
						{
							AD5791HardwareClear();
							temp = 1;
						}
						else if(temp == 0x03)
						{
							AD5791HardwareReset();
							temp = 1;
						}
						else if(temp == 0x04)
						{
							AD5791Registers[CONTROL] = CONFIG_MODE;
							WriteToAD5791ViaSPI(CONTROL, AD5791Registers);

							AD5791Registers[CLEARCODE] = MYCLR_CODE;	// clear code is set to 0
							WriteToAD5791ViaSPI(CLEARCODE, AD5791Registers);

							AD5791Registers[CONTROL] = 0;
							AD5791Registers[CLEARCODE] = MYCLR_CODE;
							temp = 1;
						}
						else if(temp == 0x05)
						{
							AD5791Registers[CLEARCODE] = 0;
							ReadFromAD5791ViaSPI(CONTROL, clrbuf);
							sjSerialSendByte(clrbuf[0]);
							sjSerialSendByte(clrbuf[1]);
							sjSerialSendByte(clrbuf[2]);
							sjSerialSendByte(clrbuf[3]);

							AD5791Registers[CLEARCODE] = 0;
							ReadFromAD5791ViaSPI(DAC_VALUE, clrbuf);
							sjSerialSendByte(clrbuf[0]);
							sjSerialSendByte(clrbuf[1]);
							sjSerialSendByte(clrbuf[2]);
							sjSerialSendByte(clrbuf[3]);

							temp = 1;
						}
						else{
							temp = 0;
						}
					}else{
						temp = 0;
					}

					sjSerialSendByte(HMARK);
					if(temp > 0) //success
					{
						sjSerialSendByte(0);
						sjSerialSendByte(0);
					}else{
						sjSerialSendByte(1);
						sjSerialSendByte(1);
					}
					sjSerialSendByte(EMARK);

}
