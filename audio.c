/******************************************************************************
*
* Filename: audio_linux.c
* Author: Krister Lagerström (krister@unidata.se)
* Description: This file contains the Linux audio functions
* Revision History: 
* Author   Date    Change
* krister  000601  Initial revision
*
******************************************************************************/

/* Include files */
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#include "MP3_Main.h"

/* External functions and variables (defined elsewhere, and used here) */
extern char filename[256];

/* Global functions and variables (defined here, and used here & elsewhere) */
void audio_write (uint32_t *samples, uint32_t nsamples, int sample_rate);
void audio_write_raw (uint32_t *samples, uint32_t nsamples);

/* Local functions and variables (defined here, used here) */
#define AUDIO "/dev/dsp"
#define FRAG_SIZE_LN2     0x0011 /* 2^17=128kb */
#define FRAG_NUMS         0x0004

static char audio_name[20] = AUDIO;

/* Start of code */


/******************************************************************************
*
* Name: audio_write
* Author: Krister Lagerström (krister@unidata.se)
* Description: This function is used to output audio data
* Parameters: Pointers to the samples, the number of samples
* Return value: None
* Revision History:
* Author   Date    Change
* krister  010101  Initial revision
*
******************************************************************************/
void
audio_write (uint32_t *samples, uint32_t nsamples, int sample_rate)
{
  static int init = 0, audio, curr_sample_rate = 0;
  int format = AFMT_S16_LE, tmp, dsp_speed = 44100, dsp_stereo = 2;


  if (init == 0) {
    init = 1;
    
    audio = open (audio_name, O_WRONLY, 0);

    if (audio == -1) {
      perror (audio_name);
      exit (-1);
    }

#if 0
    tmp = ((FRAG_NUMS) << 16) | (FRAG_SIZE_LN2);
    if (ioctl (audio, SNDCTL_DSP_SETFRAGMENT, &tmp)) {
      fprintf (stderr, "Unable to set audio fragment size\n");
      exit (-1);
    }
#endif
    
    tmp = format;
    ioctl (audio, SNDCTL_DSP_SETFMT, &format);
    if (tmp != format) {
     fprintf (stderr, "Unable to set the audio format\n");
      exit (-1);
    }

    if (ioctl (audio, SNDCTL_DSP_CHANNELS, &dsp_stereo) == -1) {
      fprintf (stderr, "Unable to set mono/stereo\n");
      perror (audio_name);
      exit (-1);
    }

  }

  if (curr_sample_rate != sample_rate) {
    curr_sample_rate = sample_rate;
    if (ioctl (audio, SNDCTL_DSP_SPEED, &dsp_speed) == -1) {
      fprintf (stderr, "Unable to set audio speed\n");
      perror (audio_name);
      exit (-1);
    }
  }

#ifdef OUTPUT_SOUND
  if (write (audio, (char *) samples, nsamples * 4) != nsamples * 4) {
    fprintf (stderr, "Unable to write audio data\n");
    perror (audio_name);
    exit (-1);
  }
#endif /* OUTPUT_SOUND */

#ifdef OUTPUT_RAW
  audio_write_raw (samples, nsamples);
#endif /* OUTPUT_RAW */

  return;

} /* audio_write() */


/******************************************************************************
*
* Name: audio_write_raw
* Author: Krister Lagerström (krister@unidata.se)
* Description: This function is used to output raw data
* Parameters: Pointers to the samples, the number of samples
* Return value: None
* Revision History:
* Author   Date    Change
* krister  010101  Initial revision
*
******************************************************************************/
void
audio_write_raw (uint32_t *samples, uint32_t nsamples)
{
  static int init = 0, fd;
  char fname[1024];
  uint32_t lo, hi;
  int i, nch;
  unsigned short s[576*2];

  if (init == 0) {
    init = 1;

    sprintf (fname, "%s.raw", filename);
    
    fd = open (fname, O_WRONLY | O_CREAT, 0666);

    if (fd == -1) {
      perror (fname);
      exit (-1);
    }
  }

  nch = (g_frame_header.mode == mpeg1_mode_single_channel ? 1 : 2);

  for (i = 0; i < nsamples; i++) {
    if (nch == 1) {
      lo = samples[i] & 0xffff;
      s[i] = lo;
    } else {
      lo = samples[i] & 0xffff;
      hi = (samples[i] & 0xffff0000) >> 16;
      s[2*i] = hi;
      s[2*i+1] = lo;
    } 
     
  }
  
  if (write (fd, (char *) s, nsamples * 2 * nch) != nsamples * 2 * nch) {
    fprintf (stderr, "Unable to write raw data\n");
    perror (audio_name);
    exit (-1);
  }

  return;

} /* audio_write_raw() */
