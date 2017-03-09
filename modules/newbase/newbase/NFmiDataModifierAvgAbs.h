// ======================================================================
/*!
 * \file NFmiDataModifierAvgAbs.h
 * \brief Interface of class NFmiDataModifierAvgAbs
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERAVGABS_H
#define NFMIDATAMODIFIERAVGABS_H

#include "NFmiDataModifier.h"

class _FMI_DLL NFmiDataModifierAvgAbs : public NFmiDataModifier
{
 public:
  virtual ~NFmiDataModifierAvgAbs(void);
  NFmiDataModifierAvgAbs(void);

  virtual void Calculate(float theValue);
  virtual void Calculate(NFmiQueryInfo* theQI);

  void Clear(void);
  using NFmiDataModifier::CalculationResult;
  virtual float CalculationResult(void);
  long Counter(void);

 protected:
  long itsCounter;
  float itsSum;
  float itsAverage;

};  // class NFmiDataModifierAvgAbs

#endif  // NFMIDATAMODIFIERAVGABS_H

// ======================================================================
