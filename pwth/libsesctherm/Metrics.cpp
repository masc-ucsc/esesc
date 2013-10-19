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
File name:      Metrics.cpp
Classes:        Metrics
********************************************************************************/

#include <iostream> 
#include <float.h> 
#include "Metrics.h"
#include "Report.h"
#include "math.h"
//#include <iomanip>
template <typename T> T tabs(const T& num) {
  return num < static_cast<T>(0) ? -num : num;
}

Metrics::Metrics(size_t flpSize)
{
	//FIXME: define as constant
	nCall    = 0;
	MDT      = 500;
	IMC      = 2.5;
	AMBT     = 303;
	CMC      = 2.35;
	VDE      = 1.1;
	KLVOFST  = 273.15;
	EM1      = 10444.0545073;
	SM1      = 29011.262502;
	TD1      = 78;
	TD2      = 0.081;
	TD3      = 8807.81930115;
	TD4      = -775180.934541;
	NB1      = 1.6328;
	NB2      = 856.064334447;
	NB3      = 795.140683155;
	NB4      = 3.33;
  NB5      = 0.01;

	MaxT = new std::vector <GStatsMax *> (flpSize);
	for (size_t i=0;i<flpSize;i++){
		(*MaxT)[i] = new GStatsMax("flpBlock[%d]_MaxT",i);
		(*MaxT)[i]-> setIgnoreSampler();
	}
	chipMaxT = new GStatsMax("ChipMaxT");
  chipMaxT -> setIgnoreSampler();
	chipAvgT = new GStatsAvg("ChipAvgT");
  chipAvgT -> setIgnoreSampler();
	chipGradT = new GStatsMax("ChipGradT");
  chipGradT -> setIgnoreSampler();
	//chipMaxTHist = new GStatsHist("ChipMaxTHist");

	EM_fit = new std::vector <GStatsAvg *> (flpSize);
	for (size_t i=0;i<flpSize;i++){
		(*EM_fit)[i] = new GStatsAvg("flpBlock[%d]_EM_fit",i);
		(*EM_fit)[i] -> setIgnoreSampler();
	}


	SM_fit = new std::vector <GStatsAvg *> (flpSize);
	for (size_t i=0;i<flpSize;i++){
		(*SM_fit)[i] = new GStatsAvg("flpBlock[%d]_SM_fit",i);
		(*SM_fit)[i] -> setIgnoreSampler();
	}

	TDDB_fit = new std::vector <GStatsAvg *> (flpSize);
	for (size_t i=0;i<flpSize;i++){
		(*TDDB_fit)[i] = new GStatsAvg("flpBlock[%d]_TDDB_fit",i);
		(*TDDB_fit)[i] -> setIgnoreSampler();
	}

	TC_fit = new std::vector <GStatsAvg *> (flpSize);
	for (size_t i=0;i<flpSize;i++){
		(*TC_fit)[i] = new GStatsAvg("flpBlock[%d]_TC_fit",i);
		(*TC_fit)[i] -> setIgnoreSampler();
	}

	NBTI_fit = new std::vector <GStatsAvg *> (flpSize);
	for (size_t i=0;i<flpSize;i++){
		(*NBTI_fit)[i] = new GStatsAvg("flpBlock[%d]_NBTI_fit",i);
		(*NBTI_fit)[i] -> setIgnoreSampler();
	}

	Power = new GStatsAvg("ChipPower");
  Power -> setIgnoreSampler();
	Leak = new GStatsAvg("ChipLeak");
  Leak -> setIgnoreSampler();
	LSQPower = new GStatsAvg("LSQPower");
  LSQPower -> setIgnoreSampler();
	DCPower = new GStatsAvg("DCPower");
  DCPower -> setIgnoreSampler();
	Throttling    = new GStatsMax("ThrottlingCycles");
  Throttling -> setIgnoreSampler();
	sescThermTime = new GStatsMax("sescThermTime");
  sescThermTime -> setIgnoreSampler();
	simulatedTime = new GStatsMax("simulatedTime");
  simulatedTime -> setIgnoreSampler();
}



