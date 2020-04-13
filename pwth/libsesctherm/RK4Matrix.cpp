/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2008 University of California, Santa Cruz.
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Ian Lee
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
File name:      RK4Matrix.cpp
Description:    This contains routines to link the model units to RK4-matrix
                elements.  It also solves the partial differential equations

                //flush repo after a while (program warmup phase)
                using Runge-Kutta 4.  ( parts of the code here have been copied
                from other sources, and cosmetically modified  at places. )
********************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <vector>

#include "ModelUnit.h"
#include "RK4Matrix.h"
#include "sesctherm3Ddefine.h"
#include "sescthermMacro.h"

// Caller should check that the numelems is what he/she passed in.
sescthermRK4Matrix::sescthermRK4Matrix(size_t numelems) {
  memset(this, 0, sizeof(sescthermRK4Matrix));
  _new_h = 1.0e-6;
  realloc_matrices(numelems);
}

// Reallocate data for RK4 matrices
void sescthermRK4Matrix::realloc_matrices(size_t newNumelems) {
  if(_numelems == newNumelems)
    return;
  free_mem();

#ifdef DEBUG
  // sescthermRK4Matrix::realloc_matrices should only be called a few times.
  // Trigger an assertion if it is called more than 5 times.
  static int call_count = 0;
  call_count++;
  I(call_count <= 5);
#endif

  unsolved_matrix_dyn_ = (MATRIX_DATA **)calloc(newNumelems, sizeof(MATRIX_DATA *));
  I(unsolved_matrix_dyn_);
  size_t matrix_size = newNumelems;
  matrix_size *= newNumelems;
  unsolved_matrix_dyn_[0] = (MATRIX_DATA *)calloc(matrix_size, sizeof(MATRIX_DATA));
  I(unsolved_matrix_dyn_[0]);

  for(size_t i = 1; i < newNumelems; i++)
    unsolved_matrix_dyn_[i] = unsolved_matrix_dyn_[0] + newNumelems * i;

  // Also allocate the array, where i-th row is a list of 7 numbers for non-zero entries
  // See .h file for more comments.
  // We allocate the arrays so their size is multiple of number of rows per block
  int num_per_blk  = (newNumelems + 16 - 1) / 16; // ceiling of division
  _numRowsInCoeffs = (num_per_blk * 16);
  int nbr_size     = _numRowsInCoeffs << 3; // we create two arrays of float4; so the memory
  // accesseses in GPGPU can be coalesced.

  _nbr_indices = (int32_t *)calloc(nbr_size, sizeof(int32_t));
  I(_nbr_indices);
  memset(_nbr_indices, 0, nbr_size * sizeof(int32_t));

  _coeffs = (MATRIX_DATA *)calloc(nbr_size, sizeof(MATRIX_DATA));
  I(_coeffs);
  memset(_coeffs, 0, nbr_size * sizeof(MATRIX_DATA));

  B = (MATRIX_DATA *)calloc(_numRowsInCoeffs, sizeof(MATRIX_DATA));
  I(B);
  memset(B, 0, sizeof(MATRIX_DATA) * _numRowsInCoeffs);

  _temporary_temp_vector = (MATRIX_DATA *)calloc(_numRowsInCoeffs, sizeof(MATRIX_DATA));
  I(_temporary_temp_vector);
  memset(_temporary_temp_vector, 0, sizeof(MATRIX_DATA) * _numRowsInCoeffs);

  _numelems = newNumelems;
  assert(newNumelems <= _numRowsInCoeffs);
}

// Free allocated memory for matrices
void sescthermRK4Matrix::free_mem() {
  if(B) {
    free(B);
    B = 0;
  }

  if(unsolved_matrix_dyn_ && _numelems) {
    free(unsolved_matrix_dyn_[0]);
    free(unsolved_matrix_dyn_);
    unsolved_matrix_dyn_ = 0;
  }

  if(_numelems) {
    if(_nbr_indices) {
      free(_nbr_indices);
      _nbr_indices = 0x0;
    }
    if(_coeffs) {
      free(_coeffs);
      _coeffs = 0x0;
    }
  }

  // temporary vectors used in rk4 computation. saved to avoid repeated alloc during iterations,
  if(k1) {
    free(k1);
    k1 = 0x0;
  }
  if(k2) {
    free(k2);
    k2 = 0x0;
  }
  if(k3) {
    free(k3);
    k3 = 0x0;
  }
  if(k4) {
    free(k4);
    k4 = 0x0;
  }
  if(t1) {
    free(t1);
    t1 = 0x0;
  }
  if(t2) {
    free(t2);
    t2 = 0x0;
  }
  if(ytemp) {
    free(ytemp);
    ytemp = 0x0;
  }

  if(_temporary_temp_vector) {
    free(_temporary_temp_vector);
    _temporary_temp_vector = 0;
  }
  _numelems = 0;
}

