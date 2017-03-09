// ======================================================================
/*!
 * \file NFmiDataModifierReplace.h
 * \brief Interface of class NFmiDataModifierReplace
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERREPLACE_H
#define NFMIDATAMODIFIERREPLACE_H

#include "NFmiDataModifier.h"

class _FMI_DLL NFmiDataModifierReplace : public NFmiDataModifier
{
 public:
  NFmiDataModifierReplace(float theOrigValue, float theNewValue);

  float FloatOperation(float theValue);

 private:
  float itsOriginalValue;
  float itsNewValue;

};  // class NFmiDataModifierReplace

#endif  // NFMIDATAMODIFIERREPLACE_H

// ======================================================================
