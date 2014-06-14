/******************************************************************************
*
* Filename: remote_control.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: The remote control module. It accepts commands from a UDP
*              to control the behaviour of the decoder, e.g. pause, ffwd.
*
******************************************************************************/

/* Include files */
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>              
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "MP3_Main.h"
#include "MP3_Decoder.h"
#include "MP3_Bitstream.h"

#include "remote.h"

/* External functions and variables (defined elsewhere, and used here) */
extern char filename[256];
extern void MPG_Decode_L3_Init_Song (void);
extern UINT32 MPG_Get_Filepos (void);
extern void MPG_Set_Filepos (UINT32 position);
extern UINT32 MPG_Get_Filesize (void);

/* Global functions and variables (defined here, and used here & elsewhere) */
int g_mode_pause = 0;

/* Local functions and variables (defined here, used here) */
static void remote_pause (void);
static void remote_play (void);
static void remote_ffwd (float arg);
static void remote_rew (float arg);
static void remote_setpos (float arg);
static void remote_skipnext (void);
static void remote_skipprev (void);


/******************************************************************************
*
* Name: remote_control
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function handles the remote control functions.
* Parameters: None
* Return value: None
*
******************************************************************************/
void
remote_control (void)
{
  static int init = 0;
  static int rcv_sock, snd_sock;
  static struct sockaddr_in rcv_addr, snd_addr;
  fd_set readfds;
  struct timeval timeout;
  int rlen;
  unsigned char rbuf[1000], tbuf[1000];
  socklen_t fromlen = sizeof (struct sockaddr_in);
  remote_command_t ctrl;
  remote_status_t status;


  /* First time init */
  if (init == 0) {
    init = 1;

    if ((snd_sock = socket (PF_INET, SOCK_DGRAM, 0)) < 0) {
      EXIT ("Cannot open remote control UDP send socket!");
    }
      
    bzero ((char *) &snd_addr, sizeof (snd_addr));
    snd_addr.sin_family = AF_INET;
    snd_addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
    snd_addr.sin_port = htons (SND_UDP_PORT);

    if ((rcv_sock = socket (PF_INET, SOCK_DGRAM, 0)) < 0) {
      EXIT ("Cannot open remote control UDP receive socket!");
    }
      
    bzero ((char *) &rcv_addr, sizeof (rcv_addr));
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    rcv_addr.sin_port = htons (RCV_UDP_PORT);

    if ((bind (rcv_sock, (struct sockaddr *) &rcv_addr, 
	       sizeof (rcv_addr))) < 0) {
      EXIT ("Cannot bind remote control UDP port to local address!");
    }
    
  }

  /* Check if there's any incoming message for us */
  FD_ZERO (&readfds);
  FD_SET (rcv_sock, &readfds);
  /* No wait */
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
    
  if (select (rcv_sock+1, &readfds, (fd_set *) NULL, (fd_set *) NULL, 
	      &timeout) > 0) {	/* Got something */
    if ((rlen = recv (rcv_sock, (void *) rbuf, sizeof (rbuf), 0)) > 0) {
      memcpy ((void *) &ctrl, (void *) rbuf, sizeof (ctrl));

      switch (ctrl.cmd) {
      case cmd_getstatus: break;
	
      case cmd_pause: remote_pause (); break;

      case cmd_play: remote_play (); break;

      case cmd_ffwd: remote_ffwd (ctrl.argument); break;

      case cmd_rew: remote_rew (ctrl.argument); break;

      case cmd_setpos: remote_setpos (ctrl.argument); break;

      case cmd_skipnext: remote_skipnext (); break;

      case cmd_skipprev: remote_skipprev (); break;

      default:
	EXIT ("Unknown remote command %d", ctrl.cmd);
      }

      /* Always return the status */
      strncpy (status.song_name, filename, sizeof (status.song_name)-1);
      status.song_name[sizeof (status.song_name)-1] = 0;
      status.position =
	((float) MPG_Get_Filepos ()) / ((float) MPG_Get_Filesize ()+1);
      status.sample_rate = 
	g_sampling_frequency[g_frame_header.sampling_frequency];
      status.stereo = 
	(g_frame_header.mode == mpeg1_mode_single_channel ? 0 : 1);
      status.bitrate = g_mpeg1_bitrates[2][g_frame_header.bitrate_index];
      status.secs = MPG_Get_Filesize () / ((status.bitrate / 8)+1);

      memcpy ((void *) tbuf, (void *) &status, sizeof (status));

      if (sendto (snd_sock, (const void *) tbuf, sizeof (status), 0,
		  (const struct sockaddr *) &snd_addr, 
		  fromlen) != sizeof (status)) {
	ERR ("Cannot send remote control UDP data to peer!");
	return;
      }
    }
  }
    
}


static void
remote_pause (void)
{
  g_mode_pause = 1;

  MPG_Decode_L3_Init_Song ();
}


static void
remote_play (void)
{
  g_mode_pause = 0;

  MPG_Decode_L3_Init_Song ();
}


static void
remote_ffwd (float arg)
{
}


static void
remote_rew (float arg)
{
}


static void
remote_setpos (float arg)
{
  UINT32 filepos;


  filepos = ((float) MPG_Get_Filesize ()) * arg;

  MPG_Set_Filepos (filepos);
  
  MPG_Decode_L3_Init_Song ();

  DBG ("Setting pos to %d out of %d", filepos, MPG_Get_Filesize ());
}


static void
remote_skipnext (void)
{
}


static void
remote_skipprev (void)
{
}
