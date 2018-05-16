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
File name:      RegressionLine.cpp
Description:    This code performs a non-linear regression as needed throughout
                the model.
********************************************************************************/

#include <complex>
#include <map>
#include <vector>

#include "RegressionLine.h"
#include "nanassert.h"
#include "sesctherm3Ddefine.h"
#include "sescthermMacro.h"

RegressionLine::RegressionLine(Points &points) {
  int n = points.size();
  if(n < 2)
    throw(std::string("Must have at least two points"));

  double sumx = 0, sumy = 0, sumx2 = 0, sumy2 = 0, sumxy = 0;
  double sxx, syy, sxy;

  // Conpute some things we need
  std::map<double, double>::const_iterator i;
  for(i = points.begin(); i != points.end(); i++) {
    double x = i->first;
    double y = i->second;

    sumx += x;
    sumy += y;
    sumx2 += (x * x);
    sumy2 += (y * y);
    sumxy += (x * y);
  }
  sxx = sumx2 - (sumx * sumx / n);
  syy = sumy2 - (sumy * sumy / n);
  sxy = sumxy - (sumx * sumy / n);

  // Infinite slope_, non existant yIntercept
  if(abs((int)sxx) == 0)
    throw(std::string("Inifinite Slope"));

  // Calculate the slope_ and yIntercept
  slope_      = sxy / sxx;
  yIntercept_ = sumy / n - slope_ * sumx / n;

  // Compute the regression coefficient
  if(abs((int)syy) == 0)
    regressionCoefficient_ = 1;
  else
    regressionCoefficient_ = sxy / sqrt(sxx * syy);
}

const double RegressionLine::slope() const {
  return slope_;
}

const double RegressionLine::yIntercept() const {
  return yIntercept_;
}

const double RegressionLine::regressionCoefficient() const {
  return regressionCoefficient_;
}

/****************************************************
 *     Polynomial Interpolation or Extrapolation        *
 *            of a Discrete Function                    *
 * -------------------------------------------------    *
 * INPUTS:                                              *
 *   (iter)->first:   (xcoords)                       *
 *   (iter)->second:   (ycoords)                      *
 *    X:    Interpolation abscissa value                *
 * OUTPUTS:                                             *
 *    y_estimate:Returned estimation of function for X  *
 ****************************************************/
double RegressionLine::quad_interpolate(Points &points, double x) {
  std::vector<double> c, d;
  double              den, dif, dift, ho, hp, w;
  double              y_estimate, y_estimate_error;

  std::vector<double> xa;
  std::vector<double> ya;

  Points_iter piter = points.begin();
  xa.push_back(0);
  for(piter = points.begin(); piter != points.end(); piter++) {
    xa.push_back(piter->first);
    ya.push_back(piter->second);
  }

  y_estimate       = 0;
  y_estimate_error = 0;
  int ns           = 1;
  dif              = fabs(x - xa[1]);
  int i            = 1;
  int n            = (int)xa.size() - 1;
  for(i = 1; i <= n; i++) {
    dift = fabs(x - xa[i]);
    if(LT(dift, dif)) {
      ns  = i; // index of closest table entry
      dif = dift;
    }

    c.push_back(ya[i]); // Initialize the C's and D's
    d.push_back(ya[i]);
  }

  y_estimate = ya[ns]; // Initial approximation of Y
  return (y_estimate);
  ns--;

  // FIXME: the following  algorithm return invalid data
  for(int m = 1; m < n; m++) {
    for(int i = 1; i <= n - m; i++) {
      ho  = xa[i] - x;
      hp  = xa[i + m] - x;
      w   = c[i + 1] - d[i];
      den = ho - hp;
      if(EQ(den, 0.0)) {
        return (y_estimate);
      }
      den  = w / den;
      d[i] = hp * den; // Update the C's and D's
      c[i] = ho * den;
    }
    if(2 * ns < n - m)                          // After each column in the tableau xa is completed,
      y_estimate_error = c[ns + 1];             // we decide which correction, C or D, we want to
    else {                                      // add to our accumulating value of Y, i.e. which
      y_estimate_error = d[ns];                 // path to take through the tableau, for (king up or
      ns--;                                     // down. We do this in such a way as to take the
    }                                           // most "straight line" route through the tableau to
    y_estimate = y_estimate + y_estimate_error; // its apex, updating NS accordingly to keep track
    // of where we are. This route keeps the partial
    // approximations centered (insofar as possible) on
  } // the target X.The last DY added is thus the error

  return (y_estimate);
}
