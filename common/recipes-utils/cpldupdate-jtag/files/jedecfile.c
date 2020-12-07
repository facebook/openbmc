
/* Jedecfile .jed file parser

Copyright (C) Uwe Bonnes 2009 bon@elektron.ikp.physik.tu-darmstadt.de

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*
 * Using a slightly corrected version from IPAL libjedec
 * Copyright (c) 2000 Stephen Williams (steve@icarus.com)
 */ 

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include "jedecfile.h"

struct jedec_data jed;

static unsigned char *allocate_fusemap(unsigned size)
{
    unsigned char *ptr = (unsigned char*) calloc(size/8 + ((size%8)?1:0), 1);
    return ptr;
}

static int __jedec_get_fuse(jedec_data_t jed, unsigned idx)
{
    unsigned int bval, bit;
    if(idx >= jed->fuse_count){
        printf("%s(%d) invalid fuse index %u\n", __FILE__, __LINE__, idx);
        return -1;
    }

    bval = idx / 8;
    bit  = idx % 8;
    return (jed->fuse_list[bval] & (1 << bit))? 1 : 0;
}

static void __jedec_set_fuse(jedec_data_t jed, unsigned idx, int blow)
{
    unsigned int bval, bit;
    if(idx >= jed->fuse_count){
        printf("%s(%d) invalid fuse index %u\n", __FILE__, __LINE__, idx);
        return;
    }

    bval = idx / 8;
    bit  = idx % 8;

    if (blow){
        jed->fuse_list[bval] |=  (1 << bit);
    }else{
        jed->fuse_list[bval] &= ~(1 << bit);
    }
}

struct state_mach {
    jedec_data_t jed;

    void (*state)(int ch, struct state_mach*m);

    unsigned int checksum;
    union {
        struct {
            unsigned cur_fuse;
        } l;
    } m;
};

static void m_startup(int ch, struct state_mach*m);
static void m_header(int ch, struct state_mach*m);
static void m_base(int ch, struct state_mach*m);
static void m_C(int ch, struct state_mach*m);
static void m_L(int ch, struct state_mach*m);
static void m_Lfuse(int ch, struct state_mach*m);
static void m_Q(int ch, struct state_mach*m);
static void m_QF(int ch, struct state_mach*m);
static void m_QP(int ch, struct state_mach*m);
static void m_skip(int ch, struct state_mach*m);
static void m_N(int ch, struct state_mach*m);

int m_N_item;
int m_N_pos;
int m_H_pos  = 0;
int m_L_cfg_valid = 0;
char m_H_string[MAX_SIZE];
char m_N_strings[MAX_ITEM][MAX_SIZE];

static void m_startup(int ch, struct state_mach *m)
{
    switch (ch) 
    {
        /* STX in ASCII table */
        case '\002':
            m->state = m_base;
            break;
          
        case 'D':
            m->state = m_header;
            break;
          
        default:
            break;
    }
}

static void m_header(int ch, struct state_mach *m)
{
    switch (ch) 
    {
        case '\n':
        case '\r':
            if (m_H_pos)
            {
                char * ptr = strchr( m_H_string, ':');
                if (ptr){
                    strncpy(m->jed->date, ptr, MAX_SIZE);
                }
            }
            m->state = m_startup;
            break;
        default:
            m_H_string[m_H_pos] = ch;
            m_H_pos++;
    }
}

static void m_base(int ch, struct state_mach*m)
{
    if (isspace(ch))
        return;

    switch (ch) {
        case 'L':
            m->state = m_L;
            m->m.l.cur_fuse = 0;
            break;
        case 'Q':
            m->state = m_Q;
            break;
        case 'C':
            m->state = m_C;
            m->jed->checksum = 0;
            break;
        case 'N':
            m->state = m_N;
            m_N_item = -1;
            break;
        default:
            m->state = m_skip;
            break;
    }
}

static void m_C(int ch, struct state_mach*m)
{
    if (isspace(ch))
        return;

    if (ch == '*') {
        m->state = m_base;
        return;
    }

    if(ch >='0' && ch <='9')
    {
        m->jed->checksum <<=4;
        m->jed->checksum += ch - '0';
        return;
    }
    ch &= 0x5f;

    if(ch >='A' && ch <= 'F')
    {
        m->jed->checksum <<=4;
        m->jed->checksum += ch - '7';
        return;
    }

    fprintf(stderr, "m_C: Dangling '%c' 0x%02x\n", ch, ch);
    fflush(stderr);
    printf("%s(%d) %s - invalid JEDEC format\n", __FILE__, __LINE__, __FUNCTION__);
}

