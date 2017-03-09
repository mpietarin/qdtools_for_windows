// ======================================================================
/*!
 * \file NFmiDataModifierBoolean.h
 * \brief Interface of class NFmiDataModifierBoolean
 */
// ======================================================================

#ifndef NFMIDATAMODIFIERBOOLEAN_H
#define NFMIDATAMODIFIERBOOLEAN_H

#include "NFmiDataModifier.h"

//! Undocumented
enum FmiModifierBoolOperations
{
  kFmiModifierValueLessThan,
  kFmiModifierValueGreaterThan,
  kFmiModifierValueLessOrEqualThan,
  kFmiModifierValueGreaterOrEqualThan,
  kFmiModifierValueIsEqual
};

//! Undocumented
class _FMI_DLL NFmiDataModifierBoolean : public NFmiDataModifier
{
 public:
  virtual ~NFmiDataModifierBoolean(void);
  NFmiDataModifierBoolean(FmiModifierBoolOperations theCondition,
                          NFmiDataModifier* theFirstValue,
                          NFmiDataModifier* theSecondValue);

  virtual bool BooleanValue(void);

  virtual std::ostream& WriteOperator(std::ostream& file) const;
  virtual std::ostream& WriteExpressionBody(std::ostream& file);

  operator bool();

 protected:
  NFmiDataModifier* itsSecondValue;
  NFmiDataModifier* itsFirstValue;
  FmiModifierBoolOperations itsCondition;
  bool itsBooleanValue;

 private:
  NFmiDataModifierBoolean(const NFmiDataModifierBoolean& theMod);
  NFmiDataModifierBoolean& operator=(const NFmiDataModifierBoolean& theMod);

};  // class NFmiDataModifierBoolean

#endif  // NFMIDATAMODIFIERBOOLEAN_H

// ======================================================================
