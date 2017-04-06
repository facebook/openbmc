/*********************************************************************************
* Lattice Semiconductor Corp. Copyright 2000-2008
*
* This is the hardware.c of ispVME V12.1 for JTAG programmable devices.
* All the functions requiring customization are organized into this file for
* the convinience of porting.
*********************************************************************************/
/*********************************************************************************
 * Revision History:
 *
 * 09/11/07 NN Type cast mismatch variables
 * 09/24/07 NN Added calibration function.
 *             Calibration will help to determine the system clock frequency
 *             and the count value for one micro-second delay of the target
 *             specific hardware.
 *             Modified the ispVMDelay function
 *             Removed Delay Percent support
 *             Moved the sclock() function from ivm_core.c to hardware.c
 *********************************************************************************/
#include "vmopcode.h"
#include <sys/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#if defined(GALAXY100_PRJ)
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/cpldupdate_dll.h>
#endif

//#define ISP_DEBUG
#ifdef ISP_DEBUG
#define DPRINTF(fmt, args...) printf(fmt, ##args);
#else
#define DPRINTF(FMT, args...)
#endif
/********************************************************************************
* Declaration of global variables
*
*********************************************************************************/
#ifdef GALAXY100_PRJ
int syscpld_update = 1;
int use_dll = 0;
const char *dll_name = NULL;
static struct cpldupdate_helper_st dll_helper;
static volatile unsigned int *gpio_base;
static volatile unsigned int *gpio_dir_base;
static int mem_fd;
static int mem_dir_fd;
static long vme_file_size = 0;
unsigned long  g_siIspPins        = 0x00000000;   /*Keeper of JTAG pin state*/
unsigned short g_usInPort         = PCA953X_INPUT;  /*Address of the TDO pin*/
unsigned short g_usOutPort	  = PCA953X_OUTPUT;  /*Address of TDI, TMS, TCK pin*/
unsigned short g_usCpu_Frequency  = 4000; // Here is Intel rangely CPU frequence   /*Enter your CPU frequency here, unit in MHz.*/
#else
unsigned long  g_siIspPins        = 0x00000000;   /*Keeper of JTAG pin state*/
unsigned short g_usInPort         = 0x548;  /*Address of the TDO pin*/
unsigned short g_usOutPort	  = 0x548;  /*Address of TDI, TMS, TCK pin*/
unsigned short g_usCpu_Frequency  = 4000; // Here is Intel rangely CPU frequence   /*Enter your CPU frequency here, unit in MHz.*/
#endif

/*********************************************************************************
* This is the definition of the bit locations of each respective
* signal in the global variable g_siIspPins.
*
* NOTE: Users must add their own implementation here to define
*       the bit location of the signal to target their hardware.
*       The example below is for the Lattice download cable on
*       on the parallel port.
*
*********************************************************************************/

