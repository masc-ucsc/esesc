/*
    ESESC: Super ESCalar simulator
    Copyright (C) 2008 University of California, Santa Cruz.
    Copyright (C) 2010 University of California, Santa Cruz.

    Contributed by  Ehsan K.Ardestani
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
File name:      Metrics.h
Classes:        Metrics
********************************************************************************/

#ifndef METRICS_H
#define METRICS_H


#include "ThermTrace.h"
#include "GStatsTherm.h"

class Metrics {
	private:
		std::vector <GStatsMax * > * MaxT;
		std::vector <GStatsAvg* > * EM_fit;
		std::vector <GStatsAvg* > * SM_fit;
		std::vector <GStatsAvg* > * TDDB_fit;
		std::vector <GStatsAvg* > * TC_fit;
		std::vector <GStatsAvg* > * NBTI_fit;
		GStatsAvg *ChipPower;
		GStatsAvg *ChipLeak;
		GStatsMax * chipMaxT;
		GStatsMax * chipGradT;
		GStatsAvg * chipAvgT;
    GStatsHist * chipMaxTHist;  
		GStatsMax * sescThermTime;
		GStatsMax * simulatedTime;
		GStatsMax * Throttling;

		GStatsAvg * LSQPower;
		GStatsAvg * DCPower;
		uint32_t nCall;
		std::vector < MATRIX_DATA > * Temperature;

		// parameters,FIXME: define as constant
		int MDT       ; 
		float IMC     ; 
		int AMBT      ; 
		float CMC     ; 
		float VDE     ; 
		float KLVOFST ; 
		float EM1     ; 
		float SM1     ; 
		int TD1       ; 
		float TD2     ; 
		float TD3     ; 
		float TD4     ; 
		float NB1     ; 
		float NB2     ; 
		float NB3     ; 
		float NB4     ; 
		float NB5     ; 

		void updateTiming();
    void updateFITs(double timeinterval, ThermTrace *trace);
	public:
		Metrics(size_t flpSize);
		~Metrics();
		void updateAllMetrics(const std::vector < MATRIX_DATA > * Temp, double interval,
			 	ThermTrace *trace, uint64_t throttleTotal, double time, double simTime);

};

#endif
