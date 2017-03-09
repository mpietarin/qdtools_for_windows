// ======================================================================
/*!
 * \file NFmiCalculator.cpp
 * \brief Implementation of class NFmiCalculator
 */
// ======================================================================
/*!
 * \class NFmiCalculator
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiCalculator.h"

// ----------------------------------------------------------------------.
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiCalculator::~NFmiCalculator(void) {}
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theDataIterator Undocumented
 * \param theDataModifier Undocumented
 */
// ----------------------------------------------------------------------

NFmiCalculator::NFmiCalculator(NFmiDataIterator* theDataIterator, NFmiDataModifier* theDataModifier)
    : itsDataModifier(theDataModifier), itsDataIterator(theDataIterator), itsData(0)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theData Undocumented
 * \param theDataModifier Undocumented
 */
// ----------------------------------------------------------------------

NFmiCalculator::NFmiCalculator(NFmiQueryInfo* theData, NFmiDataModifier* theDataModifier)
    : itsDataModifier(theDataModifier), itsDataIterator(0), itsData(theData)
{
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

double NFmiCalculator::FloatValue(void)
{
  if (itsDataIterator && itsDataModifier)
  {
    itsDataIterator->DoForEach(itsDataModifier);
    return itsDataModifier->CalculationResult();
  }
  else if (itsData && itsDataModifier)
  {
    return itsDataModifier->FloatValue();
  }
  return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiCombinedParam* NFmiCalculator::CombinedValue(void)
{
  if (itsDataIterator && itsDataModifier)
  {
    itsDataIterator->DoForEach(itsDataModifier);
    return itsDataModifier->CombinedCalculationResult();
  }
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataModifier* NFmiCalculator::CalculatedModifier(void)
{
  if (itsDataIterator && itsDataModifier)
  {
    itsDataIterator->DoForEach(itsDataModifier);
    return itsDataModifier;
  }
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

// Tämä on kuten FloatValue, paitsi että juoksutus tehdään täällä loopissa
// ja laskut tehdään täällä eikä iteratorin DoForEach-metodissa.

float NFmiCalculator::CalculatedValue(void) { return kFloatMissing; }
// ======================================================================
