// ======================================================================
/*!
 * \file NFmiDataModifierAvg.cpp
 * \brief Implementation of class NFmiDataModifierAvg
 */
// ======================================================================
/*!
 * \class NFmiDataModifierAvg
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierAvg.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierAvg::~NFmiDataModifierAvg(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierAvg::NFmiDataModifierAvg(void) : NFmiDataModifier(), itsCounter(0), itsAverage(0.)
{
  fCalculationResultOk = false;
}

NFmiDataModifierAvg::NFmiDataModifierAvg(const NFmiDataModifierAvg& theOther)
    : NFmiDataModifier(theOther), itsCounter(theOther.itsCounter), itsAverage(theOther.itsAverage)
{
}

NFmiDataModifier* NFmiDataModifierAvg::Clone(void) const { return new NFmiDataModifierAvg(*this); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierAvg::Avg(void) { return itsAverage; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierAvg::Calculate(float theValue)
{
  if (CheckMissingValues(theValue))
  {
    itsCounter++;
    itsAverage -= ((itsAverage - theValue) / itsCounter);
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierAvg::Calculate(NFmiQueryInfo* theQI) { Calculate(theQI->FloatValue()); }
// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------

void NFmiDataModifierAvg::Clear(void)
{
  itsCounter = 0;
  itsAverage = 0;
  fCalculationResultOk = false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiDataModifierAvg::CalculationResult(void)
{
  if (fCalculationResultOk)
    return itsAverage;
  else
    return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

long NFmiDataModifierAvg::Counter(void) { return itsCounter; }
// ======================================================================
