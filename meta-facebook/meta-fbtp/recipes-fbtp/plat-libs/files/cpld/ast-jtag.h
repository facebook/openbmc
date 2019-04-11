#ifndef __AST_JTAG_H__
#define __AST_JTAG_H__

typedef enum jtag_xfer_mode {
	HW_MODE = 0,
	SW_MODE
} xfer_mode;


typedef enum {
    JTAGDriverState_Master = 0,
    JTAGDriverState_Slave
} JTAGDriverState;

struct controller_mode_param {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     controller_mode;
};

struct runtest_idle {
	xfer_mode 	mode;		//0 :HW mode, 1: SW mode
	unsigned char 	reset;		//Test Logic Reset
	unsigned char 	end;		//o: idle, 1: ir pause, 2: drpause
	unsigned char 	tck;		//keep tck
};

struct sir_xfer {
	xfer_mode 	mode;		//0 :HW mode, 1: SW mode
	unsigned short length;		//bits
	unsigned int tdi;
	unsigned int tdo;
	unsigned char endir;		//0: idle, 1:pause
};

struct sdr_xfer {
	xfer_mode 	mode;		//0 :HW mode, 1: SW mode
	unsigned char 	direct; 	// 0 ; read , 1 : write
	unsigned short length;		//bits
	unsigned int *tdio;
	unsigned char enddr;		//0: idle, 1:pause
};

#define JTAGIOC_BASE		'T'

#define AST_JTAG_IOCRUNTEST	_IOW(JTAGIOC_BASE, 0, struct runtest_idle)
#define AST_JTAG_IOCSIR		_IOWR(JTAGIOC_BASE, 1, struct sir_xfer)
#define AST_JTAG_IOCSDR		_IOWR(JTAGIOC_BASE, 2, struct sdr_xfer)
#define AST_JTAG_SIOCFREQ	_IOW(JTAGIOC_BASE, 3, unsigned int)
#define AST_JTAG_GIOCFREQ	_IOR(JTAGIOC_BASE, 4, unsigned int)
#define AST_JTAG_SLAVECONTLR      _IOW( JTAGIOC_BASE, 8, struct controller_mode_param)

/******************************************************************************************************************/
extern int ast_jtag_open(void);
extern void ast_jtag_close(void);
extern unsigned int ast_get_jtag_freq(void);
extern int ast_set_jtag_freq(unsigned int freq);
extern int ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck);
extern unsigned int ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi);
extern int ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio);
extern int ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio);

#endif /* __AST_JTAG_H__ */
