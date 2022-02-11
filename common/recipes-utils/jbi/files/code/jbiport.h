/****************************************************************************/
/*                                                                          */
/*  Module:        jbiport.h                                                */
/*                                                                          */
/*                 Copyright (C) Altera Corporation 2000-2001               */
/*                                                                          */
/*  Description:   Defines porting macros                                   */
/*                                                                          */
/****************************************************************************/

#ifndef INC_JBIPORT_H
#define INC_JBIPORT_H

/*
*  PORT defines the target platform: DOS, WINDOWS, UNIX, or EMBEDDED
*
*  PORT = DOS      means a 16-bit DOS console-mode application
*
*  PORT = WINDOWS  means a 32-bit WIN32 console-mode application for
*                  Windows 95, 98, 2000, ME or NT.  On NT this will use the
*                  DeviceIoControl() API to access the Parallel Port.
*
*  PORT = UNIX     means any UNIX system.  BitBlaster access is support via
*                  the standard ANSI system calls open(), read(), write().
*                  The ByteBlaster is not supported.
*
*  PORT = EMBEDDED means all DOS, WINDOWS, and UNIX code is excluded.
*                  Remaining code supports 16 and 32-bit compilers.
*                  Additional porting steps may be necessary. See readme
*                  file for more details.
*/

#define DOS      2
#define WINDOWS  3
#define UNIX     4
#define EMBEDDED 5

#ifndef PORT
/* change this line to build a different port */
#define PORT UNIX
#endif

#define OPENBMC
/*
 * Enabling the following will make jbi to use its own nsleep,
 * instead of relying on kernel's timer which is too coarse.
 * We need this for Helium
 */
#define USER_SPACE_NSLEEP

#endif /* INC_JBIPORT_H */