static void m_L(int ch, struct state_mach*m)
{
    if (isdigit(ch)) {
        m->m.l.cur_fuse *= 10;
        m->m.l.cur_fuse += ch - '0';
        return;
    }

    if (isspace(ch)) {
        m->state = m_Lfuse;
        if(m_L_cfg_valid){
            m->jed->cfg_count = m->m.l.cur_fuse;
        }
        return;
    }

    if (ch == '*') {
        m->state = m_base;
        return;
    }

    fprintf(stderr, "m_L: Dangling '%c' 0x%02x\n", ch, ch);
    fflush(stderr);
    m->state = 0;
}

static void m_Lfuse(int ch, struct state_mach*m)
{
    switch (ch) {
        case '*':
            m->state = m_base;
            break;

        case '0':
            __jedec_set_fuse(m->jed, m->m.l.cur_fuse, 0);
            m->m.l.cur_fuse += 1;
            break;

        case '1':
            __jedec_set_fuse(m->jed, m->m.l.cur_fuse, 1);
            m->m.l.cur_fuse += 1;
            break;

        case ' ':
        case '\n':
        case '\r':
            break;

        default:
            fprintf(stderr, "m_LFuse: Dangling '%c' 0x%02x\n", ch, ch);
            fflush(stderr);
            m->state = 0;
            break;
    }
}

#if defined(__unix__) || defined(__MACH__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

static void m_N(int ch, struct state_mach*m)
{
    switch (ch) {

    case '*':
        if ((stricmp(m_N_strings[0], "DEVICE")) == 0)
        {
            m_N_strings[m_N_item][m_N_pos] = 0;
            strncpy(m->jed->device, m_N_strings[1], MAX_SIZE);
        }
    
        if ((stricmp(m_N_strings[0], "VERSION")) == 0)
        {
            m_N_strings[m_N_item][m_N_pos] = 0;
            strncpy(m->jed->version, m_N_strings[1], MAX_SIZE);
        }

        if ((stricmp(m_N_strings[0], "DATE")) == 0)
        {
            m_N_strings[m_N_item][m_N_pos] = 0;
            strncpy(m->jed->date, m_N_strings[2], MAX_SIZE);
        }

        if (((stricmp(m_N_strings[0], "END")) == 0) 
            && ((stricmp(m_N_strings[1], "CONFIG")) == 0))
        {
            m_L_cfg_valid = 1;
        }
        
        m->state = m_base;
        m_N_item= -1;
        break;
    case ' ':
        if(m_N_item >=0){
            m_N_strings[m_N_item][m_N_pos] = '\0';
        }
        
        if (m_N_item < MAX_ITEM)
        {
            /* Don't stumble on too many items like in ISE XC2C Jedecfiles */
            m_N_item++;
        }
        m_N_pos = 0;
    case '\n':
    case '\r':
        break;

    default:
        if((m_N_item >=0) && (m_N_item <MAX_ITEM) && (m_N_pos < MAX_SIZE-1)){
            m_N_strings[m_N_item][m_N_pos] = ch;
        }
        m_N_pos++;
        break;
    }
}

static void m_Q(int ch, struct state_mach*m)
{
    switch (ch) {
    case 'F':
        if (m->jed->fuse_count != 0) {
            m->state = 0;
            return;
        }
        m->state = m_QF;
        m->jed->fuse_count = 0;
        break;
    case 'P':
        if (m->jed->pin_count != 0) {
            m->state = 0;
            return;
        }
        m->state = m_QP;
        m->jed->pin_count = 0;
        break;

    default:
        m->state = m_skip;
        break;
    }
}

static void m_QF(int ch, struct state_mach*m)
{
    if (isspace(ch))
        return;

    if (isdigit(ch)) {
        m->jed->fuse_count *= 10;
        m->jed->fuse_count += ch - '0';
        return;
    }

    if (ch == '*') {
        m->state = m_base;
        m->jed->fuse_list = allocate_fusemap(m->jed->fuse_count);
        return;
    }

    printf("%s(%d) %s - invalid JEDEC format\n", __FILE__, __LINE__, __FUNCTION__);
}

