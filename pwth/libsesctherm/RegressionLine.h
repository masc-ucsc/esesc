/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2008 University of California, Santa Cruz.
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Ian Lee
                  Joseph Nayfach - Battilana
                  Jose Renau

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
File name:      RegressionLine.h
Classes:        RegressionLine
********************************************************************************/

#ifndef REGRESSION_LINE_H 
#define REGRESSION_LINE_H 

class RegressionLine {
  public:
    typedef std::map<double, double> Points;
    typedef std::map<double,double>::iterator Points_iter;

    double slope_;
    double yIntercept_;
    double regressionCoefficient_;

    RegressionLine(Points & points);

    const double slope() const;
    const double yIntercept() const;
    const double regressionCoefficient() const;

    static double quad_interpolate(Points & points, double x);
};
#endif
