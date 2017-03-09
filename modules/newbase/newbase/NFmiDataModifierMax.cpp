// ======================================================================
/*!
 * \file NFmiDataModifierMax.cpp
 * \brief Implementation of class NFmiDataModifierMax
 */
// ======================================================================

#include "NFmiDataModifierMax.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierMax::~NFmiDataModifierMax(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierMax::NFmiDataModifierMax(void) { itsExtremeValue = -3.4E+38f; }
NFmiDataModifierMax::NFmiDataModifierMax(const NFmiDataModifierMax& theOther)
    : NFmiDataModifierExtreme(theOther)
{
}

NFmiDataModifier* NFmiDataModifierMax::Clone(void) const { return new NFmiDataModifierMax(*this); }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------
/*
void NFmiDataModifierMax::Calculate(float theValue)
{
  if(theValue != kFloatMissing)
        if(itsExtremeValue < theValue)
          itsExtremeValue = theValue;
}
*/
// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierMax::Clear(void) { itsExtremeValue = -3.4E+38f; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierMax::CalculationResult(void)
{
  return itsExtremeValue != -3.4E+38f && !(!fMissingValuesAllowed && itsNumberOfMissingValues > 0)
             ? itsExtremeValue
             : kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \param value Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiDataModifierMax::IsNewExtreme(float value) { return value > itsExtremeValue; }
// ======================================================================
