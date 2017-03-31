// ======================================================================
/*!
 * \file NFmiRelativeTimeIntegrationIterator.cpp
 * \brief Implementation of class NFmiRelativeTimeIntegrationIterator
 */
// ======================================================================
/*!
 * \class NFmiRelativeTimeIntegrationIterator
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiRelativeTimeIntegrationIterator.h"
#include "NFmiDataModifier.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiRelativeTimeIntegrationIterator::~NFmiRelativeTimeIntegrationIterator() = default;
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theData Undocumented
 * \param theStepnumber Undocumented
 * \param theStartShift Undocumented
 */
// ----------------------------------------------------------------------

NFmiRelativeTimeIntegrationIterator::NFmiRelativeTimeIntegrationIterator(NFmiQueryInfo* theData,
                                                                         int theStepnumber,
                                                                         int theStartShift)
    : NFmiDataIterator(theData), itsStartShift(theStartShift), itsStepnumber(theStepnumber)
{
}

// ----------------------------------------------------------------------
/*!
 * \param theDataModifier Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRelativeTimeIntegrationIterator::DoForEach(NFmiDataModifier* theDataModifier)
{
  if (!theDataModifier) return;

  theDataModifier->Clear();

  for (int step = itsStartShift; step < itsStepnumber + itsStartShift; step++)
  {
    theDataModifier->Calculate(itsData->PeekTimeValue(-step));
  }
}

// ======================================================================

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiRelativeTimeIntegrationIterator2::~NFmiRelativeTimeIntegrationIterator2() = default;
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theData Undocumented
 * \param theStepnumber Undocumented
 * \param theStartShift Undocumented
 */
// ----------------------------------------------------------------------

NFmiRelativeTimeIntegrationIterator2::NFmiRelativeTimeIntegrationIterator2(NFmiQueryInfo* theData,
                                                                           int theStepnumber,
                                                                           int theStartShift)
    : NFmiDataIterator(theData), itsStartShift(theStartShift), itsStepnumber(theStepnumber)
{
}

// ----------------------------------------------------------------------
/*!
 * \param theDataModifier Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRelativeTimeIntegrationIterator2::DoForEach(NFmiDataModifier* theDataModifier)
{
  if (!theDataModifier) return;

  theDataModifier->Clear();

  for (int i = itsStartShift; i < itsStepnumber + itsStartShift; i++)
  {
    theDataModifier->Calculate(itsData->PeekTimeValue(i));
  }
}
