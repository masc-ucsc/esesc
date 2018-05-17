//  Contributed by  Ian Lee
//                  Joseph Nayfach - Battilana
//                  Jose Renau
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
File name:      Utilities.h
Description:    This code adds a variety of utility routines that can be used
                throughout the modeling framework.
Classes:        ValueEquals
                Utilities
                DynamicArray_Rows
                DynamicArray
********************************************************************************/

#ifndef UTILITIES_H
#define UTILITIES_H

#include <complex>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <vector>

#include "ModelUnit.h"
#include "sesctherm3Ddefine.h"

class DataLibrary;
class Utilities;
class RegressionLine;

template <typename T1, typename T2> class ValueEquals {
public:
  ValueEquals(T1 const &value)
      : value_(value) {
  }

  typedef std::pair<T1, T2> pair_type;
  bool                      operator()(pair_type p) const {
    return p.first == value_;
  }

private:
  T1 const &value_;
};

// ALL THE GENERAL UTILITY FUNCTIONS
class Utilities {
public:
  inline static std::string stringify(MATRIX_DATA x) {
    std::ostringstream o;
    if(!(o << x)) {
      std::cerr << "BadConversion: stringify(MATRIX_DATA)" << std::endl;
      exit(1);
    }
    return (o.str());
  }
  inline static std::string stringify(int x) {
    std::ostringstream o;
    if(!(o << x)) {
      std::cerr << "BadConversion: stringify(int)" << std::endl;
      exit(1);
    }

    return (o.str());
  }
  inline static std::string stringify(unsigned int x) {
    std::ostringstream o;
    if(!(o << x)) {
      std::cerr << "BadConversion: stringify(unsigned int)" << std::endl;
      exit(1);
    }
    return (o.str());
  }
  inline static MATRIX_DATA convertToDouble(const std::string &s, bool failIfLeftoverChars = true) {
    std::istringstream i(s);
    MATRIX_DATA        x;
    char               c;
    if(!(i >> x) || (failIfLeftoverChars && i.get(c))) {
      std::cerr << "BadConversion: convertToDouble(string)" << std::endl;
      exit(1);
    }
    return (x);
  }
  inline static int convertToInt(const std::string &s, bool failIfLeftoverChars = true) {
    std::istringstream i(s);
    int                x;
    char               c;
    if(!(i >> x) || (failIfLeftoverChars && i.get(c))) {
      std::cerr << "BadConversion: convertToInt(string)" << std::endl;
      exit(1);
    }
    return (x);
  }
  //----------------------------------------------------------------------------
  // Tokenize : Split up a string based upon delimiteres, defaults to space-deliminted (like strok())
  //

  static void Tokenize(const std::string &str, std::vector<std::string> &tokens, const std::string &delimiters = " ") {
    tokens.clear();
    // Skip delimiters at beginning.
    // Fine the location of the first character that is not one of the delimiters starting
    // from the beginning of the line of test
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Now find the next occurrance of a delimiter after the first one
    // (go to the end of the first character-deliminated item)
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);
    // Now keep checking until we reach the end of the line
    while(std::string::npos != pos || std::string::npos != lastPos) {
      // Found a token, add it to the vector.
      // Take the most recent token and store it to "tokens"
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters to find the beginning of the next token
      lastPos = str.find_first_not_of(delimiters, pos);
      // Find the end of the next token
      pos = str.find_first_of(delimiters, lastPos);
    }
  }
  static void fatal(std::string s) {
    std::cerr << s << std::endl;
    exit(1);
  }

  static int eq(MATRIX_DATA x, MATRIX_DATA y) {
    return (fabs(x - y) < DELTA);
  }

  static int lt(MATRIX_DATA x, MATRIX_DATA y) {
    return (x < y && !eq(x, y));
  }
  static int le(MATRIX_DATA x, MATRIX_DATA y) {
    return (x < y || eq(x, y));
  }
  static int gt(MATRIX_DATA x, MATRIX_DATA y) {
    return (x > y && !eq(x, y));
  }
  static int ge(MATRIX_DATA x, MATRIX_DATA y) {
    return (x > y || eq(x, y));
  }
  static MATRIX_DATA min(MATRIX_DATA x, MATRIX_DATA y) {
    if(le(x, y))
      return (x);
    else
      return (y);
  }
  static MATRIX_DATA max(MATRIX_DATA x, MATRIX_DATA y) {
    if(ge(x, y))
      return (x);
    else
      return (y);
  }

  static MATRIX_DATA abs(MATRIX_DATA x, MATRIX_DATA y) {
    if(ge(x, y))
      return (fabs(x - y));
    else
      return (fabs(y - x));
  }
  struct myUnique {
    bool operator()(const MATRIX_DATA &a, const MATRIX_DATA &b) {
      return (EQ(a, b));
    }
  };
};

