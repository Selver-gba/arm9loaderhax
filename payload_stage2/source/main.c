#include "common.h"
#include "sdmmc.h"
#include "i2c.h"
#include "fatfs/ff.h"

#define PAYLOAD_ADDRESS		((void*)0x23F00000)
#define PAYLOAD_FUNCTION    ((void (*)())PAYLOAD_ADDRESS)
#define PAYLOAD_SIZE		((UINT)0x00100000)

u8 arm11code[] = {
	0x3E, 0x02, 0xE0, 0xE3, 0x1C, 0x10, 0x9F, 0xE5, 
	0x00, 0x10, 0x80, 0xE5, 0x7E, 0x02, 0xE0, 0xE3, 
	0x00, 0x10, 0xA0, 0xE3, 0x00, 0x10, 0x80, 0xE5, 
	0x00, 0x20, 0x90, 0xE5, 0x02, 0x00, 0x51, 0xE1, 
	0xFC, 0xFF, 0xFF, 0x0A, 0x12, 0xFF, 0x2F, 0xE1, 
	0xBE, 0xBA, 0xAD, 0xAB,
};

void ownArm11()
{
	memcpy((void*)0x1FFF4C80, arm11code, sizeof(arm11code));
	*((u32*)0x1FFAED80) = 0xE51FF004;
	*((u32*)0x1FFAED84) = 0x1FFF4C80;
	for(int i = 0; i < 0x80000; i++)
	{
		*((u8*)0x1FFFFFF0) = 2;
	}
}

int main()
{
	FATFS fs;
	FIL payload;
	UINT bytesRead;
	
	if(f_mount(&fs, "0:", 0) == FR_OK)
	{
		if(f_open(&payload, "arm9loaderhax.bin", FA_READ | FA_OPEN_EXISTING) == FR_OK)
		{
			f_read(&payload, PAYLOAD_ADDRESS, PAYLOAD_SIZE, &bytesRead);
			ownArm11();
			PAYLOAD_FUNCTION();
		}
	}
	
	i2cWriteRegister(I2C_DEV_MCU, 0x20, (u8)(1<<0));
    return 0;
}