#ifdef GALAXY100_PRJ
unsigned long g_ucPinTDI          = 0x1 << CPLD_TDI_CONFIG;    /* Bit address of TDI */
unsigned long g_ucPinTCK          = 0x1 << CPLD_TCK_CONFIG;    /* Bit address of TCK */
unsigned long g_ucPinTMS          = 0x1 << CPLD_TMS_CONFIG;    /* Bit address of TMS */
unsigned long g_ucPinTDO          = 0x1 << CPLD_TDO_CONFIG;    /* Bit address of TDO*/
const unsigned long g_ucPinENABLE       = 0x08;    /* Bit address of ENABLE */
const unsigned long g_ucPinTRST         = 0x10;    /* Bit address of TRST */
#else
const unsigned long g_ucPinTDI          = 0x00000010;    /* Bit address of TDI */
const unsigned long g_ucPinTCK          = 0x00000020;    /* Bit address of TCK */
const unsigned long g_ucPinTMS          = 0x00000004;    /* Bit address of TMS */
const unsigned long g_ucPinENABLE       = 0x08;    /* Bit address of ENABLE */
const unsigned long g_ucPinTRST         = 0x10;    /* Bit address of TRST */
const unsigned long g_ucPinTDO          = 0x00000008;    /* Bit address of TDO*/
#endif
/***************************************************************
*
* Functions declared in hardware.c module.
*
***************************************************************/
void writePort( unsigned long a_ucPins, unsigned char a_ucValue );
unsigned char readPort();
void sclock();
void ispVMDelay( unsigned short a_usTimeDelay );
void calibration(void);
#ifdef GALAXY100_PRJ
static void isp_dll_write(unsigned long pins, int value);
static int isp_dll_read(cpldupdate_pin_en pin);
static int isp_i2c_read(int bus, int addr, unsigned char reg);
static int isp_i2c_write(int bus, int addr, unsigned char reg, unsigned value);
static int open_i2c_dev(int i2cbus, char *filename, size_t size);
static void *isp_gpio_map(unsigned int addr, int *fop);
static void isp_gpio_unmap(void *addr, int fd);
#endif
/********************************************************************************
* writePort
* To apply the specified value to the pins indicated. This routine will
* be modified for specific systems.
* As an example, this code uses the IBM-PC standard Parallel port, along with the
* schematic shown in Lattice documentation, to apply the signals to the
* JTAG pins.
*
* PC Parallel port pin    Signal name           Port bit address
*        2                g_ucPinTDI             1
*        3                g_ucPinTCK             2
*        4                g_ucPinTMS             4
*        5                g_ucPinENABLE          8
*        6                g_ucPinTRST            16
*        10               g_ucPinTDO             64
*
*  Parameters:
*   - a_ucPins, which is actually a set of bit flags (defined above)
*     that correspond to the bits of the data port. Each of the I/O port
*     bits that drives an isp programming pin is assigned a flag
*     (through a #define) corresponding to the signal it drives. To
*     change the value of more than one pin at once, the flags are added
*     together, much like file access flags are.
*
*     The bit flags are only set if the pin is to be changed. Bits that
*     do not have their flags set do not have their levels changed. The
*     state of the port is always manintained in the static global
*     variable g_siIspPins, so that each pin can be addressed individually
*     without disturbing the others.
*
*   - a_ucValue, which is either HIGH (0x01 ) or LOW (0x00 ). Only these two
*     values are valid. Any non-zero number sets the pin(s) high.
*
*********************************************************************************/

void writePort( unsigned long a_ucPins, unsigned char a_ucValue )
{
	if ( a_ucValue ) {
		g_siIspPins = (a_ucPins | g_siIspPins);
	}
	else {
		g_siIspPins = (~a_ucPins & g_siIspPins);
	}
#ifdef GALAXY100_PRJ
	if(syscpld_update) {
		*gpio_base = g_siIspPins;
		while((*gpio_base & g_siIspPins) != g_siIspPins);
	} else if (use_dll) {
		isp_dll_write(a_ucPins, a_ucValue ? 1 : 0);
	} else {
		isp_i2c_write(I2C_CPLD_BUS, I2C_CPLD_ADDRESS, g_usOutPort, g_siIspPins | (0x1 << CPLD_I2C_ENABLE_OFFSET));
	}
#endif
}

/*********************************************************************************
*
* readPort
*
* Returns the value of the TDO from the device.
*
**********************************************************************************/
unsigned char readPort()
{
	unsigned char ucRet = 0;
	int count = 3;

#ifdef GALAXY100_PRJ
	if(syscpld_update) {
		while(count--) {
			if ((*gpio_base) & g_ucPinTDO) {
				ucRet = 0x01;
			} else {
				ucRet = 0x0;
			}
		}
	} else if (use_dll) {
		ucRet = isp_dll_read(CPLDUPDATE_PIN_TDO);
	} else {
		if (isp_i2c_read(I2C_CPLD_BUS, I2C_CPLD_ADDRESS, g_usInPort) & g_ucPinTDO) {
			ucRet = 0x01;
		}
		else {
			ucRet = 0x00;
		}
	}
#endif

	return ( ucRet );
}