// Note: indexing method works like cartesian coordinate system [x][y], where [x]=column, [y]=row, thus dyn_array[x] selects COLUMN
// x (not ROW x)

template <class T> class DynamicArray_Rows {
public:
  DynamicArray_Rows() {
    nrows_       = 0;
    ncols_       = 0;
    col_         = 0;
    max_row_     = 0; // size is [0,0]
    max_col_     = 0;
    datalibrary_ = NULL;
    newalloc_pointers_.clear();
    data_.clear();
  }

  DynamicArray_Rows(DataLibrary *datalibrary) {
    datalibrary_ = datalibrary;
    nrows_       = 0;
    ncols_       = 0;
    col_         = 0;
    max_row_     = 0; // size is [0,0]
    max_col_     = 0;
    newalloc_pointers_.clear();
    data_.clear();
  }

  ~DynamicArray_Rows() {
    for(unsigned int i = 0; i < newalloc_pointers_.size(); i++) {
      std::cerr << "deleting newalloc pointers..." << std::endl;
      delete[] newalloc_pointers_[i];
    }
  }

  DynamicArray_Rows(int rows, int cols, DataLibrary *datalibrary) {
    datalibrary_ = datalibrary;
    for(int i = 0; i < rows; i++) {
      data_.push_back(std::vector<T *>(cols)); // store row
      T *newalloc = new T[cols];
      newalloc_pointers_.push_back(newalloc);
      for(int j = 0; j < cols; j++) {
        data_[i][j] = &newalloc[j];
        zero(data_[i][j]); // zero the element
      }
    }
    nrows_   = rows;
    ncols_   = cols;
    max_row_ = 0; // although we allocated the space, the actual size is [0,0]
    max_col_ = 0;
    col_     = 0;
  }

  DynamicArray_Rows(int rows, int cols) {
    datalibrary_ = NULL;
    T *newalloc;
    for(int i = 0; i < rows; i++) {
      data_.push_back(std::vector<T *>(cols)); // store row
      newalloc = new T[cols];
      newalloc_pointers_.push_back(newalloc);
      for(int j = 0; j < cols; j++) {
        data_[i][j] = &newalloc[j];
        zero(data_[i][j]); // zero the element
      }
    }
    nrows_   = rows;
    ncols_   = cols;
    max_row_ = 0; // although we allocated the space, the actual size is [0,0]
    max_col_ = 0;
    col_     = 0;
  }

  void zero(ModelUnit *value) {
    if(value)
      value->initialize(datalibrary_);
  }
  void zero(int *value) {
    *value = 0;
  }
  void zero(char *value) {
    *value = 0;
  }
  void zero(double *value) {
    *value = 0;
  }
  void zero(float *value) {
    *value = 0;
  }

  // the resize function should always INCREASE the size (not DECREASE)
  void increase_size(int rows, int cols) {
    T * newalloc;
    int old_size;
    if(rows < nrows_ || cols < ncols_) {
      std::cerr << "sesctherm3Dutil dynamic array: increase_size => attempting to decrease size (not allowed)" << std::endl;
      exit(1);
    }

    if(rows == nrows_ && cols == ncols_)
      return;

    data_.resize(rows);
    nrows_ = rows;
    ncols_ = cols;

    for(int i = 0; i < nrows_; i++) {
      if(data_[i].size() != (unsigned int)ncols_) { // resize the given row as necessary
        old_size = data_[i].size();
        newalloc = new T[ncols_ - old_size]; // allocate space for the other elements
        data_[i].resize(ncols_);             // resize the row

        newalloc_pointers_.push_back(newalloc); // save a pointer to the newly allocated elements
        int itor = 0;
        for(int j = old_size; j < ncols_; j++) { // store the newly allocated elements to the pointer locations
          data_[i][j] = &(newalloc[itor]);       // store the element
          zero(data_[i][j]);                     // zero the element
          itor++;
        }
      }
    }
  }

  int empty() {
    if(max_row_ == 0 && max_col_ == 0)
      return true;
    else
      return false;
  }

  void clear() {
    if(!empty()) {
      for(unsigned int i = 0; i < newalloc_pointers_.size(); i++)
        delete[] newalloc_pointers_[i]; // delete the data at [i][j]
    }
    for(int i = 0; i < nrows_; i++) { // delete the row vector
      data_[i].clear();
    }
    data_.clear(); // delete the column vector
    nrows_   = 0;
    ncols_   = 0;
    max_row_ = 0;
    max_col_ = 0;
  }

  const T &operator[](int index) const {

#ifdef _ESESCTHERM_DEBUG
    if(index < 0 || col_ < 0) {
      std::cerr << "Dynamic Array bounds is negative! [" << col_ << "][" << index << "]" << std::endl;
      exit(1);
    }
#endif

    max_col_ = MAX(col_, max_col_);
    max_row_ = MAX(index, max_row_);

    if(index < nrows_ && col_ < ncols_)
      return (*data_[index][col_]); // common case

    int newcolsize = ncols_;
    int newrowsize = nrows_;
    while(newcolsize <= col_ + 1) {
      newcolsize = (newcolsize + 1) * 2;
      // the col index has approached the number of cols, double the number of cols
      // keep doubling the number of cols until the index is less than half of ncols_
    }
    // If the array index selected is greater than or equal to the the number of rows
    // then double the number of rows in all of the columns
    // and keep doubling until the index is less than half of nrows_
    while(newrowsize <= index + 1) {
      newrowsize = (newrowsize + 1) * 2;
    }
    increase_size(newrowsize, newcolsize);
    return (*data_[index][col_]);
  }

  T &operator[](int index) {
#ifdef _ESESCTHERM_DEBUG
    if(index < 0 || col_ < 0) {
      std::cerr << "Dynamic Array bounds is negative! [" << col_ << "][" << index << "]" << std::endl;
      exit(1);
    }
#endif

    max_col_ = MAX(col_, max_col_);
    max_row_ = MAX(index, max_row_);

    if(index < nrows_ && col_ < ncols_)
      return (*data_[index][col_]); // common case

    int newcolsize = ncols_;
    int newrowsize = nrows_;
    while(newcolsize <= col_ + 1) {
      newcolsize = (newcolsize + 1) * 2;
      // the col index has approached the number of cols, double the number of cols
      // keep doubling the number of cols until the index is less than half of ncols_
    }
    // If the array index selected is greater than or equal to the the number of rows
    // then double the number of rows in all of the columns
    // and keep doubling until the index is less than half of nrows_
    while(newrowsize <= index + 1) {
      newrowsize = (newrowsize + 1) * 2;
    }
    increase_size(newrowsize, newcolsize);
    return (*data_[index][col_]);
  }

  std::vector<T> &getrow(int row) {
    return (data_[row]);
  }

  int col_;   // row to select
  int nrows_; // number of rows
  int ncols_; // number of columns
  int max_row_;
  int max_col_;

private:
  std::vector<std::vector<T *>> data_; // this is the matrix (stored in data_[row][column])
  std::vector<T *>              newalloc_pointers_;
  DataLibrary *                 datalibrary_;
};

