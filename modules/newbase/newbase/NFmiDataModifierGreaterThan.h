// ======================================================================
/*!
 * \file NFmiDataModifierGreaterThan.h
 * \brief Interface of class NFmiDataModifierGreaterThan
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERGREATERTHAN_H
#define NFMIDATAMODIFIERGREATERTHAN_H

#include "NFmiDataModifier.h"

class _FMI_DLL NFmiDataModifierGreaterThan : public NFmiDataModifier
{
 public:
  NFmiDataModifierGreaterThan(float theValue = kFloatMissing);
  virtual bool BoolOperation(float theValue);

 private:
  double itsLimit;

};  // class NFmiDataModifierGreaterThan

#endif  // NFMIDATAMODIFIER_H

// ======================================================================
