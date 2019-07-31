#include <stdio.h>
#include <stdint.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "max10_i2c_update.h"

extern int set_i2c_register(int file, uint8_t addr, int reg, int val);
extern int get_i2c_register(int file, uint8_t addr, int reg, int *val);


static int g_i2c_file = 0;
static uint8_t g_cpld_addr = 0;

#define DEBUGMSG(X) 
//#define DEBUGMSG(X)  printf X

// status register
#define BUSY_IDLE         0x00
#define BUSY_ERASE        0x01
#define BUSY_WRITE        0x02
#define BUSY_READ         0x03

#define READ_SUCCESS      0x04
#define WRITE_SUCCESS     0x08
#define ERASE_SUCCESS     0x10

void Max10Update_Init(int i2c_file, uint8_t addr)
{
	g_i2c_file = i2c_file;
        g_cpld_addr = addr;
#if 0
	// Set timeout /* set timeout in units of 10 ms */
	if (ioctl(g_i2c_file, I2C_TIMEOUT, 1000) < 0) 
	{
	   /* ERROR HANDLING; you can check errno to see what went wrong */
	   printf("\r\n ioctl(file, I2C_TIMEOUT) fail. \r\n");
	}

	// set retry time
	if (ioctl(g_i2c_file, I2C_RETRIES, 100) < 0) 
	{
	   /* ERROR HANDLING; you can check errno to see what went wrong */
	   printf("\r\n ioctl(file, I2C_RETRIES) fail. \r\n");
	}
#endif	
}

// Check with i2c bridge config in Quartus project.
#if 0

#define I2C_READ_BRIDGE_ID     0x57
#define I2C_WRITE_BRIDGE_ID    0x53

#else

#define I2C_READ_BRIDGE_ID     0x31
#define I2C_WRITE_BRIDGE_ID    0x31


#endif

// MAX 10 evaluation board (10M08).
#if 0

#define ON_CHIP_FLASH_IP_DATA_REG         0x00000000
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG   0x00080020
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG     0x00080024
#define DUAL_BOOT_IP_BASE                 0x00080000

#else
// MAX 10 NEEK board (10M50) / project
#define ON_CHIP_FLASH_IP_DATA_REG         0x00000000
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG   0x00200020
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG     0x00200024
#define DUAL_BOOT_IP_BASE                 0x00200000

#endif

int WriteRegister(int address, int data)
{
	int ret ;

	if(g_i2c_file == 0)
	{
		printf("i2c device not specified. \n");
		return -1;
	}
	else
	{
	    //ret = set_i2c_register(g_i2c_file, I2C_WRITE_BRIDGE_ID, address, data);
            ret = set_i2c_register(g_i2c_file, g_cpld_addr, address, data);
	    if(ret != 8) {
                printf("set_i2c_register() ERROR. ret = %d\n", ret);
	    }
	}

//	usleep(1000);
	return ret;
}


int ReadRegister(int address)
{
	int data;
	int ret;

	if(g_i2c_file == 0) {
            printf("\n i2c device not specified. \n");
	    return -1;
	} else {
	   //ret = get_i2c_register(g_i2c_file, I2C_READ_BRIDGE_ID, address, &data);
           ret = get_i2c_register(g_i2c_file, g_cpld_addr, address, &data);
	   if(ret != 4)
	   {
	   	printf("\r\n\n get_i2c_register() ERROR. ret = %d. \r\n\n", ret);
	   }
	}

//	usleep(1000);

	return data;
}

enum{
	WP_CFM0 = 0x1<<27,
	WP_CFM1 = 0x1<<26,
	WP_CFM2 = 0x1<<25,
	WP_UFM0 = 0x1<<24,
	WP_UFM1 = 0x1<<23,
};

int Max10_protectSectors(void)
{
	int ret = 0;
	int value;

	DEBUGMSG(("Enter Max10_protectSectors()\n"));

	value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
        DEBUGMSG(("%s():Read ctrl reg = 0x%08X.\n", __func__, value));
	value |= (0x1F<<23);
        DEBUGMSG(("%s():Set ctrl reg = 0x%08X.\n", __func__, value));
	ret = WriteRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG, value);


	DEBUGMSG(("Exit Max10_protectSectors().\n"));
	
	return ret;
}

int Max10_UnprotectSector(SectorType_t secType)
{
	int ret = 0;
	int value;


	DEBUGMSG(("Enter Max10_UnprotectSector(). \n"));

	value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
	DEBUGMSG(("Read ctrl reg = 0x%08X. \n", value));

	switch(secType)
	{
	case Sector_CFM0:
		value = value & (~WP_CFM0);  // set to zero
		break;

	case Sector_CFM1:
		value = value & (~WP_CFM1);
		break;

	case Sector_CFM2:
		value = value & (~WP_CFM2);
		break;

	case Sector_UFM0:
		value = value & (~WP_UFM0);
		break;

	case Sector_UFM1:
		value = value & (~WP_UFM1);
		break;

	case Sector_NotSet:
		value = 0xFFFFFFFF;
		break;
	}

	DEBUGMSG(("write ctrl reg = 0x%08X.\n", value));
	ret = WriteRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG, value);


	DEBUGMSG(("Exit Max10_UnprotectSector(). \n"));
	
	return ret;
}