template <class T> class DynamicArray {
public:
  DynamicArray() {
    data_        = new DynamicArray_Rows<T>();
    datalibrary_ = NULL;
  }

  DynamicArray(DataLibrary *datalibrary) {
    data_        = new DynamicArray_Rows<T>(datalibrary);
    datalibrary_ = datalibrary;
  }

  ~DynamicArray() {
    // data_->clear();
    delete data_;
  }
  DynamicArray(int rows, int cols, DataLibrary *datalibrary) {
    data_ = new DynamicArray_Rows<T>(rows, cols, datalibrary);
  }

  DynamicArray(int rows, int cols) {
    data_ = new DynamicArray_Rows<T>(rows, cols);
  }

  void clear() {
    data_->clear();
  }
  DynamicArray_Rows<T> &operator[](int i) {
    // cout << "Indexing row:" << i << std::endl; //TESTING
    data_->col_ = i; // store row value
    return (*data_);
  }

  const DynamicArray_Rows<T> &operator[](int i) const {
    // cout << "Indexing row:" << i << std::endl; //TESTING
    data_->col_ = i; // store row value
    return (*data_);
  }

  void increase_size(int rows, int cols) {
    data_->increase_size(rows, cols);
  }
  // Note: nrows=max_row_+1

  unsigned int nrows() {
    return (data_->max_row_ + 1);
  }
  // Note: ncols=max_col_+1
  unsigned int ncols() {
    return (data_->max_col_ + 1);
  }

private:
  DynamicArray_Rows<T> *data_;
  DataLibrary *         datalibrary_;
};

#endif
