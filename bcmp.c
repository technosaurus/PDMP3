/* 
 * bcmp compares two PCM files, and counts how many samples differ 
 * by 1 (small errors), and 2 or more (big errors). This is used to
 * verify the accuracy of the decoder compared to the IIS reference
 * implementation.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>


int
main (int ac, char *av[])
{
  FILE *fp1, *fp2;
  short a, b;
  int ctr = 0, ectr = 0, sctr = 0, diff;
  

  fp1 = fopen (av[1], "r");
  fp2 = fopen (av[2], "r");

  if (fp1 == (FILE *) NULL) {
    fprintf (stderr, "Cannot open %s for reading.\n", av[1]);
    perror("");
    exit (1);
  }
  
  if (fp1 == (FILE *) NULL) {
    fprintf (stderr, "Cannot open %s for reading.\n", av[1]);
    perror("");
    exit (1);
  }
  
  while (!feof (fp1) && !feof (fp2)) {
    fread ((void *) &a, 2, 1, fp1);
    fread ((void *) &b, 2, 1, fp2);

    diff = a - b;
    if (diff < 0) diff = -diff;

    if (diff >= 1) {
      printf ("%7d: %6d %6d  (%7d)  %7d ", ctr, a, b, ectr, a - b);
      if (diff == 1) {
         printf ("small\n");
         sctr++;
      } else {
         printf ("big\n");
         ectr++;
      }

    }
    ctr++;
  }
  
  printf ("%d samples, %d big err, %d small err\n", ctr, ectr, sctr);
  
  return (0);
}