int Max10_Erase_Sector(SectorType_t secType)
{
	int ret = 0;
	int value;
	int status;
	int done = 0;
	
	//Control register bit 20~22
	enum{
		Sector_erase_CFM0	= 0b101 << 20,
		Sector_erase_CFM1	= 0b100 << 20,
		Sector_erase_CFM2	= 0b011 << 20,
		Sector_erase_UFM0	= 0b010 << 20,
		Sector_erase_UFM1	= 0b001 << 20,
		Sector_erase_NotSet     = 0b111 << 20,
	};
	
	DEBUGMSG(("Enter Max10_Erase_Sector().\n"));

	value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
	//DEBUGMSG(("read ctrl reg = 0x%08X. \n", value));

	value &= ~(0x7<<20);  // clear bit 20~22.

	switch(secType)
	{
	case Sector_CFM0:
		value |= Sector_erase_CFM0;
		break;

	case Sector_CFM1:
		value |= Sector_erase_CFM1;
		break;

	case Sector_CFM2:
		value |= Sector_erase_CFM2;
		break;

	case Sector_UFM0:
		value |= Sector_erase_UFM0;
		break;

	case Sector_UFM1:
		value |= Sector_erase_UFM1;
		break;

	case Sector_NotSet:
		value |= Sector_erase_NotSet;
		break;
	}

	WriteRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG, value);
	DEBUGMSG(("write ctrl reg = 0x%08X. \n", value));

        value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
        DEBUGMSG(("read ctrl reg = 0x%08X. \n", value));

	//Wait erase finished.
	if(secType != Sector_NotSet)
	{
		while(!done)
		{
			status = ReadRegister(ON_CHIP_FLASH_IP_CSR_STATUS_REG);
			DEBUGMSG(("read status reg = 0x%08X. \n", status));

			if( status & BUSY_ERASE)
			{
				usleep(500*1000);
                                printf("Erase sector BUSY. \n");
				continue;
			}

			if( status & ERASE_SUCCESS)
			{
				done = 1;
				printf("Erase sector SUCCESS. \n");
			}
			else
			{
				printf("Erase sector FAIL. \n");
				ret = -1;
                                sleep(5);
			}

		}
	}
        //printf("wait 500ms...\n");
        //usleep(500*1000);
	return ret;
}

// write 4 byte into on-chip flash.
int Max10_WriteFlashData(int address, int data)
{
	int ret = 0;
        //printf("%s() write address: %x, data:%x\n", __func__, ON_CHIP_FLASH_IP_DATA_REG + address, data);
	ret = WriteRegister(ON_CHIP_FLASH_IP_DATA_REG + address, data);

	return ret;
}

// Read 4 byte from on-chip flash.
int Max10_ReadFlashData(int address)
{
	int ret = 0;

	ret = ReadRegister(ON_CHIP_FLASH_IP_DATA_REG + address);

	return ret;
}

int Max10_OnChipFlash_GetStatus(void)
{
	int ret = 0;

	ret = ReadRegister(ON_CHIP_FLASH_IP_CSR_STATUS_REG);
        //printf("%s() GetStatus: 0x%X\n", __func__, ret);
	return ret;
}

int IsBusy(void)
{
	int value;

	value = ReadRegister(ON_CHIP_FLASH_IP_CSR_STATUS_REG);

	if( value & (BUSY_ERASE|BUSY_READ|BUSY_WRITE) )
		return 1;
	else	
		return 0; // timeout
}

