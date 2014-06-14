#ifndef _HUFFMAN_TABLES_H_
#define _HUFFMAN_TABLES_H_

/* The last two tables are A and B, respectively. The three values are:
 * Pointer to the table
 * Number of entries
 * Linbits
 * In the tables each 32-bit word contains left (0) offset in the high 32 bits
 * and right (1) offset in the low 32 bits. If left offset is 0, then the 
 * xyzw values are in the low 8 bits.
 */

#include "MP3_Main.h"

extern UINT32 g_huffman_main [34][3];

#endif /* _HUFFMAN_TABLES_H_ */
