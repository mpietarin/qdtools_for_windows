// ======================================================================
/*!
 * \file NFmiDataModifierChange.cpp
 * \brief Implementation of class NFmiDataModifierChange
 */
// ======================================================================

#include "NFmiDataModifierChange.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierChange::~NFmiDataModifierChange(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierChange::NFmiDataModifierChange(void)
    : itsFirstValueOK(false), itsFirstValue(kFloatMissing), itsLastValue(kFloatMissing)
{
}

NFmiDataModifier* NFmiDataModifierChange::Clone(void) const
{
  return new NFmiDataModifierChange(*this);
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierChange::Calculate(float theValue)
{
  if (!itsFirstValueOK)
  {
    itsFirstValueOK = true;
    itsFirstValue = theValue;
  }
  itsLastValue = theValue;
}

// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierChange::Calculate(NFmiQueryInfo* theQI)
{
  if (theQI) Calculate(theQI->FloatValue());
}

// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierChange::Clear(void)
{
  itsFirstValueOK = false;
  itsFirstValue = kFloatMissing;
  itsLastValue = kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierChange::CalculationResult(void)
{
  if (!itsFirstValueOK) return kFloatMissing;

  if (itsFirstValue == kFloatMissing) return itsLastValue;

  if (itsLastValue == kFloatMissing) return kFloatMissing;

  return itsLastValue - itsFirstValue;
}

// ======================================================================