static void printVector(const char *name, MATRIX_DATA *vec, int n) {
  printf("\n============= %s ===========================\n", name);
  int cnt = 0;
  for(int i = 0; i < n; i++) {
    if(vec[i] != 0) {
      printf("(%d: %g) ", i, vec[i]);
      cnt++;
    }
    if(cnt > 10) {
      printf("\n");
      cnt = 0;
    }
  }
  printf("\n");
}

// Print the matrix. For debugging purposes only
void sescthermRK4Matrix::printMatrices() {
  printVector("B", B, _numelems);

  char row_name[20];
  for(size_t i = 0; i < _numelems; i++) {
    sprintf(row_name, "C[%zu]", i);
    printVector(row_name, unsolved_matrix_dyn_[i], _numelems);
  }
}

// This function will solve the termeprature for each time step
// solves ODE  dT/dt + CT = p between t & t+h
// Note: At this point, C is formed based on conductances between neighboring nodes.
// Our original eq. is : (pho) * C_p * V * dT/dt + CT = power_in_unit.
// gamma = (pho) * Cp* V/dt.

void sescthermRK4Matrix::solve_matrix(ConfigData *conf, SUElement_t *temp_vector, MATRIX_DATA timestep,
                                      std::vector<ModelUnit *> &matrix_model_units) {
  assert(_numelems <= _numRowsInCoeffs);
  MATRIX_DATA *power_vector = B;
  for(size_t i = 0; i < _numelems; i++) {
    ModelUnit *munit     = matrix_model_units[i];
    power_vector[i]      = munit->total_power();
    MATRIX_DATA my_gamma = munit->row_ * munit->specific_heat_ * munit->volume();
    power_vector[i] /= my_gamma;
  }

  /*** printf("************************ RK4 ODE  ************************** \n");
    for(int i = 0; i < _numelems; i++)
    matrix_model_units[i]->print_rk4_equation(power_vector[i]);
    printf("\n");   ****/

  // to avoid stability issues, first try with small step size
  const MATRIX_DATA MIN_STEP = 1e-5; // in seconds
  MATRIX_DATA       t = 0.0, h = timestep, new_h = 0.0;

  // copy new values to temp_vector
  memmove(_temporary_temp_vector, temp_vector, sizeof(MATRIX_DATA) * _numelems);

  // printf("timestep %g\n", timestep);

  int32_t num_iter = 1;

  for(t = 0, new_h = MIN_STEP; t + new_h <= timestep; t += h * num_iter) {
    // printf("t %g new_h %g\n", t, new_h);
    h     = new_h;
    new_h = RK4(unsolved_matrix_dyn_, _temporary_temp_vector, power_vector, _numelems, h, _temporary_temp_vector, num_iter);
  }
  /* remainder    */
  if(timestep > t) {
    new_h = RK4(unsolved_matrix_dyn_, _temporary_temp_vector, power_vector, _numelems, timestep - t, _temporary_temp_vector, 1);
  }

  // copy new values to temp_vector
  memmove(temp_vector, _temporary_temp_vector, sizeof(MATRIX_DATA) * _numelems);
}

// This is called anytime the thermal conductances change. This creates a smaller,
// sort of adjacency list for each node.
void sescthermRK4Matrix::initialize_Matrix(std::vector<ModelUnit *> &matrix_model_units) {
  assert(_numelems <= _numRowsInCoeffs);

  for(size_t i = 0; i < _numelems; i++) {
    ModelUnit *munit = matrix_model_units[i];
    // Initialize the _nbr_indices and _coeffs
    for(int dir = 0; dir < MAX_DIR; dir++) {
      ModelUnit *nbr = munit->model_[dir];
      set_nbr_index(i, dir, nbr ? nbr->temperature_index_ : 0);
      set_coeff(i, dir, nbr ? *(munit->t_ptr[dir]) : 0.0);
    }
    // Also set self
    set_nbr_index(i, MAX_DIR, munit->temperature_index_);
    set_coeff(i, MAX_DIR, *(munit->t_mno));
  }
}
