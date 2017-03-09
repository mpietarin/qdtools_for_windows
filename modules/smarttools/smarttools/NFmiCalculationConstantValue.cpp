//**********************************************************
// C++ Class Name : NFmiCalculationConstantValue
// ---------------------------------------------------
//  Author         : pietarin
//  Creation Date  : 9.11. 2010
//
//**********************************************************

#include "NFmiCalculationConstantValue.h"
#include <newbase/NFmiDataModifier.h>
#include <newbase/NFmiDataIterator.h>
#include <newbase/NFmiFastQueryInfo.h>

// ****************************************************************************
// *************** NFmiCalculationConstantValue *******************************
// ****************************************************************************

//--------------------------------------------------------
// Constructor/Destructor
//--------------------------------------------------------
NFmiCalculationConstantValue::NFmiCalculationConstantValue(double value) : itsValue(value)
{
}
NFmiCalculationConstantValue::~NFmiCalculationConstantValue()
{
}
NFmiCalculationConstantValue::NFmiCalculationConstantValue(
    const NFmiCalculationConstantValue &theOther)
    : NFmiAreaMaskImpl(theOther), itsValue(theOther.itsValue)
{
}

NFmiAreaMask *NFmiCalculationConstantValue::Clone(void) const
{
  return new NFmiCalculationConstantValue(*this);
}

//--------------------------------------------------------
// Value
//--------------------------------------------------------
double NFmiCalculationConstantValue::Value(const NFmiCalculationParams & /* theCalculationParams */,
                                           bool /* fUseTimeInterpolationAlways */)
{
  return itsValue;
}

// ****************************************************************************
// *************** NFmiCalculationConstantValue *******************************
// ****************************************************************************

// ****************************************************************************
// *************** NFmiCalculationSpecialCase *********************************
// ****************************************************************************

NFmiCalculationSpecialCase::NFmiCalculationSpecialCase(NFmiAreaMask::CalculationOperator theValue)
    : NFmiAreaMaskImpl()
{
  SetCalculationOperator(theValue);
}

NFmiCalculationSpecialCase::NFmiCalculationSpecialCase(const NFmiCalculationSpecialCase &theOther)
    : NFmiAreaMaskImpl(theOther)
{
}

NFmiAreaMask *NFmiCalculationSpecialCase::Clone(void) const
{
  return new NFmiCalculationSpecialCase(*this);
}
// ****************************************************************************
// *************** NFmiCalculationSpecialCase *********************************
// ****************************************************************************

// ****************************************************************************
// *************** NFmiCalculationRampFuction *********************************
// ****************************************************************************

NFmiCalculationRampFuction::NFmiCalculationRampFuction(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo, thePostBinaryOperator)
{
}

NFmiCalculationRampFuction::~NFmiCalculationRampFuction(void)
{
}
NFmiCalculationRampFuction::NFmiCalculationRampFuction(const NFmiCalculationRampFuction &theOther)
    : NFmiInfoAreaMask(theOther)
{
}

NFmiAreaMask *NFmiCalculationRampFuction::Clone(void) const
{
  return new NFmiCalculationRampFuction(*this);
}

double NFmiCalculationRampFuction::Value(const NFmiCalculationParams &theCalculationParams,
                                         bool fUseTimeInterpolationAlways)
{
  double value = NFmiInfoAreaMask::Value(theCalculationParams, fUseTimeInterpolationAlways);
  return itsMaskCondition.MaskValue(value);
}
// ****************************************************************************
// *************** NFmiCalculationRampFuction *********************************
// ****************************************************************************

// ****************************************************************************
// ************ NFmiCalculationIntegrationFuction *****************************
// ****************************************************************************

