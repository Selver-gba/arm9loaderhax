#include "common.h"
#include "hid.h"
#include "delay.h"
#include "text.h"
#include "crypto.h"
#include "fatfs/ff.h"
#include "i2c.h"
#include "nand.h"
#include "sdmmc.h"
#include "crypto.h"

extern unsigned char* topScreen;
extern unsigned char* subScreen;
extern u8 payload_bin[];
extern u32 payload_bin_size;


u8* arm9bin = (u8*)0x08006000;
u8* arm9start = (u8*)0x08006800;

void getScreenControl()
{
	void* arm11Addr = (void*)0x1FFF4C80;
	extern unsigned int arm11Code;
	extern unsigned int arm11CodeSize;
	memcpy(arm11Addr, (void*)&arm11Code, arm11CodeSize);
	
	*(unsigned int*)0x1FFFFFFC = (unsigned int)arm11Addr;
	*(unsigned int*)0x1FFFFFF8 = (unsigned int)arm11Addr;
	for(int i = 0; i < 0x46500; i++)
	{
		*((unsigned char*)topScreen + i) = 0x00;
		*((unsigned char*)subScreen + i) = 0x00;
	}
}


// temporary buffer of at least 0x4000 bytes
#define TEMP_BUFFER                 ((void*)0x21000000)
#define BYTES_PER_SECTOR            (0x200)
#define SECTORS_PER_WRITE           (0x200)
#define BYTES_PER_WRITE             (BYTES_PER_SECTOR * SECTORS_PER_WRITE)

// Even if the NAND is larger, the actual used size
// of the flash is always 988,807,168 bytes
#define BYTE_SIZE_NAND ((u32)0x3AF00000)

#define BYTE_SIZE_FIRM_PARTITION    (0x100000)
#define FIRM_PARTITION_SECTOR_COUNT ((BYTE_SIZE_FIRM_PARTITION / BYTES_PER_SECTOR) + (0 == (BYTE_SIZE_FIRM_PARTITION % BYTES_PER_SECTOR) ? 0 : 1))


// These addresses are in the .spec files
#define EXECUTABLE_DATA_SECTION_FIRM0        ((void*)0x23100000)
#define EXECUTABLE_DATA_SECTION_FIRM1        ((void*)0x23200000)
#define EXECUTABLE_DATA_SECTION_STAGE2       ((void*)0x23300000)
#define EXECUTABLE_DATA_SECTION_MAGICSECTOR  ((void*)0x23000000)

int main()
{
	getScreenControl();
	sdmmc_sdcard_init();
	FATFS fs;
	FIL file;
	UINT br;
	
	if(f_mount(&fs, "0:", 0) == FR_OK)
	{
		if(f_open(&file, "NAND.bin", FA_READ | FA_OPEN_EXISTING) == FR_OK)
		{
			
			printf("Install NAND backup...");
			for(u32 i = 0; i < BYTE_SIZE_NAND; i += BYTES_PER_WRITE)
			{
				f_lseek(&file, i); 
				f_read(&file, TEMP_BUFFER, BYTES_PER_WRITE, &br);
				sdmmc_nand_writesectors(i/BYTES_PER_SECTOR, SECTORS_PER_WRITE, TEMP_BUFFER);
				drawHex(i, 10, 210);
			}
			printf("done.");
			f_close(&file);
		}
	}
	printf("Install stage2...");
	sdmmc_nand_writesectors(0x5C000, 0x20, EXECUTABLE_DATA_SECTION_STAGE2);
	printf("done.");
	
	printf("Install sector...");
	sdmmc_nand_writesectors(0x96, 0x1, EXECUTABLE_DATA_SECTION_MAGICSECTOR);
	printf("done.");

	printf("Install firm0...");
	nand_writesectors(0, FIRM_PARTITION_SECTOR_COUNT, EXECUTABLE_DATA_SECTION_FIRM0, FIRM0);
	printf("done.");

	printf("Install firm1...");
	nand_writesectors(0, FIRM_PARTITION_SECTOR_COUNT, EXECUTABLE_DATA_SECTION_FIRM1, FIRM1);
	printf("done.");

	// This shuts down the console.
	i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    return 0;
}
