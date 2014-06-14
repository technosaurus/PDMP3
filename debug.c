/******************************************************************************
*
* Filename: debug.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains function to dump data during the
*              decoding process.
*
******************************************************************************/

/******************************* Include files *******************************/


/**** External functions and variables (defined elsewhere, and used here) ****/
#include <stdio.h>
#include <math.h>

#include "MP3_Main.h"

/** Global functions and variables (defined here, and used here & elsewhere) */


/********** Local functions and variables (defined here, used here) **********/

void
dmp_fr (t_mpeg1_header *hdr)
{
  printf ("rate %d, sfreq %d, pad %d, mod %d, modext %d, emph %d\n",
          hdr->bitrate_index, hdr->sampling_frequency, hdr->padding_bit,
          hdr->mode, hdr->mode_extension, hdr->emphasis);
}


void
dmp_si (t_mpeg1_header *hdr, t_mpeg1_side_info *si)
{
  int nch, ch, gr;

  
  nch = hdr->mode == mpeg1_mode_single_channel ? 1 : 2;

  printf ("main_data_begin %d, priv_bits %d\n", si->main_data_begin,
          si->private_bits);

  for (ch = 0; ch < nch; ch++) {

    printf ("scfsi %d %d %d %d\n", si->scfsi[ch][0], si->scfsi[ch][1],
            si->scfsi[ch][2], si->scfsi[ch][3]);

    for (gr = 0; gr < 2; gr++) {
      
      printf ("p23l %d, bv %d, gg %d, scfc %d, wsf %d, bt %d\n",
              si->part2_3_length[gr][ch], si->big_values[gr][ch],
              si->global_gain[gr][ch], si->scalefac_compress[gr][ch],
              si->win_switch_flag[gr][ch], si->block_type[gr][ch]);

      if (si->win_switch_flag[gr][ch]) {
        printf ("mbf %d, ts1 %d, ts2 %d, sbg1 %d, sbg2 %d, sbg3 %d\n",
                si->mixed_block_flag[gr][ch],
                si->table_select[gr][ch][0], si->table_select[gr][ch][1],
                si->subblock_gain[gr][ch][0],
                si->subblock_gain[gr][ch][1], si->subblock_gain[gr][ch][2]);
      } else {
        printf ("ts1 %d, ts2 %d, ts3 %d\n", 
                si->table_select[gr][ch][0], si->table_select[gr][ch][1],
                si->table_select[gr][ch][2]);
      }
      
      printf ("r0c %d, r1c %d\n", si->region0_count[gr][ch],
              si->region1_count[gr][ch]);
      
      printf ("pf %d, scfs %d, c1ts %d\n",
              si->preflag[gr][ch], si->scalefac_scale[gr][ch],
              si->count1table_select[gr][ch]);
    }
  }
  
}


void
dmp_scf (t_mpeg1_side_info *si, t_mpeg1_main_data *md, int gr, int ch)
{
  int sfb, win;

  
  if ((si->win_switch_flag[gr][ch] != 0) && 
      (si->block_type[gr][ch] == 2)) {
        
    if (si->mixed_block_flag[gr][ch] != 0) {

      /* First the long block scalefacs */
      for (sfb = 0; sfb < 8; sfb++) {
        printf ("scfl%d %d", sfb, md->scalefac_l[gr][ch][sfb]);
        if (sfb != 7) {
          printf (", ");
        } else {
          printf ("\n");
        }
      }

      /* And next the short block scalefacs */
      for (sfb = 3; sfb < 12; sfb++) {
            
        for (win = 0; win < 3; win++) {
          printf ("scfs%d,%d %d", sfb, win,
                  md->scalefac_s[gr][ch][sfb][win]);
          if (win != 2) {
            printf (", ");
          } else {
            printf ("\n");
          }
        }
      }
          
    } else {                /* Just short blocks */
      for (sfb = 0; sfb < 12; sfb++) {

        for (win = 0; win < 3; win++) {
          printf ("scfs%d,%d %d", sfb, win,
                  md->scalefac_s[gr][ch][sfb][win]);

          if (win != 2) {
            printf (", ");
          } else {
            printf ("\n");
          }
        }
      }
    }
        
  } else {                  /* Just long blocks */
    /* First the long block scalefacs */
    for (sfb = 0; sfb < 21; sfb++) {
      printf ("scfl%d %d", sfb, md->scalefac_l[gr][ch][sfb]);
      if (sfb != 20) {
        printf (", ");
      } else {
        printf ("\n");
      }
    }

  }

}


void
dmp_huff (t_mpeg1_main_data *md, int gr, int ch)
{
  int i;
  

  printf ("HUFFMAN\n");
  
  for (i = 0; i < 576; i++) {
    printf ("%d: %d\n", i, (int) md->is[gr][ch][i]);
  }

}


void
dmp_samples (t_mpeg1_main_data *md, int gr, int ch, int type)
{
  int i;
  int val;
  extern double rint (double);


  printf ("SAMPLES%d\n", type);
  
  for (i = 0; i < 576; i++) {
    val = (int) rint (md->is[gr][ch][i] * 32768.0);
    if (val >= 32768) val = 32767;
    if (val < -32768) val = -32768;
    printf ("%d: %d\n", i, val);
  }

}
