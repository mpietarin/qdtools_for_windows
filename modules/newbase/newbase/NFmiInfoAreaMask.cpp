// ======================================================================
/*!
 * \file NFmiInfoAreaMask.cpp
 * \brief Implementation of class NFmiInfoAreaMask
 */
// ======================================================================
/*!
 * \class NFmiInfoAreaMask
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiInfoAreaMask.h"
#include "NFmiFastQueryInfo.h"
#include "NFmiArea.h"
#include "NFmiMetMath.h"
#include "NFmiQueryDataUtil.h"
#include "NFmiDataModifierClasses.h"

#include <cassert>

// HUOM!!! Jostain syystä kun käytin täällä boost::math::iround -funktiota, se ei mennyt parissa
// VC++ 2013 projektissa linkkerin läpi (Unknown variable blaa blaa). Esim. SmartMet projekti meni
// läpi ilman ongelmia, mutta qdmisc -solutionin qdtempprob- ja modifyLevelData- projektit eivät
// menneet.
// Kyseiset projektit menivät läpi vielä aiemmin, mutta kun lisäsin boostin iround -kutsut, ei enää.
// SIKSI joudun käyttämään FmiRound -funktioita.

static bool IsFindFunction(NFmiAreaMask::FunctionType theFunction)
{
  if (theFunction == NFmiAreaMask::FindH || theFunction == NFmiAreaMask::FindC)
    return true;
  else
    return false;
}

static bool IsFindConditionalFunction(NFmiAreaMask::FunctionType theFunction)
{
  if (theFunction >= NFmiAreaMask::ProbOver && theFunction <= NFmiAreaMask::ProbBetweenEq)
    return true;
  else
    return false;
}

static bool CheckProbabilityCondition(NFmiAreaMask::FunctionType condition,
                                      double value,
                                      double limit1,
                                      double limit2)
{
  switch (condition)
  {
    case NFmiAreaMask::ProbOver:
      return value > limit1;
    case NFmiAreaMask::ProbOverEq:
      return value >= limit1;
    case NFmiAreaMask::ProbUnder:
      return value < limit1;
    case NFmiAreaMask::ProbUnderEq:
      return value <= limit1;
    case NFmiAreaMask::ProbEqual:
      return value == limit1;
    case NFmiAreaMask::ProbNotEqual:
      return value != limit1;
    case NFmiAreaMask::ProbBetween:
      return (value > limit1) && (value < limit2);
    case NFmiAreaMask::ProbBetweenEq:
      return (value >= limit1) && (value <= limit2);

    default:
      throw std::runtime_error("Internal error in SmartTool system, probability condition unknown");
  }
}

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiInfoAreaMask::~NFmiInfoAreaMask(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiInfoAreaMask::NFmiInfoAreaMask(void)
    : NFmiAreaMaskImpl(),
      itsInfo(),
      itsDataIdent(),
      itsLevel(),
      fIsTimeIntepolationNeededInValue(false),
      fUsePressureLevelInterpolation(false),
      itsUsedPressureLevelValue(kFloatMissing)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theOperation Undocumented
 * \param theMaskType Undocumented
 * \param theDataType Undocumented
 * \param theInfo Undocumented
 * \param ownsInfo Undocumented
 * \param thePostBinaryOperator Undocumented
 * \param destroySmartInfoData Undocumented
 */
// ----------------------------------------------------------------------

