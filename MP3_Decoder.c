/******************************************************************************
*
* Filename: MPG_Decode_L3.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains the Level 3 decoding function.
*
******************************************************************************/

/* Include files */
#include "MP3_Main.h"
#include "MP3_Decoder.h"
#include "MP3_Bitstream.h"
#include "MP3_Synth_Table.h"
#include "debug.h"

/* External functions and variables (defined elsewhere, and used here) */
extern UINT32  g_main_data_top;
void audio_write (UINT32 *samples, UINT32 nsamples, int sample_rate);

/* Global functions and variables (defined here, and used here & elsewhere) */
STATUS MPG_Decode_L3 (void);

/* Local functions and variables (defined here, used here) */
static void MPG_L3_Requantize (UINT32 gr, UINT32 ch);
static void MPG_Requantize_Process_Long (UINT32 gr, UINT32 ch, 
					 UINT32 is_pos, UINT32 sfb);
static void MPG_Requantize_Process_Short (UINT32 gr, UINT32 ch, UINT32 is_pos, 
					  UINT32 sfb, UINT32 win);
static void MPG_L3_Reorder (UINT32 gr, UINT32 ch);
static void MPG_L3_Stereo (UINT32 gr);
static void MPG_Stereo_Process_Intensity_Long (UINT32 gr, UINT32 sfb);
static void MPG_Stereo_Process_Intensity_Short (UINT32 gr, UINT32 sfb);
static void MPG_L3_Antialias (UINT32 gr, UINT32 ch);
static void MPG_L3_Hybrid_Synthesis (UINT32 gr, UINT32 ch);
static void MPG_IMDCT_Win (FLOAT32 in[18], FLOAT32 out[36], UINT32 block_type);
static void MPG_L3_Frequency_Inversion (UINT32 gr, UINT32 ch);
static void MPG_L3_Subband_Synthesis (UINT32 gr, UINT32 ch, 
				      UINT32 outdata[576]);

static UINT32 hsynth_init = 1;
static UINT32 synth_init = 1;


/******************************************************************************
*
* Name: MPG_Decode_L3_Init_Song
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function is used to reinit the decoder before playing a new
*              song, or when seeking inside the current song.
* Parameters: None
* Return value: None
*
******************************************************************************/
void
MPG_Decode_L3_Init_Song (void)
{
  hsynth_init = 1;
  synth_init = 1;
  g_main_data_top = 0;		/* Clear bit reservoir */
}


/******************************************************************************
*
* Name: MPG_Decode_L3
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function decodes a layer 3 bitstream into audio samples.
* Parameters: Outdata vector.
* Return value: OK or ERROR if the frame contains errors.
*
******************************************************************************/
STATUS
MPG_Decode_L3 (void)
{
  UINT32 gr, ch, nch;
  UINT32 out[576];		


  /* Number of channels (1 for mono and 2 for stereo) */
  nch = (g_frame_header.mode == mpeg1_mode_single_channel ? 1 : 2);

  for (gr = 0; gr < 2; gr++) {
    for (ch = 0; ch < nch; ch++) {

#ifdef OUTPUT_DBG
      dmp_scf (&g_side_info, &g_main_data, gr, ch);
      dmp_huff (&g_main_data, gr, ch);
#endif /* OUTPUT_DBG */

      /* Requantize samples */
      MPG_L3_Requantize (gr, ch);

#ifdef OUTPUT_DBG
       dmp_samples (&g_main_data, gr, ch, 0);
#endif /* OUTPUT_DBG */

      /* Reorder short blocks */
      MPG_L3_Reorder (gr, ch);

    } /* end for (ch... */

    /* Stereo processing */
    MPG_L3_Stereo (gr);		

#ifdef OUTPUT_DBG
    dmp_samples (&g_main_data, gr, 0, 1);
    dmp_samples (&g_main_data, gr, 1, 1);
#endif /* OUTPUT_DBG */

    for (ch = 0; ch < nch; ch++) {

      /* Antialias */
      MPG_L3_Antialias (gr, ch);
     
#ifdef OUTPUT_DBG
    dmp_samples (&g_main_data, gr, ch, 2);
#endif /* OUTPUT_DBG */

      /* Hybrid synthesis (IMDCT, windowing, overlapp add) */
      MPG_L3_Hybrid_Synthesis (gr, ch);

      /* Frequency inversion */
      MPG_L3_Frequency_Inversion (gr, ch);

#ifdef OUTPUT_DBG
    dmp_samples (&g_main_data, gr, ch, 3);
#endif /* OUTPUT_DBG */

      /* Polyphase subband synthesis */
      MPG_L3_Subband_Synthesis (gr, ch, out);

    } /* end for (ch... */

#ifdef OUTPUT_DBG
    {
      int i, ctr;
      short int val;
      

      ctr = 0;
      printf ("PCM:\n");
      for (i = 0; i < 576; i++) {
        val = (out[i] >> 16) & 0xffff;
        printf ("%d: %d\n", ctr, val);
        ctr++;
        if (nch == 2) {
          val = out[i] & 0xffff;
          printf ("%d: %d\n", ctr, val);
          ctr++;
        }
      }
    }
#endif /* OUTPUT_DBG */
     
    audio_write ((UINT32 *) out, 576,
                 g_sampling_frequency[g_frame_header.sampling_frequency]);

  } /* end for (gr... */

  /* Done */
  return (OK);

}


