// ======================================================================
/*!
 * \file NFmiMaskedDataIterator.cpp
 * \brief Implementation of class NFmiMaskedDataIterator
 */
// ======================================================================
/*!
 * \class NFmiMaskedDataIterator
 *
 * Undocumented
 *
 * \todo Hide member variables
 */
// ======================================================================

#include "NFmiMaskedDataIterator.h"
#include "NFmiDataModifier.h"
#include "NFmiSuperSmartInfo.h"
#include "NFmiCombinedParam.h"
#include "NFmiWeatherAndCloudiness.h"
#include "NFmiIntegrationSelector.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiMaskedDataIterator::~NFmiMaskedDataIterator(void) {}
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theData Undocumented
 * \param isCombinedParam Undocumented
 * \param theIntegrationSelector Undocumented
 */
// ----------------------------------------------------------------------

NFmiMaskedDataIterator::NFmiMaskedDataIterator(NFmiSuperSmartInfo *theData,
                                               bool isCombinedParam,
                                               NFmiIntegrationSelector *theIntegrationSelector)
    : NFmiDataIterator(theData),
      fIsCombinedParam(isCombinedParam),
      itsIntegrationSelector(theIntegrationSelector)
{
}

// ----------------------------------------------------------------------
/*!
 * \param theDataModifier Undocumented
 */
// ----------------------------------------------------------------------

void NFmiMaskedDataIterator::DoForEach(NFmiDataModifier *theDataModifier)
{
  if (!theDataModifier) return;

  NFmiSuperSmartInfo *superDataInfo = static_cast<NFmiSuperSmartInfo *>(itsData);
  unsigned long oldLocationIndex = superDataInfo->LocationIndex();
  theDataModifier->Clear();
  superDataInfo->ResetLocation();
  while (superDataInfo->NextLocation())
  {
    if (fIsCombinedParam)
    {
      NFmiWeatherAndCloudiness *combParam =
          static_cast<NFmiWeatherAndCloudiness *>(superDataInfo->CombinedValue());
      if (combParam)
        theDataModifier->Calculate(
            NFmiWeatherAndCloudiness(combParam->IntegratedLongValue(itsIntegrationSelector),
                                     kFmiPackedWeather,
                                     kFloatMissing,
                                     combParam->InfoVersion())
                .TransformedFloatValue());
    }
    else
      theDataModifier->Calculate(superDataInfo->FloatValue());
  }
  superDataInfo->LocationIndex(oldLocationIndex);
}

// ======================================================================
