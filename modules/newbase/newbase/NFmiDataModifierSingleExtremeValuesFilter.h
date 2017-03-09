// ======================================================================
/*!
 * \file NFmiDataModifierSingleExtremeValuesFilter.h
 * \brief Interface of class NFmiDataModifierSingleExtremeValuesFilter
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERSINGLEEXTREMEVALUESFILTER_H
#define NFMIDATAMODIFIERSINGLEEXTREMEVALUESFILTER_H

#include "NFmiDataModifierAvg.h"

class _FMI_DLL NFmiDataModifierSingleExtremeValuesFilter : public NFmiDataModifier
{
 public:
  virtual ~NFmiDataModifierSingleExtremeValuesFilter(void);
  NFmiDataModifierSingleExtremeValuesFilter(void);
  NFmiDataModifierSingleExtremeValuesFilter(float thePointValue, float theLimit);
  void Clear(void);
  using NFmiDataModifier::Calculate;
  using NFmiDataModifier::CalculationResult;
  virtual void Calculate(float theValue);
  virtual float CalculationResult(void);

 private:
  NFmiDataModifierAvg itsAvgModifier;
  float itsValue;
  float itsLimit;

};  // class NFmiDataModifierSingleExtremeValuesFilter

#endif  // NFMIDATAMODIFIERSINGLEEXTREMEVALUESFILTER_H

// ======================================================================
