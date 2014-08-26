/******************************************************************************
*
* Filename: MPG_Bitstream.h
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains definitions and declarations needed by 
*              functions using the bitstream functions.
*
******************************************************************************/

#ifndef _MPG_BITSTREAM_H_
#define _MPG_BITSTREAM_H_

/* Include files */
#include "MP3_Main.h"
#include "MP3_Bitstream.h"

/* External functions and variables (defined elsewhere, and used here) */


/* Global functions and variables (defined here, and used here & elsewhere) */
uint32_t MPG_Get_Byte (void);
STATUS MPG_Get_Bytes (uint32_t no_of_bytes, uint32_t data_vec[]);
uint32_t MPG_Get_Filepos (void);

/* Local functions and variables (defined here, used here) */


/* Definitions */

#define C_MPG_EOF           0xffffffff

#endif /* _MPG_BITSTREAM_H_ */