void Metrics::updateTiming(){
	double maxV = DBL_MIN;
	double minV = DBL_MAX;
	for(size_t i=0;i<(*Temperature).size();i++){
		(*MaxT)[i]->sample((*Temperature)[i]);
		chipMaxT->sample((*Temperature)[i]);

		maxV = (*Temperature)[i] > maxV ? (*Temperature)[i] : maxV;
		minV = (*Temperature)[i] < minV ? (*Temperature)[i] : minV;
	} 
   chipGradT->sample(maxV  - minV) ;// Gradients at each time snapshot
   chipAvgT->sample(maxV);
   //chipMaxTHist->sample(maxV);


}

// Computes the FIT value for each reliability metric
// TODO: figure out how to get timeinterval (Pass from Sampler?)
void Metrics::updateFITs(double timeinterval, ThermTrace *trace){


	double scaleFactor = 1e-10;// to avoid very big numbers
	double scaleFactorSM = 1e-30;// to avoid very big numbers

	double power = 0;
	double leak  = 0;


	for(size_t i=0;i<(*Temperature).size();i++){
		double em   = exp(EM1/(*Temperature)[i]) * scaleFactor ;
		(*EM_fit)[i]->sample( timeinterval/em);

		double sm   = exp(SM1/(*Temperature)[i]) * pow((MDT - (*Temperature)[i]),-IMC)  * scaleFactorSM ;
		(*SM_fit)[i]->sample((timeinterval/sm));

		//printf("sM:%ld, sM2:%lf, interv:%lf, sm:%lf temp:%lf \n", static_cast<uint64_t>((1/sm) * timeinterval), ((1/sm) * timeinterval), timeinterval, sm, (*Temperature)[i]);
		double tddb = exp(TD3/(*Temperature)[i]) * pow(pow((1/VDE),(*Temperature)[i]),TD2)
			* pow(1/VDE,TD1) * exp(TD4*pow(1/(*Temperature)[i],2))  * scaleFactor ;
		(*TDDB_fit)[i]->sample((timeinterval/tddb));

		double tc   = pow(tabs(1/((*Temperature)[i] - AMBT)),CMC) ;
		(*TC_fit)[i]->sample((timeinterval/tc));

		double nbti = pow((log(NB1/(1+2*exp(NB2/(*Temperature)[i]))) - 
					log(NB1/ (1+2*exp(NB2/(*Temperature)[i]))-NB5))* (*Temperature)[i] / 
				exp(1/(*Temperature)[i])*NB3,NB4) * scaleFactor ;
		(*NBTI_fit)[i]->sample((timeinterval/nbti));

		//double energy = exp((*flp)[i]->devType.a)*exp((*flp)[i]->devType.b/(*Temperature)[i])*pow((*flp)[i]->devType.c,(*Temperature)[i]) * timeinterval;
	}
	for (size_t i=0;i<trace->scaledLkgCntrValues_->size();i++){
		leak += trace->scaledLkgCntrValues_->at(i);
	}
	if (trace->energyCntrValues_->size()> 0){
		for(size_t i=0; i<trace->energyCntrValues_->at(0).size(); i++){
			if (i == 6){
				LSQPower -> sample(trace->energyCntrValues_->at(0)[i]);
				//printf("LSQP:%e\t",(trace->energyCntrValues_->at(0)[i]));
			}
			if (i == 1){
				DCPower -> sample(trace->energyCntrValues_->at(0)[i]);
				//printf("DCP:%e\n",(trace->energyCntrValues_->at(0)[i]));
			}
			power += trace->energyCntrValues_->at(0)[i];
		}
	}
	Leak->sample(leak);
	Power->sample(power);

}


void Metrics::updateAllMetrics(const std::vector < MATRIX_DATA > * Temp, double timeinterval, ThermTrace *trace, uint64_t throttleTotal, double time, double simTime){
  Temperature = const_cast <std::vector < MATRIX_DATA > *> (Temp);
	updateTiming();
	updateFITs(timeinterval,trace);
	Throttling->sample(static_cast<double>(throttleTotal));
	sescThermTime->sample(time);
	simulatedTime->sample(simTime);
}


Metrics::~Metrics(){
}