static void m_QP(int ch, struct state_mach*m)
{
    if (isspace(ch))
        return;

    if (isdigit(ch)) {
        m->jed->pin_count *= 10;
        m->jed->pin_count += ch - '0';
        return;
    }

    if (ch == '*') {
        m->state = m_base;
        return;
    }

    printf("%s(%d) %s - invalid JEDEC format\n", __FILE__, __LINE__, __FUNCTION__);
}

static void m_skip(int ch, struct state_mach*m)
{
    switch (ch) {
        case '*':
            m->state = m_base;
            break;

        default:
            break;
    }
}

void jedec_init()
{
    jed.checksum = 0;
    jed.fuse_count = 0;
    jed.pin_count = 0;
    jed.fuse_list = 0;
    jed.device[0] = 0;
    jed.version[0] = 0;
    jed.date[0] = 0;
}

void jedec_uninit(void)
{
    if(jed.fuse_list){
        free(jed.fuse_list);
    }
}

int jedec_read_file(FILE *fp)
{
    int ch;
    struct state_mach m;

    if(!fp) 
        return -1;

    m.jed = &jed;
    m.state = m_startup;
    while ((ch = fgetc(fp)) != EOF) {
        m.state(ch, &m);
        if (m.state == 0) {
            /* Some sort of error happened. */
            return -2;
        }
    }

    if (!jed.fuse_count){
        return -3;
    }else{
        return 0;
    }
}

void jedec_save_as_jed(const char  *device, FILE *fp)
{
    unsigned int i, b=0, l=0 ,w=0;
    unsigned short chksum=0;
    unsigned int DRegLength;
    int type=-1;

    if (!fp)
        return;
    if (strnicmp("XC9536",device, sizeof("XC9536X")-1) == 0)
    {
        type = JED_XC95;
    }
    else if (strnicmp("XC9572",device, sizeof("XC9572X")-1) == 0)
    {
        type = JED_XC95;
    }
    else if (strnicmp("XC95108",device, sizeof("XC95144X")-1) == 0)
    {
        type = JED_XC95;
    }
    else if (strnicmp("XC95144",device, sizeof("XC95288X")-1) == 0)
    {
        type = JED_XC95;
    } 
    else if (strnicmp("XC95216",device, sizeof("XC95288X")-1) == 0)
    {
        type = JED_XC95;
    } 
    else if (strnicmp("XC95288",device, sizeof("XC95288X")-1) == 0)
    {
        type = JED_XC95;
    } 
    else if (strnicmp("XC9536X",device, sizeof("XC9536X")-1) == 0)
    {
        type = JED_XC95X;
        DRegLength=2;
    }
    else if (strnicmp("XC9572X",device, sizeof("XC9572X")-1) == 0)
    {
        type = JED_XC95X;
        DRegLength=4;
    }
    else if (strnicmp("XC95144X",device, sizeof("XC95144X")-1) == 0)
    {
        type = JED_XC95X;
        DRegLength=8;
    }
    else if (strnicmp("XC95288X",device, sizeof("XC95288X")-1) == 0)
    {
        type = JED_XC95X;
        DRegLength=16;
    } 
    else if (strnicmp("XC2C",device, sizeof("XC2C")-1) == 0)
    {
        type = JED_XC2C;
    } 

    if (strlen(jed.date) == 0)
    {
        time_t t;
        struct tm *tmp;
        char outstr[200];
        t = time(NULL);
        tmp = localtime(&t);
        if (tmp != NULL)
        {
            if (strftime(outstr, sizeof(outstr), "%a %b %d %T %Y", tmp)){
                fprintf(fp, "Date Extracted: %s\n\n", outstr);
            }
        }
    }else{
        fprintf(fp, "Date Extracted%s\n\n",jed.date);
    }
    
    fprintf(fp, "\2QF%d*\nQV0*\nF0*\nX0*\nJ0 0*\n",jed.fuse_count);
    if (strlen(jed.version) == 0)
        fprintf(fp, "N VERSION XC3SPROG*\n");
    else
        fprintf(fp, "N VERSION %s*\n",jed.version);
    fprintf(fp, "N DEVICE %s*\n", device);

    if(type == JED_XC95X)
    {
        /* Xilinx Impact (10.1) needs following additional items 
         * to recognizes as a valid Jedec File
         * the 4 Digits as total Checksum
         * N DEVICE
         */

        for (i=0; i<jed.fuse_count; i++)
        {
            if(!w && !b)
            fprintf(fp, "L%07d",i);
            if (!b)
            fprintf(fp, " ");
            fprintf(fp, "%d", jedec_get_fuse(i));
            if (l<9)
            {
                if(b==7)
                    b=0;
                else
                    b++;
            }
            else 
            {
                if (b == 5)
                    b = 0;
                else
                    b++;
            }
            
            if(!b)
            {
                if (w == (DRegLength-1))
                {
                    fprintf(fp, "*\n");
                    w = 0;
                    l++;
                }else{
                    w++;
                }
            }

            if (l==15){
                l =0;
            }
        }
    }
    else if(type == JED_XC95)
    {
        for (i=0; i<jed.fuse_count; i++)
        {
            if(!b && w%5 == 0)
                fprintf(fp, "L%07d",i);
            if (!b)
                fprintf(fp, " ");
            fprintf(fp, "%d", jedec_get_fuse(i));
            if( b == (((i%9072)<7776) ? ((w %15 < 9)?7:5):((w%5== 0)?7:6)))
            {
                b=0;
                w++;
            }
            else
                b++;
            if(!b && (w %5 == 0 ))
                fprintf(fp, "*\n");
        }
    }
    else if (type == JED_XC2C)
    {
        for (i=0; i<jed.fuse_count; i++)
        {
            if ((i %64) == 0){
                fprintf(fp, "L%07d ",i);
            }
            fprintf(fp, "%d", jedec_get_fuse(i));
            if ((i %64) == 63){
                fprintf(fp, "*\n");
            }
        }
        fprintf(fp, "*\n");
    }

    for(i=0; i<(jed.fuse_count/8 + ((jed.fuse_count%8)?1:0)); i++){
        chksum += jed.fuse_list[i];
    }
    fprintf(fp, "C%04X*\n%c0000\n", chksum, 3);
}

