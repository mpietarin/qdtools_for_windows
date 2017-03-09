// ======================================================================
/*!
 * \file NFmiDataModifierAvgAbs.cpp
 * \brief Implementation of class NFmiDataModifierAvgAbs
 */
// ======================================================================
/*!
 * \class NFmiDataModifierAvgAbs
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierAvgAbs.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierAvgAbs::~NFmiDataModifierAvgAbs(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierAvgAbs::NFmiDataModifierAvgAbs(void) : NFmiDataModifier(), itsCounter(0), itsSum(0)
{
  fCalculationResultOk = false;
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierAvgAbs::Calculate(float theValue)
{
  if (CheckMissingValues(theValue))
  {
    itsCounter++;
    itsSum += std::abs(theValue);
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierAvgAbs::Calculate(NFmiQueryInfo* theQI) { Calculate(theQI->FloatValue()); }
// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierAvgAbs::Clear(void)
{
  itsCounter = 0;
  itsSum = 0;
  fCalculationResultOk = false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierAvgAbs::CalculationResult(void)
{
  if (fCalculationResultOk && itsCounter > 0)
    return itsSum / itsCounter;
  else
    return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

long NFmiDataModifierAvgAbs::Counter(void) { return itsCounter; }
// ======================================================================
