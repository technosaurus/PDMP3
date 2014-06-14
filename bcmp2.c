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
      printf ("%7d: %6d %6d  (%7d)  %7d\n", ctr, a, b, ectr, a - b);
      if (diff == 1) {
        sctr++;
      } else {
        ectr++;
      }

    }
    ctr++;
  }
  
  printf ("%d samples, %d big err, %d small err\n", ctr, ectr, sctr);
  
  return (0);
}

