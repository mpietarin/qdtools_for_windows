// ======================================================================
/*!
 * \file NFmiDataModifierSum.cpp
 * \brief Implementation of class NFmiDataModifierSum
 */
// ======================================================================
/*!
 * \class NFmiDataModifierSum
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierSum.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierSum::~NFmiDataModifierSum(void) {}
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theJoinOperator Undocumented
 * \param missingValuesAlloved Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataModifierSum::NFmiDataModifierSum(FmiJoinOperator theJoinOperator, bool missingValuesAlloved)
    : NFmiDataModifier(theJoinOperator, missingValuesAlloved), itsSum(0)
{
}

NFmiDataModifierSum::NFmiDataModifierSum(const NFmiDataModifierSum& theOther)
    : NFmiDataModifier(theOther), itsSum(theOther.itsSum)
{
}

NFmiDataModifier* NFmiDataModifierSum::Clone(void) const { return new NFmiDataModifierSum(*this); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierSum::Sum(void) { return itsSum; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierSum::Calculate(float theValue)
{
  if (CheckMissingValues(theValue))
  {
    fCalculationResultOk = true;
    if (IsInside(theValue)) itsSum += theValue;
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierSum::Calculate(NFmiQueryInfo* theQI) { Calculate(theQI->FloatValue()); }
// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierSum::Clear(void)
{
  NFmiDataModifier::Clear();
  itsSum = 0;
  fCalculationResultOk = false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierSum::CalculationResult(void)
{
  if (!fCalculationResultOk || (!fMissingValuesAllowed && itsNumberOfMissingValues > 0))
    return kFloatMissing;
  else
    return Sum();
}

// ======================================================================