NFmiCalculationIntegrationFuction::NFmiCalculationIntegrationFuction(
    boost::shared_ptr<NFmiDataIterator> &theDataIterator,
    boost::shared_ptr<NFmiDataModifier> &theDataModifier,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
    : NFmiInfoAreaMask(
          NFmiCalculationCondition(), theMaskType, theDataType, theInfo, NFmiAreaMask::kNoValue),
      itsDataModifier(theDataModifier),
      itsDataIterator(theDataIterator)
{
}

NFmiCalculationIntegrationFuction::~NFmiCalculationIntegrationFuction(void)
{
}
double NFmiCalculationIntegrationFuction::Value(const NFmiCalculationParams &theCalculationParams,
                                                bool /* fUseTimeInterpolationAlways */)
{
  // HUOM!!! Tähän tuli pikaviritys:
  // asetan vain lähimmän pisteen ja ajan kohdalleen.
  if (itsInfo->NearestPoint(theCalculationParams.itsLatlon) &&
      itsInfo->TimeToNearestStep(theCalculationParams.itsTime, kForward))
  {
    itsDataIterator->DoForEach(itsDataModifier.get());
    return itsDataModifier->CalculationResult();
  }
  return kFloatMissing;
}
// ****************************************************************************
// ************ NFmiCalculationIntegrationFuction *****************************
// ****************************************************************************

// ****************************************************************************
// ********** NFmiCalculationRampFuctionWithAreaMask **************************
// ****************************************************************************

// Ramppifunktioiden laskut AreaMask:ien avulla (mm. lat, lon ja elevationangle tapaukset).
double NFmiCalculationRampFuctionWithAreaMask::Value(
    const NFmiCalculationParams &theCalculationParams, bool fUseTimeInterpolationAlways)
{
  itsAreaMask->Time(theCalculationParams.itsTime);
  double value = itsAreaMask->Value(theCalculationParams, fUseTimeInterpolationAlways);
  return itsMaskCondition.MaskValue(value);
}

NFmiCalculationRampFuctionWithAreaMask::NFmiCalculationRampFuctionWithAreaMask(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    boost::shared_ptr<NFmiAreaMask> &theAreaMask,
    BinaryOperator thePostBinaryOperator)
    : NFmiAreaMaskImpl(theOperation, theMaskType, theDataType, thePostBinaryOperator),
      itsAreaMask(theAreaMask),
      fIsTimeIntepolationNeededInValue(false)
{
}

NFmiCalculationRampFuctionWithAreaMask::~NFmiCalculationRampFuctionWithAreaMask(void)
{
}
NFmiCalculationRampFuctionWithAreaMask::NFmiCalculationRampFuctionWithAreaMask(
    const NFmiCalculationRampFuctionWithAreaMask &theOther)
    : NFmiAreaMaskImpl(theOther),
      itsAreaMask(theOther.itsAreaMask ? theOther.itsAreaMask->Clone() : 0),
      fIsTimeIntepolationNeededInValue(theOther.fIsTimeIntepolationNeededInValue)
{
}

NFmiAreaMask *NFmiCalculationRampFuctionWithAreaMask::Clone(void) const
{
  return new NFmiCalculationRampFuctionWithAreaMask(*this);
}
// ****************************************************************************
// ********** NFmiCalculationRampFuctionWithAreaMask **************************
// ****************************************************************************

// ****************************************************************************
// **************** NFmiCalculationDeltaZValue ********************************
// ****************************************************************************
double NFmiCalculationDeltaZValue::itsHeightValue;

NFmiCalculationDeltaZValue::NFmiCalculationDeltaZValue(void) : NFmiAreaMaskImpl()
{
}
NFmiCalculationDeltaZValue::NFmiCalculationDeltaZValue(const NFmiCalculationDeltaZValue &theOther)
    : NFmiAreaMaskImpl(theOther)
{
}

NFmiAreaMask *NFmiCalculationDeltaZValue::Clone(void) const
{
  return new NFmiCalculationDeltaZValue(*this);
}
// ****************************************************************************
// **************** NFmiCalculationDeltaZValue ********************************
// ****************************************************************************

// *********************************************************************
// *************** NFmiPeekTimeMask ************************************
// *********************************************************************

NFmiPeekTimeMask::NFmiPeekTimeMask(Type theMaskType,
                                   NFmiInfoData::Type theDataType,
                                   boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                   int theArgumentCount)
    : NFmiInfoAreaMask(
          NFmiCalculationCondition(), theMaskType, theDataType, theInfo, NFmiAreaMask::kNoValue),
      itsTimeOffsetInMinutes(0)
{
  itsFunctionArgumentCount = theArgumentCount;
}

NFmiPeekTimeMask::~NFmiPeekTimeMask(void)
{
}
NFmiPeekTimeMask::NFmiPeekTimeMask(const NFmiPeekTimeMask &theOther)
    : NFmiInfoAreaMask(theOther), itsTimeOffsetInMinutes(theOther.itsTimeOffsetInMinutes)
{
}

NFmiAreaMask *NFmiPeekTimeMask::Clone(void) const
{
  return new NFmiPeekTimeMask(*this);
}
double NFmiPeekTimeMask::Value(const NFmiCalculationParams &theCalculationParams,
                               bool /* fUseTimeInterpolationAlways */)
{
  NFmiMetTime peekTime(theCalculationParams.itsTime);
  peekTime.ChangeByMinutes(itsTimeOffsetInMinutes);
  return itsInfo->InterpolatedValue(theCalculationParams.itsLatlon, peekTime);
}

void NFmiPeekTimeMask::SetArguments(std::vector<float> &theArgumentVector)
{
  // jokaiselle pisteelle ja ajanhetkelle annetaan eri argumentit tässä
  if (static_cast<long>(theArgumentVector.size()) == itsFunctionArgumentCount - 1)
  {
    itsTimeOffsetInMinutes = static_cast<long>(std::round(theArgumentVector[0] * 60));
  }
  else
    throw std::runtime_error(
        "Internal SmartMet error: PeekTime function was given invalid number of arguments, "
        "cannot calculate the macro.");
}

// *********************************************************************
// *************** NFmiPeekTimeMask ************************************
// *********************************************************************
