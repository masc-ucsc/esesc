// Contributed by Ian Lee
//                Joseph Nayfach - Battilana
//                Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/*******************************************************************************
File name:      RK4Matrix.h
Classes:        RK4Matrix
********************************************************************************/

#ifndef RK4_MATRIX_H
#define RK4_MATRIX_H

#include <stdint.h>
#include <string>
#include <vector>

#include "nanassert.h"
#include "sescthermMacro.h"

class ConfigData;
class ModelUnit;

// This class stores a dense matrix for RK4 based partial differental solver.
// If RK4 is chosen, instead of datalibrary, this should be used.
class sescthermRK4Matrix {
private:
  void free_mem();

  void matrixVectMult(MATRIX_DATA *vout, MATRIX_DATA **m, MATRIX_DATA *vin, int32_t n);

  MATRIX_DATA RK4(MATRIX_DATA **c, MATRIX_DATA *y, MATRIX_DATA *power, int32_t n, MATRIX_DATA h, MATRIX_DATA *yout,
                  int32_t num_iter);
  void RK4_core(MATRIX_DATA **c, MATRIX_DATA *y, MATRIX_DATA *k1, MATRIX_DATA *pow, int32_t n, MATRIX_DATA h, MATRIX_DATA *yout);

  // following are used in RK4 solution. kept here to avoid repeated alloc and dealloc.
  MATRIX_DATA *k1, *k2, *k3, *k4, *t1, *t2, *ytemp; // = dvector(n);

  // Following two arrays just store the non-zero coefficients and thir node indices.
  // indices 0-5 for neighbors. 6 for self. 7 is unused. made 8 to make array access faster ( use <<3 instead of multiply by 7 )
  // / if absent it is 0. coeff. is 0. so OK.
  size_t _numRowsInCoeffs;

  int32_t *    _nbr_indices;
  MATRIX_DATA *_coeffs;

public: // FIXME: make data members private
  // For solving dT + CT = p , where T = temp, p = energy vector in that time unit
  size_t _numelems;

  // actual coefficient matrix stored in dense matrix form
  MATRIX_DATA **unsolved_matrix_dyn_;

  MATRIX_DATA *_temporary_temp_vector;
  ; // scratch pad

  MATRIX_DATA *B;

  double _new_h;       // for next iteration
  double cur_time;     // total time
  double time_elapsed; // final time

public:
  sescthermRK4Matrix(size_t numelems);
  ~sescthermRK4Matrix() {
    free_mem();
  }
  void realloc_matrices(size_t numelems);

  void printMatrices();

  void print_unsolved_model_row(std::vector<std::string> &tempVector, int row);
  void initialize_Matrix(std::vector<ModelUnit *> &matrix_model_units);
  void solve_matrix(ConfigData *conf, MATRIX_DATA *temp_vector, MATRIX_DATA timestep, std::vector<ModelUnit *> &matrix_model_units);

  void set_timestep(double step) {
    _new_h = step;
  }

  // access coefficients and nbr indices. the first 4 are in one array; next 4 in 2nd array
  void set_coeff(int i, int dir, MATRIX_DATA val) {
    _coeffs[dir * _numRowsInCoeffs + i] = val;
  }

  MATRIX_DATA get_coeff(int i, int dir) const {
    return _coeffs[dir * _numRowsInCoeffs + i];
  }

  void set_nbr_index(int i, int dir, int32_t val) {
    I(val < (128 * 1024)); // A 50K node is over 4GB, this would be too big
    _nbr_indices[dir * _numRowsInCoeffs + i] = val;
  }

  int32_t get_nbr_index(int i, int dir) const {
    return _nbr_indices[dir * _numRowsInCoeffs + i];
  }

  int countGPUTempAccesses(bool device1_2, int N);
};

#endif
