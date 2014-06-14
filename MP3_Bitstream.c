/******************************************************************************
*
* Filename: MPG_Bitstream.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains an abstraction of the bitstream I/O used
*              by the MPEG decoder. It will map high-level calls to the 
*              appropiate file I/O provided by the underlying OS.
*
******************************************************************************/

/* Include files */
#include "MP3_Bitstream.h"

/* External functions and variables (defined elsewhere, and used here) */
extern char filename[];

/* Global functions and variables (defined here, and used here & elsewhere) */
UINT32 MPG_Get_Byte (void);
STATUS MPG_Get_Bytes (UINT32 no_of_bytes, UINT32 data_vec[]);
UINT32 MPG_Get_Filepos (void);
void MPG_Set_Filepos (UINT32 position);
UINT32 MPG_Get_Filesize (void);

/* Local functions and variables (defined here, used here) */
static FILE *fp = (FILE *) NULL;


/******************************************************************************
*
* Name: MPG_Get_Byte
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file returns the next byte from the bitstream, or EOF.
*              If we're not on an byte-boundary the bits remaining until
*              the next boundary are discarded before getting that byte.
* Parameters: None
* Return value: The next byte in bitstream in the lowest 8 bits, or C_MPG_EOF.
*
******************************************************************************/
UINT32
MPG_Get_Byte (void)
{
  UINT32 val;


  /* File open? */
  if (fp == (FILE *) NULL) {
    if ((fp = fopen (filename, "r")) == (FILE *) NULL) {
      EXIT ("Cannot open mp3 file \"%s\"\n", filename);
    }
  }

  /* Get byte */
  val = fgetc (fp) & 0xff;

  /* EOF? */
  if (feof (fp)) {
    val = C_MPG_EOF;
  }

  /* Done */
  return (val);

}


/******************************************************************************
*
* Name: MPG_Get_Bytes
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file reads 'no_of_bytes' bytes of data from the input
*              stream into 'data_vec[]'.
* Parameters: Number of bytes to read, vector pointer where to store them.
* Return value: OK or ERROR if the operation couldn't be performed.
*
******************************************************************************/
STATUS
MPG_Get_Bytes (UINT32 no_of_bytes, UINT32 data_vec[])
{
  int i;
  UINT32 val;


  for (i = 0; i < no_of_bytes; i++) {
    val = MPG_Get_Byte ();

    if (val == C_MPG_EOF) {
      return (C_MPG_EOF);
    } else {
      data_vec[i] = val;
    }
  }
  
  return (OK);

}


/******************************************************************************
*
* Name: MPG_Get_Filepos
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function returns the current file position in bytes.
* Parameters: None
* Return value: File pos in bytes, or 0 if no file open.
*
******************************************************************************/
UINT32
MPG_Get_Filepos (void)
{

  /* File open? */
  if (fp == (FILE *) NULL) {
    return (0);
  }

  if (feof (fp)) {
    return (C_MPG_EOF);
  } else {
    return ((UINT32) ftell (fp));
  }

}


/******************************************************************************
*
* Name: MPG_Set_Filepos
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function sets the current file position.
* Parameters: New file position
* Return value: None
*
******************************************************************************/
void
MPG_Set_Filepos (UINT32 position)
{

  /* File open? */
  if (fp == (FILE *) NULL) {
    return;
  }
  
   fseek (fp, (long) position, SEEK_SET);

}


/******************************************************************************
*
* Name: MPG_Get_Filesize
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This function returns the current file size in bytes.
* Parameters: None
* Return value: File size in bytes, or 0 if no file open.
*
******************************************************************************/
UINT32
MPG_Get_Filesize (void)
{
  UINT32 curr_pos, size;


  /* File open? */
  if (fp == (FILE *) NULL) {
    return (0);			
  }
  
  curr_pos = MPG_Get_Filepos ();
  
  fseek (fp, 0, SEEK_END);

  size = ftell (fp);

  MPG_Set_Filepos (curr_pos);

  return (size);

}
