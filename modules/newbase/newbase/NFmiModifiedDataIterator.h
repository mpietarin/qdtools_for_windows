// ======================================================================
/*!
 * \file NFmiModifiedDataIterator.h
 * \brief Interface of class NFmiModifierDataIterator
 */
// ======================================================================

#ifndef NFMIMODIFIERDATAITERATOR_H
#define NFMIMODIFIERDATAITERATOR_H

#include "NFmiRelativeDataIterator.h"

class NFmiInfoModifier;

//! Undocumented
class _FMI_DLL NFmiModifiedDataIterator : public NFmiRelativeDataIterator
{
 public:
  virtual ~NFmiModifiedDataIterator();
  NFmiModifiedDataIterator(NFmiInfoModifier* theData, long dx = 0, long dy = 0, long dt = 0);
  virtual void DoForEach(NFmiDataModifier* theDataModifier);

 protected:
  NFmiModifiedDataIterator(const NFmiModifiedDataIterator& theIter);
  NFmiModifiedDataIterator& operator=(const NFmiModifiedDataIterator& theIter);
  NFmiInfoModifier* itsPrimaryModifier;

};  // class NFmiModifiedDataIterator

#endif  // NFMIMODIFIERDATAITERATOR_H

// ======================================================================
