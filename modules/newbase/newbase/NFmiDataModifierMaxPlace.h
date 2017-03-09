// ======================================================================
/*!
 * \file NFmiDataModifierMaxPlace.h
 * \brief Interface of class NFmiDataModifierMaxPlace
 */
// ======================================================================
/*!
 * \class NFmiDataModifierMaxPlace
 *
 * Undocumented
 *
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERMAXPLACE_H
#define NFMIDATAMODIFIERMAXPLACE_H

#include "NFmiDataModifierExtremePlace.h"

//! Undocumented
class _FMI_DLL NFmiDataModifierMaxPlace : public NFmiDataModifierExtremePlace
{
 public:
  virtual ~NFmiDataModifierMaxPlace(void);
  NFmiDataModifierMaxPlace(void);

  void Clear(void);

  using NFmiDataModifier::CalculationResult;
  virtual float CalculationResult(void);

 protected:
  virtual bool IsNewExtreme(float value);

 private:
  NFmiDataModifierMaxPlace& operator=(const NFmiDataModifierMaxPlace& theOther);

};  // class NFmiDataModifierMaxPlace

#endif  // NFMIDATAMODIFIERMAXPLACE_H

// ======================================================================