/*********************************************************************************
* sclock
*
* Apply a pulse to TCK.
*
* This function is located here so that users can modify to slow down TCK if
* it is too fast (> 25MHZ). Users can change the IdleTime assignment from 0 to
* 1, 2... to effectively slowing down TCK by half, quarter...
*
*********************************************************************************/
void sclock()
{
	unsigned short IdleTime    = 0; //change to > 0 if need to slow down TCK
	unsigned short usIdleIndex = 0;
	IdleTime++;
	for ( usIdleIndex = 0; usIdleIndex < IdleTime; usIdleIndex++ ) {
		writePort( g_ucPinTCK, 0x01 );
	}
	for ( usIdleIndex = 0; usIdleIndex < IdleTime; usIdleIndex++ ) {
		writePort( g_ucPinTCK, 0x00 );
	}
}
/********************************************************************************
*
* ispVMDelay
*
*
* Users must implement a delay to observe a_usTimeDelay, where
* bit 15 of the a_usTimeDelay defines the unit.
*      1 = milliseconds
*      0 = microseconds
* Example:
*      a_usTimeDelay = 0x0001 = 1 microsecond delay.
*      a_usTimeDelay = 0x8001 = 1 millisecond delay.
*
* This subroutine is called upon to provide a delay from 1 millisecond to a few
* hundreds milliseconds each time.
* It is understood that due to a_usTimeDelay is defined as unsigned short, a 16 bits
* integer, this function is restricted to produce a delay to 64000 micro-seconds
* or 32000 milli-second maximum. The VME file will never pass on to this function
* a delay time > those maximum number. If it needs more than those maximum, the VME
* file will launch the delay function several times to realize a larger delay time
* cummulatively.
* It is perfectly alright to provide a longer delay than required. It is not
* acceptable if the delay is shorter.
*
* Delay function example--using the machine clock signal of the native CPU------
* When porting ispVME to a native CPU environment, the speed of CPU or
* the system clock that drives the CPU is usually known.
* The speed or the time it takes for the native CPU to execute one for loop
* then can be calculated as follows:
*       The for loop usually is compiled into the ASSEMBLY code as shown below:
*       LOOP: DEC RA;
*             JNZ LOOP;
*       If each line of assembly code needs 4 machine cycles to execute,
*       the total number of machine cycles to execute the loop is 2 x 4 = 8.
*       Usually system clock = machine clock (the internal CPU clock).
*       Note: Some CPU has a clock multiplier to double the system clock for
              the machine clock.
*
*       Let the machine clock frequency of the CPU be F, or 1 machine cycle = 1/F.
*       The time it takes to execute one for loop = (1/F ) x 8.
*       Or one micro-second = F(MHz)/8;
*
* Example: The CPU internal clock is set to 100Mhz, then one micro-second = 100/8 = 12
*
* The C code shown below can be used to create the milli-second accuracy.
* Users only need to enter the speed of the cpu.
*
**********************************************************************************/
void ispVMDelay( unsigned short a_usTimeDelay )
{
	unsigned short loop_index     = 0;
	unsigned short ms_index       = 0;
	unsigned short us_index       = 0;

	if ( a_usTimeDelay & 0x8000 ) /*Test for unit*/
	{
		a_usTimeDelay &= ~0x8000; /*unit in milliseconds*/
	}
	else { /*unit in microseconds*/
		a_usTimeDelay = (unsigned short) (a_usTimeDelay/1000); /*convert to milliseconds*/
		if ( a_usTimeDelay <= 0 ) {
			 a_usTimeDelay = 1; /*delay is 1 millisecond minimum*/
		}
	}
	/*Users can replace the following section of code by their own*/
	for( ms_index = 0; ms_index < a_usTimeDelay; ms_index++)
	{
		/*Loop 1000 times to produce the milliseconds delay*/
		for (us_index = 0; us_index < 1000; us_index++)
		{ /*each loop should delay for 1 microsecond or more.*/
			loop_index = 0;
			do {
				/*The NOP fakes the optimizer out so that it doesn't toss out the loop code entirely*/
				asm("nop");
			}while (loop_index++ < ((g_usCpu_Frequency/8)+(+ ((g_usCpu_Frequency % 8) ? 1 : 0))));/*use do loop to force at least one loop*/
		}
	}
}

