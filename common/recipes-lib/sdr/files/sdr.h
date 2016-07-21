/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SDR_H__
#define __SDR_H__

#include <stdbool.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SDR_LEN           64
#define NORMAL_STATE        0x00
#define MAX_SENSOR_NUM      0xFF

#define MAX_SENSOR_RATE_UNIT  7
#define MAX_SENSOR_BASE_UNIT  92

#define SETBIT(x, y)        (x | (1 << y))
#define GETBIT(x, y)        ((x & (1 << y)) > y)
#define CLEARBIT(x, y)      (x & (~(1 << y)))
#define GETMASK(y)          (1 << y)

/* To hold the sensor info and calculated threshold values from the SDR */
typedef struct {
  uint16_t flag;
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
  int curr_state;
  char name[32];
  char units[64];

} thresh_sensor_t;

/* List of all the Sensor Rate Units types. */
const char * sensor_rate_units[] = {
  "",             /* 0x00 */
  "per \xC2\xB5s",  /* 0x01 */
  "per ms",         /* 0x02 */
  "per s",          /* 0x03 */
  "per min",        /* 0x04 */
  "per hour",       /* 0x05 */
  "per day",        /* 0x06 */
  "reserved",       /* 0x07 */
};


/* List of all the Sensor Base Units types. */
const char * sensor_base_units[] = {
  "",             /* 000 */
  "C",              /* 001 */
  "F",              /* 002 */
  "K",              /* 003 */
  "Volts",          /* 004 */
  "Amps",           /* 005 */
  "Watts",          /* 006 */
  "Joules",         /* 007 */
  "Coulombs",       /* 008 */
  "VA",             /* 009 */
  "Nits",           /* 010 */
  "lumen",          /* 011 */
  "lux",			      /* 012 */
  "Candela",			  /* 013 */
  "kPa",			      /* 014 */
  "PSI",			      /* 015 */
  "Newton",			    /* 016 */
  "CFM",			      /* 017 */
  "RPM",			      /* 018 */
  "Hz",			        /* 019 */
  "\xC2\xB5s",			/* 020 */
  "ms",			        /* 021 */
  "sec",			      /* 022 */
  "min",			      /* 023 */
  "hour",			      /* 024 */
  "day",			      /* 025 */
  "week",			      /* 026 */
  "mil",			      /* 027 */
  "inches",			    /* 028 */
  "feet",			      /* 029 */
  "cu in",			    /* 030 */
  "cu feet",			  /* 031 */
  "mm",			        /* 032 */
  "cm",			        /* 033 */
  "m",			        /* 034 */
  "cu cm",			    /* 035 */
  "cu m",			      /* 036 */
  "liters",			    /* 037 */
  "fluid ounce",		/* 038 */
  "radians",			  /* 039 */
  "steradians",			/* 040 */
  "revolutions",		/* 041 */
  "cycles",			    /* 042 */
  "gravities",			/* 043 */
  "ounce",			    /* 044 */
  "pound",			    /* 045 */
  "ft-lb",			    /* 046 */
  "oz-in",			    /* 047 */
  "gauss",			    /* 048 */
  "gilberts",			  /* 049 */
  "henry",			    /* 050 */
  "millihenry",			/* 051 */
  "farad",			    /* 052 */
  "microfarad",			/* 053 */
  "ohms",			      /* 054 */
  "siemens",			  /* 055 */
  "mole",			      /* 056 */
  "becquerel",			/* 057 */
  "PPM",						/* 058 */
  "reserved",				/* 059 */
  "Db",						  /* 060 */
  "DbA",						/* 061 */
  "DbC",						/* 062 */
  "gray",						/* 063 */
  "sievert",				/* 064 */
  "color temp deg K", /* 065 */
  "bit",					  /* 066 */
  "kilobit",			 	/* 067 */
  "megabit",			 	/* 068 */
  "gigabit",			 	/* 069 */
  "B",						  /* 070 */
  "KB",						  /* 071 */
  "MB",						  /* 072 */
  "GB",						  /* 073 */
  "word",						/* 074 */
  "dword",					/* 075 */
  "qword",					/* 076 */
  "line",						/* 077 */
  "hit",						/* 078 */
  "miss",						/* 079 */
  "retry",					/* 080 */
  "reset",					/* 081 */
  "overflow",				/* 082 */
  "underrun",				/* 083 */
  "collision",			/* 084 */
  "packets",				/* 085 */
  "messages",				/* 086 */
  "characters",			/* 087 */
  "error",					/* 088 */
  "correctable error",	  /* 089 */
  "uncorrectable error",	/* 090 */
  "fatal error",					/* 091 */
  "grams",					/* 092 */
  "",						    /* 093 */
};

int sdr_get_sensor_name(uint8_t fru, uint8_t snr_num, char *name);
int sdr_get_sensor_units(uint8_t fru, uint8_t snr_num, char *units);
int sdr_get_snr_thresh(uint8_t fru, uint8_t snr_num, thresh_sensor_t *snr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __SDR_H__ */