/******************************************************************************
*
* Name: MPG_L3_Requantize
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Requantize (UINT32 gr, UINT32 ch)
{
  UINT32 sfb /* scalefac band index */;
  UINT32 next_sfb /* frequency of next sfb */;
  UINT32 sfreq, i, j, win, win_len;


  /* Setup sampling frequency index */
  sfreq = g_frame_header.sampling_frequency;

  /* Determine type of block to process */
  if ((g_side_info.win_switch_flag[gr][ch] == 1) && 
      (g_side_info.block_type[gr][ch] == 2)) { /* Short blocks */

    /* Check if the first two subbands 
     * (=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
    if (g_side_info.mixed_block_flag[gr][ch] != 0) { /* 2 longbl. sb  first */

      /* 
       * First process the 2 long block subbands at the start
       */
      sfb = 0;
      next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
      for (i = 0; i < 36; i++) {
	if (i == next_sfb) {
	  sfb++;
	  next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
	} /* end if */
	MPG_Requantize_Process_Long (gr, ch, i, sfb); 
      }

      /* 
       * And next the remaining, non-zero, bands which uses short blocks 
       */
      sfb = 3;
      next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
      win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
	g_sf_band_indices[sfreq].s[sfb];

      for (i = 36; i < g_side_info.count1[gr][ch]; /* i++ done below! */) {

	/* Check if we're into the next scalefac band */
	if (i == next_sfb) {	/* Yes */
	  sfb++;
	  next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
	  win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
	    g_sf_band_indices[sfreq].s[sfb];
	} /* end if (next_sfb) */

	for (win = 0; win < 3; win++) {
	  for (j = 0; j < win_len; j++) {
	    MPG_Requantize_Process_Short (gr, ch, i, sfb, win); 
	    i++;
	  } /* end for (win... */
	} /* end for (j... */

      }	/* end for (i... */
      
    } else {			/* Only short blocks */

      sfb = 0;
      next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
      win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
	g_sf_band_indices[sfreq].s[sfb];

      for (i = 0; i < g_side_info.count1[gr][ch]; /* i++ done below! */) {

	/* Check if we're into the next scalefac band */
	if (i == next_sfb) {	/* Yes */
	  sfb++;
	  next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
	  win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
	    g_sf_band_indices[sfreq].s[sfb];
	} /* end if (next_sfb) */

	for (win = 0; win < 3; win++) {
	  for (j = 0; j < win_len; j++) {
	    MPG_Requantize_Process_Short (gr, ch, i, sfb, win); 
	    i++;
	  } /* end for (win... */
	} /* end for (j... */

      }	/* end for (i... */

    } /* end else (only short blocks) */

  } else {			/* Only long blocks */

    sfb = 0;
    next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
    for (i = 0; i < g_side_info.count1[gr][ch]; i++) { 
      if (i == next_sfb) {
	sfb++;
	next_sfb = g_sf_band_indices[sfreq].l[sfb+1];
      } /* end if */
      MPG_Requantize_Process_Long (gr, ch, i, sfb);  
    }
    
  } /* end else (only long blocks) */

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_Requantize_Process_Long
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function is used to requantize a sample in a subband
*              that uses long blocks.
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void
MPG_Requantize_Process_Long (UINT32 gr, UINT32 ch, UINT32 is_pos, UINT32 sfb)
{
  FLOAT32 res, tmp1, tmp2, tmp3, sf_mult, pf_x_pt;
  static FLOAT32 pretab[21] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				1, 1, 1, 1, 2, 2, 3, 3, 3, 2 };

  
  sf_mult = g_side_info.scalefac_scale[gr][ch] ? 1.0 : 0.5;
  pf_x_pt = g_side_info.preflag[gr][ch] * pretab[sfb];

  tmp1 = 
    pow (2.0, -(sf_mult * (g_main_data.scalefac_l[gr][ch][sfb] + pf_x_pt)));

  tmp2 = pow (2.0, 0.25 * ((INT32) g_side_info.global_gain[gr][ch] - 210));

  if (g_main_data.is[gr][ch][is_pos] < 0.0) {
    tmp3 = pow (-g_main_data.is[gr][ch][is_pos], 4.0 / 3.0); 
    tmp3 = -tmp3;
  } else {
    tmp3 = pow (g_main_data.is[gr][ch][is_pos], 4.0 / 3.0); 
  }

  res = g_main_data.is[gr][ch][is_pos] = tmp1 * tmp2 * tmp3;

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_Requantize_Process_Short
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function is used to requantize a sample in a subband
*              that uses short blocks.
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void
MPG_Requantize_Process_Short (UINT32 gr, UINT32 ch, UINT32 is_pos, 
			      UINT32 sfb, UINT32 win)
{
  FLOAT32 res, tmp1, tmp2, tmp3, sf_mult;


  sf_mult = g_side_info.scalefac_scale[gr][ch] ? 1.0 : 0.5;

  tmp1 = 
    pow (2.0, -(sf_mult * g_main_data.scalefac_s[gr][ch][sfb][win]));

  tmp2 = 
    pow (2.0, 0.25 * ((FLOAT32) g_side_info.global_gain[gr][ch] - 210.0 -
		      8.0 * (FLOAT32) g_side_info.subblock_gain[gr][ch][win]));

  if (g_main_data.is[gr][ch][is_pos] < 0.0) {
    tmp3 = pow (-g_main_data.is[gr][ch][is_pos], 4.0 / 3.0);
    tmp3 = -tmp3;
  } else {
    tmp3 = pow (g_main_data.is[gr][ch][is_pos], 4.0 / 3.0);
  }
  
  res = g_main_data.is[gr][ch][is_pos] = tmp1 * tmp2 * tmp3;

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_L3_Reorder
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Reorder (UINT32 gr, UINT32 ch)
{
  UINT32 sfreq, i, j, next_sfb, sfb, win_len, win;
  FLOAT32 re[576]; 


  /* Setup sampling frequency index */
  sfreq = g_frame_header.sampling_frequency;

  /* Only reorder short blocks */
  if ((g_side_info.win_switch_flag[gr][ch] == 1) && 
      (g_side_info.block_type[gr][ch] == 2)) { /* Short blocks */
    
    /* Check if the first two subbands 
     * (=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
    if (g_side_info.mixed_block_flag[gr][ch] != 0) { /* 2 longbl. sb  first */
      
      /* Don't touch the first 36 samples */
      
      /* 
       * Reorder the remaining, non-zero, bands which uses short blocks 
       */
      sfb = 3;
      next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
      win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
	g_sf_band_indices[sfreq].s[sfb];
      
    } else { /* Only short blocks */
      
      sfb = 0;
      next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
      win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
	g_sf_band_indices[sfreq].s[sfb];
      
    } /* end else (only short blocks) */
    
    
    for (i = ((sfb == 0) ? 0 : 36); i < 576; /* i++ done below! */) {
      
      /* Check if we're into the next scalefac band */
      if (i == next_sfb) {	/* Yes */
        
        /* Copy reordered data back to the original vector */
        for (j = 0; j < 3*win_len; j++) {
          g_main_data.is[gr][ch][3*g_sf_band_indices[sfreq].s[sfb] + j] = 
            re[j];
        }

        /* Check if this band is above the rzero region, if so we're done */
        if (i >= g_side_info.count1[gr][ch]) {
          /* Done */
          return;
        }
        
        sfb++;
        next_sfb = g_sf_band_indices[sfreq].s[sfb+1] * 3;
        win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
          g_sf_band_indices[sfreq].s[sfb];
        
      } /* end if (next_sfb) */

      /* Do the actual reordering */
      for (win = 0; win < 3; win++) {
        for (j = 0; j < win_len; j++) {
          re[j*3 + win] = g_main_data.is[gr][ch][i];
          i++;
        } /* end for (j... */
      } /* end for (win... */
      
    }	/* end for (i... */

    /* Copy reordered data of the last band back to the original vector */
    for (j = 0; j < 3*win_len; j++) {
      g_main_data.is[gr][ch][3 * g_sf_band_indices[sfreq].s[12] + j] = re[j];
    }

  } else {			/* Only long blocks */
    /* No reorder necessary, do nothing! */
    return;

  } /* end else (only long blocks) */

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_L3_Stereo
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Stereo (UINT32 gr)
{
  UINT32 max_pos, i;
  FLOAT32 left, right;
  UINT32 sfb /* scalefac band index */;
  UINT32 sfreq;
  
  

  /* Do nothing if joint stereo is not enabled */
  if ((g_frame_header.mode != 1) || (g_frame_header.mode_extension == 0)) {
    /* Done */
    return;
  }

  /* Do Middle/Side ("normal") stereo processing */
  if (g_frame_header.mode_extension & 0x2) {

    /* Determine how many frequency lines to transform */
    if (g_side_info.count1[gr][0] > g_side_info.count1[gr][1]) {
      max_pos = g_side_info.count1[gr][0];
    } else {
      max_pos = g_side_info.count1[gr][1];
    }

    /* Do the actual processing */
    for (i = 0; i < max_pos; i++) {
      left = (g_main_data.is[gr][0][i] + g_main_data.is[gr][1][i]) 
	* (C_INV_SQRT_2);
      right = (g_main_data.is[gr][0][i] - g_main_data.is[gr][1][i]) 
	* (C_INV_SQRT_2);
      g_main_data.is[gr][0][i] = left;
      g_main_data.is[gr][1][i] = right;
    } /* end for (i... */

  } /* end if (ms_stereo... */

  /* Do intensity stereo processing */
  if (g_frame_header.mode_extension & 0x1) {

    /* Setup sampling frequency index */
    sfreq = g_frame_header.sampling_frequency;

    /* The first band that is intensity stereo encoded is the first band
     * scale factor band on or above the count1 frequency line.
     * N.B.: Intensity stereo coding is only done for the higher subbands,
     * but the logic is still included to process lower subbands.
     */

    /* Determine type of block to process */
    if ((g_side_info.win_switch_flag[gr][0] == 1) && 
        (g_side_info.block_type[gr][0] == 2)) { /* Short blocks */

      /* Check if the first two subbands 
       * (=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
      if (g_side_info.mixed_block_flag[gr][0] != 0) { /* 2 longbl. sb  first */

        /* 
         * First process the 8 sfb's at the start
         */
        for (sfb = 0; sfb < 8; sfb++) {
          
          /* Is this scale factor band above count1 for the right channel? */
          if (g_sf_band_indices[sfreq].l[sfb] >= g_side_info.count1[gr][1]) {
            MPG_Stereo_Process_Intensity_Long (gr, sfb);
          }
          
        } /* end if (sfb... */
        
        /* 
         * And next the remaining bands which uses short blocks 
         */
        for (sfb = 3; sfb < 12; sfb++) {
          
          /* Is this scale factor band above count1 for the right channel? */
          if (g_sf_band_indices[sfreq].s[sfb]*3 >= g_side_info.count1[gr][1]) {
            
            /* Perform the intensity stereo processing */
            MPG_Stereo_Process_Intensity_Short (gr, sfb);
          }
        }
      
      } else {			/* Only short blocks */

        for (sfb = 0; sfb < 12; sfb++) {
          
          /* Is this scale factor band above count1 for the right channel? */
          if (g_sf_band_indices[sfreq].s[sfb]*3 >= g_side_info.count1[gr][1]) {
            
            /* Perform the intensity stereo processing */
            MPG_Stereo_Process_Intensity_Short (gr, sfb);
          }
        }
        
      } /* end else (only short blocks) */

    } else {			/* Only long blocks */

      for (sfb = 0; sfb < 21; sfb++) {
        
        /* Is this scale factor band above count1 for the right channel? */
        if (g_sf_band_indices[sfreq].l[sfb] >= g_side_info.count1[gr][1]) {

          /* Perform the intensity stereo processing */
          MPG_Stereo_Process_Intensity_Long (gr, sfb);
        }
      }
    
    } /* end else (only long blocks) */

  } /* end if (intensity_stereo processing) */
  
}


