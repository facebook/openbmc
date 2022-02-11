/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <inttypes.h>
#include <getopt.h>
#include "mkeeprom.h"
struct wedge_eeprom_st *eeprom;
bool set_fbw_local_mac = 0;
void dump_eeprom(struct wedge_eeprom_st *eeprom) {
	printf("dump_eeprom\n");
	printf("eeprom->fbw_product_name = 0x%x\n",eeprom->fbw_magic);
	printf("eeprom->fbw_version = 0x%x\n",eeprom->fbw_version);
	printf("eeprom->fbw_product_name = %s\n",eeprom->fbw_product_name);
	printf("eeprom->fbw_product_number = %s\n",eeprom->fbw_product_number);
	printf("eeprom->fbw_assembly_number = %s\n",eeprom->fbw_assembly_number);
	printf("eeprom->fbw_facebook_pcba_number = %s\n",eeprom->fbw_facebook_pcba_number);
	printf("eeprom->fbw_facebook_pcb_number = %s\n",eeprom->fbw_facebook_pcb_number);
	printf("eeprom->fbw_odm_pcb_number = %s\n",eeprom->fbw_odm_pcb_number);
	printf("eeprom->fbw_odm_pcb_serial = %s\n",eeprom->fbw_odm_pcb_serial);
	printf("eeprom->fbw_production_state = 0%x\n",eeprom->fbw_production_state);
	printf("eeprom->fbw_product_version = 0x%x\n",eeprom->fbw_product_version);
	printf("eeprom->fbw_product_subversion = 0x%x\n",eeprom->fbw_product_subversion);
	printf("eeprom->fbw_product_serial = %s\n",eeprom->fbw_product_serial);
	printf("eeprom->fbw_product_asset = %s\n",eeprom->fbw_product_asset);
	printf("eeprom->fbw_system_manufacturer = %s\n",eeprom->fbw_system_manufacturer);
	//printf("eeprom->fbw_system_manufacturing_date = 0x%x\n",eeprom->fbw_system_manufacturing_date);
	printf("eeprom->fbw_system_manufacturing_date = %d-%d-%d\n",eeprom->fbw_system_manufacturing_year,eeprom->fbw_system_manufacturing_month,eeprom->fbw_system_manufacturing_day);
	printf("eeprom->fbw_pcb_manufacturer = %s\n",eeprom->fbw_pcb_manufacturer);
	printf("eeprom->fbw_assembled = %s\n",eeprom->fbw_assembled);
	printf("eeprom->fbw_local_mac = %s\n",eeprom->fbw_local_mac);
	printf("eeprom->fbw_mac_base = %s\n",eeprom->fbw_mac_base);
	printf("eeprom->fbw_mac_size = 0x%x\n",eeprom->fbw_mac_size);
	printf("eeprom->fbw_location = %s\n",eeprom->fbw_location);
}
void remove_all_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}
void parse_command(char* cmd, char* c) {
	char temp[100];
	printf("cmd = %s,c=%s\n",cmd,c);

	memset(temp,0,sizeof(temp));

	if (strcmp(cmd, "magic_word")==0) {

		sscanf(c, "%s %hx",cmd, &eeprom->fbw_magic);
		printf("eeprom->fbw_product_name = 0x%x\n",eeprom->fbw_magic);
	}
	if (strcmp(cmd, "format_version")==0) {
		//fbw_version

		sscanf(c, "%s %hhu",cmd, &eeprom->fbw_version);
		printf("eeprom->fbw_version = 0x%x\n",eeprom->fbw_version);
	}
	if (strcmp(cmd, "product_name")==0) {

		sscanf(c, "%s %s",cmd, eeprom->fbw_product_name);
		printf("eeprom->fbw_product_name = %s\n",eeprom->fbw_product_name);
	}
	if (strcmp(cmd, "product_number")==0) {
		//sscanf(c, "%s \"%9[^-"]\"",cmd, eeprom->fbw_product_number);
		sscanf(c, "%s %s", cmd, temp);
		  /* Product Part #: 8 byte data shown as XX-XXXXXXX */
		remove_all_chars(temp,'-');

		memcpy(eeprom->fbw_product_number,temp,FBW_EEPROM_F_PRODUCT_NUMBER);
		printf("eeprom->fbw_product_number = %s\n",eeprom->fbw_product_number);
	}
	if (strcmp(cmd, "assembly_number")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_assembly_number,temp,FBW_EEPROM_F_ASSEMBLY_NUMBER);
		printf("eeprom->fbw_assembly_number = %s\n",eeprom->fbw_assembly_number);
	}
	if (strcmp(cmd, "pcba_number")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_facebook_pcba_number,temp,FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER);
		printf("eeprom->fbw_facebook_pcba_number = %s\n",eeprom->fbw_facebook_pcba_number);
	}
	if (strcmp(cmd, "pcb_number")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_facebook_pcb_number,temp,FBW_EEPROM_F_FACEBOOK_PCB_NUMBER);
		printf("eeprom->fbw_facebook_pcb_number = %s\n",eeprom->fbw_facebook_pcb_number);
	}
	if (strcmp(cmd, "odm_pcb_number")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_odm_pcb_number,temp,FBW_EEPROM_F_ODM_PCB_NUMBER);
		printf("eeprom->fbw_odm_pcb_number = %s\n",eeprom->fbw_odm_pcb_number);
	}
	if (strcmp(cmd, "odm_pcb_serial")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_odm_pcb_serial,temp,FBW_EEPROM_F_ODM_PCB_SERIAL);
		printf("eeprom->fbw_odm_pcb_serial = %s\n",eeprom->fbw_odm_pcb_serial);
	}
	if (strcmp(cmd, "production_state")==0) {
		sscanf(c, "%s %hhu",cmd, &eeprom->fbw_production_state);
		printf("eeprom->fbw_production_state = 0%x\n",eeprom->fbw_production_state);
	}
	if (strcmp(cmd, "product_version")==0) {
		sscanf(c, "%s %hhu",cmd, &eeprom->fbw_product_version);
		printf("eeprom->fbw_product_version = 0x%x\n",eeprom->fbw_product_version);
	}
	if (strcmp(cmd, "product_subversion")==0) {
		sscanf(c, "%s %hhu",cmd, &eeprom->fbw_product_subversion);
		printf("eeprom->fbw_product_subversion = 0x%x\n",eeprom->fbw_product_subversion);
	}
	if (strcmp(cmd, "product_serial_number")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_product_serial,temp,FBW_EEPROM_F_PRODUCT_SERIAL);
		printf("eeprom->fbw_product_serial = %s\n",eeprom->fbw_product_serial);
	}
	if (strcmp(cmd, "product_asset_tag")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_product_asset,temp,FBW_EEPROM_F_PRODUCT_ASSET);
		printf("eeprom->fbw_product_asset = %s\n",eeprom->fbw_product_asset);
	}
	if (strcmp(cmd, "system_manufacturer")==0) {
		sscanf(c, "%s %s",cmd, eeprom->fbw_system_manufacturer);
		printf("eeprom->fbw_system_manufacturer = %s\n",eeprom->fbw_system_manufacturer);
	}
	if (strcmp(cmd, "system_manufacturing_date")==0) {
		sscanf(c, "%s %d-%d-%d",cmd, &eeprom->fbw_system_manufacturing_year,&eeprom->fbw_system_manufacturing_month,&eeprom->fbw_system_manufacturing_day);
		printf("eeprom->fbw_system_manufacturing_date = %d-%d-%d\n",eeprom->fbw_system_manufacturing_year,eeprom->fbw_system_manufacturing_month,eeprom->fbw_system_manufacturing_day);
	}
	if (strcmp(cmd, "pcb_manufacturer")==0) {
		sscanf(c, "%s %s",cmd, eeprom->fbw_pcb_manufacturer);
		printf("eeprom->fbw_pcb_manufacturer = %s\n",eeprom->fbw_pcb_manufacturer);
	}
	if (strcmp(cmd, "assembled_location")==0) {
		sscanf(c, "%s %s",cmd, eeprom->fbw_assembled);
		printf("eeprom->fbw_assembled = %s\n",eeprom->fbw_assembled);
	}
	if ((strcmp(cmd, "local_mac_address")==0) && (set_fbw_local_mac != 1)){
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_local_mac,temp,FBW_EEPROM_F_LOCAL_MAC);
		printf("eeprom->fbw_local_mac = %s\n",eeprom->fbw_local_mac);
	}
	if (strcmp(cmd, "extended_mac_base")==0) {
		sscanf(c, "%s %s", cmd, temp);
		remove_all_chars(temp,'-');
		memcpy(eeprom->fbw_mac_base,temp,FBW_EEPROM_F_EXT_MAC_BASE);
		printf("eeprom->fbw_mac_base = %s\n",eeprom->fbw_mac_base);
	}
	if (strcmp(cmd, "extended_mac_size")==0) {
		sscanf(c, "%s %d",cmd, &eeprom->fbw_mac_size);
		printf("eeprom->fbw_mac_size = 0x%x\n",eeprom->fbw_mac_size);
	}
	if (strcmp(cmd, "location_on_fabric")==0) {
		sscanf(c, "%s %s",cmd, eeprom->fbw_location);
		printf("eeprom->fbw_location = %s\n",eeprom->fbw_location);
	}
}

