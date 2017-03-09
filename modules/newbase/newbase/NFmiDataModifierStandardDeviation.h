// ======================================================================
/*!
 * \file NFmiDataModifierStandardDeviation.h
 * \brief Interface of class NFmiDataModifierStandardDeviation
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERSTANDARDDEVIATION_H
#define NFMIDATAMODIFIERSTANDARDDEVIATION_H

#include "NFmiDataModifier.h"

class _FMI_DLL NFmiDataModifierStandardDeviation : public NFmiDataModifier
{
 public:
  virtual ~NFmiDataModifierStandardDeviation(void);
  NFmiDataModifierStandardDeviation(void);

  void Clear(void);

  using NFmiDataModifier::Calculate;
  using NFmiDataModifier::CalculationResult;
  virtual void Calculate(float theValue);
  virtual float CalculationResult(void);

 protected:
  long itsCounter;
  float itsSum;
  float itsSquaredSum;

};  // class NFmiDataModifierStandardDeviation

#endif  // NFMIDATAMODIFIERSTANDARDDEVIATION_H

// ======================================================================