/******************************************************************************
*
* Name: MPG_Stereo_Process_Intensity_Long
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function is used to perform intensity stereo processing
*              for an entire subband that uses long blocks.
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void
MPG_Stereo_Process_Intensity_Long (UINT32 gr, UINT32 sfb)
{
  static int init = 0;
  static FLOAT32 is_ratios[6];
  UINT32 i;
  UINT32 sfreq;
  UINT32 sfb_start, sfb_stop;
  UINT32 is_pos;
  FLOAT32 is_ratio_l, is_ratio_r;
  FLOAT32 left, right;


  /* First-time init */
  if (init == 0) {
    init = 1;
    for (i = 0; i < 6; i++) {
      is_ratios[i] = tan ((i * C_PI) / 12.0);
    }

  }
    
  /* Check that ((is_pos[sfb]=scalefac) != 7) => no intensity stereo */
  if ((is_pos = g_main_data.scalefac_l[gr][0][sfb]) != 7) {

    /* Setup sampling frequency index */
    sfreq = g_frame_header.sampling_frequency;

    sfb_start = g_sf_band_indices[sfreq].l[sfb];
    sfb_stop = g_sf_band_indices[sfreq].l[sfb+1];

    /* tan((6*PI)/12 = PI/2) needs special treatment! */
    if (is_pos == 6) {
      is_ratio_l = 1.0;
      is_ratio_r = 0.0;
    } else {
      is_ratio_l = is_ratios[is_pos] / (1.0 + is_ratios[is_pos]);
      is_ratio_r = 1.0 / (1.0 + is_ratios[is_pos]);
    }
            
    /* Now decode all samples in this scale factor band */
    for (i = sfb_start; i < sfb_stop; i++) {
      left = is_ratio_l * g_main_data.is[gr][0][i];
      right = is_ratio_r * g_main_data.is[gr][0][i];
      g_main_data.is[gr][0][i] = left;
      g_main_data.is[gr][1][i] = right;
    }
  }

  /* Done */
  return;
  
} /* end MPG_Stereo_Process_Intensity_Long() */