int read_text(char* in) {
	FILE * fp;
	char * line = NULL;
	char * c = NULL;
	size_t len = 0;
	ssize_t read;
	char *comment = NULL;
	int atomct = 2;
	int linect = 0;
	char * command = (char*) malloc (101);
	int i;

	printf("Opening file %s for read\n", in);

	fp = fopen(in, "r");
	if (fp == NULL) {
		printf("Error opening input file\n");
		return -1;
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		linect++;
		c = line;

		for (i=0; i<read; i++) if (c[i]=='#') c[i]='\0';

		while (isspace(*c)) ++c;

		if (*c=='\0' || *c=='\n' || *c=='\r') {
			//empty line, do nothing
		} else if (isalnum (*c)) {
			sscanf(c, "%100s", command);
#ifdef DEBUG
			printf("Processing line %u: %s", linect, c);
			if ((*(c+strlen(c)-1))!='\n') printf("\n");
#endif
			parse_command(command, c);
		} else printf("Can't parse line %u: %s", linect, c);
	}

	printf("Done reading\n");
#if 1 //def DEBUG
	printf("Done reading to dump_eeprom\n");
	dump_eeprom(eeprom);
#endif
	return 0;
}


/*
 * The eeprom size is 8K, we only use 157 bytes for v1 format.
 * Read 256 for now.
 */
