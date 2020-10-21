/* SPDX-License-Identifier: GPL-2.0 */
// include/uapi/linux/jtag.h - JTAG class driver uapi
//
// Copyright (c) 2018 Mellanox Technologies. All rights reserved.
// Copyright (c) 2018 Oleksandr Shamray <oleksandrs@mellanox.com>

#ifndef __UAPI_LINUX_JTAG_H
#define __UAPI_LINUX_JTAG_H

/*
 * JTAG_XFER_MODE: JTAG transfer mode. Used to set JTAG controller transfer mode
 * This is bitmask for feature param in jtag_mode for ioctl JTAG_SIOCMODE
 */
#define JTAG_XFER_MODE 0
/*
 * JTAG_CONTROL_MODE: JTAG controller mode. Used to set JTAG controller mode
 * This is bitmask for feature param in jtag_mode for ioctl JTAG_SIOCMODE
 */
#define JTAG_CONTROL_MODE 1
/*
 * JTAG_SLAVE_MODE: JTAG slave mode. Used to set JTAG controller slave mode
 * This is bitmask for mode param in jtag_mode for ioctl JTAG_SIOCMODE
 */
#define JTAG_SLAVE_MODE 0
/*
 * JTAG_MASTER_MODE: JTAG master mode. Used to set JTAG controller master mode
 * This is bitmask for mode param in jtag_mode for ioctl JTAG_SIOCMODE
 */
#define JTAG_MASTER_MODE 1
/*
 * JTAG_XFER_HW_MODE: JTAG hardware mode. Used to set HW drived or bitbang
 * mode. This is bitmask for mode param in jtag_mode for ioctl JTAG_SIOCMODE
 */
#define JTAG_XFER_HW_MODE 1
/*
 * JTAG_XFER_SW_MODE: JTAG software mode. Used to set SW drived or bitbang
 * mode. This is bitmask for mode param in jtag_mode for ioctl JTAG_SIOCMODE
 */
#define JTAG_XFER_SW_MODE 0

struct scan_xfer
{
    __u8 mode;
    __u32 tap_state;
    __u32 length;
    __u8* tdi;
    __u32 tdi_bytes;
    __u8* tdo;
    __u32 tdo_bytes;
    __u32 end_tap_state;
} __attribute__((__packed__));

/**
 * enum jtag_tapstate:
 *
 * @JTAG_STATE_TLRESET: JTAG state machine Test Logic Reset state
 * @JTAG_STATE_IDLE: JTAG state machine IDLE state
 * @JTAG_STATE_SELECTDR: JTAG state machine SELECT_DR state
 * @JTAG_STATE_CAPTUREDR: JTAG state machine CAPTURE_DR state
 * @JTAG_STATE_SHIFTDR: JTAG state machine SHIFT_DR state
 * @JTAG_STATE_EXIT1DR: JTAG state machine EXIT-1 DR state
 * @JTAG_STATE_PAUSEDR: JTAG state machine PAUSE_DR state
 * @JTAG_STATE_EXIT2DR: JTAG state machine EXIT-2 DR state
 * @JTAG_STATE_UPDATEDR: JTAG state machine UPDATE DR state
 * @JTAG_STATE_SELECTIR: JTAG state machine SELECT_IR state
 * @JTAG_STATE_CAPTUREIR: JTAG state machine CAPTURE_IR state
 * @JTAG_STATE_SHIFTIR: JTAG state machine SHIFT_IR state
 * @JTAG_STATE_EXIT1IR: JTAG state machine EXIT-1 IR state
 * @JTAG_STATE_PAUSEIR: JTAG state machine PAUSE_IR state
 * @JTAG_STATE_EXIT2IR: JTAG state machine EXIT-2 IR state
 * @JTAG_STATE_UPDATEIR: JTAG state machine UPDATE IR state
 * @JTAG_STATE_CURRENT: JTAG current state, saved by driver
 */
enum jtag_tapstate
{
    JTAG_STATE_TLRESET,
    JTAG_STATE_IDLE,
    JTAG_STATE_SELECTDR,
    JTAG_STATE_CAPTUREDR,
    JTAG_STATE_SHIFTDR,
    JTAG_STATE_EXIT1DR,
    JTAG_STATE_PAUSEDR,
    JTAG_STATE_EXIT2DR,
    JTAG_STATE_UPDATEDR,
    JTAG_STATE_SELECTIR,
    JTAG_STATE_CAPTUREIR,
    JTAG_STATE_SHIFTIR,
    JTAG_STATE_EXIT1IR,
    JTAG_STATE_PAUSEIR,
    JTAG_STATE_EXIT2IR,
    JTAG_STATE_UPDATEIR,
    JTAG_STATE_CURRENT
};