/******************************************************************************
*
* Name: MPG_Stereo_Process_Intensity_Short
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function is used to perform intensity stereo processing
*              for an entire subband that uses short blocks.
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void
MPG_Stereo_Process_Intensity_Short (UINT32 gr, UINT32 sfb)
{
  UINT32 i;
  FLOAT32 left, right;
  UINT32 sfb_start, sfb_stop;
  UINT32 is_pos, is_ratio_l, is_ratio_r, is_ratios[6];
  UINT32 sfreq, win, win_len;


  /* Setup sampling frequency index */
  sfreq = g_frame_header.sampling_frequency;

  /* The window length */
  win_len = g_sf_band_indices[sfreq].s[sfb+1] - 
    g_sf_band_indices[sfreq].s[sfb];
  
  /* The three windows within the band has different scalefactors */
  for (win = 0; win < 3; win++) {

    /* Check that ((is_pos[sfb]=scalefac) != 7) => no intensity stereo */
    if ((is_pos = g_main_data.scalefac_s[gr][0][sfb][win]) != 7) {

      sfb_start = g_sf_band_indices[sfreq].s[sfb]*3 + win_len*win;
      sfb_stop = sfb_start + win_len;

      /* tan((6*PI)/12 = PI/2) needs special treatment! */
      if (is_pos == 6) {
        is_ratio_l = 1.0;
        is_ratio_r = 0.0;
      } else {
        is_ratio_l = is_ratios[is_pos] / (1.0 + is_ratios[is_pos]);
        is_ratio_r = 1.0 / (1.0 + is_ratios[is_pos]);
      }
      
      /* Now decode all samples in this scale factor band */
      for (i = sfb_start; i < sfb_stop; i++) {
        left = is_ratio_l = g_main_data.is[gr][0][i];
        right = is_ratio_r = g_main_data.is[gr][0][i];
        g_main_data.is[gr][0][i] = left;
        g_main_data.is[gr][1][i] = right;
      }
    } /* end if (not illegal is_pos) */
  } /* end for (win... */
  
  /* Done */
  return;
  
} /* end MPG_Stereo_Process_Intensity_Short() */