#define FBW_EEPROM_SIZE 256

int write_binary(char* out) {
	FILE *fp;
	int i, offset;
	short crc;
	unsigned char *pack;
	uint8_t raw[FBW_EEPROM_SIZE];

	fp=fopen(out, "wb");
	if (!fp) {
		printf("Error writing file %s\n", out);
		return -1;
	}

	//fwrite(&header, sizeof(header), 1, fp);
	offset = 0;
	//fwrite(eeprom, sizeof(struct wedge_eeprom_st), 1, fp);
	//fwrite(eeprom, FBW_EEPROM_SIZE, 1, fp);

    memset(raw,0xff,FBW_EEPROM_SIZE);

	memcpy(raw,eeprom,sizeof(struct wedge_eeprom_st));
	//dump_eeprom(eeprom);

#ifdef DEBUG
	offset = 0;
	if (offset % 16 != 0) {
			printf("\n%08X :", (offset/16)*16);

			for (i = 0; i < (offset % 16); i++) {
					printf("   ");
			}
	}
	for(i = 0; i < FBW_EEPROM_SIZE; i++)
	{
			if ((offset + i) % 16 == 0) {
					printf("\n%08X :", offset + i);
			}

			printf(" %02X", raw[i]);
	}
	printf("\n");
#endif
	fwrite(raw, FBW_EEPROM_SIZE, 1, fp);

	fflush(fp);
	fclose(fp);
	return 0;
}


int main(int argc, char **argv) {
    char *infile, *outfile;
	char temp[100];

    /* supported cmdline options */
	int c, length, max_size, result;
    char options[] = "efghvri:lm:ws:c:o:";
	int ret;
	int i, custom_o=0;

    infile = outfile = NULL;
	//allocating memory and setting up required atoms
	eeprom = (struct wedge_eeprom_st *) malloc(sizeof(struct wedge_eeprom_st));
	memset(eeprom, "\0", sizeof(*eeprom));
	eeprom->fbw_crc8 = 0;
	printf("sizeof(struct wedge_eeprom_st)=%d\n",(int)sizeof(struct wedge_eeprom_st));

    while((c = getopt(argc, argv, options)) != -1) {
        switch(c) {
            case 'm':
                //result = sscanf(optarg, "%d", &max_size);
				result = sscanf(optarg, "%s", temp);
				remove_all_chars(temp,'-');
				memcpy(eeprom->fbw_local_mac,temp,FBW_EEPROM_F_LOCAL_MAC);
				set_fbw_local_mac = 1;
                if (result == 0 || result == EOF) {
                    fprintf(stderr, "\nError! Invalid maximum file size (-s %s)\n\n",
                            optarg);
                    return -1;
                }
                break;
            case 'i':
                infile = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'v':
                return 0;
            default:
                return -1;
        }
    }


	ret = read_text(infile);
	if (ret) {
		printf("Error reading and parsing input, aborting\n");
		return 0;
	}

	ret = write_binary(outfile);
	if (ret) {
		printf("Error writing output\n");
		return 0;
	}

	printf("Done.\n");

	return 0;
}
