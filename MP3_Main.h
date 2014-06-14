/******************************************************************************
*
* Filename: MPG_Main.h
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains the definitions used by most parts of the 
*              MPEG decoder.
*
******************************************************************************/

#ifndef _MPG_MAIN_H_
#define _MPG_MAIN_H_

#define SIM_UNIX

#define OK         0
#define ERROR     -1
#define TRUE       1
#define FALSE      0

typedef unsigned int  UINT32;
typedef int           INT32;
typedef short int     INT16;
typedef unsigned short int UINT16;
typedef float         FLOAT32;
typedef double        FLOAT64;
typedef int           BOOL;
typedef int           STATUS;
typedef char *        STRING;

/* Include files */
#include <stdio.h>
#include <math.h>

#define DBG(str, args...) { printf (str, ## args); printf ("\n"); }
#define ERR(str, args...) { fprintf (stderr, str, ## args) ; fprintf (stderr, "\n"); }
#define EXIT(str, args...) { printf (str, ## args);  printf ("\n"); exit (0); }

/* External functions and variables (defined elsewhere, and used here) */

/* Global functions and variables (defined here, and used here & elsewhere) */
STATUS MPG_Read_Frame (void);
STATUS MPG_Read_Header (void) ;
STATUS MPG_Read_CRC (void);
STATUS MPG_Read_Audio_L3 (void);
STATUS MPG_Read_Main_L3 (void);
void   MPG_Read_Ancillary (void);
UINT32 MPG_Get_Side_Bits (UINT32 number_of_bits);
UINT32 MPG_Get_Main_Bits (UINT32 number_of_bits);
UINT32 MPG_Get_Main_Bit (void);
STATUS MPG_Set_Main_Pos (UINT32 bit_pos);
UINT32 MPG_Get_Main_Pos (void);

/* Local functions and variables (defined here, used here) */

/* Global definitions */

#define C_MPG_SYNC             0xfff00000

#define C_PI                   3.14159265358979323846
#define C_INV_SQRT_2           0.70710678118654752440

#define Hz                           1
#define kHz                    1000*Hz
#define bit_s                        1
#define kbit_s                 1000*bit_s

/* Types used in the frame header */

/* Layer number */
typedef enum {
  mpeg1_layer_reserved = 0,
  mpeg1_layer_3        = 1,
  mpeg1_layer_2        = 2,
  mpeg1_layer_1        = 3
} t_mpeg1_layer;

/* Modes */
typedef enum {
  mpeg1_mode_stereo = 0,
  mpeg1_mode_joint_stereo,
  mpeg1_mode_dual_channel,
  mpeg1_mode_single_channel
} t_mpeg1_mode;

/* Bitrate table for all three layers.  */
extern UINT32 g_mpeg1_bitrates[3 /* lay 1-3 */][15 /* header bitrate_index */];

/* Sampling frequencies in hertz (valid for all layers) */
extern UINT32 g_sampling_frequency[3];

/* MPEG1 Layer 1-3 frame header */
typedef struct {
  UINT32 id;				      /* 1 bit */
  t_mpeg1_layer layer;			      /* 2 bits */
  UINT32 protection_bit;		      /* 1 bit */
  UINT32 bitrate_index;			      /* 4 bits */
  UINT32 sampling_frequency;		      /* 2 bits */
  UINT32 padding_bit;			      /* 1 bit */
  UINT32 private_bit;			      /* 1 bit */
  t_mpeg1_mode mode;			      /* 2 bits */
  UINT32 mode_extension;		      /* 2 bits */
  UINT32 copyright;			      /* 1 bit */
  UINT32 original_or_copy;		      /* 1 bit */
  UINT32 emphasis;			      /* 2 bits */
} t_mpeg1_header;

/* MPEG1 Layer 3 Side Information */
/* [2][2] means [gr][ch] */
typedef struct {
  UINT32 main_data_begin;                     /* 9 bits */
  UINT32 private_bits;			      /* 3 bits in mono, 5 in stereo */
  UINT32 scfsi[2][4];			      /* 1 bit */
  UINT32 part2_3_length[2][2];		      /* 12 bits */
  UINT32 big_values[2][2];		      /* 9 bits */
  UINT32 global_gain[2][2];		      /* 8 bits */
  UINT32 scalefac_compress[2][2];	      /* 4 bits */
  UINT32 win_switch_flag[2][2];		      /* 1 bit */
  /* if (win_switch_flag[][]) */
  UINT32 block_type[2][2];		      /* 2 bits */
  UINT32 mixed_block_flag[2][2];	      /* 1 bit */
  UINT32 table_select[2][2][3];		      /* 5 bits */
  UINT32 subblock_gain[2][2][3];	      /* 3 bits */
  /* else */
  /* table_select[][][] */
  UINT32 region0_count[2][2];		      /* 4 bits */
  UINT32 region1_count[2][2];		      /* 3 bits */
  /* end */
  UINT32 preflag[2][2];			      /* 1 bit */
  UINT32 scalefac_scale[2][2];		      /* 1 bit */
  UINT32 count1table_select[2][2];	      /* 1 bit */
  UINT32 count1[2][2];		      /* Not in file, calc. by huff.dec.! */
} t_mpeg1_side_info;

/* MPEG1 Layer 3 Main Data */
typedef struct {
  UINT32  scalefac_l[2][2][21];	              /* 0-4 bits */
  UINT32  scalefac_s[2][2][12][3];	      /* 0-4 bits */
  FLOAT32 is[2][2][576];		      /* Huffman coded freq. lines */
} t_mpeg1_main_data;

/* Scale factor band indices, for long and short windows */
typedef struct  {
  UINT32 l[23];
  UINT32 s[14];
} t_sf_band_indices;

/* Global variables */
extern t_mpeg1_header     g_frame_header;
extern t_mpeg1_side_info  g_side_info;
extern t_mpeg1_main_data  g_main_data;
extern t_sf_band_indices  g_sf_band_indices[3];
extern FLOAT32            g_synth_dtbl[512];

extern UINT32  g_main_data_vec[]; 
extern UINT32 *g_main_data_ptr;	/* Pointer into the reservoir */
extern UINT32  g_main_data_idx;	/* Index into the current byte (0-7) */
extern UINT32  g_main_data_top;/* Number of bytes in reservoir (0-1024) */

#endif /* _MPG_MAIN_H_ */
