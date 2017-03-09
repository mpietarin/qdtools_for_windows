// ======================================================================
/*!
 * \file NFmiDataModifierMinMax.cpp
 * \brief Implementation of class NFmiDataModifierMinMax
 */
// ======================================================================
/*!
 * \class NFmiDataModifierMinMax
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierMinMax.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierMinMax::~NFmiDataModifierMinMax(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierMinMax::NFmiDataModifierMinMax(void) : itsMinValue(FLT_MAX), itsMaxValue(-FLT_MAX)
{
}

NFmiDataModifierMinMax::NFmiDataModifierMinMax(const NFmiDataModifierMinMax& theOther)
    : NFmiDataModifier(theOther),
      itsMinValue(theOther.itsMinValue),
      itsMaxValue(theOther.itsMaxValue)
{
}

NFmiDataModifier* NFmiDataModifierMinMax::Clone(void) const
{
  return new NFmiDataModifierMinMax(*this);
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierMinMax::MinValue(void)
{
  if (itsMinValue != FLT_MAX)
    return itsMinValue;
  else
    return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierMinMax::MaxValue(void)
{
  if (itsMaxValue != -FLT_MAX)
    return itsMaxValue;
  else
    return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierMinMax::Calculate(float theValue)
{
  if (CheckMissingValues(theValue))
  {
    if (itsMaxValue < theValue) itsMaxValue = theValue;
    if (itsMinValue > theValue) itsMinValue = theValue;
  }
}

// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierMinMax::Clear(void)
{
  NFmiDataModifier::Clear();
  itsMaxValue = -FLT_MAX;
  itsMinValue = FLT_MAX;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierMinMax::CalculationResult(void) { return kFloatMissing; }
// ======================================================================