/*********************************************************************************
*
* calibration
*
* It is important to confirm if the delay function is indeed providing
* the accuracy required. Also one other important parameter needed
* checking is the clock frequency.
* Calibration will help to determine the system clock frequency
* and the loop_per_micro value for one micro-second delay of the target
* specific hardware.
*
**********************************************************************************/
void calibration(void)
{
	/*Apply 2 pulses to TCK.*/
	writePort( g_ucPinTCK, 0x00 );
	writePort( g_ucPinTCK, 0x01 );
	writePort( g_ucPinTCK, 0x00 );
	writePort( g_ucPinTCK, 0x01 );
	writePort( g_ucPinTCK, 0x00 );

	/*Delay for 1 millisecond. Pass on 1000 or 0x8001 both = 1ms delay.*/
	ispVMDelay(0x8001);

	/*Apply 2 pulses to TCK*/
	writePort( g_ucPinTCK, 0x01 );
	writePort( g_ucPinTCK, 0x00 );
	writePort( g_ucPinTCK, 0x01 );
	writePort( g_ucPinTCK, 0x00 );
}

#ifdef GALAXY100_PRJ
int isp_dll_init(int argc, const char * const argv[]) {
  int rc;

  rc = cpldupdate_helper_open(dll_name, &dll_helper);
  if (rc) {
    goto out;
  }

  rc = cpldupdate_helper_init(&dll_helper, argc, argv);
  if (rc) {
    goto out;
  }

  g_ucPinTDI = 0x1 << CPLDUPDATE_PIN_TDI;
  g_ucPinTCK = 0x1 << CPLDUPDATE_PIN_TCK;
  g_ucPinTMS = 0x1 << CPLDUPDATE_PIN_TMS;
  g_ucPinTDO = 0x1 << CPLDUPDATE_PIN_TDO;

 out:
  return rc;
}

void isp_dll_write(unsigned long pins, int value) {
  cpldupdate_pin_en pin;
  for (pin = 0; pin < sizeof(pins) * 8 && pin < CPLDUPDATE_PIN_MAX; pin++) {
    if (!(pins & (0x1 << pin))) {
      /* not set */
      continue;
    }
    cpldupdate_helper_write_pin(
        &dll_helper, pin,
        value ? CPLDUPDATE_PIN_VALUE_HIGH : CPLDUPDATE_PIN_VALUE_LOW);
  }
}

int isp_dll_read(cpldupdate_pin_en pin) {
  cpldupdate_pin_value_en value = CPLDUPDATE_PIN_VALUE_LOW;
  cpldupdate_helper_read_pin(&dll_helper, pin, &value);
  return value;
}

int isp_gpio_i2c_init(void)
{
	unsigned char value;
	int ret;

	g_ucPinTDI          = 0x1 << CPLD_TDI_I2C_CONFIG;    /* Bit address of TDI */
	g_ucPinTCK          = 0x1 << CPLD_TCK_I2C_CONFIG;    /* Bit address of TCK */
	g_ucPinTMS          = 0x1 << CPLD_TMS_I2C_CONFIG;    /* Bit address of TMS */
	g_ucPinTDO          = 0x1 << CPLD_TDO_I2C_CONFIG;    /* Bit address of TDO*/

	/*CPLD JTAG update enable */
	isp_gpio_i2c_config(CPLD_I2C_ENABLE_OFFSET, GPIO_OUT);
	ispVMDelay(0x800b);
	value = (0x1 << CPLD_I2C_ENABLE_OFFSET);
	if(isp_i2c_write(I2C_CPLD_BUS, I2C_CPLD_ADDRESS, g_usOutPort, value) < 0) {
		printf("i2c write addr %x value 0x%x error!", g_usOutPort, value);
		return -1;
	}

	return 0;
}
void isp_gpio_i2c_config(unsigned int gpio, int dir)
{
	unsigned char value;
	int ret;

	ret = isp_i2c_read(I2C_CPLD_BUS, I2C_CPLD_ADDRESS, PCA953X_DIRECTION);
	if(ret < 0) {
		printf("i2c read addr %x error!", PCA953X_DIRECTION);
		return ;
	}
	value = (unsigned char)ret;
	if(dir) { //input
		value |= (1 << gpio);
	} else {
		value &= ~(1 << gpio);
	}
	ret = isp_i2c_write(I2C_CPLD_BUS, I2C_CPLD_ADDRESS, PCA953X_DIRECTION, value);
	if(ret < 0) {
		printf("i2c write addr %x value 0x%x error!", PCA953X_DIRECTION, value);
		return ;
	}
}

