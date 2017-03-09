// ======================================================================
/*!
 * \brief Interface of class NFmiModMeanCalculator
 */
// ======================================================================

#ifndef NFMIMODMEANCALCULATOR_H
#define NFMIMODMEANCALCULATOR_H

#include "NFmiGlobals.h"

class _FMI_DLL NFmiModMeanCalculator
{
 public:
  NFmiModMeanCalculator(float theModulo = 360);

  void operator()(float theValue, float theWeight = 1.0);
  float operator()() const;
  int count() const;

 private:
  float itsModulo;
  bool itsValid;
  int itsCount;
  float itsLastValue;
  float itsWeightedSum;
  float itsWeightSum;

};  // NFmiModMeanCalculator

#endif  // NFMIMODMEANCALCULATOR_H

// ======================================================================