/**
 * enum jtag_reset:
 *
 * @JTAG_NO_RESET: JTAG run TAP from current state
 * @JTAG_FORCE_RESET: JTAG force TAP to reset state
 */
enum jtag_reset
{
    JTAG_NO_RESET = 0,
    JTAG_FORCE_RESET = 1,
};

/**
 * enum jtag_xfer_type:
 *
 * @JTAG_SIR_XFER: SIR transfer
 * @JTAG_SDR_XFER: SDR transfer
 */
enum jtag_xfer_type
{
    JTAG_SIR_XFER = 0,
    JTAG_SDR_XFER = 1,
};

/**
 * enum jtag_xfer_direction:
 *
 * @JTAG_READ_XFER: read transfer
 * @JTAG_WRITE_XFER: write transfer
 * @JTAG_READ_WRITE_XFER: read & write transfer
 */
enum jtag_xfer_direction
{
    JTAG_READ_XFER = 1,
    JTAG_WRITE_XFER = 2,
    JTAG_READ_WRITE_XFER = 3,
};

/**
 * struct jtag_tap_state - forces JTAG state machine to go into a TAPC
 * state
 *
 * @reset: 0 - run IDLE/PAUSE from current state
 *         1 - go through TEST_LOGIC/RESET state before  IDLE/PAUSE
 * @end: completion flag
 * @tck: clock counter
 *
 * Structure provide interface to JTAG device for JTAG set state execution.
 */
struct jtag_tap_state
{
    __u8 reset;
    __u8 endstate;
    __u8 tck;
};

/**
 * struct jtag_xfer - jtag xfer:
 *
 * @type: transfer type
 * @direction: xfer direction
 * @length: xfer bits len
 * @tdio : xfer data array
 * @endir: xfer end state
 *
 * Structure provide interface to JTAG device for JTAG SDR/SIR xfer execution.
 */
struct jtag_xfer
{
    __u8 type;
    __u8 direction;
    __u8 endstate;
    __u8 padding;
    __u32 length;
    __u64 tdio;
};
/**
 * struct bitbang_packet - jtag bitbang array packet:
 *
 * @data:   JTAG Bitbang struct array pointer(input)
 * @length: array size (input)
 *
 * Structure provide interface to JTAG device for JTAG bitbang bundle execution.
 */
struct bitbang_packet
{
    struct tck_bitbang* data;
    __u32 length;
} __attribute__((__packed__));

/**
 * struct jtag_bitbang - jtag bitbang:
 *
 * @tms: JTAG TMS
 * @tdi: JTAG TDI (input)
 * @tdo: JTAG TDO (output)
 *
 * Structure provide interface to JTAG device for JTAG bitbang execution.
 */
struct tck_bitbang
{
    __u8 tms;
    __u8 tdi;
    __u8 tdo;
} __attribute__((__packed__));

/**
 * struct jtag_mode - jtag mode:
 *
 * @feature: 0 - JTAG feature setting selector for JTAG controller HW/SW
 *           1 - JTAG feature setting selector for controller
 *               bus(master/slave) mode.
 * @mode:    (0 - SW / 1 - HW) for JTAG_XFER_MODE feature(0)
 *           (0 - Slave / 1 - Master) for JTAG_CONTROL_MODE feature(1)
 *
 * Structure provide configuration modes to JTAG device.
 */
struct jtag_mode
{
    __u32 feature;
    __u32 mode;
};

/* ioctl interface */
#define __JTAG_IOCTL_MAGIC 0xb2

#define JTAG_SIOCSTATE _IOW(__JTAG_IOCTL_MAGIC, 0, struct jtag_tap_state)
#define JTAG_SIOCFREQ _IOW(__JTAG_IOCTL_MAGIC, 1, unsigned int)
#define JTAG_GIOCFREQ _IOR(__JTAG_IOCTL_MAGIC, 2, unsigned int)
#define JTAG_IOCXFER _IOWR(__JTAG_IOCTL_MAGIC, 3, struct jtag_xfer)
#define JTAG_GIOCSTATUS _IOWR(__JTAG_IOCTL_MAGIC, 4, enum jtag_tapstate)
#define JTAG_SIOCMODE _IOW(__JTAG_IOCTL_MAGIC, 5, struct jtag_mode)
#define JTAG_IOCBITBANG _IOW(__JTAG_IOCTL_MAGIC, 6, unsigned int)

#endif /* __UAPI_LINUX_JTAG_H */