void jedec_set_length(unsigned int f_count)
{    
    if(f_count > jed.fuse_count)
    {
        if (jed.fuse_list){
            free(jed.fuse_list);
        }
        jed.fuse_list = (unsigned char *)malloc(f_count/8 + ((f_count%8)?1:0));
        if(jed.fuse_list != NULL){
            memset(jed.fuse_list, 0xff, f_count/8 + ((f_count%8)?1:0));
        }else{
            printf("%s(%d) %s - failed to allocate memory!\n", __FILE__, __LINE__, __FUNCTION__);
            return;
        }
    }
    else
    {
        for (unsigned int i = f_count; i < jed.fuse_count; i++){
            jedec_set_fuse(i, 0);
        }
    }
    jed.fuse_count = f_count;
}

unsigned int jedec_get_length(){
    return jed.fuse_count;
}

void jedec_set_fuse(unsigned int idx, int blow)
{
    __jedec_set_fuse(&jed, idx,blow);
}

int jedec_get_fuse(unsigned int idx)
{
    return __jedec_get_fuse(&jed, idx);
}

unsigned char jedec_get_byte(unsigned int idx)
{
    return jed.fuse_list[idx];
}

unsigned int jedec_get_long(unsigned int idx)
{
    int l = sizeof(unsigned int);
    return (jed.fuse_list[l*idx])|(jed.fuse_list[l*idx+1]<<8) \
        |(jed.fuse_list[l*idx+2]<<16)|(jed.fuse_list[l*idx+3]<<24);
}

unsigned short jedec_calc_checksum()
{
    unsigned int i;
    unsigned short cc=0;
  
    for(i=0; i<(jed.fuse_count/8 + ((jed.fuse_count%8)?1:0)); i++){
        cc += jed.fuse_list[i];
    }

    return cc;
}

unsigned short jedec_get_checksum(){
    return jed.checksum;
}

char *jedec_get_device(){
    return jed.device;
}

char *jedec_get_version(){
    return jed.version;
}

char *jedec_get_date(){
    return jed.date;
}

int jedec_get_cfg_fuse(){
    return jed.cfg_count;
}

// TODO
int jedec_get_user_code(unsigned int *usercode) {
    return -1;
}

// TODO
int jedec_get_feature_row(unsigned char *buff, unsigned int *length) {
    return -1;
}

// TODO
int jedec_get_feature_bits(unsigned char *buff, unsigned int *length) {
    return -1;
}