/******************************************************************************
*
* Name: MPG_L3_Antialias
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Antialias (UINT32 gr, UINT32 ch)
{
  UINT32 sb /* subband of 18 samples */, i, sblim, ui, li;
  FLOAT32 ub, lb;
  static FLOAT32 cs[8], ca[8];
  static FLOAT32 ci[8] = { -0.6,   -0.535, -0.33,   -0.185, 
			   -0.095, -0.041, -0.0142, -0.0037 };
  static UINT32 init = 1;


  if (init) {
    for (i = 0; i < 8; i++) {
      cs[i] = 1.0 / sqrt (1.0 + ci[i]*ci[i]);
      ca[i] = ci[i] / sqrt (1.0 + ci[i]*ci[i]);
    }

    init = 0;
  }

  /* No antialiasing is done for short blocks */
  if ((g_side_info.win_switch_flag[gr][ch] == 1) && 
      (g_side_info.block_type[gr][ch] == 2) &&
      (g_side_info.mixed_block_flag[gr][ch]) == 0) {
    /* Done */
    return;
  }

  /* Setup the limit for how many subbands to transform */
  if ((g_side_info.win_switch_flag[gr][ch] == 1) && 
      (g_side_info.block_type[gr][ch] == 2) &&
      (g_side_info.mixed_block_flag[gr][ch]) == 1) {
    sblim = 2;
  } else {
    sblim = 32;
  }

  /* Do the actual antialiasing */
  for (sb = 1; sb < sblim; sb++) {
    for (i = 0; i < 8; i++) {
      li = 18*sb-1-i;
      ui = 18*sb+i;
      lb = g_main_data.is[gr][ch][li]*cs[i] - g_main_data.is[gr][ch][ui]*ca[i];
      ub = g_main_data.is[gr][ch][ui]*cs[i] + g_main_data.is[gr][ch][li]*ca[i];
      g_main_data.is[gr][ch][li] = lb;
      g_main_data.is[gr][ch][ui] = ub;
    }
  }

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_L3_Hybrid_Synthesis
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Hybrid_Synthesis (UINT32 gr, UINT32 ch)
{
  UINT32 sb, i, j, bt;
  FLOAT32 rawout[36];	
  static FLOAT32 store[2][32][18]; 


  if (hsynth_init) {
    /* Clear stored samples vector */
    for (j = 0; j < 2; j++) {
      for (sb = 0; sb < 32; sb++) {
	for (i = 0; i < 18; i++) {
	  store[j][sb][i] = 0.0;
	}
      }
    }
    hsynth_init = 0;
  } /* end if (init) */

  /* Loop through all 32 subbands */
  for (sb = 0; sb < 32; sb++) {

    /* Determine blocktype for this subband */
    if ((g_side_info.win_switch_flag[gr][ch] == 1) && 
	(g_side_info.mixed_block_flag[gr][ch] == 1) && (sb < 2)) {
      bt = 0;			/* Long blocks in first 2 subbands */
    } else {
      bt = g_side_info.block_type[gr][ch];
    }

    /* Do the inverse modified DCT and windowing */
    MPG_IMDCT_Win (&(g_main_data.is[gr][ch][sb*18]), rawout, bt);

    /* Overlapp add with stored vector into main_data vector */
    for (i = 0; i < 18; i++) {

      g_main_data.is[gr][ch][sb*18 + i] = rawout[i] + store[ch][sb][i]; 
      store[ch][sb][i] = rawout[i + 18]; 

    } /* end for (i... */

  } /* end for (sb... */

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_IMDCT_Win
* Author: Krister Lagerström (krister@kmlager.com)
* Description: Does inverse modified DCT and windowing.
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_IMDCT_Win (FLOAT32 in[18], FLOAT32 out[36], UINT32 block_type)
{
  static FLOAT32 g_imdct_win[4][36];	
  UINT32 i, m, N, p;
  FLOAT32 tmp[12], sum;
  FLOAT32 tin[18];
  static UINT32 init = 1;
  

  /* Setup the four (one for each block type) window vectors */
  if (init) {

    /* Blocktype 0 */
    for (i = 0; i < 36; i++)
      g_imdct_win[0][i] = sin (C_PI/36 * (i + 0.5));

    /* Blocktype 1 */
    for (i = 0; i < 18; i++)
      g_imdct_win[1][i] = sin (C_PI/36 * (i + 0.5));
    for (i = 18; i < 24; i++)
      g_imdct_win[1][i] = 1.0;
    for (i = 24; i < 30; i++)
      g_imdct_win[1][i] = sin (C_PI/12 * (i + 0.5 - 18.0));
    for(i = 30; i < 36; i++)
      g_imdct_win[1][i] = 0.0;
    
    /* Blocktype 2*/
    for (i = 0; i < 12; i++)
      g_imdct_win[2][i] = sin (C_PI/12 * (i + 0.5));
    for (i = 12; i < 36; i++)
      g_imdct_win[2][i] = 0.0;
    
    /* Blocktype 3 */
    for (i = 0; i < 6; i++)
      g_imdct_win[3][i] = 0.0;
    for (i = 6; i < 12; i++)
      g_imdct_win[3][i] = sin (C_PI/12 * (i + 0.5 - 6.0));
    for (i = 12; i < 18; i++)
      g_imdct_win[3][i] = 1.0;
    for (i = 18; i < 36; i++)
      g_imdct_win[3][i] = sin (C_PI/36 * (i + 0.5));
    
    init = 0;
  } /* end of init */
  
  for (i = 0; i < 36; i++) {
    out[i] = 0.0;
  }

  for (i = 0; i < 18; i++) {
    tin[i] = in[i];
  }

  if (block_type == 2) {	/* 3 short blocks */
    N = 12;
    for (i = 0; i < 3; i++) {

      for (p = 0; p < N; p++) {
	sum = 0.0;
	for (m = 0;m < N/2; m++) {
	  sum += tin[i+3*m] * cos (C_PI/(2*N)*(2*p+1+N/2)*(2*m+1)); 
	}
	tmp[p] = sum * g_imdct_win[block_type][p];
      }

      for (p = 0; p < N; p++) {
	out[6*i+p+6] += tmp[p];
      }

    } /* end for (i... */

  } else { /* block_type != 2 */

    N = 36;
    for (p = 0; p < N; p++){

      sum = 0.0;
      for (m = 0; m < N/2; m++) {
	sum += in[m] * cos (C_PI/(2*N)*(2*p+1+N/2)*(2*m+1)); 
      }

      out[p] = sum * g_imdct_win[block_type][p];

    }

  }

}


/******************************************************************************
*
* Name: MPG_L3_Frequency_Inversion
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Frequency_Inversion (UINT32 gr, UINT32 ch)
{
  UINT32 sb, i;


  for (sb = 1; sb < 32; sb += 2) {
    for (i = 1; i < 18; i += 2) {
      g_main_data.is[gr][ch][sb*18 + i] = -g_main_data.is[gr][ch][sb*18 + i];
    }
  }

  /* Done */
  return;

}


