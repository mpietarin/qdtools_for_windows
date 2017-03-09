// ======================================================================
/*!
 * \file NFmiDataModifierMinPlace.h
 * \brief Interface of class NFmiDataModifierMinPlace
 */
// ======================================================================
/*!
 * \class NFmiDataModifierMinPlace
 *
 * Undocumented
 *
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERMINPLACE_H
#define NFMIDATAMODIFIERMINPLACE_H

#include "NFmiDataModifierExtremePlace.h"

//! Undocumented
class _FMI_DLL NFmiDataModifierMinPlace : public NFmiDataModifierExtremePlace
{
 public:
  virtual ~NFmiDataModifierMinPlace(void);
  NFmiDataModifierMinPlace(void);

  void Clear(void);

  using NFmiDataModifier::CalculationResult;
  virtual float CalculationResult(void);

 protected:
  virtual bool IsNewExtreme(float value);

 private:
  NFmiDataModifierMinPlace& operator=(const NFmiDataModifierMinPlace& theOther);

};  // class NFmiDataModifierMinPlace

#endif  // NFMIDATAMODIFIERMINPLACE_H

// ======================================================================
