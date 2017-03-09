// ======================================================================
/*!
 * \file NFmiDataModifierMax.cpp
 * \brief Implementation of class NFmiDataModifierMax
 */
// ======================================================================

#include "NFmiDataModifierMaxPlace.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierMaxPlace::~NFmiDataModifierMaxPlace(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierMaxPlace::NFmiDataModifierMaxPlace(void) { itsExtremeValue = -3.4E+38f; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------
/*
void NFmiDataModifierMaxPlace::Calculate(float theValue)
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

void NFmiDataModifierMaxPlace::Clear(void) { itsExtremeValue = -3.4E+38f; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierMaxPlace::CalculationResult(void)
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

bool NFmiDataModifierMaxPlace::IsNewExtreme(float value) { return value > itsExtremeValue; }
// ======================================================================
