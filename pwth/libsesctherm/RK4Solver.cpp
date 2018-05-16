/*
    ESESC: Super ESCalar simulator
    Copyright (C) 2008 University of California, Santa Cruz.
    Copyright (C) 2010 University of California, Santa Cruz.

    Contributed by  Ian Lee
                    Madan Das

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

********************************************************************************
File name:      RK4Solver.cpp
Description:    This contains routines to link the model units to RK4-matrix
                elements.  It also solves the partial differential equations
                using Runge-Kutta 4.  ( parts of the code here have been copied
                from other sources, and cosmetically modified  at places. )
********************************************************************************/

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "RK4Matrix.h"

/**** Following code is for RK4 solver, inherited from RCutil.cpp in libsescspot ********/

// Define the appropriate functions
#ifdef USE_FLOAT
#define AXPY saxpy
#define GEMV sgemv
#define SBMV ssbmv
#define COPY scopy
#define GETRF_ sgetrf_
#define GETRI_ sgetri_
#define GETRS_ sgetrs_
#define POTRF_ spotrf_
#define POTRI_ spotri_
#define POTRS_ spotrs_
#define GEMM sgemm
#define IMAX isamax

#else

#define AXPY daxpy
#define GEMV dgemv
#define SBMV dsbmv
#define COPY dcopy
#define GETRF_ dgetrf_
#define GETRI_ dgetri_
#define GETRS_ dgetrs_
#define POTRF_ dpotrf_
#define POTRI_ dpotri_
#define POTRS_ dpotrs_
#define GEMM dgemm
#define IMAX idamax
#endif

/** Utility functions, from util.cpp in libsescspot ****/
static void fatal(const char *s) {
  fputs(s, stderr);
  exit(1);
}

static MATRIX_DATA *dvector(int32_t n) {
  MATRIX_DATA *v = (MATRIX_DATA *)calloc(n, sizeof(MATRIX_DATA));
  if(!v)
    fatal("allocation failure in dvector()");
  return v;
}

static void copy_dvector(MATRIX_DATA *dst, MATRIX_DATA *src, int32_t n) {
  memmove(dst, src, sizeof(MATRIX_DATA) * n);
}

/* mult of an n x n matrix and an n x 1 column vector	*/
void sescthermRK4Matrix::matrixVectMult(MATRIX_DATA *vout, MATRIX_DATA **m, MATRIX_DATA *vin, int32_t n) {
  for(int32_t i = 0; i < n; i++) {
    vout[i] = 0.0;
    for(int dir = 0; dir < 7; dir++) {
      int32_t j = get_nbr_index(i, dir);
      vout[i] += get_coeff(i, dir) * vin[j];
    }
  }
}

/** the matrix a is an in-place lower/upper triangular matrix
 * the following macros split them into their constituents */

#define LOWER(a, i, j) ((i > j) ? a[i][j] : 0)
#define UPPER(a, i, j) ((i <= j) ? a[i][j] : 0)

/* core of the 4th order Runge-Kutta method, where the Euler step
 * (y(n+1) = y(n) + k1 where k1 = h * dydx(n)) is provided as an input.
 * Given values for y, and k1, advances the solution over an interval h,
 * and returns the solution in yout. For details, see the discussion in
 * "Numerical Recipes in C", Chapter 16, from
 * http://www.library.cornell.edu/nr/bookcpdf/c16-0.pdf
 */
