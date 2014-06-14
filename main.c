/******************************************************************************
*
* Filename: main.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This is the main program file for the Linux version.
*
******************************************************************************/

/* Include files */
#include "MP3_Main.h"
#include "MP3_Decoder.h"
#include "MP3_Bitstream.h"

#include "debug.h"

/* External functions and variables (defined elsewhere, and used here) */
extern int g_mode_pause;
extern void remote_control (void);
extern void usleep (int time);

/* Global functions and variables (defined here, and used here & elsewhere) */
char filename[256];


/******************************************************************************
*
* Name: main
* Author: Krister Lagerström (krister@kmlager.com)
* Description: The main function of the program
* Parameters: File to decode
* Return value: None
*
******************************************************************************/
int
main (int ac, char *av[])
{
  BOOL decode = FALSE;
  

  if (ac != 2) {
    EXIT ("Usage: %s <filename>", av[0]);
  }

  strcpy (filename, av[1]);

  while (MPG_Get_Filepos () != C_MPG_EOF) {

    if (MPG_Read_Frame () == OK) {
      decode = TRUE;
    } else {
      if (MPG_Get_Filepos () == C_MPG_EOF) {
        goto done;
      } else {
	decode = FALSE;
	ERR ("Not enough maindata to decode frame\n");
      }
    }

    if (decode) {
      /* Decode the compressed frame into 1152 mono/stereo PCM audio samples */
      MPG_Decode_L3 ();		
    }

    /* Remote control handling */
    do {
      remote_control ();
      if (g_mode_pause) usleep (50000);
    } while (g_mode_pause);

  }

 done:
  
  /* Done */
  exit (0);

}