static int isp_i2c_read(int bus, int addr, unsigned char reg)
{
	int res = -1, file, count = 10;
	char filename[20];

	file = open_i2c_dev(bus, filename, sizeof(filename));
	if(file < 0) {
		return -1;
	}
	if(ioctl(file, I2C_SLAVE, addr) < 0) {
		return -1;
	}
	while(count-- && (res < 0)) {
		res = i2c_smbus_read_byte_data(file, reg);
	}

	close(file);
	return res;
}

static int isp_i2c_write(int bus, int addr, unsigned char reg, unsigned value)
{
	int res = -1, file, count = 10;
	char filename[20];

	file = open_i2c_dev(bus, filename, sizeof(filename));
	if(file < 0) {
		return -1;
	}
	if(ioctl(file, I2C_SLAVE, addr) < 0) {
		return -1;
	}
	while(count-- && (res < 0)) {
		res = i2c_smbus_write_byte_data(file, reg, value);
	}

	close(file);
	return res;
}
static int open_i2c_dev(int i2cbus, char *filename, size_t size)
{
	int file;

	snprintf(filename, size, "/dev/i2c-%d", i2cbus);
	filename[size - 1] = '\0';
	file = open(filename, O_RDWR);

	if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
		sprintf(filename, "/dev/i2c-%d", i2cbus);
		file = open(filename, O_RDWR);
	}

	return file;
}

