/* mcmp - compare to text output files, not used at the moment. */

#include <stdio.h>


int
main (int ac, char *av[])
{
  FILE *fp1, *fp2;
  char line1[256], line2[256];
  int lineno = 1, diff = 0, sdiff = 0;
  int val1, val2;
  int dummy;
  

  if ((fp1 = fopen (av[1], "r")) == (FILE *) NULL) {
    fprintf (stderr, "Cannot open %s for reading.\n", av[1]);
    exit (1);
  }
  
  if ((fp2 = fopen (av[2], "r")) == (FILE *) NULL) {
    fprintf (stderr, "Cannot open %s for reading.\n", av[2]);
    exit (1);
  }

  while (!feof (fp1) && !feof (fp2)) {
    fgets (line1, 250, fp1);
    fgets (line2, 250, fp2);
    
    if (strcmp (line1, line2)) {
      
      if (sscanf (line1, "%d: %d", &dummy, &val1) == 2) {
        if (sscanf (line2, "%d: %d", &dummy, &val2) == 2) {
          if ((val1 == val2+1) || (val1 == val2-1)) {
            sdiff++;
            diff++;
            lineno++;
            continue;
          }
        }
      }
          
      printf ("Line %d differ:\n", lineno);
      printf ("%s: %s", av[1], line1);
      printf ("%s: %s", av[2], line2);
      diff++;
    }
    lineno++;
  }

  printf ("Compared %d lines, %d differed, %d smalldiff.\n", lineno-1, diff,
          sdiff);

  exit (0);
        
}
