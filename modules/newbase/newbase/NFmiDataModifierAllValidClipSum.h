// ======================================================================
/*!
 * \file NFmiDataModifierAllValidClipSum.h
 * \brief Interface of class NFmiDataModifierAllValidClipSum
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERCLIPSUM_H
#define NFMIDATAMODIFIERCLIPSUM_H

#include "NFmiDataModifierAllValidSum.h"

class _FMI_DLL NFmiDataModifierAllValidClipSum : public NFmiDataModifierAllValidSum
{
 public:
  virtual ~NFmiDataModifierAllValidClipSum(void);
  NFmiDataModifierAllValidClipSum(void);

  virtual void Calculate(float theValue) { NFmiDataModifierAllValidSum::Calculate(theValue); }
  virtual void Calculate(NFmiQueryInfo* theQI);

 private:
  float itsHalfValue;

};  // class NFmiDataModifierAllValidClipSum

#endif  // NFMIDATAMODIFIERCLIPSUM_H

// ======================================================================
