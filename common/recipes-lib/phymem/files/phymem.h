#ifndef PHYMEM_H
#define PHYMEM_H

#include <sys/types.h>

typedef enum memLength {
	M_BYTE,
	M_WORD,
	M_DWORD
} memLength;

int phymem_get_byte(off_t addr, size_t offset, uint8_t *value);
int phymem_get_word(off_t addr, size_t offset, uint16_t *value);
int phymem_get_dword(off_t addr, size_t offset, uint32_t *value);

/*
 * ASPEED BMC only support 4 bytes align writing
 * Reserve for future use
 */
#if 0
int phymem_set_byte(off_t addr, size_t offset, uint8_t value);
int phymem_set_word(off_t addr, size_t offset, uint16_t value);
#endif

int phymem_set_dword(off_t addr, size_t offset, uint32_t value);
int phymem_dword_clear_bit(off_t base, size_t offset, uint8_t bit);
int phymem_dword_set_bit(off_t base, size_t offset, uint8_t bit);

#endif