NFmiInfoAreaMask::NFmiInfoAreaMask(const NFmiCalculationCondition &theOperation,
                                   Type theMaskType,
                                   NFmiInfoData::Type theDataType,
                                   const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                   BinaryOperator thePostBinaryOperator)
    : NFmiAreaMaskImpl(theOperation, theMaskType, theDataType, thePostBinaryOperator),
      itsInfo(theInfo),
      itsDataIdent(theInfo ? theInfo->Param() : NFmiDataIdent()),
      itsLevel(NFmiLevel()),
      fIsTimeIntepolationNeededInValue(false),
      fUsePressureLevelInterpolation(false),
      itsUsedPressureLevelValue(kFloatMissing)
{
  if (theInfo && theInfo->Level()) itsLevel = *theInfo->Level();
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theInfo Undocumented
 * \param ownsInfo Undocumented
 * \param thePostBinaryOperator Undocumented
 * \param destroySmartInfoData Undocumented
 */
// ----------------------------------------------------------------------

NFmiInfoAreaMask::NFmiInfoAreaMask(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                   BinaryOperator thePostBinaryOperator)
    : NFmiAreaMaskImpl(
          NFmiCalculationCondition(), kInfo, NFmiInfoData::kEditable, thePostBinaryOperator),
      itsInfo(theInfo),
      itsDataIdent(theInfo ? theInfo->Param() : NFmiDataIdent()),
      itsLevel(NFmiLevel()),
      fIsTimeIntepolationNeededInValue(false),
      fUsePressureLevelInterpolation(false),
      itsUsedPressureLevelValue(kFloatMissing)
{
  if (theInfo && theInfo->Level()) itsLevel = *theInfo->Level();
}

NFmiInfoAreaMask::NFmiInfoAreaMask(const NFmiInfoAreaMask &theOther)
    : NFmiAreaMaskImpl(theOther),
      itsInfo(NFmiAreaMask::DoShallowCopy(theOther.itsInfo)),
      itsDataIdent(theOther.itsDataIdent),
      itsLevel(theOther.itsLevel),
      fIsTimeIntepolationNeededInValue(theOther.fIsTimeIntepolationNeededInValue),
      fUsePressureLevelInterpolation(theOther.fUsePressureLevelInterpolation),
      itsUsedPressureLevelValue(theOther.itsUsedPressureLevelValue)
{
}

NFmiAreaMask *NFmiInfoAreaMask::Clone(void) const { return new NFmiInfoAreaMask(*this); }
// ----------------------------------------------------------------------
/*!
 * \param theTime Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiInfoAreaMask::Time(const NFmiMetTime &theTime)
{
  if (itsDataType == NFmiInfoData::kStationary) return true;
  assert(itsInfo);
  if (itsInfo)
  {
    itsTime = theTime;
    bool status = (true == itsInfo->Time(theTime));
    fIsTimeIntepolationNeededInValue = !status;  // jos tämän jälkeen käytetään samaa aikaa
                                                 // Value-metodissa, ei aikainterpolointia tarvitse
                                                 // tehdä, jos aika löytyi.
    return status;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theParam Undocumented
 * \param theLevel Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiInfoAreaMask::IsWantedParam(const NFmiDataIdent & /* theParam */,
                                     const NFmiLevel * /* theLevel */) const
{
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiString NFmiInfoAreaMask::MakeSubMaskString(void) const
{
  return itsDataIdent.GetParamName();
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiDataIdent *NFmiInfoAreaMask::DataIdent(void) const { return &itsDataIdent; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiParam *NFmiInfoAreaMask::Param(void) const { return itsDataIdent.GetParam(); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiLevel *NFmiInfoAreaMask::Level(void) const { return &itsLevel; }
void NFmiInfoAreaMask::Level(const NFmiLevel &theLevel) { itsLevel = theLevel; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiInfoAreaMask::UseLevelInfo(void) const { return false; }
// ----------------------------------------------------------------------
/*!
 * \param theLatLon Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

double NFmiInfoAreaMask::CalcValueFromLocation(const NFmiPoint &theLatLon) const
{
  if (fIsTimeIntepolationNeededInValue)
    return itsInfo->InterpolatedValue(theLatLon, itsTime, 360);
  else
  {
    double value = itsInfo->InterpolatedValue(theLatLon);
    if (value == kFloatMissing && itsInfo->DataType() == NFmiInfoData::kScriptVariableData)
      value = itsInfo->FloatValue();
    return value;
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theLatlon Undocumented
 * \param theTime Undocumented
 * \param theTimeIndex Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

// tätä kaytetaan smarttool-modifierin yhteydessä

double NFmiInfoAreaMask::Value(const NFmiCalculationParams &theCalculationParams,
                               bool fUseTimeInterpolationAlways)
{
  double result = kFloatMissing;
  if (UsePressureLevelInterpolation())
  {
    if (Level()->LevelType() == kFmiFlightLevel)
    {
      double P = ::CalcFlightLevelPressure(UsedPressureLevelValue() * 100);
      result = PressureValueStatic(
          P, theCalculationParams);  // Tässä ei saa kutsua virtuaalista PressureValue -metodia!!
    }
    else if (Level()->LevelType() == kFmiHeight)
    {
      result = HeightValueStatic(
          UsedPressureLevelValue(),
          theCalculationParams);  // Tässä ei saa kutsua virtuaalista PressureValue -metodia!!
    }
    else
      result = PressureValueStatic(
          UsedPressureLevelValue(),
          theCalculationParams);  // Tässä ei saa kutsua virtuaalista PressureValue -metodia!!
  }
  else
  {
    if (fUseTimeInterpolationAlways || fIsTimeIntepolationNeededInValue)
      result = itsInfo->InterpolatedValue(theCalculationParams.itsLatlon,
                                          theCalculationParams.itsTime,
                                          360);  // interpoloidaan ajassa ja paikassa
    else
      result = CalcValueFromLocation(
          theCalculationParams.itsLatlon);  // ollaan jo oikeassa ajassa, ei aikainterpolointia
  }
  return result;
}

double NFmiInfoAreaMask::HeightValue(double theHeight,
                                     const NFmiCalculationParams &theCalculationParams)
{
  return itsInfo->HeightValue(
      static_cast<float>(theHeight), theCalculationParams.itsLatlon, theCalculationParams.itsTime);
}

double NFmiInfoAreaMask::HeightValueStatic(double theHeight,
                                           const NFmiCalculationParams &theCalculationParams)
{
  return itsInfo->HeightValue(
      static_cast<float>(theHeight), theCalculationParams.itsLatlon, theCalculationParams.itsTime);
}

double NFmiInfoAreaMask::PressureValue(double thePressure,
                                       const NFmiCalculationParams &theCalculationParams)
{
  return itsInfo->PressureLevelValue(static_cast<float>(thePressure),
                                     theCalculationParams.itsLatlon,
                                     theCalculationParams.itsTime);
}

double NFmiInfoAreaMask::PressureValueStatic(double thePressure,
                                             const NFmiCalculationParams &theCalculationParams)
{
  return itsInfo->PressureLevelValue(static_cast<float>(thePressure),
                                     theCalculationParams.itsLatlon,
                                     theCalculationParams.itsTime);
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

// erikoistapaus optimoituun käyttöön (ei voi käyttää kaikille luokille!!!!)
// sopii mm. infoareamask ja binaryareamask-luokille

bool NFmiInfoAreaMask::IsMasked(int theIndex) const
{
  if (!fEnabled)
    return true;  // jos maski ei ole käytössä, on maski aina 'päällä'
  else
  {
    double testValue = itsInfo->GetFloatValue(itsInfo->Index(
        itsInfo->ParamIndex(), theIndex, itsInfo->LevelIndex(), itsInfo->TimeIndex()));
    return itsMaskCondition.IsMasked(testValue);
  }
}

// ----------------------------------------------------------------------
/*!
 * \param newInfo Undocumented
 * \param ownsInfo Undocumented
 * \param destroySmartInfoData Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------
void NFmiInfoAreaMask::Info(const boost::shared_ptr<NFmiFastQueryInfo> &newInfo)
{
  itsInfo = newInfo;
  itsDataIdent = itsInfo ? itsInfo->Param() : NFmiDataIdent();
  itsLevel = itsInfo ? *itsInfo->Level() : NFmiLevel();
}

// Tänne pitää antaa halutun infon matala kopio ja se laitetaan itsInfo:on.
// Sitten siihen laitetaan maskissa olevat parametri ja leveli kohdalleen.
void NFmiInfoAreaMask::UpdateInfo(boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  itsInfo = theInfo;
  if (itsInfo)
  {
    itsInfo->Param(static_cast<FmiParameterName>(itsDataIdent.GetParamIdent()));
    itsInfo->Level(itsLevel);
  }
}

// ======================================================================

NFmiInfoAreaMaskPeekXY::~NFmiInfoAreaMaskPeekXY(void) {}
NFmiInfoAreaMaskPeekXY::NFmiInfoAreaMaskPeekXY(void)
    : NFmiInfoAreaMask(),
      itsXOffset(0),
      itsYOffset(0),
      itsGridXDiff(1)  // dummy arvo, en laita 0, että ei tule joskus 0:lla jakoa vahingossa
      ,
      itsGridYDiff(1)  // dummy arvo, en laita 0, että ei tule joskus 0:lla jakoa vahingossa
{
}

NFmiInfoAreaMaskPeekXY::NFmiInfoAreaMaskPeekXY(const NFmiCalculationCondition &theOperation,
                                               Type theMaskType,
                                               NFmiInfoData::Type theDataType,
                                               const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                               int theXOffset,
                                               int theYOffset,
                                               BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo, thePostBinaryOperator),
      itsXOffset(theXOffset),
      itsYOffset(theYOffset),
      itsGridXDiff(1)  // dummy arvo, en laita 0, että ei tule joskus 0:lla jakoa vahingossa
      ,
      itsGridYDiff(1)  // dummy arvo, en laita 0, että ei tule joskus 0:lla jakoa vahingossa
{
  CalcGridDiffs();
}

NFmiInfoAreaMaskPeekXY::NFmiInfoAreaMaskPeekXY(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                               int theXOffset,
                                               int theYOffset,
                                               BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theInfo, thePostBinaryOperator),
      itsXOffset(theXOffset),
      itsYOffset(theYOffset),
      itsGridXDiff(1)  // dummy arvo, en laita 0, että ei tule joskus 0:lla jakoa vahingossa
      ,
      itsGridYDiff(1)  // dummy arvo, en laita 0, että ei tule joskus 0:lla jakoa vahingossa
{
  CalcGridDiffs();
}

NFmiInfoAreaMaskPeekXY::NFmiInfoAreaMaskPeekXY(const NFmiInfoAreaMaskPeekXY &theOther)
    : NFmiInfoAreaMask(theOther),
      itsXOffset(theOther.itsXOffset),
      itsYOffset(theOther.itsYOffset),
      itsGridXDiff(theOther.itsGridXDiff),
      itsGridYDiff(theOther.itsGridYDiff)
{
}

NFmiAreaMask *NFmiInfoAreaMaskPeekXY::Clone(void) const
{
  return new NFmiInfoAreaMaskPeekXY(*this);
}

void NFmiInfoAreaMaskPeekXY::CalcGridDiffs(void)
{
  // oletetaan, että on hila dataa ja area löytyy
  if (itsInfo->Grid() == NULL)
    throw std::runtime_error(
        "Error in NFmiInfoAreaMaskPeekXY::CalcGridDiffs, given data was not grid form");
  itsGridXDiff = itsInfo->Area()->Width() / (itsInfo->Grid()->XNumber() - 1);
  itsGridYDiff = itsInfo->Area()->Height() / (itsInfo->Grid()->YNumber() - 1);
}

NFmiCalculationParams NFmiInfoAreaMaskPeekXY::MakeModifiedCalculationParams(
    const NFmiCalculationParams &theCalculationParams)
{
  NFmiPoint xyPoint(itsInfo->Area()->ToXY(theCalculationParams.itsLatlon));
  xyPoint.X(xyPoint.X() + itsGridXDiff * itsXOffset);
  xyPoint.Y(xyPoint.Y() - itsGridYDiff * itsYOffset);  // huom! '-'-merkki, koska arean y-akseli on
  // käänteinen kuin tämä peek-maailma, jossa y
  // kasvaa ylöspäin
  NFmiPoint wantedLatlon(itsInfo->Area()->ToLatLon(xyPoint));
  NFmiCalculationParams modifiedCalculationParams(theCalculationParams);
  modifiedCalculationParams.itsLatlon = wantedLatlon;
  return modifiedCalculationParams;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
// huom! ei tehdää pelkkää peek juttua vaan 'interpoloitu' peek, koska pelkällä peekellä tulee
// rumaa (leegoja) jälkeä.
double NFmiInfoAreaMaskPeekXY::Value(const NFmiCalculationParams &theCalculationParams,
                                     bool fUseTimeInterpolationAlways)
{
  return NFmiInfoAreaMask::Value(MakeModifiedCalculationParams(theCalculationParams),
                                 fUseTimeInterpolationAlways);
}

double NFmiInfoAreaMaskPeekXY::PressureValue(double thePressure,
                                             const NFmiCalculationParams &theCalculationParams)
{
  return NFmiInfoAreaMask::PressureValue(thePressure,
                                         MakeModifiedCalculationParams(theCalculationParams));
}

// **********************************************************
// *****    NFmiInfoAreaMaskPeekXY2    **********************
// **********************************************************

NFmiInfoAreaMaskPeekXY2::~NFmiInfoAreaMaskPeekXY2(void) {}
NFmiInfoAreaMaskPeekXY2::NFmiInfoAreaMaskPeekXY2(void)
    : NFmiInfoAreaMask(), itsXOffset(0), itsYOffset(0), itsEditedInfo()
{
}

NFmiInfoAreaMaskPeekXY2::NFmiInfoAreaMaskPeekXY2(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const boost::shared_ptr<NFmiFastQueryInfo> &theEditedInfo,
    int theXOffset,
    int theYOffset,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo, thePostBinaryOperator),
      itsXOffset(theXOffset),
      itsYOffset(theYOffset),
      itsEditedInfo(theEditedInfo)
{
}

NFmiInfoAreaMaskPeekXY2::NFmiInfoAreaMaskPeekXY2(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const boost::shared_ptr<NFmiFastQueryInfo> &theEditedInfo,
    int theXOffset,
    int theYOffset,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theInfo, thePostBinaryOperator),
      itsXOffset(theXOffset),
      itsYOffset(theYOffset),
      itsEditedInfo(theEditedInfo)
{
}

NFmiInfoAreaMaskPeekXY2::NFmiInfoAreaMaskPeekXY2(const NFmiInfoAreaMaskPeekXY2 &theOther)
    : NFmiInfoAreaMask(theOther),
      itsXOffset(theOther.itsXOffset),
      itsYOffset(theOther.itsYOffset),
      itsEditedInfo(NFmiAreaMask::DoShallowCopy(theOther.itsEditedInfo))
{
}

NFmiAreaMask *NFmiInfoAreaMaskPeekXY2::Clone(void) const
{
  return new NFmiInfoAreaMaskPeekXY2(*this);
}

NFmiCalculationParams NFmiInfoAreaMaskPeekXY2::MakeModifiedCalculationParams(
    const NFmiCalculationParams &theCalculationParams)
{
  NFmiPoint gridPoint(itsEditedInfo->Grid()->LatLonToGrid(theCalculationParams.itsLatlon));
  gridPoint.X(gridPoint.X() + itsXOffset);
  gridPoint.Y(gridPoint.Y() + itsYOffset);
  NFmiPoint wantedLatlon(itsEditedInfo->Grid()->GridToLatLon(gridPoint));
  NFmiCalculationParams modifiedCalculationParams(theCalculationParams);
  modifiedCalculationParams.itsLatlon = wantedLatlon;
  return modifiedCalculationParams;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskPeekXY2::Value(const NFmiCalculationParams &theCalculationParams,
                                      bool fUseTimeInterpolationAlways)
{
  return NFmiInfoAreaMask::Value(MakeModifiedCalculationParams(theCalculationParams),
                                 fUseTimeInterpolationAlways);
}

double NFmiInfoAreaMaskPeekXY2::PressureValue(double thePressure,
                                              const NFmiCalculationParams &theCalculationParams)
{
  return NFmiInfoAreaMask::PressureValue(thePressure,
                                         MakeModifiedCalculationParams(theCalculationParams));
}

// **********************************************************
// *****    NFmiInfoAreaMaskPeekXY2    **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskPeekXY3    **********************
// **********************************************************

NFmiInfoAreaMaskPeekXY3::~NFmiInfoAreaMaskPeekXY3(void) {}
NFmiInfoAreaMaskPeekXY3::NFmiInfoAreaMaskPeekXY3(void)
    : NFmiInfoAreaMask(), itsXOffsetInKM(0), itsYOffsetInKM(0), itsEditedInfo()
{
}

NFmiInfoAreaMaskPeekXY3::NFmiInfoAreaMaskPeekXY3(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const boost::shared_ptr<NFmiFastQueryInfo> &theEditedInfo,
    double theXOffsetInKM,
    double theYOffsetInKM,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo, thePostBinaryOperator),
      itsXOffsetInKM(theXOffsetInKM),
      itsYOffsetInKM(theYOffsetInKM),
      itsEditedInfo(theEditedInfo)
{
}

NFmiInfoAreaMaskPeekXY3::NFmiInfoAreaMaskPeekXY3(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const boost::shared_ptr<NFmiFastQueryInfo> &theEditedInfo,
    double theXOffsetInKM,
    double theYOffsetInKM,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theInfo, thePostBinaryOperator),
      itsXOffsetInKM(theXOffsetInKM),
      itsYOffsetInKM(theYOffsetInKM),
      itsEditedInfo(theEditedInfo)
{
}

NFmiInfoAreaMaskPeekXY3::NFmiInfoAreaMaskPeekXY3(const NFmiInfoAreaMaskPeekXY3 &theOther)
    : NFmiInfoAreaMask(theOther),
      itsXOffsetInKM(theOther.itsXOffsetInKM),
      itsYOffsetInKM(theOther.itsYOffsetInKM),
      itsEditedInfo(NFmiAreaMask::DoShallowCopy(theOther.itsEditedInfo))
{
}

NFmiAreaMask *NFmiInfoAreaMaskPeekXY3::Clone(void) const
{
  return new NFmiInfoAreaMaskPeekXY3(*this);
}

static bool IsPacificViewData(boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  if (theInfo)
  {
    if (theInfo->Grid())  // trajektori datojen pitäisi olla hiladatoja
      return theInfo->Grid()->Area()->PacificView();
  }

  return false;
}

NFmiCalculationParams NFmiInfoAreaMaskPeekXY3::MakeModifiedCalculationParams(
    const NFmiCalculationParams &theCalculationParams)
{
  bool usePacificView = ::IsPacificViewData(itsInfo);
  NFmiLocation loc(theCalculationParams.itsLatlon);
  // x-suunnassa siirto ei mielestäni toimi oikein vaan piti laittaa positiiviselle ja
  // negatiiviselle tapauksille omat haarat
  if (itsXOffsetInKM > 0) loc.SetLocation(90., itsXOffsetInKM * 1000., usePacificView);
  if (itsXOffsetInKM < 0) loc.SetLocation(270., itsXOffsetInKM * 1000., usePacificView);
  // y-suunnassa offsetin merkkisyys osaa siirtää pistettä oikein
  if (itsYOffsetInKM != 0) loc.SetLocation(360., itsYOffsetInKM * 1000., usePacificView);

  NFmiCalculationParams modifiedCalculationParams(theCalculationParams);
  modifiedCalculationParams.itsLatlon = loc.GetLocation();
  return modifiedCalculationParams;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskPeekXY3::Value(const NFmiCalculationParams &theCalculationParams,
                                      bool fUseTimeInterpolationAlways)
{
  return NFmiInfoAreaMask::Value(MakeModifiedCalculationParams(theCalculationParams),
                                 fUseTimeInterpolationAlways);
}

double NFmiInfoAreaMaskPeekXY3::PressureValue(double thePressure,
                                              const NFmiCalculationParams &theCalculationParams)
{
  return NFmiInfoAreaMask::PressureValue(thePressure,
                                         MakeModifiedCalculationParams(theCalculationParams));
}

// **********************************************************
// *****    NFmiInfoAreaMaskPeekXY3    **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskMetFuncBase *********************
// **********************************************************

NFmiInfoAreaMaskMetFuncBase::~NFmiInfoAreaMaskMetFuncBase(void) {}
NFmiInfoAreaMaskMetFuncBase::NFmiInfoAreaMaskMetFuncBase(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    bool thePeekAlongTudes,
    MetFunctionDirection theMetFuncDirection,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo, thePostBinaryOperator),
      itsTimeCache(),
      itsGridSizeX(0),
      itsGridSizeY(0),
      itsGridPointWidthInMeters(0),
      itsGridPointHeightInMeters(0),
      itsMetFuncDividerX(0)  // laitetaan 0-arvot oletuksena, nämä 'dividerit' pitää alustaa sitten
                             // ennen varsinaisia laskuja!!!
      ,
      itsMetFuncDividerY(0),
      fTotalWindParam(false),
      fPeekAlongTudes(thePeekAlongTudes)
{
  // emon (NFmiAreaMaskImpl) dataosan asetetaan poikkeuksellisesti tässä
  this->itsMetFunctionDirection = theMetFuncDirection;
}

NFmiInfoAreaMaskMetFuncBase::NFmiInfoAreaMaskMetFuncBase(
    const NFmiInfoAreaMaskMetFuncBase &theOther)
    : NFmiInfoAreaMask(theOther),
      itsTimeCache(theOther.itsTimeCache),
      itsGridSizeX(theOther.itsGridSizeX),
      itsGridSizeY(theOther.itsGridSizeY),
      itsGridPointWidthInMeters(theOther.itsGridPointWidthInMeters),
      itsGridPointHeightInMeters(theOther.itsGridPointHeightInMeters),
      itsMetFuncDividerX(theOther.itsMetFuncDividerX),
      itsMetFuncDividerY(theOther.itsMetFuncDividerY),
      fTotalWindParam(theOther.fTotalWindParam),
      fPeekAlongTudes(theOther.fPeekAlongTudes)
{
}

void NFmiInfoAreaMaskMetFuncBase::Initialize(void)
{
  SetGridSizeVariables();
  if (itsInfo->Param().GetParamIdent() == kFmiTotalWindMS) fTotalWindParam = true;
}

bool NFmiInfoAreaMaskMetFuncBase::Time(const NFmiMetTime &theTime)
{
  bool status = NFmiInfoAreaMask::Time(theTime);
  itsTimeCache = itsInfo->CalcTimeCache(theTime);
  return status;
}

NFmiLocationCache NFmiInfoAreaMaskMetFuncBase::CalcLocationCache(const NFmiPoint &theLatlon)
{
  return itsInfo->CalcLocationCache(theLatlon, itsGridSizeX, itsGridSizeY);
}

void NFmiInfoAreaMaskMetFuncBase::SetGridSizeVariables(void)
{
  if (IsDataOperatable(itsInfo) == false)
    throw std::runtime_error("Error: given data for Met-function was not grid-data");

  itsGridSizeX = itsInfo->Grid()->XNumber();
  itsGridSizeY = itsInfo->Grid()->YNumber();
  double worldXYRectWidth = itsInfo->Grid()->Area()->WorldXYWidth();
  double worldXYRectHeight = itsInfo->Grid()->Area()->WorldXYHeight();
  itsGridPointWidthInMeters = static_cast<float>(worldXYRectWidth / (itsGridSizeX - 1.));
  itsGridPointHeightInMeters = static_cast<float>(worldXYRectHeight / (itsGridSizeY - 1.));
  // nyt kun hilaväli tiedetään, voidaan kutsua funktio, jolla lasketaan met-funktio jakajat
  SetDividers();
}

bool NFmiInfoAreaMaskMetFuncBase::IsDataOperatable(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo) const
{
  if (theInfo && theInfo->Grid()) return true;
  return false;
}

NFmiLocationCache NFmiInfoAreaMaskMetFuncBase::CalcPeekedLocation(
    const NFmiLocationCache &theLocationCachePoint, int theOffsetX, int theOffsetY)
{
  if (fPeekAlongTudes)
  {  // lasketaan peek-piste leveys- ja pituuspiirejä pitkin
    bool usePacificView = ::IsPacificViewData(itsInfo);
    NFmiLocation loc(itsInfo->Grid()->GridToLatLon(theLocationCachePoint.itsGridPoint));
    // x-suunnassa siirto ei mielestäni toimi oikein vaan piti laittaa positiiviselle ja
    // negatiiviselle tapauksille omat haarat
    if (theOffsetX > 0)
      loc.SetLocation(90., theOffsetX * itsGridPointWidthInMeters, usePacificView);
    if (theOffsetX < 0)
      loc.SetLocation(270., theOffsetX * itsGridPointWidthInMeters, usePacificView);
    // y-suunnassa offsetin merkkisyys osaa siirtää pistettä oikein
    if (theOffsetY != 0)
      loc.SetLocation(360., theOffsetY * itsGridPointWidthInMeters, usePacificView);
    NFmiLocationCache locationCache = CalcLocationCache(loc.GetLocation());
    return locationCache;
  }
  else  // lasketaan peek-piste datan oman hilan suuntaisesti
    return NFmiLocationCache::MakePeekedLocation(
        theLocationCachePoint, theOffsetX, theOffsetY, itsGridSizeX, itsGridSizeY);
}

// Oletus, kaikki offset-pyynnöt on valideja, joten rajoja ei tarkastella
float NFmiInfoAreaMaskMetFuncBase::Peek(const NFmiLocationCache &theLocationCachePoint,
                                        int theOffsetX,
                                        int theOffsetY)
{
  NFmiLocationCache peekPoint = CalcPeekedLocation(theLocationCachePoint, theOffsetX, theOffsetY);
  float value = kFloatMissing;
  if (UsePressureLevelInterpolation())
  {
    if (Level()->LevelType() == kFmiFlightLevel)  // Lasse poisto alkaa
    {
      double P = ::CalcFlightLevelPressure(UsedPressureLevelValue() * 100);
      value = itsInfo->CachedPressureLevelValue(static_cast<float>(P), peekPoint, itsTimeCache);
    }
    else  // Lasse poisto loppuu
      value = itsInfo->CachedPressureLevelValue(
          static_cast<float>(UsedPressureLevelValue()), peekPoint, itsTimeCache);
  }
  else
    value = itsInfo->CachedInterpolation(peekPoint, itsTimeCache);
  return value;
}

float NFmiInfoAreaMaskMetFuncBase::CalcMetFuncWithFactors(
    const NFmiLocationCache &theLocationCachePoint,
    bool fDoX,
    const CalcFactorVector &theCalcFactors)
{
  float gradValue = kFloatMissing;
  float gradValueSum = 0;
  for (size_t i = 0; i < theCalcFactors.size(); i++)
  {
    int offsetX = fDoX ? theCalcFactors[i].first : 0;
    int offsetY = fDoX ? 0 : theCalcFactors[i].first;
    float value = Peek(theLocationCachePoint, offsetX, offsetY);
    if (value == kFloatMissing)
      return kFloatMissing;  // jos yksikin kohde arvo oli puuttuvaa, koko juttu on puuttuvaa
    gradValueSum += theCalcFactors[i].second * value;
  }
  gradValue = gradValueSum / (fDoX ? itsMetFuncDividerX : itsMetFuncDividerY);
  return gradValue;
}

// jos ollaan tarpeeksi lähellä oikeaa tai ylä reunaa, myös ulkopuolella, oletetaan että ollaan
// tasan reunalla
// Kaava on (x-suunnassa): (1 * peek(-2, 0) - 4 * peek(-1, 0) + 3 * peek(0, 0)) / (2*deltaX)
float NFmiInfoAreaMaskMetFuncBase::CalcUpperEdgeMetFunc(
    const NFmiLocationCache &theLocationCachePoint, bool fDoX)
{
  float metFuncValue = CalcMetFuncWithFactors(theLocationCachePoint, fDoX, UpperEdgeFactors());
  return metFuncValue;
}

// jos ollaan tarpeeksi lähellä vasenta tai ala reunaa, myös ulkopuolella, oletetaan että ollaan
// tasan reunalla
// Kaava on (x-suunnassa): (-1 * peek(2, 0) + 4 * peek(1, 0) - 3 * peek(0, 0)) / (2*deltaX)
float NFmiInfoAreaMaskMetFuncBase::CalcLowerEdgeMetFunc(
    const NFmiLocationCache &theLocationCachePoint, bool fDoX)
{
  float metFuncValue = CalcMetFuncWithFactors(theLocationCachePoint, fDoX, LowerEdgeFactors());
  return metFuncValue;
}

// tämä on tavallinen tapaus, kun ollaan hilan sisällä
// Kaava on (x-suunnassa): (peek(1, 0) - peek(-1, 0)) / (2*deltaX)
float NFmiInfoAreaMaskMetFuncBase::CalcMiddleAreaMetFunc(
    const NFmiLocationCache &theLocationCachePoint, bool fDoX)
{
  float metFuncValue = CalcMetFuncWithFactors(theLocationCachePoint, fDoX, MiddleAreaFactors());
  return metFuncValue;
}

const double gGridEpsilon = 0.00001;

float NFmiInfoAreaMaskMetFuncBase::CalcMetFuncComponent(
    const NFmiLocationCache &theLocationCachePoint,
    bool fDoX,
    double theRealGridPointComponent,
    int theGridSizeComponent)
{
  float metFuncComponent = kFloatMissing;
  if (NFmiQueryDataUtil::IsEqualEnough(theRealGridPointComponent, 0., gGridEpsilon))
    metFuncComponent = CalcLowerEdgeMetFunc(
        theLocationCachePoint, fDoX);  // jos ollaan tarpeeksi lähellä vasenta reunaa, myös
                                       // ulkopuolella, oletetaan että ollaan tasan reunalla
  else if (NFmiQueryDataUtil::IsEqualEnough(
               theRealGridPointComponent, theGridSizeComponent - 1., gGridEpsilon))
    metFuncComponent = CalcUpperEdgeMetFunc(
        theLocationCachePoint, fDoX);  // jos ollaan tarpeeksi lähellä oikeaa reunaa, myös
                                       // ulkopuolella, oletetaan että ollaan tasan reunalla
  else if (theRealGridPointComponent < 0 || theRealGridPointComponent >= theGridSizeComponent - 1.)
    metFuncComponent =
        kFloatMissing;  // jos ollaan (tarpeeksi) hilan reunojen ulkopuolella, arvoksi missing
  else if (theRealGridPointComponent < 1)
  {  // erikoistapaus jossa ollaan vasemmassa reunassa, reunan ja 1. sarakkeen välissä, lasketaan
     // aproksimaatio
    // eli location pyöristetään lähimpään X-hilaan ja katsotaan ollaanko reunalla vai 'keskellä' ja
    // lasketaan sen mukaan
    NFmiLocationCache approximatedLocation = theLocationCachePoint;
    approximatedLocation.SetToNearestGridPoint(fDoX, !fDoX, itsGridSizeX, itsGridSizeY);
    if (approximatedLocation.itsGridPoint.X() == 0)
      metFuncComponent = CalcLowerEdgeMetFunc(approximatedLocation, fDoX);
    else
      metFuncComponent = CalcMiddleAreaMetFunc(approximatedLocation, fDoX);
  }
  else if (theRealGridPointComponent > theGridSizeComponent - 2.)
  {  // erikoistapaus jossa ollaan oikeassa reunassa, reunan ja viimeistä edellisen sarakkeen
     // välissä, lasketaan approksimaatio
    // eli location pyöristetään lähimpään X-hilaan ja katsotaan ollaanko reunalla vai 'keskellä' ja
    // lasketaan sen mukaan.
    NFmiLocationCache approximatedLocation = theLocationCachePoint;
    approximatedLocation.SetToNearestGridPoint(fDoX, !fDoX, itsGridSizeX, itsGridSizeY);
    if (approximatedLocation.itsGridPoint.X() == theGridSizeComponent - 1)
      metFuncComponent = CalcUpperEdgeMetFunc(approximatedLocation, fDoX);
    else
      metFuncComponent = CalcMiddleAreaMetFunc(approximatedLocation, fDoX);
  }
  else
  {  // tämä on tavallinen tapaus, kun ollaan hilan sisällä
    metFuncComponent = CalcMiddleAreaMetFunc(theLocationCachePoint, fDoX);
  }
  return metFuncComponent;
}

// **********************************************************
// *****    NFmiInfoAreaMaskMetFuncBase *********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskGrad    *************************
// **********************************************************

bool NFmiInfoAreaMaskGrad::fCalcFactorVectorsInitialized = false;
NFmiInfoAreaMaskGrad::CalcFactorVector NFmiInfoAreaMaskGrad::itsLowerEdgeFactors;
NFmiInfoAreaMaskGrad::CalcFactorVector NFmiInfoAreaMaskGrad::itsUpperEdgeFactors;
NFmiInfoAreaMaskGrad::CalcFactorVector NFmiInfoAreaMaskGrad::itsMiddleAreaFactors;

void NFmiInfoAreaMaskGrad::InitCalcFactorVectors(void)
{
  if (NFmiInfoAreaMaskGrad::fCalcFactorVectorsInitialized == false)
  {
    NFmiInfoAreaMaskGrad::itsLowerEdgeFactors.push_back(std::make_pair(2, -1.f));
    NFmiInfoAreaMaskGrad::itsLowerEdgeFactors.push_back(std::make_pair(1, 4.f));
    NFmiInfoAreaMaskGrad::itsLowerEdgeFactors.push_back(std::make_pair(0, -3.f));

    NFmiInfoAreaMaskGrad::itsUpperEdgeFactors.push_back(std::make_pair(-2, 1.f));
    NFmiInfoAreaMaskGrad::itsUpperEdgeFactors.push_back(std::make_pair(-1, -4.f));
    NFmiInfoAreaMaskGrad::itsUpperEdgeFactors.push_back(std::make_pair(0, 3.f));

    NFmiInfoAreaMaskGrad::itsMiddleAreaFactors.push_back(std::make_pair(1, 1.f));
    NFmiInfoAreaMaskGrad::itsMiddleAreaFactors.push_back(std::make_pair(-1, -1.f));

    NFmiInfoAreaMaskGrad::fCalcFactorVectorsInitialized = true;
  }
}

NFmiInfoAreaMaskGrad::~NFmiInfoAreaMaskGrad(void) {}
NFmiInfoAreaMaskGrad::NFmiInfoAreaMaskGrad(const NFmiCalculationCondition &theOperation,
                                           Type theMaskType,
                                           NFmiInfoData::Type theDataType,
                                           const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                           bool thePeekAlongTudes,
                                           MetFunctionDirection theMetFuncDirection,
                                           BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMaskMetFuncBase(theOperation,
                                  theMaskType,
                                  theDataType,
                                  theInfo,
                                  thePeekAlongTudes,
                                  theMetFuncDirection,
                                  thePostBinaryOperator),
      fCalculateDivergence(false)
{
  NFmiInfoAreaMaskGrad::InitCalcFactorVectors();
}

NFmiInfoAreaMaskGrad::NFmiInfoAreaMaskGrad(const NFmiInfoAreaMaskGrad &theOther)
    : NFmiInfoAreaMaskMetFuncBase(theOther), fCalculateDivergence(theOther.fCalculateDivergence)
{
}

NFmiAreaMask *NFmiInfoAreaMaskGrad::Clone(void) const { return new NFmiInfoAreaMaskGrad(*this); }
void NFmiInfoAreaMaskGrad::SetDividers(void)
{
  itsMetFuncDividerX = itsGridPointWidthInMeters * 2;
  itsMetFuncDividerY = itsGridPointHeightInMeters * 2;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskGrad::Value(const NFmiCalculationParams &theCalculationParams,
                                   bool /* fUseTimeInterpolationAlways */)
{
  // Laske gradientti haluttuun pisteeseen ja aikaan.
  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  // 2. Laske hilojen väli metreinä X- ja Y-suunnassa
  // -on jo laskettu konstruktoreissa SetGridSizeVariables-metodilla -> itsGridPointWidthInMeters ja
  // itsGridPointHeightInMeters
  // 3. Laske gradientti käyttämällä 'reaali'-hilapiste kurkkauksia ja hilojen välisiä etäisyyksiä.
  if (fTotalWindParam)
    itsInfo->Param(kFmiWindUMS);  // x-komponenttia alskettaessa käytetäänkin tuulen u-komponenttia!
  float gradX = CalcMetFuncComponent(
      locationCache, true, locationCache.itsGridPoint.X(), itsGridSizeX);  // x-komponentti
  if (fTotalWindParam)
    itsInfo->Param(kFmiWindVMS);  // y-komponenttia alskettaessa käytetäänkin tuulen v-komponenttia!
  float gradY = CalcMetFuncComponent(
      locationCache, false, locationCache.itsGridPoint.Y(), itsGridSizeY);  // y-komponentti
  if (itsMetFunctionDirection == DirectionXandY && gradX != kFloatMissing && gradY != kFloatMissing)
  {
    if (fCalculateDivergence)
    {
      float divergence = gradX + gradY;  // lasketaan vain komponentit yhteen
      return divergence;
    }
    else
    {
      float gradScalar = ::sqrt((gradX * gradX) + (gradY * gradY));  // lasketaan komponenttien
      // avulla pituus ja käytetään
      // sitä paluuarvona (skalaari
      // arvo, vaikka gradientti on
      // oikeasti vektori suure)
      return gradScalar;
    }
  }
  else if (itsMetFunctionDirection == DirectionX)
    return gradX;  // jos kyse oli X- tai Y-suuntaisesta laskuista, ei tarvitse tarkistella
                   // puuttuvia arvoja, koska arvot palautetaan vain sellaisenaan
  else if (itsMetFunctionDirection == DirectionY)
    return gradY;
  return kFloatMissing;
}

double NFmiInfoAreaMaskGrad::PressureValue(double thePressure,
                                           const NFmiCalculationParams &theCalculationParams)
{
  // Aseta korkeus jutut päälle, jotka otetaan huomioon NFmiInfoAreaMaskMetFuncBase::Peek -metodissa
  UsePressureLevelInterpolation(true);
  UsedPressureLevelValue(thePressure);

  //  Kutsu sitten lopuksi nomaalia Value-metodia
  return Value(theCalculationParams, true);
}

// **********************************************************
// *****    NFmiInfoAreaMaskGrad    *************************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskAdvection  **********************
// **********************************************************

NFmiInfoAreaMaskAdvection::~NFmiInfoAreaMaskAdvection(void) {}
NFmiInfoAreaMaskAdvection::NFmiInfoAreaMaskAdvection(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfoUwind,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfoVwind,
    bool thePeekAlongTudes,
    MetFunctionDirection theMetFuncDirection,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMaskGrad(theOperation,
                           theMaskType,
                           theDataType,
                           theInfo,
                           thePeekAlongTudes,
                           theMetFuncDirection,
                           thePostBinaryOperator),
      itsInfoUwind(theInfoUwind),
      itsInfoVwind(theInfoVwind)
{
}

NFmiInfoAreaMaskAdvection::NFmiInfoAreaMaskAdvection(const NFmiInfoAreaMaskAdvection &theOther)
    : NFmiInfoAreaMaskGrad(theOther),
      itsInfoUwind(NFmiAreaMask::DoShallowCopy(theOther.itsInfoUwind)),
      itsInfoVwind(NFmiAreaMask::DoShallowCopy(theOther.itsInfoVwind))
{
}

NFmiAreaMask *NFmiInfoAreaMaskAdvection::Clone(void) const
{
  return new NFmiInfoAreaMaskAdvection(*this);
}

double NFmiInfoAreaMaskAdvection::Value(const NFmiCalculationParams &theCalculationParams,
                                        bool /* fUseTimeInterpolationAlways */)
{
  // Laske gradientti haluttuun pisteeseen ja aikaan.
  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  // 2. Laske hilojen väli metreinä X- ja Y-suunnassa
  // -on jo laskettu konstruktoreissa SetGridSizeVariables-metodilla -> itsGridPointWidthInMeters ja
  // itsGridPointHeightInMeters
  // 3. Laske gradientti käyttämällä 'reaali'-hilapiste kurkkauksia ja hilojen välisiä etäisyyksiä.
  float gradX = CalcMetFuncComponent(
      locationCache, true, locationCache.itsGridPoint.X(), itsGridSizeX);  // x-komponentti
  float gradY = CalcMetFuncComponent(
      locationCache, false, locationCache.itsGridPoint.Y(), itsGridSizeY);  // y-komponentti
  if (itsMetFunctionDirection == DirectionXandY && gradX != kFloatMissing && gradY != kFloatMissing)
  {
    float u = itsInfoUwind->CachedInterpolation(locationCache, itsTimeCache);
    float v = itsInfoVwind->CachedInterpolation(locationCache, itsTimeCache);
    if (u != kFloatMissing && v != kFloatMissing)
    {
      float advectionValue =
          (gradX * u + gradY * v) *
          -1;  // huom. gradientti osoittaa kasvavaan suuntaan, joten että advektion
               // (esim. kylmästä lämpimään on negatiivinen advektio ei positiivinen) merkki
               // saadaan oikein, pitää gradientin merkki vaihtaa
      return advectionValue;
    }
  }
  else if (itsMetFunctionDirection == DirectionX && gradX != kFloatMissing)
  {
    float u = itsInfoUwind->CachedInterpolation(locationCache, itsTimeCache);
    if (u != kFloatMissing)
    {
      float advectionValue =
          (gradX * u) * -1;  // huom. gradientti osoittaa kasvavaan suuntaan, joten että advektion
      // (esim. kylmästä lämpimään on negatiivinen advektio ei positiivinen) merkki
      // saadaan oikein, pitää gradientin merkki vaihtaa
      return advectionValue;
    }
  }
  else if (itsMetFunctionDirection == DirectionY && gradY != kFloatMissing)
  {
    float v = itsInfoVwind->CachedInterpolation(locationCache, itsTimeCache);
    if (v != kFloatMissing)
    {
      float advectionValue =
          (gradY * v) * -1;  // huom. gradientti osoittaa kasvavaan suuntaan, joten että advektion
      // (esim. kylmästä lämpimään on negatiivinen advektio ei positiivinen) merkki
      // saadaan oikein, pitää gradientin merkki vaihtaa
      return advectionValue;
    }
  }
  return kFloatMissing;
}

// **********************************************************
// *****    NFmiInfoAreaMaskAdvection  **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskLaplace    **********************
// **********************************************************

bool NFmiInfoAreaMaskLaplace::fCalcFactorVectorsInitialized = false;
NFmiInfoAreaMaskLaplace::CalcFactorVector NFmiInfoAreaMaskLaplace::itsLowerEdgeFactors;
NFmiInfoAreaMaskLaplace::CalcFactorVector NFmiInfoAreaMaskLaplace::itsUpperEdgeFactors;
NFmiInfoAreaMaskLaplace::CalcFactorVector NFmiInfoAreaMaskLaplace::itsMiddleAreaFactors;

void NFmiInfoAreaMaskLaplace::InitCalcFactorVectors(void)
{
  if (NFmiInfoAreaMaskLaplace::fCalcFactorVectorsInitialized == false)
  {
    NFmiInfoAreaMaskLaplace::itsLowerEdgeFactors.push_back(std::make_pair(2, 1.f));
    NFmiInfoAreaMaskLaplace::itsLowerEdgeFactors.push_back(std::make_pair(1, -2.f));
    NFmiInfoAreaMaskLaplace::itsLowerEdgeFactors.push_back(std::make_pair(0, 1.f));

    NFmiInfoAreaMaskLaplace::itsUpperEdgeFactors.push_back(std::make_pair(-2, 1.f));
    NFmiInfoAreaMaskLaplace::itsUpperEdgeFactors.push_back(std::make_pair(-1, -2.f));
    NFmiInfoAreaMaskLaplace::itsUpperEdgeFactors.push_back(std::make_pair(0, 1.f));

    NFmiInfoAreaMaskLaplace::itsMiddleAreaFactors.push_back(std::make_pair(1, 1.f));
    NFmiInfoAreaMaskLaplace::itsMiddleAreaFactors.push_back(std::make_pair(0, -2.f));
    NFmiInfoAreaMaskLaplace::itsMiddleAreaFactors.push_back(std::make_pair(-1, 1.f));

    NFmiInfoAreaMaskLaplace::fCalcFactorVectorsInitialized = true;
  }
}

NFmiInfoAreaMaskLaplace::~NFmiInfoAreaMaskLaplace(void) {}
NFmiInfoAreaMaskLaplace::NFmiInfoAreaMaskLaplace(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    bool thePeekAlongTudes,
    MetFunctionDirection theMetFuncDirection,
    BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMaskMetFuncBase(theOperation,
                                  theMaskType,
                                  theDataType,
                                  theInfo,
                                  thePeekAlongTudes,
                                  theMetFuncDirection,
                                  thePostBinaryOperator)
{
  NFmiInfoAreaMaskLaplace::InitCalcFactorVectors();
}

NFmiInfoAreaMaskLaplace::NFmiInfoAreaMaskLaplace(const NFmiInfoAreaMaskLaplace &theOther)
    : NFmiInfoAreaMaskMetFuncBase(theOther)
{
}

NFmiAreaMask *NFmiInfoAreaMaskLaplace::Clone(void) const
{
  return new NFmiInfoAreaMaskLaplace(*this);
}

void NFmiInfoAreaMaskLaplace::SetDividers(void)
{
  itsMetFuncDividerX = itsGridPointWidthInMeters * itsGridPointWidthInMeters;
  itsMetFuncDividerY = itsGridPointHeightInMeters * itsGridPointHeightInMeters;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskLaplace::Value(const NFmiCalculationParams &theCalculationParams,
                                      bool /* fUseTimeInterpolationAlways */)
{
  // Laske gradientti haluttuun pisteeseen ja aikaan.
  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  // 2. Laske hilojen väli metreinä X- ja Y-suunnassa
  // -on jo laskettu konstruktoreissa SetGridSizeVariables-metodilla -> itsGridPointWidthInMeters ja
  // itsGridPointHeightInMeters
  // 3. Laske gradientti käyttämällä 'reaali'-hilapiste kurkkauksia ja hilojen välisiä etäisyyksiä.
  if (fTotalWindParam)
    itsInfo->Param(kFmiWindUMS);  // x-komponenttia alskettaessa käytetäänkin tuulen u-komponenttia!
  float laplaceX = CalcMetFuncComponent(
      locationCache, true, locationCache.itsGridPoint.X(), itsGridSizeX);  // x-komponentti
  if (fTotalWindParam)
    itsInfo->Param(kFmiWindVMS);  // y-komponenttia alskettaessa käytetäänkin tuulen v-komponenttia!
  float laplaceY = CalcMetFuncComponent(
      locationCache, false, locationCache.itsGridPoint.Y(), itsGridSizeY);  // y-komponentti
  if (itsMetFunctionDirection == DirectionXandY && laplaceX != kFloatMissing &&
      laplaceY != kFloatMissing)
  {
    float laplaceScalar = laplaceX + laplaceY;
    return laplaceScalar;
  }
  else if (itsMetFunctionDirection == DirectionX)
    return laplaceX;  // jos kyse oli X- tai Y-suuntaisesta laskuista, ei tarvitse tarkistella
                      // puuttuvia arvoja, koska arvot palautetaan vain sellaisenaan
  else if (itsMetFunctionDirection == DirectionY)
    return laplaceY;
  return kFloatMissing;
}

// **********************************************************
// *****    NFmiInfoAreaMaskLaplace    **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskRotor      **********************
// **********************************************************

NFmiInfoAreaMaskRotor::~NFmiInfoAreaMaskRotor(void) {}
NFmiInfoAreaMaskRotor::NFmiInfoAreaMaskRotor(const NFmiCalculationCondition &theOperation,
                                             Type theMaskType,
                                             NFmiInfoData::Type theDataType,
                                             const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                             bool thePeekAlongTudes,
                                             MetFunctionDirection theMetFuncDirection,
                                             BinaryOperator thePostBinaryOperator)
    : NFmiInfoAreaMaskGrad(theOperation,
                           theMaskType,
                           theDataType,
                           theInfo,
                           thePeekAlongTudes,
                           theMetFuncDirection,
                           thePostBinaryOperator)
{
}

NFmiInfoAreaMaskRotor::NFmiInfoAreaMaskRotor(const NFmiInfoAreaMaskRotor &theOther)
    : NFmiInfoAreaMaskGrad(theOther)
{
}

NFmiAreaMask *NFmiInfoAreaMaskRotor::Clone(void) const { return new NFmiInfoAreaMaskRotor(*this); }
// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskRotor::Value(const NFmiCalculationParams &theCalculationParams,
                                    bool /* fUseTimeInterpolationAlways */)
{
  // Laske gradientti haluttuun pisteeseen ja aikaan.
  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  // 2. Laske hilojen väli metreinä X- ja Y-suunnassa
  // -on jo laskettu konstruktoreissa SetGridSizeVariables-metodilla -> itsGridPointWidthInMeters ja
  // itsGridPointHeightInMeters
  // 3. Laske rotor käyttämällä 'reaali'-hilapiste kurkkauksia ja hilojen välisiä etäisyyksiä.
  itsInfo->Param(kFmiWindVMS);  // x-komponenttia alskettaessa käytetäänkin tuulen v-komponenttia!
  float rotX = CalcMetFuncComponent(
      locationCache, true, locationCache.itsGridPoint.X(), itsGridSizeX);  // x-komponentti
  itsInfo->Param(kFmiWindUMS);  // y-komponenttia alskettaessa käytetäänkin tuulen u-komponenttia!
  float rotY = CalcMetFuncComponent(
      locationCache, false, locationCache.itsGridPoint.Y(), itsGridSizeY);  // y-komponentti
  if (itsMetFunctionDirection == DirectionXandY && rotX != kFloatMissing && rotY != kFloatMissing)
  {
    float rotScalar = ::sqrt((rotX * rotX) + (rotY * rotY));  // lasketaan komponenttien avulla
                                                              // pituus ja käytetään sitä
                                                              // paluuarvona (skalaari arvo, vaikka
                                                              // rotor on oikeasti vektori suure)
    return rotScalar;
  }
  else if (itsMetFunctionDirection == DirectionX)
    return rotX;  // jos kyse oli X- tai Y-suuntaisesta laskuista, ei tarvitse tarkistella puuttuvia
                  // arvoja, koska arvot palautetaan vain sellaisenaan
  else if (itsMetFunctionDirection == DirectionY)
    return rotY;
  return kFloatMissing;
}

// **********************************************************
// *****    NFmiInfoAreaMaskRotor      **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskVertFunc   **********************
// **********************************************************

NFmiInfoAreaMaskVertFunc::~NFmiInfoAreaMaskVertFunc(void) {}
NFmiInfoAreaMaskVertFunc::NFmiInfoAreaMaskVertFunc(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    NFmiAreaMask::FunctionType thePrimaryFunc,
    NFmiAreaMask::FunctionType theSecondaryFunc,
    int theArgumentCount)
    : NFmiInfoAreaMaskMetFuncBase(
          theOperation, theMaskType, theDataType, theInfo, false, NoDirection),
      itsPrimaryFunc(thePrimaryFunc),
      itsSecondaryFunc(theSecondaryFunc),
      itsArgumentVector(),
      itsFunctionModifier(),
      itsStartLevelValue(kFloatMissing),
      itsEndLevelValue(kFloatMissing),
      itsStartLevelIndex(gMissingIndex),
      itsEndLevelIndex(gMissingIndex),
      itsUsedHeightParId(kFmiBadParameter),
      fReturnHeightValue(false),
      itsLevelIncrement(1),
      fReverseLevels(false)
{
  itsFunctionArgumentCount = theArgumentCount;
  if (itsPrimaryFunc == NFmiAreaMask::MinH || itsPrimaryFunc == NFmiAreaMask::MaxH ||
      itsPrimaryFunc == NFmiAreaMask::FindH)
    fReturnHeightValue = true;
}

NFmiInfoAreaMaskVertFunc::NFmiInfoAreaMaskVertFunc(const NFmiInfoAreaMaskVertFunc &theOther)
    : NFmiInfoAreaMaskMetFuncBase(theOther),
      itsPrimaryFunc(theOther.itsPrimaryFunc),
      itsSecondaryFunc(theOther.itsSecondaryFunc),
      itsArgumentVector(theOther.itsArgumentVector),
      itsFunctionModifier(theOther.itsFunctionModifier ? theOther.itsFunctionModifier->Clone() : 0),
      itsStartLevelValue(theOther.itsStartLevelValue),
      itsEndLevelValue(theOther.itsEndLevelValue),
      itsStartLevelIndex(theOther.itsStartLevelIndex),
      itsEndLevelIndex(theOther.itsEndLevelIndex),
      itsUsedHeightParId(theOther.itsUsedHeightParId),
      fReturnHeightValue(theOther.fReturnHeightValue),
      itsLevelIncrement(theOther.itsLevelIncrement),
      fReverseLevels(theOther.fReverseLevels)
{
}

NFmiAreaMask *NFmiInfoAreaMaskVertFunc::Clone(void) const
{
  return new NFmiInfoAreaMaskVertFunc(*this);
}

static boost::shared_ptr<NFmiDataModifier> CreateIntegrationFuction(NFmiAreaMask::FunctionType func)
{
  boost::shared_ptr<NFmiDataModifier> modifier;
  // vertCondFunc tapauksia on paljon ja niille ei tehdä integraatiota, joten tein ehdon vähentämään
  // case -tapauksia
  if (!::IsFindConditionalFunction(func))
  {
    switch (func)
    {
      case NFmiAreaMask::Avg:
        modifier = boost::shared_ptr<NFmiDataModifier>(new NFmiDataModifierAvg());
        break;
      case NFmiAreaMask::Min:
        modifier = boost::shared_ptr<NFmiDataModifier>(new NFmiDataModifierMin());
        break;
      case NFmiAreaMask::Max:
        modifier = boost::shared_ptr<NFmiDataModifier>(new NFmiDataModifierMax());
        break;
      case NFmiAreaMask::Sum:
        modifier = boost::shared_ptr<NFmiDataModifier>(new NFmiDataModifierSum());
        break;
      case NFmiAreaMask::Get:
      case NFmiAreaMask::FindH:
      case NFmiAreaMask::FindC:
      case NFmiAreaMask::MinH:
      case NFmiAreaMask::MaxH:
      case NFmiAreaMask::Grad:
        modifier = boost::shared_ptr<NFmiDataModifier>();  // get- ja find -tapauksissa palautetaan
                                                           // tyhjä-olio, koska niille ei tarvita
                                                           // erillistä integraattoria
        break;
      // HUOM!!!! Tee WAvg-modifier myös, joka on peritty Avg-modifieristä ja tee joku kerroin juttu
      // painotukseen.
      default:
        throw std::runtime_error(
            "Internal SmartMet error: Vertical function has unknown integration function,\ncan't "
            "execute the calculations.");
    }
  }
  return modifier;
}

void NFmiInfoAreaMaskVertFunc::Initialize(void)
{
  NFmiInfoAreaMaskMetFuncBase::Initialize();

  itsFunctionModifier = CreateIntegrationFuction(itsPrimaryFunc);

  switch (itsSecondaryFunc)
  {
    case NFmiAreaMask::VertP:
    case NFmiAreaMask::VertFL:
    case NFmiAreaMask::TimeVertP:
    case NFmiAreaMask::TimeVertFL:
      itsUsedHeightParId = kFmiPressure;
      break;
    case NFmiAreaMask::VertZ:
    case NFmiAreaMask::TimeVertZ:
      itsUsedHeightParId = kFmiGeomHeight;
      break;
    case NFmiAreaMask::VertHyb:
    case NFmiAreaMask::TimeVertHyb:
      itsUsedHeightParId = kFmiModelLevel;
      if (itsInfo->LevelType() != kFmiHybridLevel)
        throw std::runtime_error(
            "Model-level vertical function has called but data doesn't have model levels,\ncan't "
            "execute the calculations.");
      break;
    default:
      throw std::runtime_error(
          "Internal SmartMet error: Vertical function has unknown height parameter,\ncan't execute "
          "the calculations.");
  }
}

void NFmiInfoAreaMaskVertFunc::SetArguments(std::vector<float> &theArgumentVector)
{
  // jokaiselle pisteelle ja ajanhetkelle annetaan eri argumentit tässä
  itsArgumentVector = theArgumentVector;
  if (static_cast<int>(itsArgumentVector.size()) !=
      itsFunctionArgumentCount - 1)  // -1 tarkoittaa että funktion 1. argumentti tulee suoraan
                                     // itsIfo:sta, eli sitä ei anneta argumentti-listassa
    throw std::runtime_error(
        "Internal SmartMet error: Vertical function was given invalid number of arguments, cannot "
        "calculate the macro.");
}

static bool IsBetweenValues(double value, double value1, double value2)
{
  if (value >= value1 && value <= value2) return true;
  if (value >= value2 && value <= value1) return true;
  return false;
}

void NFmiInfoAreaMaskVertFunc::SetLevelIndexies(float theHeightValue)
{
  if (theHeightValue != kFloatMissing)
  {
    if (::IsBetweenValues(theHeightValue, itsStartLevelValue, itsEndLevelValue))
    {  // eli löytyi ei puuttuva paine arvo ja se oli haluttujen paine-rajojen sisällä
      if (itsStartLevelIndex ==
          gMissingIndex)  // jos ei ole vielä alaraja indeksiä, laitetaan se nyt
        itsStartLevelIndex = itsInfo->LevelIndex();
      itsEndLevelIndex =
          itsInfo
              ->LevelIndex();  // päivitetään ylärajaindeksiä niin kauan kuin ollaan rajojen sisällä
    }
  }
}

static bool IsInsideLimits(float low, float high, float value)
{
  return (value >= low) && (value <= high);
}

void NFmiInfoAreaMaskVertFunc::SearchLevels(const NFmiLocationCache &theLocationCache)
{
  if (itsInfo)
  {
    if (itsUsedHeightParId == kFmiModelLevel)
    {
      itsInfo->LastLevel();
      float lastExistingLevelValue = itsInfo->Level()->LevelValue();

      itsInfo->FirstLevel();
      float firstExistingLevelValue = itsInfo->Level()->LevelValue();

      if (firstExistingLevelValue > lastExistingLevelValue)
        std::swap(firstExistingLevelValue, lastExistingLevelValue);

      if (itsStartLevelValue > itsEndLevelValue) std::swap(itsStartLevelValue, itsEndLevelValue);

      if (itsStartLevelValue < firstExistingLevelValue)
        itsStartLevelValue = firstExistingLevelValue;

      if (itsEndLevelValue > lastExistingLevelValue) itsEndLevelValue = lastExistingLevelValue;

      // float closestInsideStartLevelValue = kFloatMissing;
      float minStartDiff = 99999999.f;
      // float closestInsideEndLevelValue = kFloatMissing;
      float minEndDiff = 99999999.f;
      for (itsInfo->FirstLevel(); itsInfo->NextLevel();)
      {
        const NFmiLevel *aLevel = itsInfo->Level();

        if (::IsInsideLimits(itsStartLevelValue, itsEndLevelValue, aLevel->LevelValue()))
        {
          // alarajan etsintä koodia
          if (minStartDiff > 0)  // ei ole vielä löytynyt tarkkaa leveliä
          {
            float currentDiff = ::fabs(itsStartLevelValue - aLevel->LevelValue());
            if (currentDiff < minStartDiff)
            {
              // closestInsideStartLevelValue = aLevel->LevelValue();
              minStartDiff = currentDiff;
              itsStartLevelIndex = itsInfo->LevelIndex();
            }
          }

          // ylärajan etsintä koodia
          if (minEndDiff > 0)  // ei ole vielä löytynyt tarkkaa leveliä
          {
            float currentDiff = ::fabs(itsEndLevelValue - aLevel->LevelValue());
            if (currentDiff < minEndDiff)
            {
              // closestInsideEndLevelValue = aLevel->LevelValue();
              minEndDiff = currentDiff;
              itsEndLevelIndex = itsInfo->LevelIndex();
            }
          }
        }
      }

      if (itsStartLevelIndex > itsEndLevelIndex) std::swap(itsStartLevelIndex, itsEndLevelIndex);
    }
    else
    {
      for (itsInfo->FirstLevel(); itsInfo->NextLevel();)
      {
        float Pvalue =
            itsInfo->GetLevelHeightValue(itsUsedHeightParId, theLocationCache, itsTimeCache);
        SetLevelIndexies(Pvalue);
      }
    }
  }
}

static float ConvertFL2P(float FL)
{
  if (FL != kFloatMissing)
    return static_cast<float>(::CalcFlightLevelPressure(FL));
  else
    return kFloatMissing;
}

void NFmiInfoAreaMaskVertFunc::SetLevelValues(void)
{
  // otetaan argumentteina annetut arvot haettaviin level-rajoihin
  itsStartLevelValue = itsArgumentVector[0];
  itsEndLevelValue = kFloatMissing;  // Huom! tämä voi olla puuttuva, jos kyse on simppelistä
                                     // get-funktiosta (esim. vertp_get(WS_Hir, pressure))
  if (itsArgumentVector.size() > 1) itsEndLevelValue = itsArgumentVector[1];
  // mahdolliset FL:t pitää muuttaa paineeksi [hPa]
  if (itsSecondaryFunc == NFmiAreaMask::VertFL)
  {  // annetut lentopinnat ovat hehto jalkoina, ne pitää muuttaa jaloiksi ennen konversiota
    itsStartLevelValue = ::ConvertFL2P(itsStartLevelValue * 100);
    itsEndLevelValue = ::ConvertFL2P(itsEndLevelValue * 100);
  }
}

void NFmiInfoAreaMaskVertFunc::FindCalculatedLeves(const NFmiLocationCache &theLocationCache)
{
  // nollataan ensin level-indeksit
  itsStartLevelIndex = gMissingIndex;
  itsEndLevelIndex = gMissingIndex;
  itsLevelIncrement = 1;
  fReverseLevels = false;

  SearchLevels(theLocationCache);

  // Nyt on aloitus ja lopetus levelit löydetty. Nyt on vielä pääteltävä mihin suuntaan levelit
  // menevät.
  // Data on tarkoitus käydä läpi mannpinnasta ylöspäin, joten on tehtävä lisä tarkasteluja.
  // Eli jos datan-levelien suunta on alaspäin = avaruudesta kohti maanpintaa, on incrementti -1 ja
  // levelIndeksit käännetään.
  if (itsStartLevelIndex != gMissingIndex &&
      itsEndLevelIndex != gMissingIndex)  // lisäksi levelIndeksien pitää olla järkeviä
  {
    if (itsUsedHeightParId == kFmiPressure)
    {
      if (itsInfo->PressureParamIsRising())
        itsLevelIncrement = -1;  // PressureParamIsRising = true -> levelien vertikaali suunta on
                                 // alaspäin (avaruudesta kohti maanpintaa)
    }
    else
    {
      if (itsInfo->HeightParamIsRising() == false)
        itsLevelIncrement = -1;  // HeightParamIsRising = false -> levelien vertikaali suunta on
                                 // alaspäin (avaruudesta kohti maanpintaa)
    }

    if (itsLevelIncrement == -1)
    {
      std::swap(itsStartLevelIndex,
                itsEndLevelIndex);  // jos incrementti oli -1, pitää levelIndeksit vaihtaa päittäin
      fReverseLevels = true;
    }
  }
}

float NFmiInfoAreaMaskVertFunc::DoGetFunction(const NFmiLocationCache &theLocationCache,
                                              const NFmiCalculationParams &theCalculationParams,
                                              float theLevelValue)
{
  if (itsSecondaryFunc == NFmiAreaMask::VertZ)
    return itsInfo->HeightValue(
        theLevelValue, theCalculationParams.itsLatlon, theCalculationParams.itsTime);
  else if (itsSecondaryFunc == NFmiAreaMask::VertP || itsSecondaryFunc == NFmiAreaMask::VertFL)
    return itsInfo->PressureLevelValue(
        theLevelValue, theCalculationParams.itsLatlon, theCalculationParams.itsTime);
  else
  {  // else hybrid arvo suoraan
    itsInfo->FirstLevel();
    NFmiLevel aLevel(*itsInfo->Level());
    aLevel.LevelValue(theLevelValue);
    itsInfo->Level(aLevel);
    return itsInfo->CachedInterpolation(theLocationCache, itsTimeCache);
  }
}

static bool IsUnder(float theSearchedValue, float theCurrentValue)
{
  // OLETUS! Verrattavat arvot eivät saa olla puuttuvia!!!!
  return theSearchedValue < theCurrentValue;
}

unsigned long NFmiInfoAreaMaskVertFunc::GetNonMissingStartLevelIndex(
    const NFmiLocationCache &theLocationCache)
{
  for (int levelIndex = static_cast<int>(itsStartLevelIndex);
       fReverseLevels ? levelIndex >= static_cast<int>(itsEndLevelIndex)
                      : levelIndex <= static_cast<int>(itsEndLevelIndex);
       levelIndex += itsLevelIncrement)
  {
    if (itsInfo->LevelIndex(levelIndex))  // pitäisi olla aina totta
    {
      float value = itsInfo->CachedInterpolation(theLocationCache, itsTimeCache);
      if (value != kFloatMissing) return levelIndex;
    }
  }
  return gMissingIndex;
}

// Laskee logaritmisessa asteikossa interpoloidun arvon.
// Käytetään esim. logaritmisen paine asteikon kanssa.
// Palauttaa x:ää vastaavan y:n, kun x1 arvoa vastaa y1 ja x2:n arvoa vastaa y2.
// eli x-parametrit ovat logaritmisella asteikolla (eli paineet ovat x-lukuja).
// Palauttaa x:ää vastaavan luvun y, joka on siis interpoloitu logaritmisesti.
// *********************************************************************************
// HUOM! Tämä on kopioitu suoraan NFmiFastQueryInfo.cpp:stä. Funktio pitäisi laittaa
// johonkin ylaiseen paikkaan molempien käytettäväksi!!!!
// *********************************************************************************
/*
static double CalcLogInterpolatedValue(double x1, double x2, double x, double y1, double y2)
{
    double y = kFloatMissing;
    if(x1 != kFloatMissing && x2 != kFloatMissing && x != kFloatMissing)
    {
        if(x1 == x2)
            y = y1 != kFloatMissing ? y1 : y2;
        else if(y1 != kFloatMissing && y2 != kFloatMissing)
        {
            double w = (::log(x)-::log(x1)) / (::log(x2)-::log(x1));
            y = (1-w)*y1 + w*y2;
        }
        else if(y1 != kFloatMissing) // jos toinen -arvoista puuttuu annetaan arvoksi toinen
            y = y1;
        else if(y2 != kFloatMissing) // jos toinen -arvoista puuttuu annetaan arvoksi toinen
            y = y2;
    }
    return y;
}
*/

float NFmiInfoAreaMaskVertFunc::GetLevelHeightValue(const NFmiLocationCache &theLocationCache)
{
  if (itsUsedHeightParId == kFmiModelLevel)
    return itsInfo->Level()->LevelValue();
  else
    return itsInfo->GetLevelHeightValue(itsUsedHeightParId, theLocationCache, itsTimeCache);
}

float NFmiInfoAreaMaskVertFunc::DoFindFunction(const NFmiLocationCache &theLocationCache)
{
  bool findHeight = itsPrimaryFunc == NFmiAreaMask::FindH;
  if (itsArgumentVector.size() < 3) return kFloatMissing;
  float searchedValue = itsArgumentVector[2];
  if (searchedValue == kFloatMissing) return kFloatMissing;
  if (findHeight && itsArgumentVector.size() < 4) return kFloatMissing;
  int search_nth_value =
      findHeight ? static_cast<int>(::round(itsArgumentVector[3]))
                 : 0;  // kuinka mones osuma haetaan, 1 on 1. 2 on 2. jne. 0 (tai <0) on viimeinen

  float value1 = kFloatMissing;  // tässä on tallessa viimeistä edellinen kunnollinen arvo
  float value2 = kFloatMissing;  // tässä on tallessa viimeisin kunnollinen arvo
  float height1 = kFloatMissing;  // tässä on tallessa viimeistä edellisen kunnollisen arvon korkeus
  float height2 = kFloatMissing;  // tässä on tallessa viimeisimmän kunnollisen arvon korkeus
  float foundHeight = kFloatMissing;
  int foundCount = 0;
  unsigned long realStartLevelIndex = GetNonMissingStartLevelIndex(theLocationCache);
  if (realStartLevelIndex != gMissingIndex)
  {
    itsInfo->LevelIndex(realStartLevelIndex);
    value1 = itsInfo->CachedInterpolation(theLocationCache, itsTimeCache);
    height1 = GetLevelHeightValue(theLocationCache);

    bool isUnder = ::IsUnder(searchedValue, value1);
    for (int levelIndex = static_cast<int>(realStartLevelIndex + itsLevelIncrement);
         fReverseLevels ? levelIndex >= static_cast<int>(itsEndLevelIndex)
                        : levelIndex <= static_cast<int>(itsEndLevelIndex);
         levelIndex += itsLevelIncrement)
    {
      itsInfo->LevelIndex(levelIndex);
      value2 = itsInfo->CachedInterpolation(theLocationCache, itsTimeCache);
      if (value2 != kFloatMissing)
      {
        height2 = GetLevelHeightValue(theLocationCache);
        bool isUnder2 = ::IsUnder(searchedValue, value2);
        if (isUnder != isUnder2)
        {
          foundCount++;
          // löytyi etsittävä arvo, lasketaan korkeudelle arvio
          foundHeight = static_cast<float>(
              NFmiInterpolation::Linear(searchedValue,
                                        value1,
                                        value2,
                                        height1,
                                        height2));  // CalcLogInterpolatedValue(value1, value2,
                                                    // searchedValue, height1, height2);
          if (search_nth_value > 0 && search_nth_value <= foundCount)  // jos etsitään tiettyä esim.
                                                                       // 3. löytöä ja foundCount
            // pääsee 3:een, palautetaan
            // välittömästi arvo
            return foundHeight;
        }
        value1 = value2;    // huom! vain ei puuttuvia arvoja saa siirtää!
        height1 = height2;  // huom! vain ei puuttuvia arvoja saa siirtää!
        isUnder = isUnder2;
      }
    }
  }
  if (findHeight)
    return foundHeight;  // palautetaan viimeisin löytynyt arvo (tai missing, jos ei löytynyt
                         // yhtään)
  else
    return static_cast<float>(foundCount);
}

float NFmiInfoAreaMaskVertFunc::DoVerticalGrad(const NFmiLocationCache &theLocationCache,
                                               const NFmiCalculationParams &theCalculationParams)
{
  float value1 = DoGetFunction(theLocationCache, theCalculationParams, itsStartLevelValue);
  float value2 = DoGetFunction(theLocationCache, theCalculationParams, itsEndLevelValue);
  if (value1 != kFloatMissing && value2 != kFloatMissing)
  {
    float result = ::fabs(value2 - value1) / ::fabs(itsEndLevelValue - itsStartLevelValue);
    return result;
  }
  else
    return kFloatMissing;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskVertFunc::Value(const NFmiCalculationParams &theCalculationParams,
                                       bool /* fUseTimeInterpolationAlways */)
{
  SetLevelValues();
  if (itsStartLevelValue == kFloatMissing)
    return kFloatMissing;  // jos jo alku level arvo on puuttuvaa, ei voi tehdä mitään järkevää
  if (itsEndLevelValue == kFloatMissing && itsPrimaryFunc != NFmiAreaMask::Get)
    return kFloatMissing;  // jos jo loppu level arvo on puuttuvaa, eikä kyse ollut get-funktiosta,
                           // ei voi tehdä mitään järkevää

  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);

  if (itsPrimaryFunc == NFmiAreaMask::Get)
    return DoGetFunction(locationCache, theCalculationParams, itsStartLevelValue);
  else if (itsPrimaryFunc == NFmiAreaMask::Grad)
    return DoVerticalGrad(locationCache, theCalculationParams);

  // 2. Käy läpi haluttu level korkeus/level väli ja laske haluttu operaatio niille
  FindCalculatedLeves(locationCache);

  if (itsStartLevelIndex == gMissingIndex || itsEndLevelIndex == gMissingIndex)
    return kFloatMissing;  // jos jompikumpi level indekseistä osoittaa 'kuuseen', ei kannata jatkaa

  float value = kFloatMissing;
  if (::IsFindFunction(itsPrimaryFunc))
    value = DoFindFunction(locationCache);
  else
    value = DoNormalFunction(locationCache);

  if (fReturnHeightValue && itsSecondaryFunc == NFmiAreaMask::VertFL)
    value = static_cast<float>(::CalcPressureFlightLevel(value));
  return value;
}

class DoubleValueSearcher
{
 public:
  DoubleValueSearcher(bool doSearchMax)
      : fSearchMax(doSearchMax),
        itsExtremeValue(kFloatMissing),
        itsExtremeSecondaryValue(kFloatMissing)
  {
  }

  void Values(float primaryValue, float secondaryValue)
  {
    if (primaryValue != kFloatMissing && secondaryValue != kFloatMissing)
    {
      if (IsExtreme(primaryValue))
      {
        itsExtremeValue = primaryValue;
        itsExtremeSecondaryValue = secondaryValue;
      }
    }
  }

  float ExtremeValue(void) const { return itsExtremeValue; }
  float ExtremeSecondaryValue(void) const { return itsExtremeSecondaryValue; }
 private:
  bool IsExtreme(float primaryValue)
  {
    if (primaryValue != kFloatMissing && itsExtremeValue != kFloatMissing)
    {
      if (fSearchMax)
        return primaryValue > itsExtremeValue;
      else
        return primaryValue < itsExtremeValue;
    }
    else if (primaryValue != kFloatMissing)
      return true;
    return false;
  }

  bool fSearchMax;                 // jos tämä on true, etsii maksimi arvoa, muuten minimiä
  float itsExtremeValue;           // tähän talletetaan min/max arvo
  float itsExtremeSecondaryValue;  // tähän talletetaan toisio arvo, mitä loppujen lopuksi kaivataan
                                   // (esim. jonkin parametrin max-arvon korkeutta)
};

float NFmiInfoAreaMaskVertFunc::DoNormalFunction(const NFmiLocationCache &theLocationCache)
{
  if (fReturnHeightValue)
  {
    DoubleValueSearcher valueSearcher(itsPrimaryFunc == NFmiAreaMask::MaxH);
    for (int levelIndex = static_cast<int>(itsStartLevelIndex);
         fReverseLevels ? levelIndex >= static_cast<int>(itsEndLevelIndex)
                        : levelIndex <= static_cast<int>(itsEndLevelIndex);
         levelIndex += itsLevelIncrement)
    {
      itsInfo->LevelIndex(levelIndex);
      float primaryValue = itsInfo->CachedInterpolation(theLocationCache, itsTimeCache);
      float heightValue = GetLevelHeightValue(theLocationCache);
      valueSearcher.Values(primaryValue, heightValue);
    }
    return valueSearcher.ExtremeSecondaryValue();
  }
  else
  {
    // 3. Nollaa integraattori-laskuri
    itsFunctionModifier->Clear();
    // 7. käy levelit läpi ja sijoita arvot itsFunctionModifier:iin
    for (int levelIndex = static_cast<int>(itsStartLevelIndex);
         fReverseLevels ? levelIndex >= static_cast<int>(itsEndLevelIndex)
                        : levelIndex <= static_cast<int>(itsEndLevelIndex);
         levelIndex += itsLevelIncrement)
    {
      itsInfo->LevelIndex(levelIndex);
      itsFunctionModifier->Calculate(itsInfo->CachedInterpolation(theLocationCache, itsTimeCache));
    }
    // 8. palauta laskennan tulos
    return itsFunctionModifier->CalculationResult();
  }
}

// **********************************************************
// *****    NFmiInfoAreaMaskVertFunc   **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskVertConditionalFunc  ************
// **********************************************************

NFmiInfoAreaMaskVertConditionalFunc::~NFmiInfoAreaMaskVertConditionalFunc(void) {}
NFmiInfoAreaMaskVertConditionalFunc::NFmiInfoAreaMaskVertConditionalFunc(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    NFmiAreaMask::FunctionType thePrimaryFunc,
    NFmiAreaMask::FunctionType theSecondaryFunc,
    int theArgumentCount)
    : NFmiInfoAreaMaskVertFunc(theOperation,
                               theMaskType,
                               theDataType,
                               theInfo,
                               thePrimaryFunc,
                               theSecondaryFunc,
                               theArgumentCount),
      itsLimit1(kFloatMissing),
      itsLimit2(kFloatMissing)
{
  if (::IsFindConditionalFunction(itsPrimaryFunc)) fReturnHeightValue = true;
}

NFmiInfoAreaMaskVertConditionalFunc::NFmiInfoAreaMaskVertConditionalFunc(
    const NFmiInfoAreaMaskVertConditionalFunc &theOther)
    : NFmiInfoAreaMaskVertFunc(theOther),
      itsLimit1(theOther.itsLimit1),
      itsLimit2(theOther.itsLimit2)
{
}

NFmiAreaMask *NFmiInfoAreaMaskVertConditionalFunc::Clone(void) const
{
  return new NFmiInfoAreaMaskVertConditionalFunc(*this);
}

void NFmiInfoAreaMaskVertConditionalFunc::Initialize(void)
{
  NFmiInfoAreaMaskVertFunc::Initialize();
}

// Tätä kutsutaan jokaiselle erillis pistelaskulle erikseen value-funktiossa.
bool NFmiInfoAreaMaskVertConditionalFunc::InitializeFromArguments(void)
{
  itsStartLevelValue = itsArgumentVector[0];
  itsEndLevelValue = itsArgumentVector[1];
  itsLimit1 = itsArgumentVector[2];
  itsLimit2 = kFloatMissing;  // Huom! tämä voi olla puuttuva, jos kyse on simppelistä
                              // get-funktiosta (esim. vertp_get(WS_Hir, pressure))
  if (itsArgumentVector.size() > 4) itsLimit2 = itsArgumentVector[4];

  if (itsStartLevelValue == kFloatMissing || itsEndLevelValue == kFloatMissing)
    return false;  // jos alku/loppu level arvo on puuttuvaa, ei voi tehdä mitään järkevää

  return true;
}

double NFmiInfoAreaMaskVertConditionalFunc::Value(const NFmiCalculationParams &theCalculationParams,
                                                  bool fUseTimeInterpolationAlways)
{
  float value = kFloatMissing;
  if (InitializeFromArguments())
  {
    SetLevelValues();
    // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
    NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);

    // 2. Käy läpi haluttu level korkeus/level väli ja laske haluttu operaatio niille
    FindCalculatedLeves(locationCache);

    if (itsStartLevelIndex != gMissingIndex && itsEndLevelIndex != gMissingIndex)
    {
      value = DoFindConditionalFunction(locationCache);
      if (fReturnHeightValue && itsSecondaryFunc == NFmiAreaMask::VertFL)
        value = static_cast<float>(::CalcPressureFlightLevel(value));
    }
  }
  return value;
}

bool NFmiInfoAreaMaskVertConditionalFunc::CheckProbabilityCondition(double value)
{
  return ::CheckProbabilityCondition(itsPrimaryFunc, value, itsLimit1, itsLimit2);
}

float NFmiInfoAreaMaskVertConditionalFunc::DoFindConditionalFunction(
    const NFmiLocationCache &theLocationCache)
{
  if (fReturnHeightValue)
  {
    for (int levelIndex = static_cast<int>(itsStartLevelIndex);
         fReverseLevels ? levelIndex >= static_cast<int>(itsEndLevelIndex)
                        : levelIndex <= static_cast<int>(itsEndLevelIndex);
         levelIndex += itsLevelIncrement)
    {
      itsInfo->LevelIndex(levelIndex);
      float value = itsInfo->CachedInterpolation(theLocationCache, itsTimeCache);
      if (CheckProbabilityCondition(value))
      {
        float heightValue = GetLevelHeightValue(theLocationCache);
        return heightValue;
      }
    }
    return kFloatMissing;
  }

  throw std::runtime_error(
      "Error in program's logic: vertical conditional find function didn't return height value...");
}

// **********************************************************
// *****    NFmiInfoAreaMaskVertConditionalFunc  ************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskTimeVertFunc  *******************
// **********************************************************
NFmiInfoAreaMaskTimeVertFunc::~NFmiInfoAreaMaskTimeVertFunc(void) {}
NFmiInfoAreaMaskTimeVertFunc::NFmiInfoAreaMaskTimeVertFunc(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    NFmiAreaMask::FunctionType thePrimaryFunc,
    NFmiAreaMask::FunctionType theSecondaryFunc,
    int theArgumentCount)
    : NFmiInfoAreaMaskVertFunc(theOperation,
                               theMaskType,
                               theDataType,
                               theInfo,
                               thePrimaryFunc,
                               theSecondaryFunc,
                               theArgumentCount),
      itsStartTimeOffsetInHours(0),
      itsEndTimeOffsetInHours(0)
{
}

NFmiInfoAreaMaskTimeVertFunc::NFmiInfoAreaMaskTimeVertFunc(
    const NFmiInfoAreaMaskTimeVertFunc &theOther)
    : NFmiInfoAreaMaskVertFunc(theOther),
      itsStartTimeOffsetInHours(theOther.itsStartTimeOffsetInHours),
      itsEndTimeOffsetInHours(theOther.itsEndTimeOffsetInHours)
{
}

NFmiAreaMask *NFmiInfoAreaMaskTimeVertFunc::Clone(void) const
{
  return new NFmiInfoAreaMaskTimeVertFunc(*this);
}

void NFmiInfoAreaMaskTimeVertFunc::SetRangeValuesFromArguments(void)
{
  itsStartTimeOffsetInHours = itsArgumentVector[0];
  itsEndTimeOffsetInHours = itsArgumentVector[1];
  // otetaan argumentteina annetut arvot haettaviin level-rajoihin
  itsStartLevelValue = itsArgumentVector[2];
  itsEndLevelValue = itsArgumentVector[3];

  // mahdolliset FL:t pitää muuttaa paineeksi [hPa]
  if (itsSecondaryFunc == NFmiAreaMask::TimeVertFL)
  {  // annetut lentopinnat ovat hehto jalkoina, ne pitää muuttaa jaloiksi ennen konversiota
    itsStartLevelValue = ::ConvertFL2P(itsStartLevelValue * 100);
    itsEndLevelValue = ::ConvertFL2P(itsEndLevelValue * 100);
  }
}

// Etsii halutun aika loopituksen alku- ja loppuaika indeksejä annetusta infosta.
// Palauttaa mahdollisen interpolaatio ajan, jos startOffset ja endOffsetit ovat samoja, käytetään
// kyseistä aikaa aikainterpolaatiossa.
static bool CalcTimeLoopIndexies(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                 const NFmiCalculationParams &theCalculationParams,
                                 double theStartTimeOffsetInHours,
                                 double theEndTimeOffsetInHours,
                                 unsigned long *theStartTimeIndexOut,
                                 unsigned long *theEndTimeIndexOut)
{
  bool status = false;
  if (theInfo)
  {
    unsigned long origTimeIndex = theInfo->TimeIndex();  // Otetaan aikaindeksi talteen, jotta se
                                                         // voidaan lopuksi palauttaa takaisin

    NFmiMetTime startTime = theCalculationParams.itsTime;
    startTime.ChangeByMinutes(FmiRound(theStartTimeOffsetInHours * 60));
    NFmiMetTime endTime = theCalculationParams.itsTime;
    endTime.ChangeByMinutes(FmiRound(theEndTimeOffsetInHours * 60));

    NFmiTimeDescriptor origTimes = theInfo->TimeDescriptor();  // pitää ottaa kopio ajoista, koska
                                                               // GetIntersection -metodi ei ole
                                                               // const.
    NFmiTimeDescriptor times = origTimes.GetIntersection(startTime, endTime);

    if (times.IsEmpty())
    {
      *theStartTimeIndexOut = *theEndTimeIndexOut = gMissingIndex;
      status = false;
    }
    else if (times.Size() == 1)
    {
      theInfo->Time(times.FirstTime());
      *theStartTimeIndexOut = *theEndTimeIndexOut = theInfo->TimeIndex();
      status = true;
    }
    else
    {
      theInfo->Time(times.FirstTime());
      *theStartTimeIndexOut = theInfo->TimeIndex();
      theInfo->Time(times.LastTime());
      *theEndTimeIndexOut = theInfo->TimeIndex();
      status = true;
    }

    theInfo->TimeIndex(origTimeIndex);
  }

  return status;
}

// min ja max funktioille voidaan laskuissa ottaa interpoloidun arvon sijasta kaikki interpolaatio
// pistettä ympäröivät arvot sellaisenaan. Interpolaatio kun muuttaa min/max arvoja ikävästi
// neljästä
// luvusta lasketuksi painotetuksi keskiarvoksi.
static void AddExtremeValues(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             boost::shared_ptr<NFmiDataModifier> &theFunctionModifier,
                             const NFmiPoint &theLatlon)
{
  NFmiLocationCache locCache = theInfo->CalcLocationCache(theLatlon);
  if (!locCache.NoValue())
  {
    std::vector<float> values(4, kFloatMissing);
    theInfo->GetCachedValues(locCache, values);
    for (size_t i = 0; i < values.size(); i++)
      theFunctionModifier->Calculate(values[i]);
  }
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskTimeVertFunc::Value(const NFmiCalculationParams &theCalculationParams,
                                           bool /* fUseTimeInterpolationAlways */)
{
  SetRangeValuesFromArguments();
  if (itsStartLevelValue == kFloatMissing)
    return kFloatMissing;  // jos jo alku level arvo on puuttuvaa, ei voi tehdä mitään järkevää
  if (itsEndLevelValue == kFloatMissing)
    return kFloatMissing;  // jos jo loppu level arvo on puuttuvaa, eikä kyse ollut get-funktiosta,
                           // ei voi tehdä mitään järkevää

  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  if (locationCache.NoValue()) return kFloatMissing;

  // 2. Käy läpi haluttu level korkeus/level väli ja laske haluttu operaatio niille
  FindCalculatedLeves(locationCache);

  if (itsStartLevelIndex == gMissingIndex || itsEndLevelIndex == gMissingIndex)
    return kFloatMissing;  // jos jompikumpi level indekseistä osoittaa 'kuuseen', ei kannata jatkaa

  itsFunctionModifier->Clear();

  // Lasketaan aikaloopitus rajat
  unsigned long origTimeIndex = itsInfo->TimeIndex();  // Otetaan aikaindeksi talteen, jotta se
                                                       // voidaan lopuksi palauttaa takaisin
  unsigned long startTimeIndex = origTimeIndex;
  unsigned long endTimeIndex = origTimeIndex;
  if (::CalcTimeLoopIndexies(itsInfo,
                             theCalculationParams,
                             itsStartTimeOffsetInHours,
                             itsEndTimeOffsetInHours,
                             &startTimeIndex,
                             &endTimeIndex))
  {
    for (unsigned long timeIndex = startTimeIndex; timeIndex <= endTimeIndex; timeIndex++)
    {
      itsInfo->TimeIndex(timeIndex);
      // for -loopin juoksutus kopsattu NFmiInfoAreaMaskVertFunc::DoNormalFunction -metodista
      for (int levelIndex = static_cast<int>(itsStartLevelIndex);
           fReverseLevels ? levelIndex >= static_cast<int>(itsEndLevelIndex)
                          : levelIndex <= static_cast<int>(itsEndLevelIndex);
           levelIndex += itsLevelIncrement)
      {
        itsInfo->LevelIndex(levelIndex);
        if (itsFunctionType == NFmiAreaMask::Max || itsFunctionType == NFmiAreaMask::Min)
          ::AddExtremeValues(itsInfo, itsFunctionModifier, theCalculationParams.itsLatlon);
        else
          itsFunctionModifier->Calculate(
              itsInfo->InterpolatedValue(theCalculationParams.itsLatlon));
      }
    }
    itsInfo->TimeIndex(origTimeIndex);
  }

  return itsFunctionModifier->CalculationResult();
}

// **********************************************************
// *****    NFmiInfoAreaMaskTimeVertFunc  *******************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskProbFunc   **********************
// **********************************************************

NFmiInfoAreaMaskProbFunc::~NFmiInfoAreaMaskProbFunc(void) {}
NFmiInfoAreaMaskProbFunc::NFmiInfoAreaMaskProbFunc(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    NFmiAreaMask::FunctionType thePrimaryFunc,
    NFmiAreaMask::FunctionType theSecondaryFunc,
    int theArgumentCount)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo),
      itsPrimaryFunc(thePrimaryFunc),
      itsSecondaryFunc(theSecondaryFunc),
      itsArgumentVector(),
      itsSearchRangeInKM(0),
      itsLimit1(kFloatMissing),
      itsLimit2(kFloatMissing),
      itsStartTimeOffsetInHours(0),
      itsEndTimeOffsetInHours(0),
      itsGridPointRectSizeX(0),
      itsGridPointRectSizeY(0),
      itsConditionFullfilledGridPointCount(0)
{
  itsFunctionArgumentCount = theArgumentCount;
}

NFmiInfoAreaMaskProbFunc::NFmiInfoAreaMaskProbFunc(const NFmiInfoAreaMaskProbFunc &theOther)
    : NFmiInfoAreaMask(theOther),
      itsPrimaryFunc(theOther.itsPrimaryFunc),
      itsSecondaryFunc(theOther.itsSecondaryFunc),
      itsArgumentVector(theOther.itsArgumentVector),
      itsSearchRangeInKM(theOther.itsSearchRangeInKM),
      itsLimit1(theOther.itsLimit1),
      itsLimit2(theOther.itsLimit2),
      itsStartTimeOffsetInHours(theOther.itsStartTimeOffsetInHours),
      itsEndTimeOffsetInHours(theOther.itsEndTimeOffsetInHours),
      itsGridPointRectSizeX(theOther.itsGridPointRectSizeX),
      itsGridPointRectSizeY(theOther.itsGridPointRectSizeY),
      itsConditionFullfilledGridPointCount(theOther.itsConditionFullfilledGridPointCount)
{
}

NFmiAreaMask *NFmiInfoAreaMaskProbFunc::Clone(void) const
{
  return new NFmiInfoAreaMaskProbFunc(*this);
}

void NFmiInfoAreaMaskProbFunc::Initialize(void) {}
// Tätä kutsutaan jokaiselle erillis pistelaskulle erikseen value-funktiossa.
void NFmiInfoAreaMaskProbFunc::InitializeFromArguments(void)
{
  itsSearchRangeInKM = itsArgumentVector[0];
  itsStartTimeOffsetInHours = itsArgumentVector[1];
  itsEndTimeOffsetInHours = itsArgumentVector[2];
  itsLimit1 = itsArgumentVector[3];
  itsLimit2 = kFloatMissing;  // Huom! tämä voi olla puuttuva, jos kyse on simppelistä
                              // get-funktiosta (esim. vertp_get(WS_Hir, pressure))
  if (itsArgumentVector.size() > 4) itsLimit2 = itsArgumentVector[4];

  itsGridPointRectSizeX = 0;
  itsGridPointRectSizeY = 0;
  // Laske mikä on annetun infon maailmassa annetun search-rangen mukaiset itsGridPointRectSizeX/Y:t
  if (itsSearchRangeInKM > 0 && itsSearchRangeInKM != kFloatMissing && itsInfo)
  {
    const NFmiGrid *grid = itsInfo->Grid();
    if (grid)
    {
      double gridSizeXInKM = grid->Area()->WorldXYWidth() / (grid->XNumber() - 1) * 0.001;
      itsGridPointRectSizeX = FmiRound(2. * itsSearchRangeInKM / gridSizeXInKM);
      itsGridPointRectSizeX = std::max(itsGridPointRectSizeX, 2);  // 2 on minimi hakulaatikon koko

      double gridSizeYInKM = grid->Area()->WorldXYHeight() / (grid->YNumber() - 1) * 0.001;
      itsGridPointRectSizeY = FmiRound(2. * itsSearchRangeInKM / gridSizeYInKM);
      itsGridPointRectSizeY = std::max(itsGridPointRectSizeY, 2);  // 2 on minimi hakulaatikon koko
    }
    else
    {
      // pitäisikö asemadatoille tehdä omat laskut perustuen pelkästään etäisyyksiin asemille?
    }
  }
}

void NFmiInfoAreaMaskProbFunc::SetArguments(std::vector<float> &theArgumentVector)
{
  // jokaiselle pisteelle ja ajanhetkelle annetaan eri argumentit tässä
  itsArgumentVector = theArgumentVector;
  if (static_cast<int>(itsArgumentVector.size()) !=
      itsFunctionArgumentCount - 1)  // -1 tarkoittaa että funktion 1. argumentti tulee suoraan
                                     // itsIfo:sta, eli sitä ei anneta argumentti-listassa
    throw std::runtime_error(
        "Internal SmartMet error: Probability function was given invalid number of arguments, "
        "cannot calculate the macro.");
}

static bool IsOdd(int value) { return value % 2 == 1; }
static void CalcPeekLoopLimits(int rectSize, double interpolationPoint, int &limit1, int &limit2)
{
  if (::IsOdd(rectSize))
  {
    // Pariton on helppo tapaus, looppi alkaa -x:sta ja menee x:ään ja x = (int)(theRectSize/2)
    limit2 = static_cast<int>(rectSize / 2);
    limit1 = -limit2;
  }
  else
  {
    // Parillinen tapaus on hankalampi. Pitää katsoa millä puolella rajaa ollaan lähimmästä
    // hilapisteestä pisteestä (= kokonaisluvusta) ja sen mukaan laskea eri arvot limiteille.
    if (interpolationPoint > FmiRound(interpolationPoint))
    {
      // Interpolointi piste on suurempi kuin nearest-point arvo, tällöin limit2 saa yhden suuremman
      // (itseis)arvon
      limit2 = static_cast<int>(rectSize / 2);
      limit1 = -(limit2 - 1);
    }
    else
    {
      // Interpolointi piste on pienempi kuin nearest-point arvo, tällöin limit1 saa yhden suuremman
      // (itseis)arvon
      limit1 = static_cast<int>(-rectSize / 2);
      limit2 = -limit1 - 1;
    }
  }
}

bool NFmiInfoAreaMaskProbFunc::CheckProbabilityCondition(double value)
{
  return ::CheckProbabilityCondition(itsPrimaryFunc, value, itsLimit1, itsLimit2);
}

// Etsii halutun aika loopituksen alku- ja loppuaika indeksejä annetusta infosta.
// Palauttaa mahdollisen interpolaatio ajan, jos startOffset ja endOffsetit ovat samoja, käytetään
// kyseistä aikaa aikainterpolaatiossa.
NFmiMetTime NFmiInfoAreaMaskProbFunc::CalcTimeLoopLimits(
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiCalculationParams &theCalculationParams,
    double theStartTimeOffsetInHours,
    double theEndTimeOffsetInHours,
    unsigned long *theStartTimeIndexOut,
    unsigned long *theEndTimeIndexOut,
    bool *doSpecialCalculation,
    bool allowInterpolation)
{
  NFmiMetTime interpolationTime = theCalculationParams.itsTime;
  *doSpecialCalculation = false;
  if (theInfo)
  {
    NFmiMetTime startTime = theCalculationParams.itsTime;
    startTime.ChangeByMinutes(FmiRound(theStartTimeOffsetInHours * 60));

    if (allowInterpolation && theStartTimeOffsetInHours == theEndTimeOffsetInHours)
      interpolationTime = startTime;
    else
    {
      unsigned long usedTimeRangeInMinutes = static_cast<unsigned long>(
          std::abs(theEndTimeOffsetInHours - theStartTimeOffsetInHours) * 60);
      bool foundStartTime = theInfo->FindNearestTime(startTime, kForward, usedTimeRangeInMinutes);
      *theStartTimeIndexOut = theInfo->TimeIndex();

      NFmiMetTime endTime = theCalculationParams.itsTime;
      endTime.ChangeByMinutes(FmiRound(theEndTimeOffsetInHours * 60));
      bool foundEndTime = theInfo->FindNearestTime(endTime, kBackward, usedTimeRangeInMinutes);
      *theEndTimeIndexOut = theInfo->TimeIndex();

      if (foundStartTime && !foundEndTime)
      {
        *theEndTimeIndexOut = *theStartTimeIndexOut;
        *doSpecialCalculation = true;
      }
      else if (!foundStartTime && foundEndTime)
      {
        *theStartTimeIndexOut = *theEndTimeIndexOut;
        *doSpecialCalculation = true;
      }
      else if (*theStartTimeIndexOut != gMissingIndex && *theEndTimeIndexOut == gMissingIndex)
      {  // alkuaika on datan sisällä, mutta loppuaika ei, laitetaan loppuaika viimeiseen
         // aika-askeleeseen
        *theEndTimeIndexOut = theInfo->SizeTimes() - 1;
        *doSpecialCalculation = true;
      }
    }
  }
  return interpolationTime;
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskProbFunc::Value(const NFmiCalculationParams &theCalculationParams,
                                       bool /* fUseTimeInterpolationAlways */)
{
  InitializeFromArguments();
  if (itsGridPointRectSizeX && itsGridPointRectSizeY)
  {
    NFmiLocationCache locCache = itsInfo->CalcLocationCache(theCalculationParams.itsLatlon);
    if (!locCache.NoValue())
    {
      itsConditionFullfilledGridPointCount = 0;  // Aloitetaan  osumat nollasta
      double calculatedGridPoints = 0;           // Kaikki hilapisteet, missä oli ei-puuttuva arvo
      NFmiLocation location(theCalculationParams.itsLatlon);

      // Lasketaan laatikon loopitus rajat, x1 on mistä x-suuntainen peek-indeksi
      // alkaa ja x2 mihin se loppuu (esim. -2 ja 2, jos laatkion x-koko on 5).
      // Sama periaate y1 ja y2:lla.
      int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
      ::CalcPeekLoopLimits(itsGridPointRectSizeX, locCache.itsGridPoint.X(), x1, x2);
      ::CalcPeekLoopLimits(itsGridPointRectSizeY, locCache.itsGridPoint.Y(), y1, y2);

      // Lasketaan aikaloopitus rajat
      unsigned long origTimeIndex = itsInfo->TimeIndex();  // Otetaan aikaindeksi talteen, jotta se
                                                           // voidaan lopuksi palauttaa takaisin
      unsigned long startTimeIndex = origTimeIndex;
      unsigned long endTimeIndex = origTimeIndex;
      bool doSpecialCalculation = false;
      NFmiMetTime interpolationTime =
          NFmiInfoAreaMaskProbFunc::CalcTimeLoopLimits(itsInfo,
                                                       theCalculationParams,
                                                       itsStartTimeOffsetInHours,
                                                       itsEndTimeOffsetInHours,
                                                       &startTimeIndex,
                                                       &endTimeIndex,
                                                       &doSpecialCalculation,
                                                       true);

      for (unsigned long timeIndex = startTimeIndex; timeIndex <= endTimeIndex; timeIndex++)
      {
        itsInfo->TimeIndex(timeIndex);
        // Jos käydään läpi yhtä ajan hetkeä, tehdään interpolaatio, ja käytetään originaali
        // laskenta aikaa.
        // Jos käydään läpi aikajakso datan originaali aikaresoluutiossa, käytetään infoon asetettua
        // aikaa
        bool useInterpolatedTime = (startTimeIndex == endTimeIndex);
        if (doSpecialCalculation) useInterpolatedTime = false;
        for (int offsetY = y1; offsetY <= y2; offsetY++)
        {
          for (int offsetX = x1; offsetX <= x2; offsetX++)
          {
            if (itsSecondaryFunc == NFmiAreaMask::ProbCircle)
            {  // ympyrä tapauksessa tarkastetaan jokainen piste erikseen, onko se halutun säteisen
               // ympyrän sisällä
              double distanceInKM =
                  location.Distance(itsInfo->PeekLocationLatLon(offsetX, offsetY)) * 0.001;
              if (distanceInKM > itsSearchRangeInKM)
                continue;  // kyseinen piste oli ympyrän ulkopuolella
            }

            float value = kFloatMissing;
            if (useInterpolatedTime)
              value = itsInfo->PeekLocationValue(offsetX, offsetY, interpolationTime);
            else
              value = itsInfo->PeekLocationValue(offsetX, offsetY);

            if (value != kFloatMissing)
            {
              calculatedGridPoints++;
              if (CheckProbabilityCondition(value)) itsConditionFullfilledGridPointCount++;
            }
          }
        }
        if (startTimeIndex == gMissingIndex || endTimeIndex == gMissingIndex ||
            startTimeIndex > endTimeIndex)
          break;  // time-looppi voi mennä gMissingIndex => gMissingIndex, jolloin ++-operaatio
                  // veisi luvun takaisin 0:aan, siksi tämä ehto ja loopin breikki
      }
      itsInfo->TimeIndex(origTimeIndex);

      if (calculatedGridPoints)
        return itsConditionFullfilledGridPointCount / calculatedGridPoints * 100.;
    }
  }

  itsConditionFullfilledGridPointCount =
      static_cast<int>(kFloatMissing);  // Jos hilamuotoisia laskuja ei voida suorittaa, laitetaan
                                        // osumien arvoksi missing

  return kFloatMissing;
}

// **********************************************************
// *****    NFmiInfoAreaMaskProbFunc   **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoAreaMaskTimeRange  **********************
// **********************************************************

NFmiInfoAreaMaskTimeRange::~NFmiInfoAreaMaskTimeRange(void) {}
NFmiInfoAreaMaskTimeRange::NFmiInfoAreaMaskTimeRange(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    NFmiAreaMask::FunctionType theIntegrationFunc,
    int theArgumentCount)
    : NFmiInfoAreaMask(theOperation, theMaskType, theDataType, theInfo),
      itsIntegrationFunc(theIntegrationFunc),
      itsFunctionModifier(),
      itsArgumentVector(),
      itsStartTimeOffsetInHours(0),
      itsEndTimeOffsetInHours(0)
{
  itsFunctionArgumentCount = theArgumentCount;
  itsFunctionModifier = ::CreateIntegrationFuction(itsIntegrationFunc);
}

NFmiInfoAreaMaskTimeRange::NFmiInfoAreaMaskTimeRange(const NFmiInfoAreaMaskTimeRange &theOther)
    : NFmiInfoAreaMask(theOther),
      itsIntegrationFunc(theOther.itsIntegrationFunc),
      itsFunctionModifier(theOther.itsFunctionModifier ? theOther.itsFunctionModifier->Clone() : 0),
      itsArgumentVector(theOther.itsArgumentVector),
      itsStartTimeOffsetInHours(theOther.itsStartTimeOffsetInHours),
      itsEndTimeOffsetInHours(theOther.itsEndTimeOffsetInHours)
{
}

NFmiAreaMask *NFmiInfoAreaMaskTimeRange::Clone(void) const
{
  return new NFmiInfoAreaMaskTimeRange(*this);
}

void NFmiInfoAreaMaskTimeRange::Initialize(void) {}
// Tätä kutsutaan jokaiselle erillis pistelaskulle erikseen value-funktiossa.
void NFmiInfoAreaMaskTimeRange::InitializeFromArguments(void)
{
  itsStartTimeOffsetInHours = itsArgumentVector[0];
  itsEndTimeOffsetInHours = itsArgumentVector[1];
}

void NFmiInfoAreaMaskTimeRange::SetArguments(std::vector<float> &theArgumentVector)
{
  // jokaiselle pisteelle ja ajanhetkelle annetaan eri argumentit tässä
  itsArgumentVector = theArgumentVector;
  if (static_cast<int>(itsArgumentVector.size()) !=
      itsFunctionArgumentCount - 1)  // -1 tarkoittaa että funktion 1. argumentti tulee suoraan
                                     // itsIfo:sta, eli sitä ei anneta argumentti-listassa
    throw std::runtime_error(
        "Internal SmartMet error: Probability function was given invalid number of arguments, "
        "cannot calculate the macro.");
}

// tätä kaytetaan smarttool-modifierin yhteydessä
double NFmiInfoAreaMaskTimeRange::Value(const NFmiCalculationParams &theCalculationParams,
                                        bool /* fUseTimeInterpolationAlways */)
{
  InitializeFromArguments();
  itsFunctionModifier->Clear();
  NFmiLocationCache locCache = itsInfo->CalcLocationCache(theCalculationParams.itsLatlon);
  if (!locCache.NoValue())
  {
    // Lasketaan aikaloopitus rajat
    unsigned long origTimeIndex = itsInfo->TimeIndex();  // Otetaan aikaindeksi talteen, jotta se
                                                         // voidaan lopuksi palauttaa takaisin
    unsigned long startTimeIndex = origTimeIndex;
    unsigned long endTimeIndex = origTimeIndex;
    if (::CalcTimeLoopIndexies(itsInfo,
                               theCalculationParams,
                               itsStartTimeOffsetInHours,
                               itsEndTimeOffsetInHours,
                               &startTimeIndex,
                               &endTimeIndex))
    {
      for (unsigned long timeIndex = startTimeIndex; timeIndex <= endTimeIndex; timeIndex++)
      {
        itsInfo->TimeIndex(timeIndex);
        if (itsIntegrationFunc == NFmiAreaMask::Max || itsIntegrationFunc == NFmiAreaMask::Min)
          ::AddExtremeValues(itsInfo, itsFunctionModifier, theCalculationParams.itsLatlon);
        else
          itsFunctionModifier->Calculate(
              itsInfo->InterpolatedValue(theCalculationParams.itsLatlon));
      }
      itsInfo->TimeIndex(origTimeIndex);
    }
  }

  return itsFunctionModifier->CalculationResult();
}
// **********************************************************
// *****    NFmiInfoAreaMaskTimeRange  **********************
// **********************************************************

// **********************************************************
// *****    NFmiInfoTimeIntegrator   ************************
// **********************************************************

NFmiInfoTimeIntegrator::~NFmiInfoTimeIntegrator(void) {}
NFmiInfoTimeIntegrator::NFmiInfoTimeIntegrator(const NFmiCalculationCondition &theOperation,
                                               Type theMaskType,
                                               NFmiInfoData::Type theDataType,
                                               const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                               NFmiAreaMask::FunctionType theIntegrationFunc,
                                               int theStartTimeOffset,
                                               int theEndTimeOffset)
    : NFmiInfoAreaMaskMetFuncBase(
          theOperation, theMaskType, theDataType, theInfo, false, NoDirection),
      itsIntegrationFunc(theIntegrationFunc),
      itsFunctionModifier(),
      itsStartTimeOffset(theStartTimeOffset),
      itsEndTimeOffset(theEndTimeOffset)
{
  itsFunctionModifier = ::CreateIntegrationFuction(itsIntegrationFunc);
}

NFmiInfoTimeIntegrator::NFmiInfoTimeIntegrator(const NFmiInfoTimeIntegrator &theOther)
    : NFmiInfoAreaMaskMetFuncBase(theOther),
      itsIntegrationFunc(theOther.itsIntegrationFunc),
      itsFunctionModifier(theOther.itsFunctionModifier ? theOther.itsFunctionModifier->Clone() : 0),
      itsStartTimeOffset(theOther.itsStartTimeOffset),
      itsEndTimeOffset(theOther.itsEndTimeOffset)
{
}

NFmiAreaMask *NFmiInfoTimeIntegrator::Clone(void) const
{
  return new NFmiInfoTimeIntegrator(*this);
}

double NFmiInfoTimeIntegrator::Value(const NFmiCalculationParams &theCalculationParams,
                                     bool /* fUseTimeInterpolationAlways */)
{
  if (itsTimeCache.NoValue())
    return kFloatMissing;  // jos mentiin jo originaalisti datan aikojen ulkopuolelle, ei voi mitään

  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  if (locationCache.NoValue())
    return kFloatMissing;  // jos mentiin datan alueen ulkopuolelle, palautetaan puuttuvaa

  unsigned long oldTimeIndex = itsInfo->TimeIndex();
  itsFunctionModifier->Clear();

  unsigned long startTimeIndex =
      itsTimeCache.itsOffset <= 0.5
          ? itsTimeCache.itsTimeIndex1
          : itsTimeCache.itsTimeIndex2;  // haetaan lähempi timeindeksi 'nolla'-kohdaksi
  int usedStartIndex = startTimeIndex - itsStartTimeOffset;
  int usedEndIndex = startTimeIndex + itsEndTimeOffset;
  for (int i = usedStartIndex; i < usedEndIndex; i++)
  {
    itsInfo->TimeIndex(i);
    float tmpValue = itsInfo->CachedInterpolation(locationCache);
    itsFunctionModifier->Calculate(tmpValue);
  }
  itsInfo->TimeIndex(oldTimeIndex);
  return itsFunctionModifier->CalculationResult();
}

// **********************************************************
// *****    NFmiInfoTimeIntegrator   ************************
// **********************************************************

// **********************************************************
// *****  NFmiInfoRectAreaIntegrator ************************
// **********************************************************

NFmiInfoRectAreaIntegrator::~NFmiInfoRectAreaIntegrator(void) {}
NFmiInfoRectAreaIntegrator::NFmiInfoRectAreaIntegrator(
    const NFmiCalculationCondition &theOperation,
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    NFmiAreaMask::FunctionType theIntegrationFunc,
    int theStartXOffset,
    int theEndXOffset,
    int theStartYOffset,
    int theEndYOffset)
    : NFmiInfoAreaMaskMetFuncBase(
          theOperation, theMaskType, theDataType, theInfo, false, NoDirection),
      itsIntegrationFunc(theIntegrationFunc),
      itsFunctionModifier(),
      itsStartXOffset(theStartXOffset),
      itsEndXOffset(theEndXOffset),
      itsStartYOffset(theStartYOffset),
      itsEndYOffset(theEndYOffset)
{
  itsFunctionModifier = ::CreateIntegrationFuction(itsIntegrationFunc);
}

NFmiInfoRectAreaIntegrator::NFmiInfoRectAreaIntegrator(const NFmiInfoRectAreaIntegrator &theOther)
    : NFmiInfoAreaMaskMetFuncBase(theOther),
      itsIntegrationFunc(theOther.itsIntegrationFunc),
      itsFunctionModifier(theOther.itsFunctionModifier ? theOther.itsFunctionModifier->Clone() : 0),
      itsStartXOffset(theOther.itsStartXOffset),
      itsEndXOffset(theOther.itsEndXOffset),
      itsStartYOffset(theOther.itsStartYOffset),
      itsEndYOffset(theOther.itsEndYOffset)
{
}

NFmiAreaMask *NFmiInfoRectAreaIntegrator::Clone(void) const
{
  return new NFmiInfoRectAreaIntegrator(*this);
}

double NFmiInfoRectAreaIntegrator::Value(const NFmiCalculationParams &theCalculationParams,
                                         bool /* fUseTimeInterpolationAlways */)
{
  if (itsTimeCache.NoValue())
    return kFloatMissing;  // jos mentiin originaalisti datan aikojen ulkopuolelle, ei voi mitään

  // 1. Laske latlon-pistettä vastaava 'reaali'-hilapiste.
  NFmiLocationCache locationCache = CalcLocationCache(theCalculationParams.itsLatlon);
  if (locationCache.NoValue())
    return kFloatMissing;  // jos mentiin datan alueen ulkopuolelle, palautetaan puuttuvaa

  itsFunctionModifier->Clear();

  for (int j = itsStartYOffset; j < itsEndYOffset; j++)
  {
    for (int i = itsStartXOffset; i < itsEndXOffset; i++)
    {
      itsFunctionModifier->Calculate(Peek(locationCache, i, j));
    }
  }
  return itsFunctionModifier->CalculationResult();
}

// **********************************************************
// *****  NFmiInfoRectAreaIntegrator ************************
// **********************************************************
