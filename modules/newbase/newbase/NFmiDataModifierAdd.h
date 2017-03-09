// ======================================================================
/*!
 * \file NFmiDataModifierAdd.h
 * \brief Interface of class NFmiDataModifierAdd
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERADD_H
#define NFMIDATAMODIFIERADD_H

#include "NFmiDataModifier.h"

class _FMI_DLL NFmiDataModifierAdd : public NFmiDataModifier
{
 public:
  NFmiDataModifierAdd(float theAddValue = 0);
  virtual float FloatOperation(float theValue);

 private:
  float itsAddValue;

};  // class NFmiDataModifierAdd

#endif  // NFMIDATAMODIFIERADD_H

// ======================================================================
