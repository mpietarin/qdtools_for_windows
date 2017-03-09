// ======================================================================
/*!
 * \file NFmiRegressionItem.cpp
 * \brief Implementation of class NFmiRegressionItem
 */
// ======================================================================
/*!
 * \class NFmiRegressionItem
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiRegressionItem.h"
#include "NFmiSuperSmartInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiRegressionItem::~NFmiRegressionItem() {}
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theCoefficient Undocumented
 * \param theDataIdent Undocumented
 * \param theLevel Undocumented
 * \param theData Undocumented
 */
// ----------------------------------------------------------------------

NFmiRegressionItem::NFmiRegressionItem(double theCoefficient,
                                       NFmiDataIdent *theDataIdent,
                                       NFmiLevel *theLevel,
                                       NFmiQueryInfo *theData)
    : NFmiInfoModifier(theDataIdent, theLevel, theData),
      itsReturnValue(),
      itsCoefficient(theCoefficient),
      itsConstant()
{
}

// ----------------------------------------------------------------------
/*!
 * \param theData Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRegressionItem::Calculate(NFmiQueryInfo *theData)
{
  if (itsParam)
  {
    if (theData)
    {
      NFmiDataIdent tmpIdent(theData->Param());  // hidasta koodia, pitäisi olla PeekValue!!!!!!!!
      theData->Param(*itsParam);
      itsReturnValue = itsConstant + itsCoefficient * theData->FloatValue();
      theData->Param(tmpIdent);
    }
    itsReturnValue = kFloatMissing;
  }
  itsReturnValue = itsConstant;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiRegressionItem::CalculationResult(void) { return static_cast<float>(itsReturnValue); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

double NFmiRegressionItem::FloatValue(void)
{
  if (itsParam)
  {
    if (itsData)
    {
      NFmiParam tmpParam(
          *itsData->Param().GetParam());  // hidasta koodia, pitäisi olla PeekValue!!!!!!!!
      itsData->Param(*itsParam->GetParam());
      itsReturnValue = itsCoefficient * itsData->FloatValue();
      itsData->Param(tmpParam);

      return itsReturnValue;
    }
    return kFloatMissing;
  }
  return itsCoefficient;
}

// ======================================================================
