/******************************************************************************
*
* Filename: MPG_Huffman.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains functions for reading the Huffman coded data
*              from the bitstream.
*
******************************************************************************/

/* Include files */
#include "MP3_Main.h"
#include "MP3_Huffman.h"
#include "MP3_Huffman_Table.h"

/* External functions and variables (defined elsewhere, and used here) */
extern UINT32 MPG_Get_Main_Bits (UINT32 number_of_bits);
extern STATUS MPG_Set_Main_Pos (UINT32 bit_pos);
extern UINT32 MPG_Get_Main_Pos (void);

extern t_sf_band_indices g_sf_band_indices[3];

/* Global functions and variables (defined here, and used here & elsewhere) */
void MPG_Read_Huffman (UINT32 part_2_start, UINT32 gr, UINT32 ch);

/* Local functions and variables (defined here, used here) */
static STATUS MPG_Huffman_Decode (UINT32 table_num, INT32 *x, INT32 *y, 
				  INT32 *v, INT32 *w);

/******************************************************************************
*
* Name: MPG_Read_Huffman
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function is called by MPG_Read_Main_L3 to read the Huffman
*              coded data from the bitstream.
* Parameters: None
* Return value: None. The data is stored in g_main_data.is[ch][gr][freqline].
*
******************************************************************************/
void 
MPG_Read_Huffman (UINT32 part_2_start, UINT32 gr, UINT32 ch)
{
  INT32 x, y, v, w;
  UINT32 table_num, is_pos, bit_pos_end, sfreq;
  UINT32 region_1_start, region_2_start; /* region_0_start = 0 */
  

  /* Check that there is any data to decode. If not, zero the array. */
  if (g_side_info.part2_3_length[gr][ch] == 0) {

    for (is_pos = 0; is_pos < 576; is_pos++) {
      g_main_data.is[gr][ch][is_pos] = 0.0;
    }

    return;
  }
  
  /* Calculate bit_pos_end which is the index of the last bit for this part. */
  bit_pos_end = part_2_start + g_side_info.part2_3_length[gr][ch] - 1;
  
  /* Determine region boundaries */
  if ((g_side_info.win_switch_flag[gr][ch] == 1) && 
      (g_side_info.block_type[gr][ch] == 2)) { 

    region_1_start = 36;  /* sfb[9/3]*3=36 */
    region_2_start = 576; /* No Region2 for short block case. */

  } else {
    sfreq = g_frame_header.sampling_frequency;
    region_1_start = 
      g_sf_band_indices[sfreq].l[g_side_info.region0_count[gr][ch] + 1]; 
    region_2_start = 
      g_sf_band_indices[sfreq].l[g_side_info.region0_count[gr][ch] + 
				g_side_info.region1_count[gr][ch] + 2];
  }

  /* Read big_values using tables according to region_x_start */
  for (is_pos = 0; is_pos < g_side_info.big_values[gr][ch] * 2; is_pos++) {
    
    if (is_pos < region_1_start) {
      table_num = g_side_info.table_select[gr][ch][0];
    } else if (is_pos < region_2_start) {
      table_num = g_side_info.table_select[gr][ch][1];
    } else {
      table_num = g_side_info.table_select[gr][ch][2];
    }
    
    /* Get next Huffman coded words */
    (void) MPG_Huffman_Decode (table_num, &x, &y, &v, &w); 

    /* In the big_values area there are two freq lines per Huffman word */
    g_main_data.is[gr][ch][is_pos++] = x;
    g_main_data.is[gr][ch][is_pos] = y;

  }

  /* Read small values until is_pos = 576 or we run out of huffman data */
  table_num = g_side_info.count1table_select[gr][ch] + 32;
  for (is_pos = g_side_info.big_values[gr][ch] * 2; 
       (is_pos <= 572) && (MPG_Get_Main_Pos () <= bit_pos_end); is_pos++) {

    /* Get next Huffman coded words */
    (void) MPG_Huffman_Decode (table_num, &x, &y, &v, &w); 
    
    g_main_data.is[gr][ch][is_pos++] = v;
    if (is_pos >= 576) break;	

    g_main_data.is[gr][ch][is_pos++] = w;
    if (is_pos >= 576) break;
    
    g_main_data.is[gr][ch][is_pos++] = x;
    if (is_pos >= 576) break;
    
    g_main_data.is[gr][ch][is_pos] = y;
  }

  /* Check that we didn't read past the end of this section */
  if (MPG_Get_Main_Pos () > (bit_pos_end+1)) {
    /* Remove last words read */
    is_pos -= 4;
  }

  /* Setup count1 which is the index of the first sample in the rzero reg. */
  g_side_info.count1[gr][ch] = is_pos;
  
  /* Zero out the last part if necessary */
  for (/* is_pos comes from last for-loop */; is_pos < 576; is_pos++) {
    g_main_data.is[gr][ch][is_pos] = 0.0;
  }

  /* Set the bitpos to point to the next part to read */
  (void) MPG_Set_Main_Pos (bit_pos_end+1);

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_Huffman_Decode
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function reads and decodes the next Huffman code word from
*              the main_data bit reservoir.
* Parameters: Huffman table number and four pointers for the return values.
* Return value: Two (x, y) or four (x, y, v, w) decoded Huffman words.
*
******************************************************************************/
static STATUS
MPG_Huffman_Decode (UINT32 table_num, INT32 *x, INT32 *y, INT32 *v, INT32 *w)
{
  UINT32 point, error, bitsleft, treelen, linbits;
  UINT32 *htptr;


  point = 0;
  error = 1;
  bitsleft = 32;

  /* Check for empty tables */
  if (g_huffman_main[table_num][1 /* treelen */] == 0) {
    *x = *y = *v = *w = 0;
    return (OK);
  }

  treelen = g_huffman_main[table_num][1 /* treelen */];
  linbits = g_huffman_main[table_num][2 /* linbits */];
  htptr = (UINT32 *) g_huffman_main[table_num][0];


  /* Start reading the Huffman code word, bit by bit */
  do {
    /* Check if we've matched a code word */
    if ((htptr[point] & 0xffff0000) == 0x00000000) {
      error = 0;
      *x = (htptr[point] >> 4) & 0xf;
      *y = htptr[point] & 0xf;
      break;
    }

    if (MPG_Get_Main_Bit ()) { /* Go right in tree */
      while ((htptr[point] & 0xff) >= 250) {
	point += htptr[point] & 0xff; 
      }
      point += htptr[point] & 0xff;
    } else { /* Go left in tree */
      while ((htptr[point] >> 16) >= 250) {
	point += htptr[point] >> 16; 
      }
      point += htptr[point] >> 16;
    }

  } while ((--bitsleft > 0) && (point < treelen));

  /* Check for error. */
  if (error) {
    ERR ("Illegal Huff code in data. bleft = %d, point = %d. tab = %d.",
	 bitsleft, point, table_num);
    *x = *y = 0;
  }

  /* Process sign encodings for quadruples tables. */
  if (table_num > 31) {
    *v = (*y >> 3) & 1;
    *w = (*y >> 2) & 1;
    *x = (*y >> 1) & 1;
    *y = *y & 1;

    if (*v > 0)
      if (MPG_Get_Main_Bit () == 1) *v = -*v;
    if (*w > 0)
      if (MPG_Get_Main_Bit () == 1) *w = -*w;
    if (*x > 0)
      if (MPG_Get_Main_Bit () == 1) *x = -*x;
    if (*y > 0)
      if (MPG_Get_Main_Bit () == 1) *y = -*y;

  } else {
    /* Get linbits */
    if ((linbits > 0) && (*x == 15)) {
      *x += MPG_Get_Main_Bits (linbits);
    }

    /* Get sign bit */
    if (*x > 0) {
      if (MPG_Get_Main_Bit () == 1) *x = -*x;
    }

    /* Get linbits */
    if ((linbits > 0) && (*y == 15)) {
      *y += MPG_Get_Main_Bits (linbits);
    }

    /* Get sign bit */
    if (*y > 0) {
      if (MPG_Get_Main_Bit () == 1) *y = -*y;
    }

  }
  
  /* Done */
  return (error ? ERROR : OK);  

} 
