/******************************************************************************
*
* Filename: requant_sim.c
* Author: Krister Lagerström (krister@kmlager.com)
* Description: This file contains the simulation code for the Newton
*              method of finding the solution to y=x**4/3
*
******************************************************************************/

/******************************* Include files *******************************/


/**** External functions and variables (defined elsewhere, and used here) ****/
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <math.h>

/********** Local functions and variables (defined here, used here) **********/
#define DIM 3

#define ABS(a)  (a > 0 ? a : -a)

typedef double  *Vector;
typedef double  *Matrix;

double p[DIM] = { 32.0, 300.0, 4000.0 };

/* double p[DIM] = { 32.0, 500.0, 2000.0, 6000.0 }; >18 bits, 2 iterations */

float coeff[DIM];

float table[32];
float guess[65];

typedef float float;
typedef unsigned int uint32_t;

float func (float x, float c[DIM]);
void calc_pol (void);

/******************************* Start of code *******************************/

void
init (void)
{
  int i;


  for (i = 0; i < 32; i++) {
    table[i] = pow ((float) i, 4.0/3.0);
  }

  for (i = 0; i < 65; i++) {
    guess[i] = pow ((float) (i+0.5)*128, 4.0/3.0);
  }
}


float
newton (int is_pos)
{
  float a4, a2;
  float x, x2, x3, x_next;
  static float powtab34[32];
  static uint32_t init = 0;
  uint32_t i;


  /* First time initialization */
  if (init == 0) {
    for (i = 0; i < 32; i++) {
      powtab34[i] = pow ((float) i, 4.0 / 3.0);
    }
    init = 1;
  }

  /* We use a table for 0<is_pos<32 since they are so common */
  if (is_pos < 32) return (powtab34[is_pos]);
  
  a2 = is_pos * is_pos;
  a4 = a2 * a2;

  x = func (is_pos, coeff);

  for (i = 0; i < 3; i++) {

    x2 = x*x;
    x3 = x*x2;
    
    x_next = (2*x3 + a4) / (3*x2);  

    x = x_next;
  }

  return (x);

}


int
main (void)
{
  int i;
  float res;
  float pow_43;
  float rel_err;
  float bits;


  init ();

  calc_pol ();
  
  for (i = 0; i < 8207; i++) {
    pow_43 = pow ((float) i, 4.0/3.0);
    res =  newton (i);
    rel_err = 1.0/(fabs(res - pow_43)/res);
    if (res == pow_43) {
      bits = 24.0;
    } else {
      bits = log (rel_err) / log (2.0);
    }
    
    printf ("%4d:   res=%e   abs=%e  rel=%e  acc=%2.2f\n", i, res,
            fabs (pow_43 - res), fabs ((pow_43 - res)/pow_43), bits);
  }
  
  exit (0);
  
}


  
void dump (double mat[DIM][DIM], double vec[DIM])
{
  int i, j;


  printf ("Dump:\n");
  
  for (i = 0; i < DIM; i++) {
    printf ("Row %d: ", i);
    for (j = 0; j < DIM; j++) {
      printf ("%20.9f ", mat[i][j]);
    }
    printf (" = %20.9f\n", vec[i]);
  }

}


void
elim (double mat[DIM][DIM], double vec[DIM])
{
  int i, j, k;
  double p, a, b;
  

  for (i = 0; i < DIM-1; i++) {

    /*  printf ("Elim: Starting on pivot %d=%5.2f\n", i, mat[i][i]); */
    
    for (j = i+1; j < DIM; j++) {

      a = mat[j][i];
      b = mat[i][i];
      
      p = mat[j][i] / mat[i][i];
      
      /*  printf ("\tElim: Starting on row %d=%5.2f\n", j, p); */
      
      for (k = 0; k < DIM; k++) {

        /* printf ("\t\tElim: Elem %d,%d = %5.2f - (%5.2f / %5.2f) * %5.2f\n",
           j, k, mat[j][k], a, b, mat[i][k]); */
        
        mat[j][k] = mat[j][k] - p * mat[i][k];
        
      }

      /* printf ("\tElim: Vec %d = %5.2f - (%5.2f / %5.2f) * %5.2f\n",
         j, mat[j][k], a, b, vec[i]); */
      
      vec[j] = vec[j] - p * vec[i];
      
    }
    
  }
  
}


void
back (double mat[DIM][DIM], double vec[DIM])
{
  int i, j;


  for (i = DIM-1; i >= 0; i--) {

    vec[i] /= mat[i][i];
    mat[i][i] /= mat[i][i];

    for (j = 0; j < i; j++) {

      vec[j] -= vec[i] * mat[j][i];
      
      mat[j][i] = 0.0;
    }
  }
}
    

float
func (float x, float c[DIM])
{
  int i;
  float res, is_f1, is_f2, is_f3;


  is_f1 = (float) x;
  is_f2 = is_f1 * is_f1;
  is_f3 = is_f1 * is_f2;
  
  /*  res = coeff[0] + coeff[1]*is_f1 + coeff[2]*is_f2 + coeff[3]*is_f3; */
  res = coeff[0] + coeff[1]*is_f1 + coeff[2]*is_f2 + coeff[3]*is_f3;


#if 0
  for (i = 0; i < DIM; i++) {
    res += c[i] * pow (x, i);
  }
#endif
  
  
  return (res);
}


void
calc_pol (void)
{
  double mat[DIM][DIM];
  double vec[DIM];
  double a, b;
  int i, j;
  

  for (i = 0; i < DIM; i++) {
    for (j = 0; j < DIM; j++) {
      mat[i][j] = pow (p[i], (double) j);
    }
    vec[i] = pow (p[i], 4.0 / 3.0);
  }

  /*  dump (mat, vec); */

  elim (mat, vec);
  
  /*  dump (mat, vec); */

  back (mat, vec);
  
  /*  dump (mat, vec); */

  printf ("  static float coeff[%d] = {\n", DIM);
  
  for (i = 0; i < DIM; i++) {

    coeff[i] = vec[i];

    printf ("    %1.9e,\n", coeff[i]);
    
  }

  printf ("  };\n");
  
#if 0
  for (i = 0; i < DIM; i++) {

    printf ("Val %d = ", (int) p[i]);
    
    for (j = 0; j < DIM; j++) {

      printf ("%6.4f*x^%d", vec[j], j);

      if (j != DIM-1) printf (" + ");
    }

    printf (" = %6.4f. x^1.333 = %6.4f\n", func (p[i], vec),
            pow (p[i], 4.0 / 3.0));
  }

  for (i = 0; i < 8207; i++) {
    a = func ((double) i, vec);
    b = pow ((double) i, 4.0 / 3.0);
    
    printf ("%4d: %20.8f   %20.8f  %3d%%\n", i, a, b,
            ABS ((int) (((a - b)/b) * 100.0)));
  }
#endif
  
}
