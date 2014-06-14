/* Program to convert IIS reference mpeg1 decoder huffman tables to a format
 * we can use in our huffman decoder.
 */

#include <stdio.h>

#define DBG(str, args...) { printf (str, ## args); printf ("\n"); }
#define ERR(str, args...) { printf (str, ## args) ; printf ("\n"); }
#define EXIT(str, args...) { printf (str, ## args);  printf ("\n"); exit (0); }

int tabnum;
int numentries;
int table[1024];
int linbits;
int ref;			/* -1 = no reference */

int h_num[34];
int h_lbits[34];
int h_ref[34];

void Read_Table (FILE *fp);
void Print_Table (void);
void Print_Table_2 (void);


void
main (void)
{
  FILE *fp;


  /* Open input file */
  if ((fp = fopen ("huffdec", "r")) == (FILE *) NULL) {
    ERR ("Cannot open \"huffdec\".");
    exit (1);
  }
  
  printf ("/* Huffman tables generated on %s %s */\n\n", __DATE__, __TIME__);

  printf ("#include \"MPG_Huffman_Tables.h\"\n\n");

  do {
    Read_Table (fp);
    h_num[tabnum] = numentries;
    h_lbits[tabnum] = linbits;
    h_ref[tabnum] = ref;
    if (ref == tabnum) {
      Print_Table ();
    }
  } while ((!feof (fp)) && (tabnum < 33));
  
  Print_Table_2 ();

  /* Done */
  /* DBG ("Done"); */

}


void
Print_Table (void)
{
  int i;

  
  if (numentries == 0) {
    return;
  }

  printf ("UINT32 g_huffman_table_%d[%d] = {", tabnum, numentries);

  for (i = 0; i < numentries; i++) {
    if ((i % 5) == 0) {
      printf ("\n\t");
    }
    printf ("0x%08x, ", table[i]);
  }
  
  printf ("\n};\n\n");

  return;

}



void
Print_Table_2 (void)
{
  int i;

  
  printf ("UINT32 g_huffman_main [34][3] = {\n");

  for (i = 0; i < 34; i++) {
    if (h_num[h_ref[i]] == 0) {
      printf ("\t{ (UINT32) 0                 , %3d, %2d },\t/* Table %2d */\n", 
	      h_num[h_ref[i]], h_lbits[i], i);
    } else {
      printf ("\t{ (UINT32) g_huffman_table_%-2d, %3d, %2d },\t/* Table %2d */\n", 
	      h_ref[i], h_num[h_ref[i]], h_lbits[i], i);
    }
  }

  printf ("};\n\n");

}


void
Read_Table (FILE *fp)
{
  char cmd[256];
  int num, treelen, xlen, ylen, lbits, refnum;
  int i;
  int val1, val2;


  /* Scan for ".table num treelen xlen ylen linbits" in input file */
  do {
    cmd[0] = 0;
    fscanf (fp, "%s %d %d %d %d %d", cmd, &num, &treelen, &xlen, &ylen, 
	    &lbits);

  } while ((!feof (fp)) && (strcmp (".table", cmd) != 0));

  tabnum = num;

  /* DBG ("cmd = %s  num = %d  treelen = %d  xlen = %d  ylen = %d  lbits = %d",
       cmd, num, treelen, xlen, ylen, lbits); */

  /* Read next line, which should be ".treedata" or ".reference num" */
  fscanf (fp, "%s", cmd);

  if (strcmp (".treedata", cmd) == 0) {
    /* DBG ("Treedata"); */

    /* Read treedata if present */
    for (i = 0; i < treelen; i++) {
      if (fscanf (fp, "%x %x", &val1, &val2) != 2) {
	ERR ("Cannot read value");
	exit (1);
      }
      /* DBG ("Huff code: 0x%02x 0x%02x", val1, val2); */
      table [i] = ((val1 & 0xff) << 16) | (val2 & 0xff);
    }

    ref = tabnum;
    numentries = treelen;
    linbits = lbits;
    
  } else if (strcmp (".reference", cmd) == 0) {

    fscanf (fp, "%d", &refnum);

    /* DBG ("Ref: %d", refnum); */
    ref = refnum;
    linbits = lbits;
  } else if (strcmp (".end", cmd) == 0) {		
    /* DBG ("Found end", cmd); */
    ref = -1;
  } else {		
    ERR ("Unknown command \"%s\"", cmd);
    exit (1);
  }

  /* Done */

}