void sescthermRK4Matrix::RK4_core(MATRIX_DATA **c, MATRIX_DATA *y, MATRIX_DATA *k1, MATRIX_DATA *pow, int32_t n, MATRIX_DATA h,
                                  MATRIX_DATA *yout) {
  if(!k2) {
    k2 = dvector(n);
    k3 = dvector(n);
    k4 = dvector(n);
  }
  I(k2);
  I(k3);
  I(k4);

  /* k2 = k1 - h/2*c*k1	*/
  for(int i = 0; i < n; i++) {
    float temp = 0.0;
    for(int d = 0; d < 7; d++) {
      int j = get_nbr_index(i, d);
      temp += get_coeff(i, d) * k1[j];
    }
    temp *= (h / 2.0);
    k2[i] = k1[i] - temp;
  }

  /* k3 = k1 - h/2*c*k2	*/
  for(int i = 0; i < n; i++) {
    float temp = 0;
    for(int d = 0; d < 7; d++) {
      int j = get_nbr_index(i, d);
      temp += get_coeff(i, d) * k2[j];
    }
    temp *= (h / 2.0);
    k3[i] = k1[i] - temp;
  }

  /* k4 = k1 - h*c*k3	*/
  for(int i = 0; i < n; i++) {
    float temp = 0.0;
    for(int d = 0; d < 7; d++) {
      int j = get_nbr_index(i, d);
      temp += get_coeff(i, d) * k3[j];
    }
    temp *= h;
    k4[i] = k1[i] - temp;
  }

  /* yout = y + k1/6 + k2/3 + k3/3 + k4/6	*/
  for(int i = 0; i < n; i++)
    yout[i] = y[i] + (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]) / 6.0;
}

/*
 * 4th order Runge Kutta solver	with adaptive step sizing.
 * It integrates and solves the ODE dy + cy = power between
 * t and t+h. It returns the correct step size to be used
 * next time.
 */
#define RK4_SAFETY 0.95
#define RK4_MAXUP 5.0
#define RK4_MAXDOWN 10.0
// #define RK4_PRECISION	0.01   ( original)
#define RK4_PRECISION 0.2
MATRIX_DATA sescthermRK4Matrix::RK4(MATRIX_DATA **c, MATRIX_DATA *y, MATRIX_DATA *power, int32_t n, MATRIX_DATA h,
                                    MATRIX_DATA *yout, int32_t num_iter) {
  int32_t     i;
  MATRIX_DATA max = 0.0, new_h = h;

  if(!k1)
    k1 = dvector(n);
  if(!t1)
    t1 = dvector(_numRowsInCoeffs);
  if(!t2)
    t2 = dvector(_numRowsInCoeffs);
  if(!ytemp)
    ytemp = dvector(_numRowsInCoeffs);

  /*
   * find k1 in the Euler step: y(n+1) = y(n) + h * dy
   * where, k1 = h * dy which, for our given equation
   * is h*(p-cy)
   */
  matrixVectMult(t2, c, y, n);
  for(i = 0; i < n; i++)
    k1[i] = h * (power[i] - t2[i]);

  /* try until accuracy is achieved	*/
  do {
    h = new_h;

    /* try RK4 once with normal step size	*/
    RK4_core(c, y, k1, power, n, h, ytemp);

    if(true) {
      /* repeat it with two half-steps	*/
      RK4_core(c, y, k1, power, n, h / 2.0, t1);

      /* y after 1st half-step is in t1. re-evaluate k1 for this	*/
      matrixVectMult(t2, c, t1, n);
      for(i = 0; i < n; i++)
        k1[i] = h * (power[i] - t2[i]);

      /* get output of the second half-step in t2	*/
      RK4_core(c, t1, k1, power, n, h / 2.0, t2);

      /* find the max diff between these two results:
       * use t1 to store the diff
       */
      for(i = 0; i < n; i++)
        t1[i] = fabs(ytemp[i] - t2[i]);
      max = t1[0];
      for(i = 1; i < n; i++)
        if(max < t1[i])
          max = t1[i];

      /*
       * compute the correct step size: see equation
       * 16.2.10  in chapter 16 of "Numerical Recipes
       * in C"
       */
      /* accuracy OK. increase step size	*/
      if(max <= RK4_PRECISION) {
        new_h = RK4_SAFETY * h * pow(fabs(RK4_PRECISION / max), 0.2);
        if(new_h > RK4_MAXUP * h)
          new_h = RK4_MAXUP * h;
        /* inaccuracy error. decrease step size	and compute again */
      } else {
        new_h = RK4_SAFETY * h * pow(fabs(RK4_PRECISION / max), 0.25);
        if(new_h < h / RK4_MAXDOWN)
          new_h = h / RK4_MAXDOWN;
      }
    } else {
      new_h = h;
      break;
    }
  } while(new_h < h);

  /* commit ytemp to yout	*/
  copy_dvector(yout, ytemp, n);

  /* return the step-size	*/
  return new_h;
}
