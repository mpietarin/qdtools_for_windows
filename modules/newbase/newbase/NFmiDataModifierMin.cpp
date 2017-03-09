// ======================================================================
/*!
 * \file NFmiDataModifierMin.cpp
 * \brief Implementation of class NFmiDataModifierMin
 */
// ======================================================================
/*!
 * \class NFmiDataModifierMin
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierMin.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierMin::~NFmiDataModifierMin(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierMin::NFmiDataModifierMin(void) { itsExtremeValue = 3.4E+38f; }
NFmiDataModifierMin::NFmiDataModifierMin(const NFmiDataModifierMin& theOther)
    : NFmiDataModifierExtreme(theOther)
{
}

NFmiDataModifier* NFmiDataModifierMin::Clone(void) const { return new NFmiDataModifierMin(*this); }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------
/*
void NFmiDataModifierMin::Calculate(float theValue)
{
  if(theValue != kFloatMissing)
        if(itsExtremeValue > theValue)
          itsExtremeValue = theValue;
}
*/
// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierMin::Clear(void) { itsExtremeValue = 3.4E+38f; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierMin::CalculationResult(void)
{
  return itsExtremeValue != 3.4E+38f && !(!fMissingValuesAllowed && itsNumberOfMissingValues > 0)
             ? itsExtremeValue
             : kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \param value Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiDataModifierMin::IsNewExtreme(float value) { return value < itsExtremeValue; }
// ======================================================================