/*for GPIO JTAG*/
static void *isp_gpio_map(unsigned int addr, int *fop)
{
	int fd;
	void *base, *virt_addr;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		printf("open /dev/mem error!\n");
		return NULL;
	}
	base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~MAP_MASK);
	if(base == (void *)-1) {
		printf("map base is NULL!\n");
		return NULL;
	}
	virt_addr = base + (addr & MAP_MASK);
	*fop = fd;

	return virt_addr;
}
static void isp_gpio_unmap(void *addr, int fd)
{
	munmap(addr, MAP_SIZE);
	close(fd);
}
int isp_gpio_init(void)
{
	void *scu80_addr, *scu84_addr;
	int scu80_fd, scu84_fd, gpiof_data_addr, gpiof_data_fd, gpiof_dir_addr, gpiof_dir_fd;
	unsigned int value, temp;

	g_ucPinTDI          = 0x1 << CPLD_TDI_CONFIG;    /* Bit address of TDI */
	g_ucPinTCK          = 0x1 << CPLD_TCK_CONFIG;    /* Bit address of TCK */
	g_ucPinTMS          = 0x1 << CPLD_TMS_CONFIG;    /* Bit address of TMS */
	g_ucPinTDO          = 0x1 << CPLD_TDO_CONFIG;    /* Bit address of TDO*/

	/*enable GPIOF to GPIO mode*/
	scu80_addr = isp_gpio_map(SYSTEM_SCU_ADDR(SYSTEM_SCU80), &scu80_fd);
	value = *(volatile unsigned int *)scu80_addr;
	DPRINTF("scu80_addr origin value=0x%08x\n", value);
	*(volatile unsigned int *)scu80_addr = value & (~CPLD_UPDATE_GPIOF_ENABLE);
	value = *(volatile unsigned int *)scu80_addr; //read back
	DPRINTF("GPIOF mode  enable, value=0x%08x\n", value);
	isp_gpio_unmap(scu80_addr, scu80_fd);
	/*set GPIOF2 direction to output*/

	/*compatible FC and LC*/
	gpiof_dir_addr = isp_gpio_map(GPIOF_DIR_ADDR, &gpiof_dir_fd);
	value = *(volatile unsigned int *)gpiof_dir_addr;
	DPRINTF("gpiof_dir_addr(%p) origin value=0x%08x\n", gpiof_dir_addr, value);
	*(volatile unsigned int *)gpiof_dir_addr = value & ~(GPIOF_OFFSET);//input mode
	value = *(volatile unsigned int *)gpiof_dir_addr; //read back
	DPRINTF("gpiof_dir_addr set input mode, value=0x%08x\n", value);
	gpiof_data_addr = isp_gpio_map(GPIOF_DATA_ADDR, &gpiof_data_fd);
	value = *(volatile unsigned int *)gpiof_data_addr;
	temp = (~(value & (GPIOF_OFFSET))) & GPIOF_OFFSET;//value negation
	DPRINTF("gpiof_data_addr(%p) origin value=0x%08x, and temp = 0x%08x\n", gpiof_data_addr, value, temp);

	value = *(volatile unsigned int *)gpiof_dir_addr;
	*(volatile unsigned int *)gpiof_dir_addr = value | (GPIOF_OFFSET);//output mode
	value = *(volatile unsigned int *)gpiof_dir_addr; //read back
	DPRINTF("gpiof_dir_addr(%p) set output mode, value=0x%08x\n", gpiof_dir_addr, value);
	/*set GPIOF2 output to negation*/
	value = *(volatile unsigned int *)gpiof_data_addr;
	DPRINTF("gpiof_data_addr(%p) set value=0x%08x\n", gpiof_data_addr, value);
	*(volatile unsigned int *)gpiof_data_addr = (value & (~GPIOF_OFFSET)) | temp;
	value = *(volatile unsigned int *)gpiof_data_addr; //read back
	DPRINTF("gpiof_data_addr readback=0x%08x\n", value);
	isp_gpio_unmap(gpiof_dir_addr, gpiof_dir_fd);
	isp_gpio_unmap(gpiof_data_addr, gpiof_data_fd);

	/*enable CPLD JTAG GPIO to GPIO mode*/
	scu84_addr = isp_gpio_map(SYSTEM_SCU_ADDR(SYSTEM_SCU84), &scu84_fd);
	value = *(volatile unsigned int *)scu84_addr;
	*(volatile unsigned int *)scu84_addr = value & (~CPLD_JTAG_GPIOL_ENABLE);
	value = *(volatile unsigned int *)scu84_addr; //read back
	DPRINTF("scu84_addr(%p) set value=0x%08x\n", scu84_addr, value);
	isp_gpio_unmap(scu84_addr, scu84_fd);
	/*get CPLD JTAG GPIO base address*/
	gpio_base = (volatile unsigned int *)isp_gpio_map(GPIOL_DATA_ADDR, &mem_fd);
	gpio_dir_base = (volatile unsigned int *)isp_gpio_map(GPIOL_DIR_ADDR, &mem_dir_fd);

	return 0;
}
void isp_gpio_uninit(void)
{
	munmap(gpio_base, MAP_SIZE);
	munmap(gpio_dir_base, MAP_SIZE);
	close(mem_fd);
	close(mem_dir_fd);
}
void isp_gpio_config(unsigned int gpio, int dir)
{
	if(dir) { //1:in, 0:out
		*(gpio_dir_base) &= ~(0x1 << gpio);
	} else {
		*(gpio_dir_base) |= (0x1 << gpio);
	}
}

int isp_vme_file_size_set(char *file_name)
{
	struct stat statbuf;

	stat(file_name, &statbuf);
	vme_file_size = statbuf.st_size;

	return 0;
}

long isp_vme_file_size_get(void)
{
	return vme_file_size;
}
int isp_print_progess_bar(long pec)
{
	int i = 0;

	printf("\033[?251");
	printf("\r");

	for(i = 0; i < pec / 2; i++) {
		printf("\033[;42m \033[0m");
	}
	for(i = pec / 2; i < 50; i++) {
		printf("\033[47m \033[0m");
	}
	printf(" [%d%%]", pec);
	fflush(stdout);
	if(pec == 100) {
		printf("\n");
	}

	return 0;
}
#endif
