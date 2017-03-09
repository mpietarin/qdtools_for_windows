// ======================================================================
/*!
 * \file NFmiDataModifierMax.h
 * \brief Interface of class NFmiDataModifierMax
 */
// ======================================================================
/*!
 * \class NFmiDataModifierMax
 *
 * Undocumented
 *
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERMAX_H
#define NFMIDATAMODIFIERMAX_H

#include "NFmiDataModifierExtreme.h"

//! Undocumented
class _FMI_DLL NFmiDataModifierMax : public NFmiDataModifierExtreme
{
 public:
  virtual ~NFmiDataModifierMax(void);
  NFmiDataModifierMax(void);
  NFmiDataModifierMax(const NFmiDataModifierMax& theOther);
  NFmiDataModifier* Clone(void) const;

  void Clear(void);
  using NFmiDataModifier::CalculationResult;
  virtual float CalculationResult(void);

 protected:
  virtual bool IsNewExtreme(float value);

 private:
  NFmiDataModifierMax& operator=(const NFmiDataModifierMax& theOther);

};  // class NFmiDataModifierMax

#endif  // NFMIDATAMODIFIERMAX_H

// ======================================================================