/******************************************************************************
*
* Name: MPG_L3_Subband_Synthesis
* Author: Krister Lagerström (krister@kmlager.com)
* Description: TBD
* Parameters: TBD
* Return value: TBD
*
******************************************************************************/
static void 
MPG_L3_Subband_Synthesis (UINT32 gr, UINT32 ch, UINT32 outdata[576])
{
  FLOAT32 u_vec[512];		
  FLOAT32 s_vec[32], sum; /* u_vec can be used insted of s_vec */
  INT32 samp;
  UINT32 ss;
  static UINT32 init = 1;
  UINT32 i, j;
  UINT32 nch;
  static FLOAT32 g_synth_n_win[64][32];	
  static FLOAT32 v_vec[2 /* ch */][1024]; 


  /* Number of channels (1 for mono and 2 for stereo) */
  nch = (g_frame_header.mode == mpeg1_mode_single_channel ? 1 : 2);


  /* Setup the n_win windowing vector and the v_vec intermediate vector */
  if (init) {

    for (i = 0; i < 64; i++) {
      for (j = 0; j < 32; j++) {
	g_synth_n_win[i][j] = cos (((FLOAT32) (16+i)*(2*j+1)) * (C_PI/64.0));
      }
    }
    
    /* Setup the v_vec intermediate vector */
    for (i = 0; i < 2; i++) {
      for (j = 0; j < 1024; j++) {
	v_vec[i][j] = 0.0;
      }
    }
  
    init = 0;
  } /* end if (init) */

  if (synth_init) {

    /* Setup the v_vec intermediate vector */
    for (i = 0; i < 2; i++) {
      for (j = 0; j < 1024; j++) {
	v_vec[i][j] = 0.0;
      }
    }
  
    synth_init = 0;
  } /* end if (synth_init) */

  /* Loop through the 18 samples in each of the 32 subbands */
  for (ss = 0; ss < 18; ss++) {

    /* Shift up the V vector */
    for (i = 1023; i > 63; i--) {
      v_vec[ch][i] = v_vec[ch][i-64];
    }

    /* Copy the next 32 time samples to a temp vector */
    for (i = 0; i < 32; i++) {
      s_vec[i] = ((FLOAT32) g_main_data.is[gr][ch][i*18 + ss]);
    }

    /* Matrix multiply the input data vector with the n_win[][] matrix */
    for (i = 0; i < 64; i++) {
      sum = 0.0;
      for (j = 0; j < 32; j++) {
	sum += g_synth_n_win[i][j] * s_vec[j];
      }
      v_vec[ch][i] = sum; 
    } /* end for (i... */

    /* Build the U vector */
    for (i = 0; i < 8; i++) {
      for (j = 0; j < 32; j++) {
	/* u_vec[i*64 + j]      = v_vec[ch][i*128 + j];
	   u_vec[i*64 + j + 32] = v_vec[ch][i*128 + j + 96]; */
	u_vec[(i << 6) + j]      = v_vec[ch][(i << 7) + j];
	u_vec[(i << 6) + j + 32] = v_vec[ch][(i << 7) + j + 96];
      }
    } /* end for (i... */

    /* Window by u_vec[i] with g_synth_dtbl[i] */
    for (i = 0; i < 512; i++) {
      u_vec[i] = u_vec[i] * g_synth_dtbl[i];
    }

    /* Calculate 32 samples and store them in the outdata vector */
    for (i = 0; i < 32; i++) {
      sum = 0.0;
      for (j = 0; j < 16; j++) {
	/* sum += u_vec[j*32 + i]; */
	sum += u_vec[(j << 5) + i];
      }
      
      /* sum now contains time sample 32*ss+i. Convert to 16-bit signed int */
      samp = (INT32) (sum * 32767.0);
      if (samp > 32767) {
	samp = 32767;
      } else if (samp < -32767) {
	samp = -32767;
      } 

      samp &= 0xffff;

      /* This function must be called for channel 0 first */
      if (ch == 0) {
        /* We always run the audio system in stereo mode, and duplicate
         * the channels here for mono */
        if (nch == 1) {
          
          outdata[32*ss + i] = (samp << 16) | (samp);
          
        } else {
          outdata[32*ss + i] = samp << 16;
        }
      } else {
	outdata[32*ss + i] |= samp;
      }


    } /* end for (i... */

  } /* end for (ss... */

  /* Done */
  return;

}