int Max10_Update_Rpd(unsigned char* rpd_file, SectorType_t sectorType , int cfm_start_addr, int cfm_end_addr )
{
	int ret = 0;
	int address;
	int data;
	int receivedHex[4];
	int byte;
	int bContinue = 1;
	int status;
	int offset;
//	int regval;

	printf("OnChip Flash Status = 0x%X.\n", Max10_OnChipFlash_GetStatus() );

	/*State 2: Write Control Register of On-Chip Flash IP to un-protect and erase operation*/
        if(sectorType == Sector_CFM1 || sectorType == Sector_CFM2) {
	    Max10_UnprotectSector(Sector_CFM1);
	    Max10_UnprotectSector(Sector_CFM2);
	} else {
	    Max10_UnprotectSector(sectorType);
        }

	// Erase CFM0, CFM1, CFM2
	if(sectorType == Sector_CFM1 || sectorType == Sector_CFM2) {
	    Max10_Erase_Sector(Sector_CFM1);
            Max10_Erase_Sector(Sector_CFM2);
	} else {
            ret = Max10_Erase_Sector(sectorType);
	    printf("Max10_Erase_Sector(), ret = %d. \n", ret);
	}
	
        if(ret != 0) {
            printf("Erase sector fail. \n");
	    return -1;
	}

	// Set erase none.
	ret = Max10_Erase_Sector(Sector_NotSet);
	printf("Max10_protectSectors(Sector_NotSet), ret = %d. \n", ret);

	// start program
	offset = 0;
	for(address = cfm_start_addr; address < cfm_end_addr; address += 4) {
            /*Get 4 bytes from UART Terminal*/
            receivedHex[0] = rpd_file[offset + 0];
	    receivedHex[1] = rpd_file[offset + 1];
	    receivedHex[2] = rpd_file[offset + 2];
	    receivedHex[3] = rpd_file[offset + 3];

	    /*Swap LSB with MSB before write into CFM*/
 	    for(byte=0; byte<4; byte++) {
                receivedHex[byte] = (((receivedHex[byte] & 0xaa)>>1)|((receivedHex[byte] & 0x55)<<1));
 		receivedHex[byte] = (((receivedHex[byte] & 0xcc)>>2)|((receivedHex[byte] & 0x33)<<2));
 		receivedHex[byte] = (((receivedHex[byte] & 0xf0)>>4)|((receivedHex[byte] & 0x0f)<<4));
 	    }

 	    /*Combine 4 bytes to become 1 word before write operation*/
            data = (receivedHex[0]<<24)|(receivedHex[1]<<16)|(receivedHex[2]<<8)|(receivedHex[3]);
        
            /*Command to write into On-Chip Flash IP*/	
	    //usleep(10*1000);		   	
	    Max10_WriteFlashData(address, data);
	    usleep(50);

            //offset+= 4;
            //printf("Writing Data: %d/%d (%.2f%%) \r", offset, total, (offset*100)/(float)total);
	    // Wait for write operation
	    bContinue = 1;

#define STATUS_BIT_MASK  0x1F		   
	    while( bContinue ) {
                status = Max10_OnChipFlash_GetStatus();
		status &= STATUS_BIT_MASK;
  		   	
  		if(  (status & BUSY_WRITE) == BUSY_WRITE ) {  		   		
  		    continue;
  		}
  		   		
				
		if( (status & WRITE_SUCCESS) == WRITE_SUCCESS) {
		    bContinue = 0;
		    continue;
		}
 
		if(status != BUSY_IDLE) {
		    printf("Write to addr: 0x%X failed. status = 0x%X. \n", address, status);
		    bContinue = 0;
		    continue;
	        }	   
            }
#if 1
   	    // show percentage
	    {
	        static int last_percent = 0;
                int total = cfm_end_addr - cfm_start_addr;
		int percent = ((address - cfm_start_addr)*100)/total;

		if(last_percent != percent) {
                    last_percent = percent;
		    printf(" Prograss  %d %%.  addr: 0x%X \n", percent, address);
		}
            }	
#endif			
            //
	    offset+= 4;
            //printf("Writing Data: %d/%d (%.2f%%) \r", offset, total, (offset*100)/(float)total);
    }

    //IOWR(ONCHIP_FLASH_0_CSR_BASE, 1, 0xffffffff); 
    /*Re-protect the sector*/
    Max10_protectSectors();

    //
    g_i2c_file = 0;

    return ret;
}

/*
   Dual configuration IP on MAX 10

*/
int Max10_DualBoot_IsBusy(void)
{
	int value;
	
	// offset 3 register.
	value = ReadRegister(DUAL_BOOT_IP_BASE + 3*4);	
	if(value == 0)
		return 0;
	else
		return 1;
}

Max10_Config_Sel_t Max10_DualBoot_ConfigSel_Get(void)
{
	int value;
	
	// offset 7 register.
	value = ReadRegister(DUAL_BOOT_IP_BASE + 7*4);	

	if(value & (0x1<<1))
		return CONFIG_1;
	else
		return CONFIG_0;
	
}

int Max10_DualBoot_ConfigSel_Set(Max10_Config_Sel_t img_no)
{
	int value;
	int address = DUAL_BOOT_IP_BASE + 4;  // offset 1
	int ret ;
	
	//IOWR(DUAL_BOOT_0_BASE, 1, 0x00000001);

	// offset 1 (write only), bit 0, config_sel_overwrite
	    value = 0x1;	//config_sel_overwrite = 1
	    
	    switch(img_no)
	    {
	    	case CONFIG_0:
	    	break;

	    	case CONFIG_1:
	    		value |= 0x1<<1;
	    	break;	    	
	    }
	    
	// 
		if(Max10_DualBoot_IsBusy() == 1)
		{
			return -1;			
		}

		ret = WriteRegister(address, value);

	return ret;
}

int Max10_TrigReconfig(void)
{
	/*
	if(IORD(DUAL_BOOT_0_BASE,3) == 0)
	{
		IOWR(DUAL_BOOT_0_BASE, 0, 0x00000001);
		state=0;
	}
	*/

	int value = 0x1;
	int address = DUAL_BOOT_IP_BASE + 0;  // offset 0
	int ret ;


	if( Max10_DualBoot_IsBusy() == 0 )
		ret = WriteRegister(address, value);
	else
		return -1;


	return ret;
}
