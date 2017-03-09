// ======================================================================
/*!
 * \file NFmiMaskedDataIterator.h
 * \brief Interface of class NFmiMaskedDataIterator
 */
// ======================================================================

#ifndef NFMIMASKEDDATAITERATOR_H
#define NFMIMASKEDDATAITERATOR_H

#include "NFmiDataIterator.h"

class NFmiSuperSmartInfo;
class NFmiIntegrationSelector;

//! Undocumented
class _FMI_DLL NFmiMaskedDataIterator : public NFmiDataIterator
{
 public:
  virtual ~NFmiMaskedDataIterator();
  NFmiMaskedDataIterator(NFmiSuperSmartInfo* theData,
                         bool isCombinedParam = false,
                         NFmiIntegrationSelector* theIntegrationSelector = 0);

  virtual void DoForEach(NFmiDataModifier* theModifier);

  bool fIsCombinedParam;
  NFmiIntegrationSelector* itsIntegrationSelector;

 private:
  NFmiMaskedDataIterator(const NFmiMaskedDataIterator& theIter);
  NFmiMaskedDataIterator& operator=(const NFmiMaskedDataIterator& theIter);

};  // class NFmiMaskedDataIterator

#endif  // NFMIMASKEDDATAITERATOR_H

// ======================================================================
