//**********************************************************
// C++ Class Name : NFmiSmartToolModifier
// ---------------------------------------------------------
//  Author         : pietarin
//  Creation Date  : 8.11. 2010
//**********************************************************
#ifdef _MSC_VER
#pragma warning(disable : 4786)  // poistaa n kpl VC++ kääntäjän varoitusta
#endif

#include "NFmiSmartToolModifier.h"

#include "NFmiAreaMaskInfo.h"
#include "NFmiAreaMaskSectionInfo.h"
#include "NFmiCalculationConstantValue.h"
#include "NFmiDictionaryFunction.h"
#include "NFmiDrawParam.h"
#include "NFmiExtraMacroParamData.h"
#include "NFmiInfoAreaMaskOccurrance.h"
#include "NFmiInfoAreaMaskSoundingIndex.h"
#include "NFmiInfoOrganizer.h"
#include "NFmiLocalAreaMinMaxMask.h"
#include "NFmiMetEditorTypes.h"
#include "NFmiSimpleConditionInfo.h"
#include "NFmiSmartInfo.h"
#include "NFmiSmartToolCalculation.h"
#include "NFmiSmartToolCalculationInfo.h"
#include "NFmiSmartToolCalculationSection.h"
#include "NFmiSmartToolCalculationSectionInfo.h"
#include "NFmiSmartToolIntepreter.h"

#include <boost/math/special_functions/round.hpp>
#include <newbase/NFmiBitMask.h>
#include <newbase/NFmiCalculatedAreaMask.h>
#include <newbase/NFmiDataModifierClasses.h>
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiInfoAreaMask.h>
#include <newbase/NFmiQueryData.h>
#include <newbase/NFmiQueryDataUtil.h>
#include <newbase/NFmiRelativeDataIterator.h>
#include <newbase/NFmiRelativeTimeIntegrationIterator.h>
#include <newbase/NFmiSimpleCondition.h>

#include <stdexcept>

#ifdef _MSC_VER
#pragma warning( \
    disable : 4244 4267 4512)  // boost:in thread kirjastosta tulee ikävästi 4244 varoituksia
#endif
#include "NFmiStation2GridMask.h"

#include <boost/thread.hpp>

#ifdef _MSC_VER
#pragma warning(default : 4244 4267 4512)  // laitetaan 4244 takaisin päälle, koska se on tärkeä
                                           // (esim. double -> int auto castaus varoitus)
#endif

using namespace std;

namespace
{
// Tähän laitetaan talteen multi-thread funktioissa olevat poikkeus viestit, jotta sanoma saadaan
// ulos SmartMetille. Asetus ja pyynti funktiot pitää tehdä threadi turvallisiksi, crash-reporteista
// on näkynyt monia kaatumisia, kun tähän on asetettu 'raakana' tekstiä.
std::string g_lastExceptionMessageFromThreads_dontUseDirectly;
std::mutex g_LastExceptionMessageMutex;

void SetLastExceptionMessage(const std::string &message)
{
  std::lock_guard<std::mutex> lock(g_LastExceptionMessageMutex);
  g_lastExceptionMessageFromThreads_dontUseDirectly = message;
}

std::string GetLastExceptionMessage()
{
  std::lock_guard<std::mutex> lock(g_LastExceptionMessageMutex);
  return g_lastExceptionMessageFromThreads_dontUseDirectly;
}

void ClearLastExceptionMessage()
{
  std::lock_guard<std::mutex> lock(g_LastExceptionMessageMutex);
  g_lastExceptionMessageFromThreads_dontUseDirectly.clear();
}

}  // namespace

static std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> DoShallowCopy(
    const std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> &theCalculationBlockVector)
{
  std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> returnVector(
      theCalculationBlockVector.size());
  for (size_t i = 0; i < theCalculationBlockVector.size(); i++)
    returnVector[i] = boost::shared_ptr<NFmiSmartToolCalculationBlock>(
        new NFmiSmartToolCalculationBlock(*theCalculationBlockVector[i]));
  return returnVector;
}

NFmiSmartToolCalculationBlockVector::NFmiSmartToolCalculationBlockVector() : itsCalculationBlocks()
{
}

NFmiSmartToolCalculationBlockVector::NFmiSmartToolCalculationBlockVector(
    const NFmiSmartToolCalculationBlockVector &theOther)
    : itsCalculationBlocks(::DoShallowCopy(theOther.itsCalculationBlocks))
{
}

NFmiSmartToolCalculationBlockVector::~NFmiSmartToolCalculationBlockVector() {}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolCalculationBlockVector::FirstVariableInfo()
{
  Iterator it = Begin();
  Iterator endIt = End();
  for (; it != endIt; ++it)
    if ((*it)->FirstVariableInfo())  // pitäisi löytyä aina jotain!!!
      return (*it)->FirstVariableInfo();
  return boost::shared_ptr<NFmiFastQueryInfo>();
}

void NFmiSmartToolCalculationBlockVector::SetTime(const NFmiMetTime &theTime)
{
  Iterator it = Begin();
  Iterator endIt = End();
  for (; it != endIt; ++it)
    (*it)->Time(theTime);
}

void NFmiSmartToolCalculationBlockVector::Calculate(
    const NFmiCalculationParams &theCalculationParams, NFmiMacroParamValue &theMacroParamValue)
{
  Iterator it = Begin();
  Iterator endIt = End();
  for (; it != endIt; ++it)
    (*it)->Calculate(theCalculationParams, theMacroParamValue);
}

void NFmiSmartToolCalculationBlockVector::Calculate_ver2(
    const NFmiCalculationParams &theCalculationParams)
{
  Iterator it = Begin();
  Iterator endIt = End();
  for (; it != endIt; ++it)
    (*it)->Calculate_ver2(theCalculationParams);
}

void NFmiSmartToolCalculationBlockVector::Add(
    const boost::shared_ptr<NFmiSmartToolCalculationBlock> &theBlock)
{  // ottaa omistukseen theBlock:in!!
  itsCalculationBlocks.push_back(theBlock);
}

NFmiSmartToolCalculationBlock::NFmiSmartToolCalculationBlock()
    : itsFirstCalculationSection(),
      itsIfAreaMaskSection(),
      itsIfCalculationBlocks(),
      itsElseIfAreaMaskSection(),
      itsElseIfCalculationBlocks(),
      itsElseCalculationBlocks(),
      itsLastCalculationSection()
{
}

NFmiSmartToolCalculationBlock::NFmiSmartToolCalculationBlock(
    const NFmiSmartToolCalculationBlock &theOther)
    : itsFirstCalculationSection(
          theOther.itsFirstCalculationSection
              ? new NFmiSmartToolCalculationSection(*theOther.itsFirstCalculationSection)
              : 0),
      itsIfAreaMaskSection(theOther.itsIfAreaMaskSection
                               ? new NFmiSmartToolCalculation(*theOther.itsIfAreaMaskSection)
                               : 0),
      itsIfCalculationBlocks(
          theOther.itsIfCalculationBlocks
              ? new NFmiSmartToolCalculationBlockVector(*theOther.itsIfCalculationBlocks)
              : 0),
      itsElseIfAreaMaskSection(
          theOther.itsElseIfAreaMaskSection
              ? new NFmiSmartToolCalculation(*theOther.itsElseIfAreaMaskSection)
              : 0),
      itsElseIfCalculationBlocks(
          theOther.itsElseIfCalculationBlocks
              ? new NFmiSmartToolCalculationBlockVector(*theOther.itsElseIfCalculationBlocks)
              : 0),
      itsElseCalculationBlocks(
          theOther.itsElseCalculationBlocks
              ? new NFmiSmartToolCalculationBlockVector(*theOther.itsElseCalculationBlocks)
              : 0),
      itsLastCalculationSection(
          theOther.itsLastCalculationSection
              ? new NFmiSmartToolCalculationSection(*theOther.itsLastCalculationSection)
              : 0)
{
}

NFmiSmartToolCalculationBlock::~NFmiSmartToolCalculationBlock() {}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolCalculationBlock::FirstVariableInfo()
{
  boost::shared_ptr<NFmiFastQueryInfo> info;
  if (itsFirstCalculationSection)
    info = itsFirstCalculationSection->FirstVariableInfo();
  if (info == 0 && itsIfCalculationBlocks)
    info = itsIfCalculationBlocks->FirstVariableInfo();
  if (info == 0 && itsElseIfCalculationBlocks)
    info = itsElseIfCalculationBlocks->FirstVariableInfo();
  if (info == 0 && itsElseCalculationBlocks)
    info = itsElseCalculationBlocks->FirstVariableInfo();
  if (info == 0 && itsLastCalculationSection)
    info = itsLastCalculationSection->FirstVariableInfo();
  return info;
}

void NFmiSmartToolCalculationBlock::Time(const NFmiMetTime &theTime)
{
  if (itsFirstCalculationSection)
    itsFirstCalculationSection->SetTime(theTime);

  if (itsIfAreaMaskSection)
    itsIfAreaMaskSection->Time(theTime);
  if (itsIfCalculationBlocks)
    itsIfCalculationBlocks->SetTime(theTime);
  if (itsElseIfAreaMaskSection)
    itsElseIfAreaMaskSection->Time(theTime);
  if (itsElseIfCalculationBlocks)
    itsElseIfCalculationBlocks->SetTime(theTime);
  if (itsElseCalculationBlocks)
    itsElseCalculationBlocks->SetTime(theTime);

  if (itsLastCalculationSection)
    itsLastCalculationSection->SetTime(theTime);
}

void NFmiSmartToolCalculationBlock::Calculate(const NFmiCalculationParams &theCalculationParams,
                                              NFmiMacroParamValue &theMacroParamValue)
{
  if (itsFirstCalculationSection)
    itsFirstCalculationSection->Calculate(theCalculationParams, theMacroParamValue);

  if (itsIfAreaMaskSection && itsIfAreaMaskSection->IsMasked(theCalculationParams))
    itsIfCalculationBlocks->Calculate(theCalculationParams, theMacroParamValue);
  else if (itsElseIfAreaMaskSection && itsElseIfAreaMaskSection->IsMasked(theCalculationParams))
    itsElseIfCalculationBlocks->Calculate(theCalculationParams, theMacroParamValue);
  else if (itsElseCalculationBlocks)
    itsElseCalculationBlocks->Calculate(theCalculationParams, theMacroParamValue);

  if (itsLastCalculationSection)
    itsLastCalculationSection->Calculate(theCalculationParams, theMacroParamValue);
}

void NFmiSmartToolCalculationBlock::Calculate_ver2(
    const NFmiCalculationParams &theCalculationParams, bool fDoMiddlePartOnly)
{
  if (fDoMiddlePartOnly == false)
  {
    if (itsFirstCalculationSection)
      itsFirstCalculationSection->Calculate_ver2(theCalculationParams);
  }

  if (itsIfAreaMaskSection && itsIfAreaMaskSection->IsMasked(theCalculationParams))
    itsIfCalculationBlocks->Calculate_ver2(theCalculationParams);
  else if (itsElseIfAreaMaskSection && itsElseIfAreaMaskSection->IsMasked(theCalculationParams))
    itsElseIfCalculationBlocks->Calculate_ver2(theCalculationParams);
  else if (itsElseCalculationBlocks)
    itsElseCalculationBlocks->Calculate_ver2(theCalculationParams);

  if (fDoMiddlePartOnly == false)
  {
    if (itsLastCalculationSection)
      itsLastCalculationSection->Calculate_ver2(theCalculationParams);
  }
}

bool NFmiSmartToolModifier::fUseVisualizationOptimazation = false;

//--------------------------------------------------------
// Constructor/Destructor
//--------------------------------------------------------
NFmiSmartToolModifier::NFmiSmartToolModifier(NFmiInfoOrganizer *theInfoOrganizer)
    : itsInfoOrganizer(theInfoOrganizer),
      itsSmartToolIntepreter(new NFmiSmartToolIntepreter(0)),
      fModifySelectedLocationsOnly(false),
      itsIncludeDirectory(),
      itsModifiedTimes(0),
      fMacroParamCalculation(false),
      fHeightFunctionFlag(false),
      fUseLevelData(false),
      fDoCrossSectionCalculation(false),
      itsCommaCounter(0),
      itsParethesisCounter(0),
      itsWorkingGrid(new MyGrid()),
      itsModifiedLevel(),
      itsGriddingHelper(0)
{
}
NFmiSmartToolModifier::~NFmiSmartToolModifier() {}
//--------------------------------------------------------
// InitSmartTool
//--------------------------------------------------------
// Tulkitsee macron, luo tavittavat systeemit, että makro voidaan suorittaa.
void NFmiSmartToolModifier::InitSmartTool(const std::string &theSmartToolText,
                                          bool fThisIsMacroParamSkript)
{
  fMacroRunnable = true;
  itsErrorText = "";
  try
  {
    itsSmartToolIntepreter->IncludeDirectory(itsIncludeDirectory);
    itsSmartToolIntepreter->Interpret(theSmartToolText, fThisIsMacroParamSkript);
    itsExtraMacroParamData = itsSmartToolIntepreter->ExtraMacroParamData();
    itsExtraMacroParamData.FinalizeData(*itsInfoOrganizer);
  }
  catch (...)
  {
    fMacroRunnable = false;
    throw;
  }
}

void NFmiSmartToolModifier::GetExtraMacroParamDataFromIntepreter()
{
  itsExtraMacroParamData = itsSmartToolIntepreter->ExtraMacroParamData();
}

void NFmiSmartToolModifier::InitSmartToolForMacroParam(
    const std::string &theSmartToolText,
    boost::shared_ptr<NFmiFastQueryInfo> &possibleSpacedOutMacroInfo,
    boost::shared_ptr<NFmiArea> &mapViewArea,
    bool doProbing,
    const NFmiPoint &spaceOutSkipFactors)
{
  fMacroRunnable = true;
  itsErrorText = "";
  try
  {
    itsSmartToolIntepreter->IncludeDirectory(itsIncludeDirectory);
    itsSmartToolIntepreter->Interpret(theSmartToolText, true);
    GetExtraMacroParamDataFromIntepreter();
    // Näitä kutsutaan vasta kun smarttool skripti on tulkittu
    itsExtraMacroParamData.FinalizeData(*itsInfoOrganizer);
    SetPossibleSpacedOutMacroInfo(possibleSpacedOutMacroInfo);
    SetUsedMapViewArea(mapViewArea);
    DoFixedDataSetup(doProbing, spaceOutSkipFactors);
  }
  catch (...)
  {
    fMacroRunnable = false;
    throw;
  }
}

void NFmiSmartToolModifier::DoFixedDataSetup(bool doProbing, const NFmiPoint &spaceOutSkipFactors)
{
  if (doProbing)
  {
    if (itsPossibleFixedBaseMacroParamData && itsExtraMacroParamData.FixedBaseDataInfo() ||
        itsExtraMacroParamData.WantedFixedBaseData().IsInUse())
    {
      itsExtraMacroParamData.IsFixedSpacedOutDataCase(true);
    }
  }
  else
  {
    MakePossibleFixedBaseData(spaceOutSkipFactors);
  }
}

boost::shared_ptr<NFmiSmartToolCalculationBlockVector>
NFmiSmartToolModifier::CreateCalculationBlockVector(
    const boost::shared_ptr<NFmiSmartToolCalculationBlockInfoVector> &theBlockInfoVector)
{
  if (theBlockInfoVector && (!theBlockInfoVector->Empty()))
  {
    boost::shared_ptr<NFmiSmartToolCalculationBlockVector> vec(
        new NFmiSmartToolCalculationBlockVector());
    NFmiSmartToolCalculationBlockInfoVector::Iterator it = theBlockInfoVector->Begin();
    NFmiSmartToolCalculationBlockInfoVector::Iterator endIt = theBlockInfoVector->End();
    for (; it != endIt; ++it)
      vec->Add(CreateCalculationBlock(*((*it).get())));
    return vec;
  }
  return boost::shared_ptr<NFmiSmartToolCalculationBlockVector>();
}

boost::shared_ptr<NFmiSmartToolCalculationBlock> NFmiSmartToolModifier::CreateCalculationBlock(
    NFmiSmartToolCalculationBlockInfo &theBlockInfo)
{
  boost::shared_ptr<NFmiSmartToolCalculationBlock> block(new NFmiSmartToolCalculationBlock());

  block->itsFirstCalculationSection =
      CreateCalculationSection(theBlockInfo.itsFirstCalculationSectionInfo);
  block->itsIfAreaMaskSection = CreateConditionalSection(theBlockInfo.itsIfAreaMaskSectionInfo);
  if (block->itsIfAreaMaskSection)
  {
    block->itsIfCalculationBlocks =
        CreateCalculationBlockVector(theBlockInfo.itsIfCalculationBlockInfos);
    if (!block->itsIfCalculationBlocks)
    {
      string errorText(::GetDictionaryString("SmartToolModifierErrorIfClause"));
      throw runtime_error(errorText);
    }
    block->itsElseIfAreaMaskSection =
        CreateConditionalSection(theBlockInfo.itsElseIfAreaMaskSectionInfo);
    if (block->itsElseIfAreaMaskSection)
    {
      block->itsElseIfCalculationBlocks =
          CreateCalculationBlockVector(theBlockInfo.itsElseIfCalculationBlockInfos);
      if (!block->itsElseIfCalculationBlocks)
      {
        string errorText(::GetDictionaryString("SmartToolModifierErrorElseIfClause"));
        throw runtime_error(errorText);
      }
    }
    if (theBlockInfo.fElseSectionExist)
    {
      block->itsElseCalculationBlocks =
          CreateCalculationBlockVector(theBlockInfo.itsElseCalculationBlockInfos);
      if (!block->itsElseCalculationBlocks)
      {
        string errorText(::GetDictionaryString("SmartToolModifierErrorElseClause"));
        throw runtime_error(errorText);
      }
    }
  }
  block->itsLastCalculationSection =
      CreateCalculationSection(theBlockInfo.itsLastCalculationSectionInfo);
  return block;
}

boost::shared_ptr<NFmiSmartToolCalculation> NFmiSmartToolModifier::CreateConditionalSection(
    const boost::shared_ptr<NFmiAreaMaskSectionInfo> &theAreaMaskSectionInfo)
{
  boost::shared_ptr<NFmiSmartToolCalculation> areaMaskHandler;
  if (theAreaMaskSectionInfo)
  {
    std::vector<boost::shared_ptr<NFmiAreaMaskInfo>> &maskInfos =
        theAreaMaskSectionInfo->GetAreaMaskInfoVector();
    size_t size = maskInfos.size();
    if (size)
    {
      areaMaskHandler = boost::shared_ptr<NFmiSmartToolCalculation>(new NFmiSmartToolCalculation());
      areaMaskHandler->SetCalculationText(theAreaMaskSectionInfo->GetCalculationText());
      for (size_t i = 0; i < size; i++)
        // HUOM!!!! editoitavaN DATAN QDatasta pitää tehdä kopiot, muuten maskit eivät toimi
        // kaikissa tilanteissa oikein!! KORJAA TÄMÄ!!!!!
        areaMaskHandler->AddCalculation(CreateAreaMask(*maskInfos[i]));
      // loppuun lisätään vielä lopetus 'merkki'
      areaMaskHandler->AddCalculation(CreateEndingAreaMask());

      return areaMaskHandler;
    }
  }
  return areaMaskHandler;
}

boost::shared_ptr<NFmiSmartToolCalculationSection> NFmiSmartToolModifier::CreateCalculationSection(
    const boost::shared_ptr<NFmiSmartToolCalculationSectionInfo> &theCalcSectionInfo)
{
  boost::shared_ptr<NFmiSmartToolCalculationSection> section;
  if (theCalcSectionInfo)
  {
    std::vector<boost::shared_ptr<NFmiSmartToolCalculationInfo>> &calcInfos =
        theCalcSectionInfo->GetCalculationInfos();
    size_t size = calcInfos.size();
    if (size)
    {
      section =
          boost::shared_ptr<NFmiSmartToolCalculationSection>(new NFmiSmartToolCalculationSection());
      for (size_t i = 0; i < size; i++)
      {
        section->AddCalculations(CreateCalculation(calcInfos[i]));
      }
      return section;
    }
  }
  return section;
}

boost::shared_ptr<NFmiSmartToolCalculation> NFmiSmartToolModifier::CreateCalculation(
    const boost::shared_ptr<NFmiSmartToolCalculationInfo> &theCalcInfo)
{
  boost::shared_ptr<NFmiSmartToolCalculation> calculation;
  size_t size = theCalcInfo->GetCalculationOperandInfoVector().size();
  if (size)
  {
    std::vector<boost::shared_ptr<NFmiAreaMaskInfo>> &areaMaskInfos =
        theCalcInfo->GetCalculationOperandInfoVector();
    calculation = boost::shared_ptr<NFmiSmartToolCalculation>(new NFmiSmartToolCalculation());
    calculation->SetCalculationText(theCalcInfo->GetCalculationText());
    bool mustUsePressureInterpolation =
        false;  // tätäei käytetä tässä, mutta pakko laittaa metodin interfacen takia
    calculation->SetResultInfo(
        CreateInfo(*theCalcInfo->GetResultDataInfo(), mustUsePressureInterpolation));
    if (calculation->GetResultInfo() && calculation->GetResultInfo()->Grid())
      itsWorkingGrid = boost::shared_ptr<MyGrid>(
          new MyGrid(*calculation->GetResultInfo()
                          ->Grid()));  // tämä työskentely alueen hila ja area otettava talteen

    float lowerLimit = kFloatMissing;
    float upperLimit = kFloatMissing;
    bool checkLimits = true;  // yleensä parametreille käytetdään min/max rajoja, mutta ei esim
                              // TotalWind tai W&C:lle
    GetParamValueLimits(*theCalcInfo->GetResultDataInfo(), &lowerLimit, &upperLimit, &checkLimits);
    calculation->SetLimits(lowerLimit, upperLimit, checkLimits);
    calculation->AllowMissingValueAssignment(theCalcInfo->AllowMissingValueAssignment());
    for (size_t i = 0; i < size; i++)
    {
      if (areaMaskInfos[i] != 0)
        calculation->AddCalculation(
            CreateAreaMask(*areaMaskInfos[i]));  // HUOM! TÄHÄN KAATUU JOSKUS, TUTKI ASIAA!!!!!
      else
      {
        string errStr(
            "Error in application: NFmiSmartToolModifier::CreateCalculation - zero pointer error "
            "with calculation\n");
        errStr += theCalcInfo->GetCalculationText();
        errStr += "\nNotify application developer with this.";
        throw runtime_error(errStr);
      }
    }
    // loppuun lisätään vielä loputus 'merkki'
    calculation->AddCalculation(CreateEndingAreaMask());
    return calculation;
  }
  return calculation;
}

static size_t CalcMatrixToVectorIndex(size_t xIndex,
                                      size_t yIndex,
                                      const NFmiDataMatrix<float> &matrix)
{
  return xIndex + yIndex * matrix.NX();
}

// DataMatrix => vector täyttöjärjestys on x-suuntainen rivi kerrallaan alhaalta ylös.
// Latlon pisteistä saadaan x-dimensio ja paine vektorista y-dimensio.
static std::vector<NFmiMacroParamValue> MakeMacroParamValueVectorForCrossSection(
    const NFmiDataMatrix<float> &theValues,
    const std::vector<float> &thePressures,
    const std::vector<NFmiPoint> &theLatlonPoints,
    const std::vector<NFmiMetTime> &thePointTimes)
{
  size_t sizeX = theLatlonPoints.size();
  size_t sizeY = thePressures.size();
  std::vector<NFmiMacroParamValue> macroParamValueVector(sizeX * sizeY);

  NFmiMacroParamValue macroParamValue;
  macroParamValue.fSetValue = true;
  macroParamValue.fDoCrossSectionCalculations = true;

  // lasketaan läpi yksittäisiä arvoja kaikille halutuille pisteille
  for (size_t i = 0; i < sizeX; i++)
  {
    macroParamValue.itsLatlon = theLatlonPoints[i];
    macroParamValue.itsTime = thePointTimes[i];
    for (size_t j = 0; j < sizeY; j++)
    {
      macroParamValue.itsPressureHeight = thePressures[j];
      macroParamValueVector[::CalcMatrixToVectorIndex(i, j, theValues)] = macroParamValue;
    }
  }
  return macroParamValueVector;
}

// Vastaava täyttö Timeserial tapauksessa on simppeli, theTimes -> macroParamValueVector täyttö
// menee suoraan vektorista vektoriin.
static std::vector<NFmiMacroParamValue> MakeMacroParamValueVectorForTimeSerial(
    const NFmiPoint &theLatlonPoint, const std::vector<NFmiMetTime> &theTimes)
{
  auto timeSize = theTimes.size();
  std::vector<NFmiMacroParamValue> macroParamValueVector(timeSize);

  NFmiMacroParamValue macroParamValue;
  macroParamValue.fSetValue = true;
  macroParamValue.fDoTimeSerialCalculations = true;
  macroParamValue.itsLatlon = theLatlonPoint;

  // lasketaan läpi yksittäisiä arvoja kaikille halutuille ajoille haluttuun pisteeseen
  for (size_t i = 0; i < timeSize; i++)
  {
    macroParamValue.itsTime = theTimes[i];
    macroParamValueVector[i] = macroParamValue;
  }
  return macroParamValueVector;
}

static void FillMatrixFromMacroParamValueVector(
    NFmiDataMatrix<float> &theValues, std::vector<NFmiMacroParamValue> &macroParamValueVector)
{
  for (size_t i = 0; i < macroParamValueVector.size(); i++)
  {
    size_t xIndex = i % theValues.NX();
    size_t yIndex = i / theValues.NX();
    theValues[xIndex][yIndex] = macroParamValueVector[i].itsValue;
  }
}

static void FillVectorFromMacroParamValueVector(
    std::vector<float> &theValues, std::vector<NFmiMacroParamValue> &macroParamValueVector)
{
  auto valueSize = macroParamValueVector.size();
  theValues.resize(valueSize, kFloatMissing);
  for (size_t i = 0; i < valueSize; i++)
  {
    theValues[i] = macroParamValueVector[i].itsValue;
  }
}

// Laskee smarttool-systeemin avulla halutun poikkileikkauksen arvo-matriisin.
// Tämä on universaali metodi, joka hoitaa kaikki kolme tapausta: poikkileikkaus,
// aika-poikkileikkaus ja reitti-poikkileikkaus.
// Parametreina annetaa aina sopiva pisteet ja ajat, että saadaan eri efektit aikaan. Eli
// aika-poikkileikkauksessa
// kaikki pisteet ovat samoja, normaali poikkileikkauksessa kaikki ajat ovat samoja ja reitti
// versiossa ajat ja pisteet muuttuvat.
void NFmiSmartToolModifier::CalcCrossSectionSmartToolValues(
    NFmiDataMatrix<float> &theValues,
    std::vector<float> &thePressures,
    std::vector<NFmiPoint> &theLatlonPoints,
    const std::vector<NFmiMetTime> &thePointTimes)
{
  fDoCrossSectionCalculation = true;
  size_t sizeX = theLatlonPoints.size();
  if (sizeX != thePointTimes.size())
    throw runtime_error(
        "NFmiSmartToolModifier::CalcCrossSectionSmartToolValues - latlon point count and time "
        "count must be the same, Error in program.");
  size_t sizeY = thePressures.size();
  if (sizeX < 1 || sizeY < 1)
    throw runtime_error(
        "NFmiSmartToolModifier::CalcCrossSectionSmartToolValues - invalid data-matrix size, Error "
        "in program.");
  theValues.Resize(sizeX, sizeY, kFloatMissing);
  auto macroParamValueVector = ::MakeMacroParamValueVectorForCrossSection(
      theValues, thePressures, theLatlonPoints, thePointTimes);
  NFmiTimeBag dummyTimeBag(thePointTimes[0], thePointTimes[0], 60);
  NFmiTimeDescriptor dummyTimes(thePointTimes[0], dummyTimeBag);
  ModifyData_ver2(&dummyTimes, false, true, nullptr, &macroParamValueVector);
  ::FillMatrixFromMacroParamValueVector(theValues, macroParamValueVector);
}

void NFmiSmartToolModifier::CalcTimeSerialSmartToolValues(std::vector<float> &theValues,
                                                          const NFmiPoint &theLatlonPoint,
                                                          const std::vector<NFmiMetTime> &theTimes)
{
  fDoTimeSerialCalculation = true;
  if (theTimes.size() < 1)
    throw runtime_error(
        "NFmiSmartToolModifier::CalcTimeSerialSmartToolValues - invalid time vector size (0), "
        "Error "
        "in program.");
  theValues.resize(theTimes.size(), kFloatMissing);
  auto macroParamValueVector = ::MakeMacroParamValueVectorForTimeSerial(theLatlonPoint, theTimes);
  NFmiTimeBag dummyTimeBag(theTimes[0], theTimes[0], 60);
  NFmiTimeDescriptor dummyTimes(theTimes[0], dummyTimeBag);
  ModifyData_ver2(&dummyTimes, false, true, nullptr, &macroParamValueVector);
  ::FillVectorFromMacroParamValueVector(theValues, macroParamValueVector);
}

float NFmiSmartToolModifier::CalcSmartToolValue(NFmiMacroParamValue &theMacroParamValue)
{
  NFmiTimeBag validTimes(theMacroParamValue.itsTime, theMacroParamValue.itsTime, 60);
  NFmiTimeDescriptor times(validTimes, theMacroParamValue.itsTime);
  // oikeasti tässä ei modifioida mitään, vaan palautetaan vain yksi arvo
  ModifyData(&times, false, true, theMacroParamValue, 0);
  return theMacroParamValue.itsValue;
}

float NFmiSmartToolModifier::CalcSmartToolValue(const NFmiMetTime &theTime,
                                                const NFmiPoint &theLatlon)
{
  NFmiMacroParamValue macroParamValue;
  macroParamValue.fSetValue = true;
  macroParamValue.itsLatlon = theLatlon;
  macroParamValue.itsTime = theTime;
  return CalcSmartToolValue(macroParamValue);
}

//--------------------------------------------------------
// ModifyData
//--------------------------------------------------------
// Suorittaa varsinaiset modifikaatiot. Käyttäjä voi antaa parametrina rajoitetun ajan
// modifioinneille, jos theModifiedTimes on 0-pointteri, tehdään operaatiot kaikille
// datan ajoille.
void NFmiSmartToolModifier::ModifyData(NFmiTimeDescriptor *theModifiedTimes,
                                       bool fSelectedLocationsOnly,
                                       bool isMacroParamCalculation,
                                       NFmiThreadCallBacks *theThreadCallBacks)
{
  NFmiMacroParamValue macroParamValue;
  ModifyData(theModifiedTimes,
             fSelectedLocationsOnly,
             isMacroParamCalculation,
             macroParamValue,
             theThreadCallBacks);
}

// tässä lasketaan jos käytössä on progress-dialogi, smarttool:in kokonais steppi määrä ja asetetaan
// se dialogille
static void CalcTotalProgressStepCount(
    std::vector<NFmiSmartToolCalculationBlockInfo> &theCalcInfoVector,
    NFmiTimeDescriptor *theModifiedTimes,
    NFmiThreadCallBacks *theThreadCallBacks)
{
  if (theThreadCallBacks && theModifiedTimes)
  {
    int sizeTimes = static_cast<int>(theModifiedTimes->Size());
    int totalStepCount = 0;
    for (size_t i = 0; i < theCalcInfoVector.size(); i++)
    {
      NFmiSmartToolCalculationBlockInfo &blockInfo = theCalcInfoVector[i];
      if (blockInfo.itsFirstCalculationSectionInfo)
        totalStepCount += static_cast<int>(
            blockInfo.itsFirstCalculationSectionInfo->GetCalculationInfos().size() * sizeTimes);
      if (blockInfo.itsIfAreaMaskSectionInfo &&
          blockInfo.itsIfAreaMaskSectionInfo->GetAreaMaskInfoVector().size())
        totalStepCount += sizeTimes;
      if (blockInfo.itsLastCalculationSectionInfo)
        totalStepCount += static_cast<int>(
            blockInfo.itsLastCalculationSectionInfo->GetCalculationInfos().size() * sizeTimes);
    }
    NFmiQueryDataUtil::SetRange(theThreadCallBacks, 0, totalStepCount, 1);
  }
}

void NFmiSmartToolModifier::ModifyData(NFmiTimeDescriptor *theModifiedTimes,
                                       bool fSelectedLocationsOnly,
                                       bool isMacroParamCalculation,
                                       NFmiMacroParamValue &theMacroParamValue,
                                       NFmiThreadCallBacks *theThreadCallBacks)
{
  itsModifiedTimes = theModifiedTimes;
  fMacroParamCalculation = isMacroParamCalculation;
  fModifySelectedLocationsOnly = fSelectedLocationsOnly;
  try
  {
    std::vector<NFmiSmartToolCalculationBlockInfo> &smartToolCalculationBlockInfos =
        itsSmartToolIntepreter->SmartToolCalculationBlocks();
    ::CalcTotalProgressStepCount(
        smartToolCalculationBlockInfos, theModifiedTimes, theThreadCallBacks);
    size_t size = smartToolCalculationBlockInfos.size();
    for (size_t i = 0; i < size; i++)
    {
      NFmiSmartToolCalculationBlockInfo &blockInfo = smartToolCalculationBlockInfos[i];
      boost::shared_ptr<NFmiSmartToolCalculationBlock> block = CreateCalculationBlock(blockInfo);
      if (block)
      {
        ModifyBlockData(block, theMacroParamValue, theThreadCallBacks);
      }
    }
    ClearScriptVariableInfos();  // lopuksi nämä skripti-muuttujat tyhjennetään
  }
  catch (...)
  {
    ClearScriptVariableInfos();  // lopuksi nämä skripti-muuttujat tyhjennetään
    fMacroRunnable = false;
    throw;
  }
}

// Make mask vector if there is CalculationPoint's used in the smarttool script.
// This mask is used to skip points not needed in final result.
// If calculationPoints is empty, return empty pointer.
std::unique_ptr<CalculationPointMaskData> NFmiSmartToolModifier::MakePossibleCalculationPointMask(
    const std::vector<NFmiPoint> &calculationPoints)
{
  if (!calculationPoints.empty())
  {
    auto editedInfo = UsedMacroParamData();
    if (editedInfo)
    {
      double maxAllowedDistanceInMetres =
          (itsExtraMacroParamData.ObservationRadiusInKm() == kFloatMissing)
              ? 99999999
              : itsExtraMacroParamData.ObservationRadiusInKm() * 1000. + 0.00000001;
      std::unique_ptr<CalculationPointMaskData> calculationPointMask(new CalculationPointMaskData(
          editedInfo->SizeLocations(), std::make_pair(nullptr, maxAllowedDistanceInMetres)));
      for (auto &latlon : calculationPoints)
      {
        if (editedInfo->NearestPoint(latlon))
        {
          auto distanceInMeters = NFmiLocation(latlon).Distance(editedInfo->LatLon());
          auto &maskLocationPtr = (*calculationPointMask)[editedInfo->LocationIndex()].first;
          auto &maskLocationDist = (*calculationPointMask)[editedInfo->LocationIndex()].second;
          if (distanceInMeters < maskLocationDist)
          {
            maskLocationPtr = &latlon;
            maskLocationDist = distanceInMeters;
          }
        }
      }

      return calculationPointMask;
    }
  }

  return std::unique_ptr<CalculationPointMaskData>();
}

void NFmiSmartToolModifier::ModifyData_ver2(
    NFmiTimeDescriptor *theModifiedTimes,
    bool fSelectedLocationsOnly,
    bool isMacroParamCalculation,
    NFmiThreadCallBacks *theThreadCallBacks,
    std::vector<NFmiMacroParamValue> *macroParamValuesVectorForSpecialCalculations)
{
  itsModifiedTimes = theModifiedTimes;
  fMacroParamCalculation = isMacroParamCalculation;
  fModifySelectedLocationsOnly = fSelectedLocationsOnly;
  itsLastExceptionMessageFromThreads.clear();
  ::ClearLastExceptionMessage();
  try
  {
    std::vector<NFmiSmartToolCalculationBlockInfo> &smartToolCalculationBlockInfos =
        itsSmartToolIntepreter->SmartToolCalculationBlocks();
    ::CalcTotalProgressStepCount(
        smartToolCalculationBlockInfos, theModifiedTimes, theThreadCallBacks);
    size_t size = smartToolCalculationBlockInfos.size();
    auto calculationPointMaskPtr = MakePossibleCalculationPointMask(CalculationPoints());
    for (size_t i = 0; i < size; i++)
    {
      NFmiSmartToolCalculationBlockInfo blockInfo = smartToolCalculationBlockInfos[i];
      boost::shared_ptr<NFmiSmartToolCalculationBlock> block = CreateCalculationBlock(blockInfo);
      if (block)
      {
        ModifyBlockData_ver2(block,
                             theThreadCallBacks,
                             calculationPointMaskPtr.get(),
                             macroParamValuesVectorForSpecialCalculations);
      }
    }
    ClearScriptVariableInfos();  // lopuksi nämä skripti-muuttujat tyhjennetään
    itsLastExceptionMessageFromThreads = ::GetLastExceptionMessage();
  }
  catch (...)
  {
    ClearScriptVariableInfos();  // lopuksi nämä skripti-muuttujat tyhjennetään
    fMacroRunnable = false;
    itsLastExceptionMessageFromThreads = GetLastExceptionMessage();
    throw;
  }
}

// Kun intepreter on tulkinnut smarttool-tekstin, voidaan kysyä, onko kyseinen makro ns.
// macroParam-skripti eli sisältääkö se RESULT = ??? tapaista tekstiä
bool NFmiSmartToolModifier::IsInterpretedSkriptMacroParam()
{
  return itsSmartToolIntepreter ? itsSmartToolIntepreter->IsInterpretedSkriptMacroParam() : false;
}

void NFmiSmartToolModifier::ModifyBlockData(
    const boost::shared_ptr<NFmiSmartToolCalculationBlock> &theCalculationBlock,
    NFmiMacroParamValue &theMacroParamValue,
    NFmiThreadCallBacks *theThreadCallBacks)
{
  // HUOM!! Koska jostain syystä alku ja loppu CalculationSection:it lasketaan erikseen, pitää
  // muistaa
  // että ModifyConditionalData-osiossa ei saa käsitellä näitä sectioneita!!!!
  // Ok, nyt tiedän, että tämä johtuu siitä että ModifyData2(_ver2) -funktioissa laskut suoritetaan
  // aina rivi kerrallaa (kaikki ajat ja paikat lääpi),
  // kun taas if-elseif-else -rakenteissa lasketaan koko hökötys kerrallaan läpi.
  ModifyData2(
      theCalculationBlock->itsFirstCalculationSection, theMacroParamValue, theThreadCallBacks);
  ModifyConditionalData(theCalculationBlock, theMacroParamValue, theThreadCallBacks);
  ModifyData2(
      theCalculationBlock->itsLastCalculationSection, theMacroParamValue, theThreadCallBacks);
}

void NFmiSmartToolModifier::ModifyBlockData_ver2(
    const boost::shared_ptr<NFmiSmartToolCalculationBlock> &theCalculationBlock,
    NFmiThreadCallBacks *theThreadCallBacks,
    CalculationPointMaskData *calculationPointMask,
    std::vector<NFmiMacroParamValue> *macroParamValuesVectorForSpecialCalculations)
{
  // HUOM!! Koska jostain syystä alku ja loppu CalculationSection:it lasketaan erikseen, pitää
  // muistaa
  // että ModifyConditionalData-osiossa ei saa käsitellä näitä sectioneita!!!!
  // Ok, nyt tiedän, että tämä johtuu siitä että ModifyData2(_ver2) -funktioissa laskut suoritetaan
  // aina rivi kerrallaa (kaikki ajat ja paikat lääpi),
  // kun taas if-elseif-else -rakenteissa lasketaan koko hökötys kerrallaan läpi.
  ModifyData2_ver2(theCalculationBlock->itsFirstCalculationSection,
                   theThreadCallBacks,
                   calculationPointMask,
                   macroParamValuesVectorForSpecialCalculations);
  ModifyConditionalData_ver2(theCalculationBlock,
                             theThreadCallBacks,
                             calculationPointMask,
                             macroParamValuesVectorForSpecialCalculations);
  ModifyData2_ver2(theCalculationBlock->itsLastCalculationSection,
                   theThreadCallBacks,
                   calculationPointMask,
                   macroParamValuesVectorForSpecialCalculations);
}

void NFmiSmartToolModifier::ModifyConditionalData(
    const boost::shared_ptr<NFmiSmartToolCalculationBlock> &theCalculationBlock,
    NFmiMacroParamValue &theMacroParamValue,
    NFmiThreadCallBacks *theThreadCallBacks)
{
  if (theCalculationBlock->itsIfAreaMaskSection && theCalculationBlock->itsIfCalculationBlocks)
  {
    if (theCalculationBlock->FirstVariableInfo() == 0)
      throw runtime_error(::GetDictionaryString("SmartToolModifierErrorUnknownProblem"));
    boost::shared_ptr<NFmiFastQueryInfo> info(
        dynamic_cast<NFmiFastQueryInfo *>(theCalculationBlock->FirstVariableInfo()->Clone()));

    try
    {
      NFmiCalculationParams calculationParams;
      calculationParams.itsObservationRadiusInKm = ExtraMacroParamData().ObservationRadiusInKm();
      SetInfosMaskType(info);
      NFmiTimeDescriptor modifiedTimes(itsModifiedTimes ? *itsModifiedTimes
                                                        : info->TimeDescriptor());
      for (modifiedTimes.Reset(); modifiedTimes.Next();)
      {
        if (info->Time(modifiedTimes.Time()))
        {
          NFmiQueryDataUtil::CheckIfStopped(theThreadCallBacks);
          NFmiQueryDataUtil::DoStepIt(
              theThreadCallBacks);  // stepataan vasta 0-tarkastuksen jälkeen!
          calculationParams.itsTime = modifiedTimes.Time();
          if (theMacroParamValue.fSetValue)
            calculationParams.itsTime = theMacroParamValue.itsTime;
          calculationParams.itsTimeIndex = info->TimeIndex();
          theCalculationBlock->itsIfAreaMaskSection->Time(
              calculationParams.itsTime);  // yritetään optimoida laskuja hieman kun mahdollista
          theCalculationBlock->itsIfCalculationBlocks->SetTime(
              calculationParams.itsTime);  // yritetään optimoida laskuja hieman kun mahdollista
          if (theCalculationBlock->itsElseIfAreaMaskSection &&
              theCalculationBlock->itsElseIfCalculationBlocks)
          {
            theCalculationBlock->itsElseIfAreaMaskSection->Time(calculationParams.itsTime);
            theCalculationBlock->itsElseIfCalculationBlocks->SetTime(calculationParams.itsTime);
          }
          if (theCalculationBlock->itsElseCalculationBlocks)
            theCalculationBlock->itsElseCalculationBlocks->SetTime(calculationParams.itsTime);

          for (info->ResetLocation(); info->NextLocation();)
          {
            calculationParams.SetModifiedLatlon(info->LatLon(), false);
            if (theMacroParamValue.fSetValue)
            {
              calculationParams.SetModifiedLatlon(theMacroParamValue.itsLatlon, false);
              // pitää laittaa nearestlocation päälle, että tuloksia voidaan
              // myöhemmin hakea interpolaation avulla
              info->Location(calculationParams.UsedLatlon());
            }
            calculationParams.itsLocationIndex =
                info->LocationIndex();  // tämä locationindex juttu liittyy kai optimointiin, jota
                                        // ei tehdä enää, pitäisikö poistaa
            if (theCalculationBlock->itsIfAreaMaskSection->IsMasked(calculationParams))
              theCalculationBlock->itsIfCalculationBlocks->Calculate(calculationParams,
                                                                     theMacroParamValue);
            else if (theCalculationBlock->itsElseIfAreaMaskSection &&
                     theCalculationBlock->itsElseIfCalculationBlocks &&
                     theCalculationBlock->itsElseIfAreaMaskSection->IsMasked(calculationParams))
            {
              theCalculationBlock->itsElseIfCalculationBlocks->Calculate(calculationParams,
                                                                         theMacroParamValue);
            }
            else if (theCalculationBlock->itsElseCalculationBlocks)
              theCalculationBlock->itsElseCalculationBlocks->Calculate(calculationParams,
                                                                       theMacroParamValue);
            if (theMacroParamValue.fSetValue)
            {
              return;  // eli jos oli yhden pisteen laskusta kyse, lopetetaan loppi heti
            }
          }
        }
      }
    }
    catch (...)
    {
      throw;
    }
  }
}

// Kun dataa käydään läpi NextLocation-menetelmällä, ja kyseessä on NFmiSmartInfo-olio, on niillä
// tiedossa sisäinen bitmaski, jonka
// avulla osataan tarvittaessa hyppiä ei kiinnostavien paikkojen yli. Nyt en halua tehdä joka
// threadille aina Clone:a näistä infoista.
// Mutta otan talteen tarvittavan bitmaksin, jos sellainen oli käytössä ja hypin sen avulla ohi
// ei-toivottujen pisteiden.
static const NFmiBitMask *GetUsedBitmask(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                         bool modifySelectedLocationsOnly)
{
  if (modifySelectedLocationsOnly)
  {
    NFmiSmartInfo *smartInfo = dynamic_cast<NFmiSmartInfo *>(theInfo.get());
    if (smartInfo)
    {
      return &(smartInfo->Mask(NFmiMetEditorTypes::kFmiSelectionMask));
    }
  }
  return 0;  // ei ole maskia käytössä
}

// globaali asetus luokka for_each-funktioon
template <typename T>
class TimeSetter
{
 public:
  TimeSetter(const NFmiMetTime &theTime) : itsTime(theTime) {}
  void operator()(boost::shared_ptr<T> &theMask) { theMask->Time(itsTime); }
  NFmiMetTime itsTime;
};

static bool DoCalculationPointMaskCheck(const CalculationPointMaskData *calculationPointMask,
                                        unsigned long index)
{
  return (calculationPointMask == nullptr || (*calculationPointMask)[index].first);
}

static void DoPartialGridCalculationBlockInThread(
    NFmiLocationIndexRangeCalculator &theLocationIndexRangeCalculator,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    boost::shared_ptr<NFmiSmartToolCalculationBlock> &theCalculationBlock,
    NFmiCalculationParams &theCalculationParams,
    const NFmiBitMask *theUsedBitmask,
    const CalculationPointMaskData *calculationPointMask)
{
  try
  {
    unsigned long startIndex = 0;
    unsigned long endIndex = 0;
    for (; theLocationIndexRangeCalculator.GetCurrentLocationRange(startIndex, endIndex);)
    {
      for (unsigned long i = startIndex; i <= endIndex; i++)
      {
        if (theUsedBitmask == 0 || theUsedBitmask->IsMasked(i))
        {
          if (::DoCalculationPointMaskCheck(calculationPointMask, i))
          {
            if (theInfo->LocationIndex(i))
            {
              theCalculationParams.SetModifiedLatlon(theInfo->LatLon(), false);
              theCalculationParams.itsLocationIndex = theInfo->LocationIndex();
              if (calculationPointMask)
                theCalculationParams.itsActualCalculationPoint = (*calculationPointMask)[i].first;
              // TUON LOCATIONINDEX jutun voisi kai poistaa, kun kyseistä optimointi juttua ei kai
              // enää käytetä
              theCalculationBlock->Calculate_ver2(theCalculationParams, true);
            }
          }
        }
      }
    }
  }
  catch (std::exception & /* e */)
  {
    //		int x = 0;
    //		std::cerr << "Error in DoPartialGridCalculationBlockInThread: " << e.what() <<
    // std::endl;
  }
  catch (...)
  {
    //		int x = 0;
    //		std::cerr << "Unknown Error in DoPartialGridCalculationBlockInThread." << std::endl;
  }
}

static NFmiCalculationParams MakeCalculationParams(const NFmiMacroParamValue &macroParamValue,
                                                   unsigned long locationIndex,
                                                   unsigned long timeIndex)
{
  return NFmiCalculationParams(
      macroParamValue.itsLatlon,
      locationIndex,
      macroParamValue.itsTime,
      timeIndex,
      macroParamValue.fDoCrossSectionCalculations || macroParamValue.fDoTimeSerialCalculations);
}

static void DoPartialSpecialTypeCalculationBlockInThread(
    NFmiLocationIndexRangeCalculator &theLocationIndexRangeCalculator,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,  // onko theInfo turha?
    boost::shared_ptr<NFmiSmartToolCalculationBlock> &theCalculationBlock,
    std::vector<NFmiMacroParamValue> &macroParamValueVector)
{
  try
  {
    unsigned long startIndex = 0;
    unsigned long endIndex = 0;
    for (; theLocationIndexRangeCalculator.GetCurrentLocationRange(startIndex, endIndex);)
    {
      for (unsigned long i = startIndex; i <= endIndex; i++)
      {
        // Tässä käydäänkin läpi eri laskenta pisteita, jotka on annettu vektorissa
        auto &macroParamValue = macroParamValueVector[i];
        // NFmiCalculationParams:in locationIndex saadaan vector:in i-indeksin mukaan ja timeIndex
        // on macroParam tapauksissa aina 0.
        NFmiCalculationParams calculationParams(::MakeCalculationParams(macroParamValue, i, 0));
        theCalculationBlock->Calculate(calculationParams, macroParamValue);
      }
    }
  }
  catch (...)
  {
    // pakko ottaa vain vastaan poikkeukset, ei tehdä mitään
  }
}

static void DoPartialGridCalculationInThread(
    NFmiLocationIndexRangeCalculator &theLocationIndexRangeCalculator,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    boost::shared_ptr<NFmiSmartToolCalculation> &theCalculation,
    NFmiCalculationParams &theCalculationParams,
    const NFmiBitMask *theUsedBitmask,
    const CalculationPointMaskData *calculationPointMask)
{
  try
  {
    unsigned long startIndex = 0;
    unsigned long endIndex = 0;
    for (; theLocationIndexRangeCalculator.GetCurrentLocationRange(startIndex, endIndex);)
    {
      for (unsigned long i = startIndex; i <= endIndex; i++)
      {
        if (theUsedBitmask == nullptr || theUsedBitmask->IsMasked(i))
        {
          if (::DoCalculationPointMaskCheck(calculationPointMask, i))
          {
            if (theInfo->LocationIndex(i))
            {
              theCalculationParams.SetModifiedLatlon(theInfo->LatLon(), false);
              theCalculationParams.itsLocationIndex = theInfo->LocationIndex();
              if (calculationPointMask)
                theCalculationParams.itsActualCalculationPoint = (*calculationPointMask)[i].first;
              // TUON LOCATIONINDEX jutun voisi kai poistaa, kun kyseistä optimointi juttua ei kai
              // enää käytetä
              theCalculation->Calculate_ver2(theCalculationParams);
            }
          }
        }
      }
    }
  }
  catch (std::exception &e)
  {
    std::string lastExceptionMessageFromThreads = "Smarttool calculation error: ";
    lastExceptionMessageFromThreads += e.what();
    ::SetLastExceptionMessage(lastExceptionMessageFromThreads);
  }
  catch (...)
  {
    std::string lastExceptionMessageFromThreads = "Unknown error in smarttool calculation";
    ::SetLastExceptionMessage(lastExceptionMessageFromThreads);
  }
}

static void DoPartialSpecialTypeCalculationInThread(
    NFmiLocationIndexRangeCalculator &theLocationIndexRangeCalculator,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,  // onko theInfo turha?
    boost::shared_ptr<NFmiSmartToolCalculation> &theCalculation,
    std::vector<NFmiMacroParamValue> &macroParamValueVector)
{
  try
  {
    unsigned long startIndex = 0;
    unsigned long endIndex = 0;
    for (; theLocationIndexRangeCalculator.GetCurrentLocationRange(startIndex, endIndex);)
    {
      for (unsigned long i = startIndex; i <= endIndex; i++)
      {
        if (theInfo->LocationIndex(i))
        {
          // Tässä käydäänkin läpi eri laskenta pisteita, jotka on annettu vektorissa
          auto &macroParamValue = macroParamValueVector[i];
          // NFmiCalculationParams:in locationIndex saadaan vector:in i-indeksin mukaan ja timeIndex
          // on macroParam tapauksissa aina 0.
          NFmiCalculationParams calculationParams(::MakeCalculationParams(macroParamValue, i, 0));
          theCalculation->Calculate(calculationParams, macroParamValue);
        }
      }
    }
  }
  catch (...)
  {
    // pakko ottaa vain vastaan poikkeukset, ei tehdä mitään
  }
}

// Tämä on vain koodin lyhennys funktio
template <typename Container>
static void SetTimes(Container &container, const NFmiCalculationParams &calculationParams)
{
  std::for_each(container.begin(),
                container.end(),
                TimeSetter<Container::value_type::element_type>(calculationParams.itsTime));
}

static std::vector<boost::shared_ptr<NFmiFastQueryInfo>> MakeInfoCopyVector(
    size_t threadCount, boost::shared_ptr<NFmiFastQueryInfo> &info)
{
  std::vector<boost::shared_ptr<NFmiFastQueryInfo>> infoVector;
  if (threadCount > 1)
  {
    for (size_t i = 0; i < threadCount; i++)
      infoVector.push_back(NFmiAreaMask::DoShallowCopy(info));
  }
  else
  {
    // Jos vain yksi laskenta threadi käytössä, laitetaan originaali info vain sellaisenaan 'kopio'
    // vectoriin.
    infoVector.push_back(info);
  }
  return infoVector;
}

static std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> MakeCalculationBlockVector(
    size_t threadCount, const boost::shared_ptr<NFmiSmartToolCalculationBlock> &calculationBlock)
{
  std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> calculationBlockVector;
  if (threadCount > 1)
  {
    for (size_t i = 0; i < threadCount; i++)
    {
      calculationBlockVector.push_back(boost::shared_ptr<NFmiSmartToolCalculationBlock>(
          new NFmiSmartToolCalculationBlock(*calculationBlock)));
    }
  }
  else
  {
    // Jos vain yksi laskenta threadi käytössä, laitetaan originaali calculationBlock vain
    // sellaisenaan 'kopio' vectoriin.
    calculationBlockVector.push_back(calculationBlock);
  }
  return calculationBlockVector;
}

void NFmiSmartToolModifier::CalculateUsedWorkingThreadCount(double wantedHardwareThreadPercent,
                                                            int userGivenWorkingThreadCount,
                                                            bool macroParamCase)
{
  int maxThreadCount = std::thread::hardware_concurrency();
  if (userGivenWorkingThreadCount > 0)
  {
    itsUsedThreadCount = std::min(maxThreadCount, userGivenWorkingThreadCount);
  }
  else
  {
    if (macroParamCase)
    {
      // macroParam laskuissa ei ole hyötyä olla paljoa threadeja rinnakkain laskemassa juttuja.
      // Jos käytössä on asema dataa, silloin paras olisi vain 1 threadi, koska eri threadeille
      // tehdään paljon asemien kopiointia (jokaiselle asemadata parametrille vielä erikseen).
      // Lisäksi testeissä osoittautui että vain yhden aika-askeleen laskuissa n. 3-4 threadin
      // käyttö on hyödyllistä vaikka kyse olisi hiladata laskuista (isommat määrät eivät tuo
      // hyötyä, vain haittaa).
      CalculateOptimalWorkingThreadCount();

      itsUsedThreadCount = std::min(itsOptimalThreadCount, maxThreadCount);
    }
    else
    {
      itsUsedThreadCount =
          NFmiQueryDataUtil::GetReasonableWorkingThreadCount(wantedHardwareThreadPercent);
    }
  }

  //  itsUsedThreadCount = 1; // Debuggaustestejä varten
  itsUsedThreadCounts.insert(itsUsedThreadCount);
}

// Kun yhden aika-askeleen hilan laskenta jaetaan eri säikeille osiin,
// käy yksi säie aina läpi näin monen hilapisteen, ennen kuin pyytää lisää laskettavaa.
// Tämä oli 100, mutta pienempi ChunkSize takaa että työt jaetaan paremmin.
const unsigned long g_UsedMultiThreadChunkSize = 6;

void NFmiSmartToolModifier::ModifyConditionalData_ver2(
    const boost::shared_ptr<NFmiSmartToolCalculationBlock> &theCalculationBlock,
    NFmiThreadCallBacks *theThreadCallBacks,
    CalculationPointMaskData *calculationPointMask,
    std::vector<NFmiMacroParamValue> *macroParamValuesVectorForSpecialCalculations)
{
  if (theCalculationBlock->itsIfAreaMaskSection && theCalculationBlock->itsIfCalculationBlocks)
  {
    if (theCalculationBlock->FirstVariableInfo() == 0)
      throw runtime_error(::GetDictionaryString("SmartToolModifierErrorUnknownProblem"));
    boost::shared_ptr<NFmiFastQueryInfo> info(
        dynamic_cast<NFmiFastQueryInfo *>(theCalculationBlock->FirstVariableInfo()->Clone()));
    if (info == 0)
      return;

    try
    {
      // Tämä LatLon kutsu on tehtävä kerran multi-thread jutuissa, koska tämä rakentaa kaikille
      // info-kopioille yhteisen latlon-cache:n
      info->LatLon();
      NFmiCalculationParams calculationParams;
      SetInfosMaskType(info);
      NFmiTimeDescriptor modifiedTimes(itsModifiedTimes ? *itsModifiedTimes
                                                        : info->TimeDescriptor());
      const NFmiBitMask *usedBitmask = ::GetUsedBitmask(info, fModifySelectedLocationsOnly);
      calculationParams.itsObservationRadiusInKm = ExtraMacroParamData().ObservationRadiusInKm();
      CalculateUsedWorkingThreadCount(
          75, ExtraMacroParamData().WorkingThreadCount(), fMacroParamCalculation);

      std::vector<boost::shared_ptr<NFmiFastQueryInfo>> infoVector =
          ::MakeInfoCopyVector(itsUsedThreadCount, info);
      // tehdään joka coren säikeelle oma calculaatioBlokki kopio
      std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> calculationBlockVector =
          ::MakeCalculationBlockVector(itsUsedThreadCount, theCalculationBlock);

      for (modifiedTimes.Reset(); modifiedTimes.Next();)
      {
        if (info->Time(modifiedTimes.Time()))
        {
          NFmiQueryDataUtil::CheckIfStopped(theThreadCallBacks);
          NFmiQueryDataUtil::DoStepIt(
              theThreadCallBacks);  // stepataan vasta 0-tarkastuksen jälkeen!
          calculationParams.itsTime = modifiedTimes.Time();
          calculationParams.itsTimeIndex = info->TimeIndex();
          theCalculationBlock->Time(calculationParams.itsTime);
          // calculaatioiden kopioiden ajat pitää myös asettaa
          ::SetTimes(calculationBlockVector, calculationParams);
          // info kopioiden ajat pitää myös asettaa
          ::SetTimes(infoVector, calculationParams);

          if (macroParamValuesVectorForSpecialCalculations)
            DoMultiThreadConditionalBlockCalculationsForSpecialCalculations(
                itsUsedThreadCount,
                infoVector,
                calculationBlockVector,
                *macroParamValuesVectorForSpecialCalculations);
          else
            DoMultiThreadConditionalBlockCalculations(itsUsedThreadCount,
                                                      infoVector,
                                                      calculationBlockVector,
                                                      calculationParams,
                                                      usedBitmask,
                                                      calculationPointMask);
        }
      }
    }
    catch (...)
    {
      throw;
    }
  }
}

void NFmiSmartToolModifier::DoMultiThreadConditionalBlockCalculations(
    size_t threadCount,
    std::vector<boost::shared_ptr<NFmiFastQueryInfo>> &infoVector,
    std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> &calculationBlockVector,
    NFmiCalculationParams &calculationParams,
    const NFmiBitMask *usedBitmask,
    CalculationPointMaskData *calculationPointMask)
{
  NFmiLocationIndexRangeCalculator locationIndexRangeCalculator(infoVector[0]->SizeLocations(),
                                                                g_UsedMultiThreadChunkSize);
  std::vector<NFmiCalculationParams> calculationParamsVector(threadCount, calculationParams);

  boost::thread_group calcParts;
  for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
    calcParts.add_thread(new boost::thread(::DoPartialGridCalculationBlockInThread,
                                           boost::ref(locationIndexRangeCalculator),
                                           boost::ref(infoVector[threadIndex]),
                                           boost::ref(calculationBlockVector[threadIndex]),
                                           boost::ref(calculationParamsVector[threadIndex]),
                                           usedBitmask,
                                           calculationPointMask));
  calcParts.join_all();  // odotetaan että threadit lopettavat
}

void NFmiSmartToolModifier::DoMultiThreadConditionalBlockCalculationsForSpecialCalculations(
    size_t threadCount,
    std::vector<boost::shared_ptr<NFmiFastQueryInfo>> &infoVector,
    std::vector<boost::shared_ptr<NFmiSmartToolCalculationBlock>> &calculationBlockVector,
    std::vector<NFmiMacroParamValue> &macroParamValuesVector)
{
  NFmiLocationIndexRangeCalculator locationIndexRangeCalculator(
      static_cast<unsigned long>(macroParamValuesVector.size()), g_UsedMultiThreadChunkSize);

  boost::thread_group calcParts;
  for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
    calcParts.add_thread(new boost::thread(::DoPartialSpecialTypeCalculationBlockInThread,
                                           boost::ref(locationIndexRangeCalculator),
                                           boost::ref(infoVector[threadIndex]),
                                           boost::ref(calculationBlockVector[threadIndex]),
                                           boost::ref(macroParamValuesVector)));
  calcParts.join_all();  // odotetaan että threadit lopettavat
}

static void DoSafeMaskOperation(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo1,
                                boost::shared_ptr<NFmiFastQueryInfo> &theInfo2)
{
  NFmiSmartInfo *info1 = dynamic_cast<NFmiSmartInfo *>(theInfo1.get());
  NFmiSmartInfo *info2 = dynamic_cast<NFmiSmartInfo *>(theInfo2.get());
  if (info1 && info2)
  {
    info1->Mask(info2->Mask(NFmiMetEditorTypes::kFmiSelectionMask),
                NFmiMetEditorTypes::kFmiSelectionMask);
  }
}

void NFmiSmartToolModifier::SetInfosMaskType(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  if (fModifySelectedLocationsOnly)
  {
    boost::shared_ptr<NFmiFastQueryInfo> editedInfo =
        itsInfoOrganizer->FindInfo(NFmiInfoData::kEditable);
    if (theInfo->DataType() == NFmiInfoData::kScriptVariableData && editedInfo)
    {  // skripti muuttujalle pitää asettaa sama valittujen pisteiden maski kuin on editoidulla
       // datalla
      ::DoSafeMaskOperation(theInfo, editedInfo);
    }
    theInfo->MaskType(NFmiMetEditorTypes::kFmiSelectionMask);
  }
  else
    theInfo->MaskType(NFmiMetEditorTypes::kFmiNoMask);
}

void NFmiSmartToolModifier::ModifyData2(
    boost::shared_ptr<NFmiSmartToolCalculationSection> &theCalculationSection,
    NFmiMacroParamValue &theMacroParamValue,
    NFmiThreadCallBacks *theThreadCallBacks)
{
  if (theCalculationSection && theCalculationSection->FirstVariableInfo())
  {
    boost::shared_ptr<NFmiFastQueryInfo> info(
        dynamic_cast<NFmiFastQueryInfo *>(theCalculationSection->FirstVariableInfo()->Clone()));
    if (info == 0)
      return;
    try
    {
      NFmiCalculationParams calculationParams;
      calculationParams.itsObservationRadiusInKm = ExtraMacroParamData().ObservationRadiusInKm();
      SetInfosMaskType(info);
      NFmiTimeDescriptor modifiedTimes(itsModifiedTimes ? *itsModifiedTimes
                                                        : info->TimeDescriptor());

      // Muutin lasku systeemin suoritusta, koska tuli ongelmia mm. muuttujien kanssa, kun niitä
      // käytettiin samassa calculationSectionissa
      // CalculationSection = lasku rivejä peräkkäin esim.
      // T = T + 1
      // P = P + 1
      // jne. ilman IF-lauseita
      // ENNEN laskettiin tälläinen sectio siten että käytiin läpi koko sectio samalla paikalla ja
      // ajalla ja sitten siirryttiin eteenpäin.
      // NYT lasketaan aina yksi laskurivi läpi kaikkien aikojen ja paikkojen, ja sitten siirrytään
      // seuraavalle lasku riville.
      size_t size = theCalculationSection->GetCalculations().size();
      for (size_t i = 0; i < size; i++)
      {
        for (modifiedTimes.Reset(); modifiedTimes.Next();)
        {
          calculationParams.itsTime = modifiedTimes.Time();
          if (theMacroParamValue.fSetValue)
            calculationParams.itsTime = theMacroParamValue.itsTime;
          if (info->Time(
                  calculationParams.itsTime))  // asetetaan myös tämä, että saadaan oikea timeindex
          {
            NFmiQueryDataUtil::CheckIfStopped(theThreadCallBacks);
            NFmiQueryDataUtil::DoStepIt(
                theThreadCallBacks);  // stepataan vasta 0-tarkastuksen jälkeen!
            theCalculationSection->SetTime(
                calculationParams.itsTime);  // yritetään optimoida laskuja hieman kun mahdollista
            for (info->ResetLocation(); info->NextLocation();)
            {
              calculationParams.SetModifiedLatlon(info->LatLon(), false);
              if (theMacroParamValue.fSetValue)
              {
                calculationParams.SetModifiedLatlon(theMacroParamValue.itsLatlon, false);
                // pitää laittaa nearestlocation päälle, että tuloksia voidaan
                // myöhemmin hakea interpolaation avulla
                info->Location(calculationParams.UsedLatlon());
              }
              calculationParams.itsLocationIndex = info->LocationIndex();
              // TUON LOCATIONINDEX jutun voisi kai poistaa, kun kyseistä optimointi juttua ei kai
              // enää käytetä
              theCalculationSection->GetCalculations()[i]->Calculate(calculationParams,
                                                                     theMacroParamValue);

              if (theMacroParamValue.fSetValue)
                break;
            }
          }
          if (theMacroParamValue.fSetValue)
            break;
        }
      }
    }
    catch (...)
    {
      throw;
    }
  }
}

static std::vector<boost::shared_ptr<NFmiSmartToolCalculation>> MakeCalculationVector(
    size_t threadCount, boost::shared_ptr<NFmiSmartToolCalculation> &smartToolCalculation)
{
  std::vector<boost::shared_ptr<NFmiSmartToolCalculation>> calculationVector;
  for (size_t j = 0; j < threadCount; j++)
    calculationVector.push_back(boost::shared_ptr<NFmiSmartToolCalculation>(
        new NFmiSmartToolCalculation(*smartToolCalculation)));
  return calculationVector;
}

void NFmiSmartToolModifier::ModifyData2_ver2(
    boost::shared_ptr<NFmiSmartToolCalculationSection> &theCalculationSection,
    NFmiThreadCallBacks *theThreadCallBacks,
    CalculationPointMaskData *calculationPointMask,
    std::vector<NFmiMacroParamValue> *macroParamValuesVectorForSpecialCalculations)
{
  if (theCalculationSection && theCalculationSection->FirstVariableInfo())
  {
    boost::shared_ptr<NFmiFastQueryInfo> info(
        dynamic_cast<NFmiFastQueryInfo *>(theCalculationSection->FirstVariableInfo()->Clone()));
    if (info == 0)
      return;
    try
    {
      // Tämä LatLon kutsu on tehtävä kerran multi-thread jutuissa datalle, koska tämä rakentaa
      // kaikille info-kopioille yhteisen latlon-cache:n
      info->LatLon();
      NFmiCalculationParams calculationParams;
      SetInfosMaskType(info);
      NFmiTimeDescriptor modifiedTimes(itsModifiedTimes ? *itsModifiedTimes
                                                        : info->TimeDescriptor());
      const NFmiBitMask *usedBitmask = ::GetUsedBitmask(info, fModifySelectedLocationsOnly);
      calculationParams.itsObservationRadiusInKm = ExtraMacroParamData().ObservationRadiusInKm();
      CalculateUsedWorkingThreadCount(
          75, ExtraMacroParamData().WorkingThreadCount(), fMacroParamCalculation);

      std::vector<boost::shared_ptr<NFmiFastQueryInfo>> infoVector =
          ::MakeInfoCopyVector(itsUsedThreadCount, info);

      // Muutin lasku systeemin suoritusta, koska tuli ongelmia mm. muuttujien kanssa, kun niitä
      // käytettiin samassa calculationSectionissa
      // CalculationSection = lasku rivejä peräkkäin esim.
      // T = T + 1
      // P = P + 1
      // jne. ilman IF-lauseita
      // ENNEN laskettiin tälläinen sectio siten että käytiin läpi koko sectio samalla paikalla ja
      // ajalla ja sitten siirryttiin eteenpäin.
      // NYT lasketaan aina yksi laskurivi läpi kaikkien aikojen ja paikkojen, ja sitten siirrytään
      // seuraavalle lasku riville.
      std::vector<boost::shared_ptr<NFmiSmartToolCalculation>> &calculationVector =
          theCalculationSection->GetCalculations();
      for (size_t i = 0; i < calculationVector.size(); i++)
      {
        boost::shared_ptr<NFmiSmartToolCalculation> smartToolCalculation = calculationVector[i];
        // tehdään joka coren säikeelle oma calculaatio kopio
        std::vector<boost::shared_ptr<NFmiSmartToolCalculation>> calculationVectorForThread =
            ::MakeCalculationVector(itsUsedThreadCount, smartToolCalculation);

        for (modifiedTimes.Reset(); modifiedTimes.Next();)
        {
          calculationParams.itsTime = modifiedTimes.Time();
          // Asetetaan myös haluttu aika käytettyyn info:on, että saadaan oikea timeindex, PAITSI
          // jos kyse on poikkileikkaus laskuista
          if (macroParamValuesVectorForSpecialCalculations || info->Time(calculationParams.itsTime))
          {
            NFmiQueryDataUtil::CheckIfStopped(theThreadCallBacks);
            // stepataan vasta 0-tarkastuksen jälkeen!
            NFmiQueryDataUtil::DoStepIt(theThreadCallBacks);
            // yritetään optimoida laskuja hieman kun mahdollista
            smartToolCalculation->Time(calculationParams.itsTime);
            ::SetTimes(calculationVectorForThread, calculationParams);
            ::SetTimes(infoVector, calculationParams);

            if (macroParamValuesVectorForSpecialCalculations)
            {
              DoMultiThreadCalculationsForSpecialCalculations(
                  itsUsedThreadCount,
                  infoVector,
                  calculationVectorForThread,
                  *macroParamValuesVectorForSpecialCalculations);
            }
            else
            {
              DoMultiThreadCalculations(itsUsedThreadCount,
                                        infoVector,
                                        calculationVectorForThread,
                                        calculationParams,
                                        usedBitmask,
                                        calculationPointMask);
            }
          }
        }
      }
    }
    catch (...)
    {
      throw;
    }
  }
}

void NFmiSmartToolModifier::DoMultiThreadCalculations(
    size_t threadCount,
    std::vector<boost::shared_ptr<NFmiFastQueryInfo>> &infoVector,
    std::vector<boost::shared_ptr<NFmiSmartToolCalculation>> &calculationVector,
    NFmiCalculationParams &calculationParams,
    const NFmiBitMask *usedBitmask,
    CalculationPointMaskData *calculationPointMask)
{
  std::vector<NFmiCalculationParams> calculationParamsVector(threadCount, calculationParams);
  NFmiLocationIndexRangeCalculator locationIndexRangeCalculator(infoVector[0]->SizeLocations(),
                                                                g_UsedMultiThreadChunkSize);

  boost::thread_group calcParts;
  for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
    calcParts.add_thread(new boost::thread(::DoPartialGridCalculationInThread,
                                           boost::ref(locationIndexRangeCalculator),
                                           boost::ref(infoVector[threadIndex]),
                                           boost::ref(calculationVector[threadIndex]),
                                           boost::ref(calculationParamsVector[threadIndex]),
                                           usedBitmask,
                                           calculationPointMask));
  calcParts.join_all();  // odotetaan että threadit lopettavat
}

void NFmiSmartToolModifier::DoMultiThreadCalculationsForSpecialCalculations(
    size_t threadCount,
    std::vector<boost::shared_ptr<NFmiFastQueryInfo>> &infoVector,
    std::vector<boost::shared_ptr<NFmiSmartToolCalculation>> &calculationVector,
    std::vector<NFmiMacroParamValue> &macroParamValuesVector)
{
  // indexRangeCalculator ottaa rajansa laskenta-piste vektorista crossSection tapauksessa
  NFmiLocationIndexRangeCalculator locationIndexRangeCalculator(
      static_cast<unsigned long>(macroParamValuesVector.size()), g_UsedMultiThreadChunkSize);

  boost::thread_group calcParts;
  for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
    calcParts.add_thread(new boost::thread(::DoPartialSpecialTypeCalculationInThread,
                                           boost::ref(locationIndexRangeCalculator),
                                           boost::ref(infoVector[threadIndex]),
                                           boost::ref(calculationVector[threadIndex]),
                                           boost::ref(macroParamValuesVector)));
  calcParts.join_all();  // odotetaan että threadit lopettavat
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreatePeekFunctionAreaMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &fMustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiAreaMask> areaMask;
  // HUOM!! Tähän vaaditaan syvä data kopio!!!
  // JOS kyseessä on ehtolauseen muuttujasta, joka on editoitavaa dataa.
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, fMustUsePressureInterpolation);
  if (theAreaMaskInfo.GetUseDefaultProducer())
  {  // Pitää tehdä syvä kopio datasta, että datan muuttuminen ei vaikuta laskuihin.
    boost::shared_ptr<NFmiFastQueryInfo> tmp(dynamic_cast<NFmiFastQueryInfo *>(info->Clone()));
    info = tmp;
  }
  NFmiAreaMask::CalculationOperationType maskType = theAreaMaskInfo.GetOperationType();
  const auto &offsetPoint1 = theAreaMaskInfo.GetOffsetPoint1();
  int offsetX = boost::math::iround(offsetPoint1.X());
  int offsetY = boost::math::iround(offsetPoint1.Y());
  if (maskType == NFmiAreaMask::FunctionPeekXY)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskPeekXY(theAreaMaskInfo.GetMaskCondition(),
                                   NFmiAreaMask::kInfo,
                                   theAreaMaskInfo.GetDataType(),
                                   info,
                                   offsetX,
                                   offsetY,
                                   theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                   NFmiAreaMask::kNoValue));
  else if (maskType == NFmiAreaMask::FunctionPeekXY2)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskPeekXY2(theAreaMaskInfo.GetMaskCondition(),
                                    NFmiAreaMask::kInfo,
                                    theAreaMaskInfo.GetDataType(),
                                    info,
                                    GetUsedEditedInfo(),
                                    offsetX,
                                    offsetY,
                                    theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                    NFmiAreaMask::kNoValue));
  else if (maskType == NFmiAreaMask::FunctionPeekXY3)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskPeekXY3(theAreaMaskInfo.GetMaskCondition(),
                                    NFmiAreaMask::kInfo,
                                    theAreaMaskInfo.GetDataType(),
                                    info,
                                    GetUsedEditedInfo(),
                                    offsetX,
                                    offsetY,
                                    theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                    NFmiAreaMask::kNoValue));

  if (fUseLevelData)
    itsParethesisCounter++;

  if (areaMask && theAreaMaskInfo.TimeOffsetInHours() != 0)
    areaMask->FunctionDataTimeOffsetInHours(theAreaMaskInfo.TimeOffsetInHours());

  return areaMask;
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::CreateInfo(
    const NFmiAreaMaskInfo &theAreaMaskInfo,
    bool &mustUsePressureInterpolation,
    unsigned long theWantedParamId)
{
  NFmiAreaMaskInfo wantedAreaMaskInfo(theAreaMaskInfo);
  NFmiDataIdent dataIdent = wantedAreaMaskInfo.GetDataIdent();
  dataIdent.GetParam()->SetIdent(theWantedParamId);
  wantedAreaMaskInfo.SetDataIdent(dataIdent);
  return CreateInfo(wantedAreaMaskInfo, mustUsePressureInterpolation);
}

void DoErrorExceptionForMetFunction(const NFmiAreaMaskInfo &theAreaMaskInfo,
                                    const std::string &theStartStr,
                                    const std::string &theMiddleStr)
{
  std::string errorStr(theStartStr);
  errorStr += " '";
  errorStr += theAreaMaskInfo.GetMaskText();
  errorStr += "' ";
  errorStr += theMiddleStr;
  errorStr += ":\n";
  errorStr += theAreaMaskInfo.GetOrigLineText();
  throw runtime_error(errorStr);
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateMetFunctionAreaMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &fMustUsePressureInterpolation)
{
  NFmiAreaMask::FunctionType funcType = theAreaMaskInfo.GetFunctionType();
  if (funcType == NFmiAreaMask::LatestValue)
    return CreateLatestValueMask(theAreaMaskInfo, fMustUsePressureInterpolation);

  boost::shared_ptr<NFmiAreaMask> areaMask;
  // HUOM!! Tähän vaaditaan syvä data kopio!!!
  // JOS kyseessä on ehtolauseen muuttujasta, joka on editoitavaa dataa.
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, fMustUsePressureInterpolation);
  if (info == 0)
    DoErrorExceptionForMetFunction(
        theAreaMaskInfo,
        ::GetDictionaryString("Can't find wanted parameter for given function"),
        ::GetDictionaryString("with line"));

  if (theAreaMaskInfo.GetUseDefaultProducer())
  {  // Pitää tehdä syvä kopio datasta, että datan muuttuminen ei vaikuta laskuihin.
    boost::shared_ptr<NFmiFastQueryInfo> tmp(dynamic_cast<NFmiFastQueryInfo *>(info->Clone()));
    info = tmp;
  }
  bool peekAlongTudes = false;
  if (funcType == NFmiAreaMask::Grad2 || funcType == NFmiAreaMask::Divergence2 ||
      funcType == NFmiAreaMask::Adv2 || funcType == NFmiAreaMask::Lap2 ||
      funcType == NFmiAreaMask::Rot2)
    peekAlongTudes = true;
  if (funcType == NFmiAreaMask::Grad || funcType == NFmiAreaMask::Grad2 ||
      funcType == NFmiAreaMask::Divergence || funcType == NFmiAreaMask::Divergence2)
  {
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskGrad(theAreaMaskInfo.GetMaskCondition(),
                                 NFmiAreaMask::kInfo,
                                 theAreaMaskInfo.GetDataType(),
                                 info,
                                 peekAlongTudes,
                                 theAreaMaskInfo.MetFunctionDirection(),
                                 theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                 NFmiAreaMask::kNoValue));
    if (funcType == NFmiAreaMask::Divergence || funcType == NFmiAreaMask::Divergence2)
      dynamic_cast<NFmiInfoAreaMaskGrad *>(areaMask.get())->CalculateDivergence(true);
  }
  else if (funcType == NFmiAreaMask::Adv || funcType == NFmiAreaMask::Adv2)
  {
    boost::shared_ptr<NFmiFastQueryInfo> infoUwind =
        CreateInfo(theAreaMaskInfo, fMustUsePressureInterpolation, kFmiWindUMS);
    boost::shared_ptr<NFmiFastQueryInfo> infoVwind =
        CreateInfo(theAreaMaskInfo, fMustUsePressureInterpolation, kFmiWindVMS);
    if (infoUwind && infoVwind)
      areaMask = boost::shared_ptr<NFmiAreaMask>(
          new NFmiInfoAreaMaskAdvection(theAreaMaskInfo.GetMaskCondition(),
                                        NFmiAreaMask::kInfo,
                                        theAreaMaskInfo.GetDataType(),
                                        info,
                                        infoUwind,
                                        infoVwind,
                                        peekAlongTudes,
                                        theAreaMaskInfo.MetFunctionDirection(),
                                        theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                        NFmiAreaMask::kNoValue));
    else
      DoErrorExceptionForMetFunction(
          theAreaMaskInfo,
          ::GetDictionaryString(
              "Can't find u- or -v wind components for wanted parameter in given function"),
          ::GetDictionaryString("with line"));
  }
  else if (funcType == NFmiAreaMask::Lap || funcType == NFmiAreaMask::Lap2)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskLaplace(theAreaMaskInfo.GetMaskCondition(),
                                    NFmiAreaMask::kInfo,
                                    theAreaMaskInfo.GetDataType(),
                                    info,
                                    peekAlongTudes,
                                    theAreaMaskInfo.MetFunctionDirection(),
                                    theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                    NFmiAreaMask::kNoValue));
  else if (funcType == NFmiAreaMask::Rot || funcType == NFmiAreaMask::Rot2)
  {
    if (theAreaMaskInfo.GetDataIdent().GetParamIdent() == kFmiTotalWindMS)
      areaMask = boost::shared_ptr<NFmiAreaMask>(
          new NFmiInfoAreaMaskRotor(theAreaMaskInfo.GetMaskCondition(),
                                    NFmiAreaMask::kInfo,
                                    theAreaMaskInfo.GetDataType(),
                                    info,
                                    peekAlongTudes,
                                    theAreaMaskInfo.MetFunctionDirection(),
                                    theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                    NFmiAreaMask::kNoValue));
    else
      DoErrorExceptionForMetFunction(
          theAreaMaskInfo,
          ::GetDictionaryString("Only usable param with rot-function in wind (=par19)"),
          ::GetDictionaryString("in the line"));
  }
  else
    DoErrorExceptionForMetFunction(
        theAreaMaskInfo,
        ::GetDictionaryString("SmartMet program error with Met-function"),
        ::GetDictionaryString("error with line"));

  return areaMask;
}

// Jos areaMaskin info on havaittua luotausdataa, pitää tehdä leveliin liittyvä fiksaus.
void NFmiSmartToolModifier::MakeSoundingLevelFix(boost::shared_ptr<NFmiAreaMask> &theAreaMask,
                                                 const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  if (theAreaMask && theAreaMaskInfo.GetLevel())
  {
    if (theAreaMask->Info()->LevelType() == kFmiSoundingLevel)
    {  // Luotaus data on poikkeus, jonka haluttu painepinta level pitää asettaa tässä erikseen.
      // Lisäksi levelType pitää vaihtaa pressuresta kFmiSoundingLevel!
      NFmiLevel soundingLevel(kFmiSoundingLevel,
                              theAreaMaskInfo.GetLevel()->GetName(),
                              theAreaMaskInfo.GetLevel()->LevelValue());
      theAreaMask->Level(soundingLevel);
    }
  }
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateInfoVariableMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);

  if (theAreaMaskInfo.TimeOffsetInHours())
  {
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiTimeShiftedInfoAreaMask(theAreaMaskInfo.GetMaskCondition(),
                                        NFmiAreaMask::kInfo,
                                        info->DataType(),
                                        info,
                                        theAreaMaskInfo.TimeOffsetInHours(),
                                        theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                        NFmiAreaMask::kNoValue));
  }
  else
  {
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMask(theAreaMaskInfo.GetMaskCondition(),
                             NFmiAreaMask::kInfo,
                             info->DataType(),
                             info,
                             theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                             NFmiAreaMask::kNoValue));
  }
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateRampFunctionMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  if (fUseLevelData)
    itsParethesisCounter++;
  NFmiInfoData::Type type = theAreaMaskInfo.GetDataType();
  if (type != NFmiInfoData::kCalculatedValue)
  {
    boost::shared_ptr<NFmiFastQueryInfo> info =
        CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiCalculationRampFuction(theAreaMaskInfo.GetMaskCondition(),
                                       NFmiAreaMask::kInfo,
                                       theAreaMaskInfo.GetDataType(),
                                       info,
                                       theAreaMaskInfo.GetDataIdent().GetParamIdent(),
                                       NFmiAreaMask::kNoValue));
  }
  else
  {
    boost::shared_ptr<NFmiAreaMask> areaMask2 = CreateCalculatedAreaMask(theAreaMaskInfo);
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiCalculationRampFuctionWithAreaMask(theAreaMaskInfo.GetMaskCondition(),
                                                   NFmiAreaMask::kInfo,
                                                   theAreaMaskInfo.GetDataType(),
                                                   areaMask2,
                                                   NFmiAreaMask::kNoValue));
  }
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateAreaIntegrationMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo,
    NFmiAreaMask::CalculationOperationType maskType,
    bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  if (theAreaMaskInfo.GetUseDefaultProducer())
  {  // Pitää tehdä syvä kopio datasta, että datan muuttuminen ei vaikuta laskuihin.
    boost::shared_ptr<NFmiFastQueryInfo> tmp(dynamic_cast<NFmiFastQueryInfo *>(info->Clone()));
    info = tmp;
  }

  if (fUseLevelData)
    itsParethesisCounter++;

  int startX = static_cast<int>(theAreaMaskInfo.GetOffsetPoint1().X());
  int startY = static_cast<int>(theAreaMaskInfo.GetOffsetPoint1().Y());
  int endX = static_cast<int>(theAreaMaskInfo.GetOffsetPoint2().X());
  int endY = static_cast<int>(theAreaMaskInfo.GetOffsetPoint2().Y());

  if (maskType == NFmiAreaMask::FunctionAreaIntergration)
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoRectAreaIntegrator(theAreaMaskInfo.GetMaskCondition(),
                                       NFmiAreaMask::kInfo,
                                       theAreaMaskInfo.GetDataType(),
                                       info,
                                       theAreaMaskInfo.GetFunctionType(),
                                       startX,
                                       endX,
                                       startY,
                                       endY,
                                       theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  else
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoTimeIntegrator(theAreaMaskInfo.GetMaskCondition(),
                                   NFmiAreaMask::kInfo,
                                   theAreaMaskInfo.GetDataType(),
                                   info,
                                   theAreaMaskInfo.GetFunctionType(),
                                   startX,
                                   startY,
                                   theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateStartParenthesisMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  if (fUseLevelData)
    itsParethesisCounter++;
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiCalculationSpecialCase(theAreaMaskInfo.GetCalculationOperator()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateEndParenthesisMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  if (fUseLevelData)
  {
    itsParethesisCounter--;
    if (itsParethesisCounter <= 0)
    {
      fHeightFunctionFlag = false;
      fUseLevelData = false;
    }
  }

  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiCalculationSpecialCase(theAreaMaskInfo.GetCalculationOperator()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateCommaOperatorMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  if (fHeightFunctionFlag)
  {
    itsCommaCounter++;
    if (itsCommaCounter >= 2)
    {                            // kun pilkku-laskuri tuli täyteen
      fUseLevelData = true;      // on aika ruveta käyttämään level-dataa infoissa
      itsParethesisCounter = 1;  // lisäksi ruvetaan metsästämään sulkuja,
                                 // että tiedetään milloin funktio ja level datan käyttö loppuu
    }
  }
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiCalculationSpecialCase(theAreaMaskInfo.GetCalculationOperator()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateMathFunctionStartMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  boost::shared_ptr<NFmiAreaMask> areaMask =
      boost::shared_ptr<NFmiAreaMask>(new NFmiCalculationSpecialCase());
  areaMask->SetMathFunctionType(theAreaMaskInfo.GetMathFunctionType());
  if (fUseLevelData)
    itsParethesisCounter++;
  return areaMask;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateThreeArgumentFunctionStartMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  boost::shared_ptr<NFmiAreaMask> areaMask =
      boost::shared_ptr<NFmiAreaMask>(new NFmiCalculationSpecialCase());
  areaMask->SetFunctionType(theAreaMaskInfo.GetFunctionType());
  areaMask->IntegrationFunctionType(theAreaMaskInfo.IntegrationFunctionType());
  if (theAreaMaskInfo.IntegrationFunctionType() == 2 ||
      theAreaMaskInfo.IntegrationFunctionType() == 3)
  {  // jos funktio oli SumZ tai MinH tyyppinen, laitetaan seuraavat jutut 'päälle'
    fHeightFunctionFlag = true;
    fUseLevelData = false;
    itsCommaCounter = 0;
  }
  return areaMask;
}

// Tutkii onko ns. synopX tapaus.
// 1. Jos on aseta tuottajan ident normaaliin kFmiSYNOP:iin
// 2. Palauta synopX tapauksen arvo
static bool SynopXCaseSettings(const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  bool synopXCase =
      theAreaMaskInfo.GetDataIdent().GetProducer()->GetIdent() == NFmiInfoData::kFmiSpSynoXProducer;
  if (synopXCase)
    theAreaMaskInfo.GetDataIdent().GetProducer()->SetIdent(kFmiSYNOP);
  return synopXCase;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateOccurrenceMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  boost::shared_ptr<NFmiArea> calculationArea(UsedMacroParamData()->Area()->Clone());
  if (theAreaMaskInfo.GetFunctionType() == NFmiAreaMask::ProbSimpleCondition)
  {
    return boost::shared_ptr<NFmiAreaMask>(new NFmiInfoAreaMaskOccurranceSimpleCondition(
        theAreaMaskInfo.GetMaskCondition(),
        NFmiAreaMask::kInfo,
        info->DataType(),
        info,
        theAreaMaskInfo.GetFunctionType(),
        theAreaMaskInfo.GetSecondaryFunctionType(),
        theAreaMaskInfo.FunctionArgumentCount(),
        calculationArea,
        theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  }
  else
  {
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskOccurrance(theAreaMaskInfo.GetMaskCondition(),
                                       NFmiAreaMask::kInfo,
                                       info->DataType(),
                                       info,
                                       theAreaMaskInfo.GetFunctionType(),
                                       theAreaMaskInfo.GetSecondaryFunctionType(),
                                       theAreaMaskInfo.FunctionArgumentCount(),
                                       calculationArea,
                                       theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  }
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateTimeRangeMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiInfoAreaMaskTimeRange(theAreaMaskInfo.GetMaskCondition(),
                                    NFmiAreaMask::kInfo,
                                    info->DataType(),
                                    info,
                                    theAreaMaskInfo.GetFunctionType(),
                                    theAreaMaskInfo.FunctionArgumentCount(),
                                    theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateSecondParamFromExtremeTimeMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  boost::shared_ptr<NFmiFastQueryInfo> secondaryParamInfo =
      CreateSecondaryParamInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiInfoAreaMaskTimeRangeSecondParValue(theAreaMaskInfo.GetMaskCondition(),
                                                  NFmiAreaMask::kInfo,
                                                  info->DataType(),
                                                  info,
                                                  secondaryParamInfo,
                                                  theAreaMaskInfo.GetFunctionType(),
                                                  theAreaMaskInfo.FunctionArgumentCount(),
                                                  theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::CreateSecondaryParamInfo(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  NFmiAreaMaskInfo secondaryParamAreaMaskInfo;
  secondaryParamAreaMaskInfo.SetDataIdent(theAreaMaskInfo.GetSecondaryParam());
  secondaryParamAreaMaskInfo.SetLevel(theAreaMaskInfo.GetSecondaryParamLevel());
  secondaryParamAreaMaskInfo.SetDataType(theAreaMaskInfo.GetSecondaryParamDataType());
  secondaryParamAreaMaskInfo.SetUseDefaultProducer(
      theAreaMaskInfo.GetSecondaryParamUseDefaultProducer());
  return CreateInfo(secondaryParamAreaMaskInfo, mustUsePressureInterpolation);
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreatePreviousFullDaysMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiInfoAreaMaskPreviousFullDays(theAreaMaskInfo.GetMaskCondition(),
                                           NFmiAreaMask::kInfo,
                                           info->DataType(),
                                           info,
                                           theAreaMaskInfo.GetFunctionType(),
                                           theAreaMaskInfo.FunctionArgumentCount(),
                                           theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateTimeDurationMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiInfoAreaMaskTimeDuration(theAreaMaskInfo.GetMaskCondition(),
                                       NFmiAreaMask::kInfo,
                                       info->DataType(),
                                       info,
                                       theAreaMaskInfo.FunctionArgumentCount(),
                                       theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateLocalExtremeMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  // Käytetty macroParam laskentahila pitää antaa luokalle että tuloshila voidaan laskea oikein
  NFmiGrid calculationGrid(itsWorkingGrid->itsArea, itsWorkingGrid->itsNX, itsWorkingGrid->itsNY);
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiLocalAreaMinMaxMask(NFmiAreaMask::kInfo,
                                  info->DataType(),
                                  info,
                                  theAreaMaskInfo.FunctionArgumentCount(),
                                  calculationGrid,
                                  theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

static bool IsProbTypeFunction(NFmiAreaMask::FunctionType functionType)
{
  if (functionType >= NFmiAreaMask::ProbOver && functionType <= NFmiAreaMask::ProbBetweenEq)
    return true;
  else
    return false;
}

static bool IsAreaProbabilityFunction(NFmiAreaMask::FunctionType functionType)
{
  if (functionType == NFmiAreaMask::ProbSimpleCondition)
    return true;
  else
    return false;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateAreaRelatedFunctionMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  auto functionType = theAreaMaskInfo.GetFunctionType();
  if (::IsAreaProbabilityFunction(functionType))
  {
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskAreaProbFunc(theAreaMaskInfo.GetMaskCondition(),
                                         NFmiAreaMask::kInfo,
                                         info->DataType(),
                                         info,
                                         functionType,
                                         theAreaMaskInfo.GetSecondaryFunctionType(),
                                         theAreaMaskInfo.FunctionArgumentCount(),
                                         theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  }
  else if (::IsProbTypeFunction(functionType))
  {
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskProbFunc(theAreaMaskInfo.GetMaskCondition(),
                                     NFmiAreaMask::kInfo,
                                     info->DataType(),
                                     info,
                                     functionType,
                                     theAreaMaskInfo.GetSecondaryFunctionType(),
                                     theAreaMaskInfo.FunctionArgumentCount(),
                                     theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  }
  else
  {
    return boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaIntegrationFunc(theAreaMaskInfo.GetMaskCondition(),
                                        NFmiAreaMask::kInfo,
                                        info->DataType(),
                                        info,
                                        functionType,
                                        theAreaMaskInfo.GetSecondaryFunctionType(),
                                        theAreaMaskInfo.FunctionArgumentCount(),
                                        theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  }
}

static void MakeMissingWorkingGridAreaError(const std::string &callingFunctionName,
                                            const std::string &errorLineText)
{
  std::string errorMessage = callingFunctionName;
  errorMessage += " - itsWorkingGrid->itsArea was nullptr for unknown reason with '";
  errorMessage += errorLineText;
  throw std::runtime_error(errorMessage);
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateClosestObsValueMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  if (info->IsGrid())
    throw std::runtime_error(
        "With closestvalue -function you must choose observation (station) data producer, "
        "not data with grid.");
  if (itsWorkingGrid->itsArea)
  {
    NFmiNearestObsValue2GridMask *nearestObsValue2GridMask =
        new NFmiNearestObsValue2GridMask(NFmiAreaMask::kInfo,
                                         info->DataType(),
                                         info,
                                         theAreaMaskInfo.FunctionArgumentCount(),
                                         theAreaMaskInfo.GetDataIdent().GetParamIdent());
    nearestObsValue2GridMask->SetGriddingHelpers(
        itsWorkingGrid->itsArea,
        itsGriddingHelper,
        NFmiPoint(itsWorkingGrid->itsNX, itsWorkingGrid->itsNY));
    boost::shared_ptr<NFmiAreaMask> areaMask =
        boost::shared_ptr<NFmiAreaMask>(nearestObsValue2GridMask);
    MakeSoundingLevelFix(areaMask, theAreaMaskInfo);
    return areaMask;
  }
  else
    ::MakeMissingWorkingGridAreaError(__FUNCTION__, theAreaMaskInfo.GetMaskText());

  // Never should get here, but compiler would make warning because it doesn't know that
  // MakeMissingWorkingGridAreaError function throws...
  return nullptr;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateLatestValueMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  if (itsWorkingGrid->itsArea)
  {
    NFmiLastTimeValueMask *latestValueMask =
        new NFmiLastTimeValueMask(NFmiAreaMask::kInfo,
                                  info->DataType(),
                                  info,
                                  theAreaMaskInfo.FunctionArgumentCount(),
                                  theAreaMaskInfo.GetDataIdent().GetParamIdent());
    auto isCalculationPointsUsed = !CalculationPoints().empty();
    latestValueMask->SetGriddingHelpers(itsWorkingGrid->itsArea,
                                        itsGriddingHelper,
                                        NFmiPoint(itsWorkingGrid->itsNX, itsWorkingGrid->itsNY),
                                        itsExtraMacroParamData.ObservationRadiusInKm(),
                                        isCalculationPointsUsed);
    boost::shared_ptr<NFmiAreaMask> areaMask = boost::shared_ptr<NFmiAreaMask>(latestValueMask);
    MakeSoundingLevelFix(areaMask, theAreaMaskInfo);
    return areaMask;
  }
  else
    ::MakeMissingWorkingGridAreaError(__FUNCTION__, theAreaMaskInfo.GetMaskText());

  // Never should get here, but compiler would make warning because it doesn't know that
  // MakeMissingWorkingGridAreaError function throws...
  return nullptr;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreatePeekTimeMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  // Pitää fiksata mahdollinen synopX tuottaja takaisin normaaliksi synop tuottajaksi, koska
  // NFmiPeekTimeMask hakee vain normaalit synop-datat ja jättää laivat ja poijut huomiotta.
  ::SynopXCaseSettings(theAreaMaskInfo);
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  return boost::shared_ptr<NFmiAreaMask>(
      new NFmiPeekTimeMask(NFmiAreaMask::kInfo,
                           info->DataType(),
                           info,
                           theAreaMaskInfo.FunctionArgumentCount(),
                           theAreaMaskInfo.GetDataIdent().GetParamIdent()));
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateNormalVertFuncMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  NFmiAreaMask::FunctionType secondaryFunc = theAreaMaskInfo.GetSecondaryFunctionType();
  fUseLevelData = true;
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  boost::shared_ptr<NFmiAreaMask> areaMask;
  if (secondaryFunc == NFmiAreaMask::TimeVertP || secondaryFunc == NFmiAreaMask::TimeVertFL ||
      secondaryFunc == NFmiAreaMask::TimeVertZ || secondaryFunc == NFmiAreaMask::TimeVertHyb)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskTimeVertFunc(theAreaMaskInfo.GetMaskCondition(),
                                         NFmiAreaMask::kInfo,
                                         info->DataType(),
                                         info,
                                         theAreaMaskInfo.GetFunctionType(),
                                         theAreaMaskInfo.GetSecondaryFunctionType(),
                                         theAreaMaskInfo.FunctionArgumentCount(),
                                         theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  else
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiInfoAreaMaskVertFunc(theAreaMaskInfo.GetMaskCondition(),
                                     NFmiAreaMask::kInfo,
                                     info->DataType(),
                                     info,
                                     theAreaMaskInfo.GetFunctionType(),
                                     theAreaMaskInfo.GetSecondaryFunctionType(),
                                     theAreaMaskInfo.FunctionArgumentCount(),
                                     theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  fUseLevelData = false;  // en tiedä pitääkö tämä laittaa takaisin falseksi, mutta laitan
                          // varmuuden vuoksi
  return areaMask;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateVertConditionalMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  fUseLevelData = true;
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  boost::shared_ptr<NFmiAreaMask> areaMask(
      new NFmiInfoAreaMaskVertConditionalFunc(theAreaMaskInfo.GetMaskCondition(),
                                              NFmiAreaMask::kInfo,
                                              info->DataType(),
                                              info,
                                              theAreaMaskInfo.GetFunctionType(),
                                              theAreaMaskInfo.GetSecondaryFunctionType(),
                                              theAreaMaskInfo.FunctionArgumentCount(),
                                              theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  fUseLevelData = false;  // en tiedä pitääkö tämä laittaa takaisin falseksi, mutta laitan
                          // varmuuden vuoksi
  return areaMask;
}

static bool IsVertConditionalFunction(const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  NFmiAreaMask::FunctionType primaryFunction = theAreaMaskInfo.GetFunctionType();
  if (primaryFunction >= NFmiInfoAreaMask::ProbOver &&
      primaryFunction <= NFmiInfoAreaMask::ProbBetweenEq)
  {
    NFmiAreaMask::FunctionType secondaryFunction = theAreaMaskInfo.GetSecondaryFunctionType();
    if (secondaryFunction >= NFmiInfoAreaMask::VertP &&
        secondaryFunction <= NFmiInfoAreaMask::VertHyb)
      return true;
  }

  return false;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateVertFunctionStartMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiAreaMask> areaMask;
  auto functionType = theAreaMaskInfo.GetSecondaryFunctionType();
  if (functionType == NFmiAreaMask::Occurrence || functionType == NFmiAreaMask::Occurrence2)
  {
    areaMask = CreateOccurrenceMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::PreviousFullDays)
  {
    areaMask = CreatePreviousFullDaysMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::TimeDuration)
  {
    areaMask = CreateTimeDurationMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::LocalExtremes)
  {
    areaMask = CreateLocalExtremeMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::TimeRange)
  {
    areaMask = CreateTimeRangeMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::SecondParamFromExtremeTime)
  {
    areaMask = CreateSecondParamFromExtremeTimeMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::AreaRect || functionType == NFmiAreaMask::AreaCircle)
  {
    areaMask = CreateAreaRelatedFunctionMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::ClosestObsValue)
  {
    areaMask = CreateClosestObsValueMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (functionType == NFmiAreaMask::PeekT)
  {
    areaMask = CreatePeekTimeMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else if (::IsVertConditionalFunction(theAreaMaskInfo))
  {
    areaMask = CreateVertConditionalMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else
  {
    areaMask = CreateNormalVertFuncMask(theAreaMaskInfo, mustUsePressureInterpolation);
  }

  return areaMask;
}

// Eri tyyppisiltä funktioilta pitää kysyä funktiotyyppiä eri funktioilla (esim. met- vs. vert
// -funktiot).
static NFmiAreaMask::FunctionType GetFunctionType(const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  NFmiAreaMask::FunctionType functionType = theAreaMaskInfo.GetSecondaryFunctionType();
  if (functionType == NFmiAreaMask::NotFunction)
    functionType = theAreaMaskInfo.GetFunctionType();
  return functionType;
}

bool NFmiSmartToolModifier::IsMultiDataSynopCase(const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  auto producerId = theAreaMaskInfo.GetDataIdent().GetProducer()->GetIdent();
  if (producerId == kFmiSYNOP || producerId == NFmiInfoData::kFmiSpSynoXProducer)
  {
    auto infoVector = itsInfoOrganizer->GetInfos(kFmiSYNOP);
    if (infoVector.size() > 1)
      return true;
  }
  return false;
}

void NFmiSmartToolModifier::DoFinalAreaMaskInitializations(
    boost::shared_ptr<NFmiAreaMask> &areaMask,
    const NFmiAreaMaskInfo &theAreaMaskInfo,
    bool &mustUsePressureInterpolation)
{
  if (areaMask)
  {
    NFmiAreaMask::CalculationOperationType maskType = theAreaMaskInfo.GetOperationType();
    if (areaMask->Info() && areaMask->Info()->Grid() == 0)
    {  // jos oli info dataa ja vielä asemadatasta, tarkistetaan että kyse oli vielä
       // infoData-tyypistä, muuten oli virheellinen lauseke
      static const std::vector<NFmiAreaMask::FunctionType> functionsThatAllowObservations{
          NFmiAreaMask::ClosestObsValue,
          NFmiAreaMask::Occurrence,
          NFmiAreaMask::Occurrence2,
          NFmiAreaMask::PeekT,
          NFmiAreaMask::TimeRange,
          NFmiAreaMask::LatestValue,
          NFmiAreaMask::PreviousFullDays,
          NFmiAreaMask::TimeDuration,
          NFmiAreaMask::AreaRect,
          NFmiAreaMask::AreaCircle,
          NFmiAreaMask::SecondParamFromExtremeTime};
      NFmiAreaMask::FunctionType functionType = ::GetFunctionType(theAreaMaskInfo);
      auto allowedIter = std::find(functionsThatAllowObservations.begin(),
                                   functionsThatAllowObservations.end(),
                                   functionType);
      auto isCalculationPointsUsed = (!CalculationPoints().empty());
      auto useSimpleConditionAreaMaskAsStationData =
          (theAreaMaskInfo.GetSecondaryFunctionType() ==
           NFmiAreaMask::SimpleConditionUsedAsStationData);
      auto isStationDataAllowingFunction = (allowedIter != functionsThatAllowObservations.end());
      auto keepStationDataForm =
          (isCalculationPointsUsed || useSimpleConditionAreaMaskAsStationData ||
           isStationDataAllowingFunction);
      if (keepStationDataForm)
      {  // tämä on ok, ei tarvitse tehdä mitään
      }
      else if (maskType == NFmiAreaMask::InfoVariable)
      {
        //        if (!keepStationDataForm)
        {
          if (itsWorkingGrid->itsArea)
          {
            boost::shared_ptr<NFmiFastQueryInfo> info = areaMask->Info();
            NFmiStation2GridMask *station2GridMask = nullptr;
            if (theAreaMaskInfo.TimeOffsetInHours())
              station2GridMask =
                  new NFmiStation2GridTimeShiftMask(areaMask->MaskType(),
                                                    areaMask->GetDataType(),
                                                    info,
                                                    theAreaMaskInfo.TimeOffsetInHours(),
                                                    theAreaMaskInfo.GetDataIdent().GetParamIdent());
            else
              station2GridMask =
                  new NFmiStation2GridMask(areaMask->MaskType(),
                                           areaMask->GetDataType(),
                                           info,
                                           theAreaMaskInfo.GetDataIdent().GetParamIdent());

            station2GridMask->SetGriddingHelpers(
                itsWorkingGrid->itsArea,
                itsGriddingHelper,
                NFmiPoint(itsWorkingGrid->itsNX, itsWorkingGrid->itsNY),
                itsExtraMacroParamData.ObservationRadiusInKm(),
                isCalculationPointsUsed);
            areaMask = boost::shared_ptr<NFmiAreaMask>(station2GridMask);
            MakeSoundingLevelFix(areaMask, theAreaMaskInfo);
          }
          else
            ::MakeMissingWorkingGridAreaError(__FUNCTION__, std::string(areaMask->MaskString()));
        }
      }
      else
      {
        std::string errStr;
        errStr += ::GetDictionaryString(
            "Trying to use unsupported smarttool function with station (non-grid) data.\n'");
        errStr += theAreaMaskInfo.GetMaskText();
        errStr += ::GetDictionaryString("' ");
        errStr += ::GetDictionaryString("in line:");
        errStr += ::GetDictionaryString("\n");
        errStr += theAreaMaskInfo.GetOrigLineText();
        throw std::runtime_error(errStr);
      }
    }

    DoSimpleConditionInitialization(areaMask, theAreaMaskInfo);

    areaMask->Initialize();  // virtuaalinen initialisointi konstruktion jälkeen
    areaMask->SetCalculationOperationType(maskType);
    if (mustUsePressureInterpolation)
    {
      areaMask->UsePressureLevelInterpolation(true);
      areaMask->UsedPressureLevelValue(theAreaMaskInfo.GetLevel()->LevelValue());
      if (theAreaMaskInfo.GetLevel()->LevelType() == kFmiFlightLevel)
        const_cast<NFmiLevel *>(areaMask->Level())
            ->SetIdent(static_cast<unsigned long>(kFmiFlightLevel));
      else if (theAreaMaskInfo.GetLevel()->LevelType() == kFmiHeight)
        const_cast<NFmiLevel *>(areaMask->Level())
            ->SetIdent(static_cast<unsigned long>(kFmiHeight));
    }
  }
}

// Simple-condition tapauksessa pitää tietää ovatko annetut maski-parametrit vertikaali dataa vai
// ei. Tehdään päätelmä käytetyn datan ja sen perusteella että onko erityistä leveliä valittuna.
static bool UsesVerticalData(boost::shared_ptr<NFmiFastQueryInfo> &usedBaseInfo,
                             const NFmiLevel *usedLevel)
{
  if (usedBaseInfo)
  {
    return usedBaseInfo->SizeLevels() > 1 && usedLevel == nullptr;
  }
  return false;
}

void NFmiSmartToolModifier::DoSimpleConditionInitialization(
    boost::shared_ptr<NFmiAreaMask> &areaMask, const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  if (areaMask && theAreaMaskInfo.AllowSimpleCondition())
  {
    auto simpleConditionInfo = theAreaMaskInfo.SimpleConditionInfo();
    if (simpleConditionInfo)
    {
      areaMask->SimpleCondition(CreateSimpleCondition(
          simpleConditionInfo, ::UsesVerticalData(areaMask->Info(), theAreaMaskInfo.GetLevel())));
    }
  }
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateSimpleConditionAreaMask(
    const NFmiAreaMaskInfo &theInfo, bool usesVerticalData)
{
  bool doUseLevelDataForThisMask = (usesVerticalData && theInfo.GetLevel() == nullptr);
  bool oldUseLevelDataValue = fUseLevelData;
  if (doUseLevelDataForThisMask)
    fUseLevelData = true;
  auto areaMask = CreateAreaMask(theInfo);
  if (doUseLevelDataForThisMask)
    fUseLevelData = oldUseLevelDataValue;
  return areaMask;
}

boost::shared_ptr<NFmiSimpleConditionPart> NFmiSmartToolModifier::CreateSimpleConditionPart(
    boost::shared_ptr<NFmiSimpleConditionPartInfo> &theSimpleConditionPartInfo,
    bool usesVerticalData)
{
  auto areaMask1 =
      CreateSimpleConditionAreaMask(*theSimpleConditionPartInfo->Mask1(), usesVerticalData);
  boost::shared_ptr<NFmiAreaMask> areaMask2;
  auto areaMaskInfo2 = theSimpleConditionPartInfo->Mask2();
  if (areaMaskInfo2)
  {
    areaMask2 = CreateSimpleConditionAreaMask(*areaMaskInfo2, usesVerticalData);
  }
  return boost::shared_ptr<NFmiSimpleConditionPart>(new NFmiSimpleConditionPart(
      areaMask1, theSimpleConditionPartInfo->CalculationOperator(), areaMask2));
}

boost::shared_ptr<NFmiSingleCondition> NFmiSmartToolModifier::CreateSingleCondition(
    boost::shared_ptr<NFmiSingleConditionInfo> &theSingleConditionInfo, bool usesVerticalData)
{
  auto part1 = CreateSimpleConditionPart(theSingleConditionInfo->Part1(), usesVerticalData);
  auto part2 = CreateSimpleConditionPart(theSingleConditionInfo->Part2(), usesVerticalData);
  boost::shared_ptr<NFmiSimpleConditionPart> part3;
  auto partInfo3 = theSingleConditionInfo->Part3();
  if (partInfo3)
  {
    part3 = CreateSimpleConditionPart(partInfo3, usesVerticalData);
  }
  return boost::shared_ptr<NFmiSingleCondition>(
      new NFmiSingleCondition(part1,
                              theSingleConditionInfo->ConditionOperand1(),
                              part2,
                              theSingleConditionInfo->ConditionOperand2(),
                              part3));
}

// Oletus: theSimpleCondition pointer on jo tarkastettu, ettei se ole nullptr
boost::shared_ptr<NFmiSimpleCondition> NFmiSmartToolModifier::CreateSimpleCondition(
    boost::shared_ptr<NFmiSimpleConditionInfo> &theSimpleConditionInfo, bool usesVerticalData)
{
  auto singlecondition1 =
      CreateSingleCondition(theSimpleConditionInfo->Condition1(), usesVerticalData);
  boost::shared_ptr<NFmiSingleCondition> singlecondition2;
  auto singleconditionInfo2 = theSimpleConditionInfo->Condition2();
  if (singleconditionInfo2)
  {
    singlecondition2 = CreateSingleCondition(singleconditionInfo2, usesVerticalData);
  }
  return boost::shared_ptr<NFmiSimpleCondition>(new NFmiSimpleCondition(
      singlecondition1, theSimpleConditionInfo->ConditionOperator(), singlecondition2));
}

//--------------------------------------------------------
// CreateAreaMask
//--------------------------------------------------------
boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateAreaMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  NFmiAreaMask::CalculationOperationType maskType = theAreaMaskInfo.GetOperationType();
  boost::shared_ptr<NFmiAreaMask> areaMask;
  bool mustUsePressureInterpolation = false;

  switch (maskType)
  {
    case NFmiAreaMask::InfoVariable:
    {
      areaMask = CreateInfoVariableMask(theAreaMaskInfo, mustUsePressureInterpolation);
      break;
    }
    case NFmiAreaMask::RampFunction:
    {
      areaMask = CreateRampFunctionMask(theAreaMaskInfo, mustUsePressureInterpolation);
      break;
    }
    case NFmiAreaMask::FunctionAreaIntergration:
    case NFmiAreaMask::FunctionTimeIntergration:
    {
      areaMask = CreateAreaIntegrationMask(theAreaMaskInfo, maskType, mustUsePressureInterpolation);
      break;
    }
    case NFmiAreaMask::FunctionPeekXY:
    case NFmiAreaMask::FunctionPeekXY2:
    case NFmiAreaMask::FunctionPeekXY3:
    {
      areaMask = CreatePeekFunctionAreaMask(theAreaMaskInfo, mustUsePressureInterpolation);
      break;
    }
    case NFmiAreaMask::MetFunction:
    {
      areaMask = CreateMetFunctionAreaMask(theAreaMaskInfo, mustUsePressureInterpolation);
      break;
    }
    case NFmiAreaMask::CalculatedVariable:
    {
      areaMask = CreateCalculatedAreaMask(theAreaMaskInfo);
      break;
    }
    case NFmiAreaMask::Constant:
    {
      areaMask = boost::shared_ptr<NFmiAreaMask>(
          new NFmiCalculationConstantValue(theAreaMaskInfo.GetMaskCondition().LowerLimit()));
      break;
    }
    case NFmiAreaMask::Operator:
    {
      areaMask = boost::shared_ptr<NFmiAreaMask>(
          new NFmiCalculationSpecialCase(theAreaMaskInfo.GetCalculationOperator()));
      break;
    }
    case NFmiAreaMask::StartParenthesis:
    {
      areaMask = CreateStartParenthesisMask(theAreaMaskInfo);
      break;
    }
    case NFmiAreaMask::EndParenthesis:
    {
      areaMask = CreateEndParenthesisMask(theAreaMaskInfo);
      break;
    }
    case NFmiAreaMask::CommaOperator:
    {
      areaMask = CreateCommaOperatorMask(theAreaMaskInfo);
      break;
    }
    case NFmiAreaMask::Comparison:
    {
      areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiCalculationSpecialCase());
      areaMask->Condition(theAreaMaskInfo.GetMaskCondition());
      break;
    }
    case NFmiAreaMask::BinaryOperatorType:
    {
      areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiCalculationSpecialCase());
      areaMask->PostBinaryOperator(theAreaMaskInfo.GetBinaryOperator());
      break;
    }
    case NFmiAreaMask::MathFunctionStart:
    {
      areaMask = CreateMathFunctionStartMask(theAreaMaskInfo);
      break;
    }
    case NFmiAreaMask::ThreeArgumentFunctionStart:
    {
      areaMask = CreateThreeArgumentFunctionStartMask(theAreaMaskInfo);
      break;
    }
    case NFmiAreaMask::VertFunctionStart:
    {
      areaMask = CreateVertFunctionStartMask(theAreaMaskInfo, mustUsePressureInterpolation);
      break;
    }
    case NFmiAreaMask::DeltaZFunction:
    {
      areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiCalculationDeltaZValue());
      break;
    }
    case NFmiAreaMask::SoundingIndexFunction:
    {
      areaMask = CreateSoundingIndexFunctionAreaMask(theAreaMaskInfo);
      break;
    }
    default:
      throw runtime_error(::GetDictionaryString("SmartToolModifierErrorStrangeDataType"));
  }

  DoFinalAreaMaskInitializations(areaMask, theAreaMaskInfo, mustUsePressureInterpolation);
  return areaMask;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateSoundingIndexFunctionAreaMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  // HUOM!! Tähän vaaditaan syvä data kopio!!!
  // JOS kyseessä on ehtolauseen muuttujasta, joka on editoitavaa dataa.
  bool mustUsePressureInterpolation =
      false;  // tätäei käytetä tässä, mutta pakko laittaa metodin interfacen takia
  boost::shared_ptr<NFmiFastQueryInfo> info =
      CreateInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  if (theAreaMaskInfo.GetUseDefaultProducer())
  {  // Pitää tehdä syvä kopio datasta, että datan muuttuminen ei vaikuta laskuihin.
    boost::shared_ptr<NFmiFastQueryInfo> tmp(dynamic_cast<NFmiFastQueryInfo *>(info->Clone()));
    info = tmp;
  }
  boost::shared_ptr<NFmiAreaMask> areaMask(new NFmiInfoAreaMaskSoundingIndex(
      info, theAreaMaskInfo.SoundingParameter(), theAreaMaskInfo.GetDataIdent().GetParamIdent()));
  return areaMask;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateCalculatedAreaMask(
    const NFmiAreaMaskInfo &theAreaMaskInfo)
{
  boost::shared_ptr<NFmiAreaMask> areaMask;
  FmiParameterName parId = FmiParameterName(theAreaMaskInfo.GetDataIdent().GetParamIdent());
  if (parId == kFmiLatitude || parId == kFmiLongitude)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiLatLonAreaMask(theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiElevationAngle)
    areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiElevationAngleAreaMask(
        theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiDay)
    areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiJulianDayAreaMask(
        theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiHour)
    areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiLocalHourAreaMask(
        theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiSecond)
    areaMask = boost::shared_ptr<NFmiAreaMask>(new NFmiUtcHourAreaMask(
        theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiMinute)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiMinuteAreaMask(theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiForecastPeriod)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiForecastHourAreaMask(itsInfoOrganizer->FindInfo(NFmiInfoData::kEditable),
                                     theAreaMaskInfo.GetDataIdent(),
                                     theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiDeltaTime)
    areaMask = boost::shared_ptr<NFmiAreaMask>(
        new NFmiTimeStepAreaMask(itsInfoOrganizer->FindInfo(NFmiInfoData::kEditable),
                                 theAreaMaskInfo.GetDataIdent(),
                                 theAreaMaskInfo.GetMaskCondition()));
  else if (parId == kFmiLastParameter)
    areaMask =
        boost::shared_ptr<NFmiAreaMask>(new NFmiGridSizeAreaMask(GetUsedEditedInfo(),
                                                                 theAreaMaskInfo.GetDataIdent(),
                                                                 theAreaMaskInfo.GetMaskCondition(),
                                                                 true));
  else if (parId == kFmiLastParameter + 1)
    areaMask =
        boost::shared_ptr<NFmiAreaMask>(new NFmiGridSizeAreaMask(GetUsedEditedInfo(),
                                                                 theAreaMaskInfo.GetDataIdent(),
                                                                 theAreaMaskInfo.GetMaskCondition(),
                                                                 false));

  if (areaMask)
    return areaMask;

  throw runtime_error(::GetDictionaryString("SmartToolModifierErrorStrangeVariable"));
}

boost::shared_ptr<NFmiDataIterator> NFmiSmartToolModifier::CreateIterator(
    const NFmiAreaMaskInfo &theAreaMaskInfo, const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  boost::shared_ptr<NFmiDataIterator> iterator;
  NFmiAreaMask::CalculationOperationType mType = theAreaMaskInfo.GetOperationType();
  switch (mType)
  {
    case NFmiAreaMask::FunctionAreaIntergration:
      // HUOM!! NFmiRelativeDataIterator:iin pitää tehdä joustavampi 'laatikon' säätö systeemi, että
      // laatikko ei olisi aina keskitetty
      iterator = boost::shared_ptr<NFmiDataIterator>(
          new NFmiRelativeDataIterator(theInfo.get(),
                                       static_cast<long>(theAreaMaskInfo.GetOffsetPoint1().X()),
                                       static_cast<long>(theAreaMaskInfo.GetOffsetPoint1().Y()),
                                       0,
                                       static_cast<long>(theAreaMaskInfo.GetOffsetPoint2().X()),
                                       static_cast<long>(theAreaMaskInfo.GetOffsetPoint2().Y()),
                                       0));
      break;
    case NFmiAreaMask::FunctionTimeIntergration:
    {
      NFmiPoint p(theAreaMaskInfo.GetOffsetPoint1());
      iterator = boost::shared_ptr<NFmiDataIterator>(new NFmiRelativeTimeIntegrationIterator2(
          theInfo.get(), static_cast<int>(p.Y() - p.X() + 1), static_cast<int>(p.Y())));
      break;
    }
    default:
      throw runtime_error(::GetDictionaryString("SmartToolModifierErrorStrangeIteratorType"));
  }
  return iterator;
}

boost::shared_ptr<NFmiAreaMask> NFmiSmartToolModifier::CreateEndingAreaMask()
{
  boost::shared_ptr<NFmiAreaMask> areaMask(new NFmiCalculationSpecialCase());
  areaMask->SetCalculationOperationType(NFmiAreaMask::EndOfOperations);
  return areaMask;
}

static bool IsBetweenValues(double value, double value1, double value2)
{
  if (value >= value1 && value <= value2)
    return true;
  if (value >= value2 && value <= value1)
    return true;
  return false;
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::GetPossibleLevelInterpolatedInfo(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  boost::shared_ptr<NFmiFastQueryInfo> info;
  boost::shared_ptr<NFmiFastQueryInfo> possiblePressureLevelDataInfo;
  if (theAreaMaskInfo.GetLevel() != 0 && theAreaMaskInfo.GetLevel()->LevelType() != kFmiHybridLevel)
  {
    bool flightLevelWanted = theAreaMaskInfo.GetLevel()->LevelType() == kFmiFlightLevel;
    std::vector<boost::shared_ptr<NFmiFastQueryInfo>> infoVector =
        itsInfoOrganizer->GetInfos(theAreaMaskInfo.GetDataIdent().GetProducer()->GetIdent());
    for (size_t i = 0; i < infoVector.size(); i++)
    {
      boost::shared_ptr<NFmiFastQueryInfo> tmpInfo = infoVector[i];
      if (flightLevelWanted ? (tmpInfo->HeightValueAvailable())
                            : (tmpInfo->PressureDataAvailable()))
      {
        if (NFmiInfoOrganizer::CheckForDataIdent(tmpInfo, theAreaMaskInfo.GetDataIdent(), true))
        {
          tmpInfo->FirstLevel();
          if (tmpInfo->Level()->GetIdent() == kFmiHybridLevel)
          {  // lähdetään tässä siitä että jos löytyy mallipinta-dataa, mikä sopii tarkoitukseen, se
            // valitaan ensisijaisesti
            info = tmpInfo;
            break;
          }
          else if (tmpInfo->Level()->GetIdent() == kFmiPressureLevel)
          {
            if (flightLevelWanted)
              possiblePressureLevelDataInfo = tmpInfo;
            else
            {
              double levelValue1 = tmpInfo->Level()->LevelValue();
              tmpInfo->LastLevel();
              double levelValue2 = tmpInfo->Level()->LevelValue();
              if (::IsBetweenValues(
                      theAreaMaskInfo.GetLevel()->LevelValue(), levelValue1, levelValue2))
              {
                possiblePressureLevelDataInfo = tmpInfo;
              }
            }
          }
        }
      }
    }
  }
  if (info == 0 && possiblePressureLevelDataInfo != 0)  // jos ei löytynyt sopivaa mallipinta-dataa,
    // mutta painepinta-dataa löytyi, otetaan se
    // käyttöön
    info = possiblePressureLevelDataInfo;
  if (info)
  {
    // Tarkistetaan vielä haluttiinko vanhaa malliajodataa
    if (theAreaMaskInfo.ModelRunIndex() < 0)
    {
      std::vector<boost::shared_ptr<NFmiFastQueryInfo>> infos =
          itsInfoOrganizer->GetInfos(info->DataFilePattern());
      if (infos.size())
        info = infos[0];
    }

    info = NFmiSmartInfo::CreateShallowCopyOfHighestInfo(info);
    mustUsePressureInterpolation = true;
  }
  return info;
}

// tämä funktio on tehty siksi että voidaan hanskata z-parametri (poikkeus) yhtenäisellä tavalla
boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::GetInfoFromOrganizer(
    const NFmiDataIdent &theIdent,
    const NFmiLevel *theLevel,
    NFmiInfoData::Type theType,
    bool fUseParIdOnly,
    bool fLevelData,
    int theModelRunIndex)
{
  boost::shared_ptr<NFmiFastQueryInfo> info = itsInfoOrganizer->Info(
      theIdent, theLevel, theType, fUseParIdOnly, fLevelData, theModelRunIndex);
  if (info == 0)
  {
    std::string parName(theIdent.GetParamName());
    NFmiStringTools::LowerCase(parName);
    if (parName == "z")
    {  // z-parametri on poikkeus (siis smarttool-kielen tekstinä annettu "z" -parametri), eli tätä
      // yritetään hakea sekä kFmiGeomHeight:ista (id=3), joka on default, ja kFmiGeopHeight:ista
      // (id=2)
      NFmiParam secondaryParam(*theIdent.GetParam());
      secondaryParam.SetIdent(kFmiGeopHeight);
      NFmiDataIdent secondaryDataIdent(secondaryParam, *theIdent.GetProducer());
      info = itsInfoOrganizer->Info(
          secondaryDataIdent, theLevel, theType, fUseParIdOnly, fLevelData, theModelRunIndex);
    }
  }
  return info;
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::CreateCopyOfAnalyzeInfo(
    const NFmiDataIdent &theDataIdent, const NFmiLevel *theLevel)
{
  boost::shared_ptr<NFmiFastQueryInfo> info =
      GetInfoFromOrganizer(theDataIdent, theLevel, NFmiInfoData::kAnalyzeData);
  if (info)
  {
    if (info->Param(static_cast<FmiParameterName>(theDataIdent.GetParamIdent())) &&
        (theLevel == 0 || info->Level(*theLevel)))
      return NFmiSmartInfo::CreateShallowCopyOfHighestInfo(info);
  }
  return info;
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::GetWantedAreaMaskData(
    const NFmiAreaMaskInfo &theAreaMaskInfo,
    bool fUseParIdOnly,
    NFmiInfoData::Type theOverRideDataType,
    FmiLevelType theOverRideLevelType)
{
  NFmiInfoData::Type usedDataType = theAreaMaskInfo.GetDataType();
  if (theOverRideDataType != NFmiInfoData::kNoDataType)
    usedDataType = theOverRideDataType;

  boost::shared_ptr<NFmiFastQueryInfo> info;
  if (theOverRideLevelType == kFmiNoLevelType)
    info = GetInfoFromOrganizer(theAreaMaskInfo.GetDataIdent(),
                                theAreaMaskInfo.GetLevel(),
                                usedDataType,
                                fUseParIdOnly,
                                fUseLevelData | fDoCrossSectionCalculation,
                                theAreaMaskInfo.ModelRunIndex());
  else
  {
    if (theAreaMaskInfo.GetLevel())  // level voi olla 0-pointteri, joten se pitää tarkistaa
    {
      NFmiLevel aLevel(*theAreaMaskInfo.GetLevel());
      aLevel.SetIdent(theOverRideLevelType);
      info = GetInfoFromOrganizer(theAreaMaskInfo.GetDataIdent(),
                                  &aLevel,
                                  usedDataType,
                                  fUseParIdOnly,
                                  fUseLevelData,
                                  theAreaMaskInfo.ModelRunIndex());
    }
  }
  UpdateInfoVariableStatistics(info);
  return NFmiSmartInfo::CreateShallowCopyOfHighestInfo(
      info);  // tehdään vielä 'kevyt' kopio löytyneestä datasta
}

void NFmiSmartToolModifier::UpdateInfoVariableStatistics(
    const boost::shared_ptr<NFmiFastQueryInfo> &info)
{
  if (info)
  {
    itsInfoVariableCount++;
    if (!info->IsGrid())
    {
      itsStationInfoVariableCount++;
      itsVariableStationCountSum += info->SizeLocations();
    }
  }
}

void NFmiSmartToolModifier::CalculateOptimalWorkingThreadCount()
{
  // MacroParam laskut sopivat rinnakkais ajoihin huonosti, maksimissaan 4 threadia,
  // jos kaikki muuttujat ovat grid datoista.
  int maxThreadCount = 4;
  if (itsInfoVariableCount && itsStationInfoVariableCount)
  {
    if (itsVariableStationCountSum < 700)
      itsOptimalThreadCount = 2;
    else
      itsOptimalThreadCount = 1;
  }
  else
    itsOptimalThreadCount = maxThreadCount;
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::CreateInfo(
    const NFmiAreaMaskInfo &theAreaMaskInfo, bool &mustUsePressureInterpolation)
{
  mustUsePressureInterpolation = false;
  boost::shared_ptr<NFmiFastQueryInfo> info;
  if (theAreaMaskInfo.GetDataType() == NFmiInfoData::kScriptVariableData)
    info = CreateScriptVariableInfo(theAreaMaskInfo.GetDataIdent());
  else if (theAreaMaskInfo.GetUseDefaultProducer() ||
           theAreaMaskInfo.GetDataType() == NFmiInfoData::kCopyOfEdited)
  {
    if (theAreaMaskInfo.GetDataType() == NFmiInfoData::kMacroParam)
    {  // tämä macroParam data viritys on multi threaddaavaa serveriä varten, eli macroparam data
       // pitää olla thread-kohtainen
      // ja se on aina annettu luodulle NFmiSmartToolModifier-luokan instansille erikseen.
      auto usedMacroParamData = UsedMacroParamData();
      if (usedMacroParamData)
        info = NFmiSmartInfo::CreateShallowCopyOfHighestInfo(usedMacroParamData);
      else
        throw runtime_error(
            "NFmiSmartToolModifier::CreateInfo - error in program, no macroParam data available.");
    }
    else
    {
      info = GetWantedAreaMaskData(theAreaMaskInfo, true);
      if (info && itsModifiedLevel)
        info->Level(*itsModifiedLevel);
    }
    if (info == 0)
      info = GetPossibleLevelInterpolatedInfo(theAreaMaskInfo, mustUsePressureInterpolation);
  }
  else
  {
    // Jos pitää käyttää level dataa (SumZ ja MinH funktiot), ei saa antaa level infoa parametrin
    // yhteydessä
    if (fUseLevelData && theAreaMaskInfo.GetLevel() != nullptr)
      throw runtime_error(::GetDictionaryString("SmartToolModifierErrorParamNoLevel") + "\n" +
                          theAreaMaskInfo.GetMaskText());

    if (fUseLevelData || fDoCrossSectionCalculation)  // jos leveldata-flagi päällä, yritetään
                                                      // ensin, löytyykö hybridi dataa
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kHybridData);
    if (info == 0)
      info = GetWantedAreaMaskData(theAreaMaskInfo, false);
    if (info == 0 &&
        theAreaMaskInfo.GetDataType() ==
            NFmiInfoData::kAnalyzeData)  // analyysi datalle piti tehdä pika viritys tähän
      info = CreateCopyOfAnalyzeInfo(theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetLevel());
    if (info == 0)
      info = GetPossibleLevelInterpolatedInfo(theAreaMaskInfo, mustUsePressureInterpolation);
    if (info == 0 &&
        theAreaMaskInfo.GetLevel() != 0)  // kokeillaan vielä jos halutaan hybridi datan leveliä
      info =
          GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kHybridData, kFmiHybridLevel);
    if (info == 0 && theAreaMaskInfo.GetLevel() !=
                         0)  // kokeillaan vielä jos halutaan 'height' (type 105) datan leveliä
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kViewable, kFmiHeight);

    if (info == 0)  // kokeillaan vielä model-help-dataa (esim. EPS-data, sounding-index-data jne)
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kModelHelpData);
    if (info == 0)  // kokeillaan vielä analyysi-dataa (esim. mesan, LAPS jne) HUOM! tämä on eri
                    // asia kuin edella oli analyysi-dataa. tässä on pyydetty PAR_PRODid
                    // yhdistelmällä analyysi-dataa
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kAnalyzeData);
    if (info == 0)  // kokeillaan vielä fraktiili-dataa (esim. EC:n fraktiili dataa jne)
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kClimatologyData);

    if (info == 0)  // kokeillaan vielä havainto dataa (eli ne on yleensä asemadataa)
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kObservations);
    if (info == 0)  // kokeillaan vielä eri level vaihtoehtoja
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kObservations, kFmiHeight);
    if (info == 0)  // kokeillaan vielä luotaus datan leveltyyppi
      info = GetWantedAreaMaskData(
          theAreaMaskInfo, false, NFmiInfoData::kObservations, kFmiSoundingLevel);
    if (info == 0)  // kokeillaan vielä yksittäisten tutkien dataa level moodissa
      info = GetWantedAreaMaskData(
          theAreaMaskInfo, false, NFmiInfoData::kSingleStationRadarData, kFmiPressureLevel);
    if (info == 0)  // kokeillaan vielä yksittäisten tutkien dataa ilman level moodia (esim.
                    // vertikaali-funktioiden yhteydessä)
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kSingleStationRadarData);
    if (info == 0)  // kokeillaan vielä salama dataa
      info = GetWantedAreaMaskData(theAreaMaskInfo, false, NFmiInfoData::kFlashData);
  }
  if (!info)
    throw runtime_error(::GetDictionaryString("SmartToolModifierErrorParamNotFound") + "\n" +
                        theAreaMaskInfo.GetMaskText());
  info->LatLon();  // tämä on varmistus uusia multi-thread laskuja varten, että jokaisella infolla
                   // on olemassa latlon-cache, ennen kuin mennään eri säikeisiin hommiin
  return info;
}

void NFmiSmartToolModifier::GetParamValueLimits(const NFmiAreaMaskInfo &theAreaMaskInfo,
                                                float *theLowerLimit,
                                                float *theUpperLimit,
                                                bool *fCheckLimits)
{
  FmiParameterName parName =
      static_cast<FmiParameterName>(theAreaMaskInfo.GetDataIdent().GetParamIdent());
  // yhdistelmä parametreille ei ole rajoja, samoin par-id kFmiBadParameter, koska silloin on kyse
  // erikoisfunktioista kuten grad tms.
  if (parName == kFmiTotalWindMS || parName == kFmiWeatherAndCloudiness ||
      parName == kFmiBadParameter)
    *fCheckLimits = false;
  else if (theAreaMaskInfo.GetDataType() == NFmiInfoData::kScriptVariableData)
    *fCheckLimits = false;
  else
  {
    boost::shared_ptr<NFmiDrawParam> drawParam = itsInfoOrganizer->CreateDrawParam(
        theAreaMaskInfo.GetDataIdent(), theAreaMaskInfo.GetLevel(), theAreaMaskInfo.GetDataType());
    if (drawParam)
    {
      *theLowerLimit = static_cast<float>(drawParam->AbsoluteMinValue());
      *theUpperLimit = static_cast<float>(drawParam->AbsoluteMaxValue());
    }
  }
}

struct FindScriptVariable
{
  FindScriptVariable(int theParId) : itsParId(theParId) {}
  bool operator()(boost::shared_ptr<NFmiFastQueryInfo> &thePtr)
  {
    return itsParId == static_cast<int>(thePtr->Param().GetParamIdent());
  }

  int itsParId;
};

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::CreateScriptVariableInfo(
    const NFmiDataIdent &theDataIdent)
{
  boost::shared_ptr<NFmiFastQueryInfo> tmp = GetScriptVariableInfo(theDataIdent);
  if (tmp)
    return NFmiSmartInfo::CreateShallowCopyOfHighestInfo(tmp);
  else  // pitää vielä luoda kyseinen skripti-muuttuja, koska sitä käytetään nyt 1. kertaa
  {
    boost::shared_ptr<NFmiFastQueryInfo> tmp2 = CreateRealScriptVariableInfo(theDataIdent);
    if (tmp2)
    {
      itsScriptVariableInfos.push_back(tmp2);
      tmp = GetScriptVariableInfo(theDataIdent);
      if (tmp)
        return NFmiSmartInfo::CreateShallowCopyOfHighestInfo(tmp);
    }
  }

  throw runtime_error(::GetDictionaryString("SmartToolModifierErrorStrange1") + " " +
                      string(theDataIdent.GetParamName()) + " " +
                      ::GetDictionaryString("SmartToolModifierErrorStrange2"));
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::GetScriptVariableInfo(
    const NFmiDataIdent &theDataIdent)
{
  std::vector<boost::shared_ptr<NFmiFastQueryInfo>>::iterator it =
      std::find_if(itsScriptVariableInfos.begin(),
                   itsScriptVariableInfos.end(),
                   FindScriptVariable(theDataIdent.GetParamIdent()));
  if (it != itsScriptVariableInfos.end())
    return *it;
  return boost::shared_ptr<NFmiFastQueryInfo>();
}

void NFmiSmartToolModifier::ClearScriptVariableInfos()
{
  itsScriptVariableInfos.clear();
}

boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::CreateRealScriptVariableInfo(
    const NFmiDataIdent &theDataIdent)
{
  boost::shared_ptr<NFmiFastQueryInfo> baseInfo = GetUsedEditedInfo();
  NFmiParamBag paramBag;
  paramBag.Add(theDataIdent);
  NFmiParamDescriptor paramDesc(paramBag);
  NFmiQueryInfo innerInfo(paramDesc,
                          itsModifiedTimes ? *itsModifiedTimes : baseInfo->TimeDescriptor(),
                          baseInfo->HPlaceDescriptor(),
                          baseInfo->VPlaceDescriptor());
  NFmiQueryData *data = new NFmiQueryData(innerInfo);
  data->Init();
  boost::shared_ptr<NFmiFastQueryInfo> returnInfo(
      new NFmiSmartInfo(data, NFmiInfoData::kScriptVariableData, "", ""));
  return returnInfo;
}

NFmiParamBag NFmiSmartToolModifier::ModifiedParams()
{
  return itsSmartToolIntepreter->ModifiedParams();
}

const std::string &NFmiSmartToolModifier::GetStrippedMacroText() const
{
  return itsSmartToolIntepreter->GetStrippedMacroText();
}

static unsigned long GetDataGridSize(boost::shared_ptr<NFmiFastQueryInfo> &data)
{
  if (data && data->IsGrid())
  {
    return data->GridXNumber() * data->GridYNumber();
  }
  return 0;
}

static boost::shared_ptr<NFmiFastQueryInfo> GetSmallerGridData(
    boost::shared_ptr<NFmiFastQueryInfo> &data1, boost::shared_ptr<NFmiFastQueryInfo> &data2)
{
  if (::GetDataGridSize(data1) <= ::GetDataGridSize(data2))
  {
    return data1;
  }
  return data2;
}

static boost::shared_ptr<NFmiFastQueryInfo> GetOptimalResolutionMacroParamData(
    bool useSpecialResolution,
    boost::shared_ptr<NFmiFastQueryInfo> &resolutionMacroParamData,
    boost::shared_ptr<NFmiFastQueryInfo> &macroParamData,
    boost::shared_ptr<NFmiFastQueryInfo> &optimizedVisualizationMacroParamData,
    bool useCalculationPoints,
    boost::shared_ptr<NFmiFastQueryInfo> &possibleFixedBaseMacroParamData)
{
  if (!NFmiSmartToolModifier::UseVisualizationOptimazation() || useCalculationPoints)
  {
    // Jos ei käytetä visualisointien optimointia tai jos käytetään
    // CalculationPoints laskentoja (ei saa harventaa optimaatioilla)
    if (possibleFixedBaseMacroParamData)
    {
      return possibleFixedBaseMacroParamData;
    }
    else if (useSpecialResolution)
    {
      return resolutionMacroParamData;
    }
    else
    {
      return macroParamData;
    }
  }
  else
  {
    if (!useSpecialResolution)
    {
      return ::GetSmallerGridData(macroParamData, optimizedVisualizationMacroParamData);
    }
    else
    {
      return ::GetSmallerGridData(resolutionMacroParamData, optimizedVisualizationMacroParamData);
    }
  }
}

// MacroParam data voi tilanteesta riippuen olla joku neljästä lähteestä:
// 1. Jos ollaan poikkileikkaus laskuissa, käytetään sille tehtyä dataa
// 2. Jos halutaan laskea harvennettua dataa symboli piirtoa varten, käytetään sitä
// 3. Jos datalle on laitettu joku tietty hila macroParam skriptissä, käytetään sitä.
// 4. Muuten käytetään yleista macroParam dataa, jolle on annettu kaikille yhteinen hilakoko
// smartTool dialogissa.
boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::UsedMacroParamData()
{
  if (fDoCrossSectionCalculation)
    return itsInfoOrganizer->CrossSectionMacroParamData();
  else if (fDoTimeSerialCalculation)
    return itsInfoOrganizer->TimeSerialMacroParamData();
  else
  {
    auto useCalculationPoints = !CalculationPoints().empty();
    auto optimalMacroParamData = ::GetOptimalResolutionMacroParamData(
        itsExtraMacroParamData.UseSpecialResolution(),
        itsExtraMacroParamData.ResolutionMacroParamData(),
        itsInfoOrganizer->MacroParamData(),
        itsInfoOrganizer->OptimizedVisualizationMacroParamData(),
        useCalculationPoints,
        itsPossibleFixedBaseMacroParamData);

    if (optimalMacroParamData == itsPossibleFixedBaseMacroParamData)
    {  // itsPossibleFixedBaseMacroParamData:lle on jo laskettu mahdolliset symboliharvennukset,
       // voidaan heti palauttaa se
      return optimalMacroParamData;
    }
    else if (!useCalculationPoints && itsPossibleSpacedOutMacroInfo)
    {
      return ::GetSmallerGridData(optimalMacroParamData, itsPossibleSpacedOutMacroInfo);
    }
    else
      return optimalMacroParamData;
  }
}

static boost::shared_ptr<NFmiFastQueryInfo> CreateCroppedInfo(
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    int x1,
    int y1,
    int x2,
    int y2,
    int columns,
    int rows)
{
  if (theInfo && theInfo->IsGrid())
  {
    auto bottomLeftLatlon = theInfo->Grid()->GridToLatLon(NFmiPoint(x1, y1));
    auto topRightLatlon = theInfo->Grid()->GridToLatLon(NFmiPoint(x2, y2));
    std::unique_ptr<NFmiArea> newAreaPtr(
        theInfo->Area()->CreateNewArea(bottomLeftLatlon, topRightLatlon));
    return NFmiInfoOrganizer::CreateNewMacroParamData_checkedInput(
        columns, rows, NFmiInfoData::kMacroParam, newAreaPtr.get());
  }
  return nullptr;
}

static int FindLowIndex(int lowLimit, int spaceOutSkipFactor)
{
  int lowIndex = 0;
  for (int index = 0; index <= lowLimit; index++)
  {
    if (index % spaceOutSkipFactor == 0)
    {
      lowIndex = index;
    }
  }
  return lowIndex;
}

static int FindHighIndex(int lowLimit, int highLimit, int gridSize, int spaceOutSkipFactor)
{
  int highIndex = lowLimit;
  for (int index = lowLimit; index <= gridSize; index++)
  {
    if (index % spaceOutSkipFactor == 0)
    {
      highIndex = index;
      if (index >= highLimit)
      {
        break;
      }
    }
  }
  return highIndex;
}

bool NFmiSmartToolModifier::MakeFixedBaseDataFromSpacedOutGrid(
    int x1, int y1, int x2, int y2, const NFmiPoint &spaceOutSkipFactors)
{
  if (itsPossibleSpacedOutMacroInfo)
  {
    auto fixedBaseData = itsExtraMacroParamData.FixedBaseDataInfo();
    if (fixedBaseData)
    {
      if (spaceOutSkipFactors.X() <= 1 && spaceOutSkipFactors.Y() <= 1)
      {
        // Kaikki croppialueen hilapisteet tulevat lopulliseen dataan
        auto columnCount = x2 - x1 + 1;
        auto rowCount = y2 - y1 + 1;
        itsPossibleFixedBaseMacroParamData =
            ::CreateCroppedInfo(fixedBaseData, x1, y1, x2, y2, columnCount, rowCount);
      }
      else
      {
        int gridSizeX = int(fixedBaseData->GridXNumber());
        int gridSizeY = int(fixedBaseData->GridYNumber());
        int spaceOutSkipFactorX = static_cast<int>(spaceOutSkipFactors.X());
        int spaceOutSkipFactorY = static_cast<int>(spaceOutSkipFactors.Y());
        // Harvennetussa symbolipiirrossa aloitetaan bottom-left kulmasta hilapisteiden piirto.
        // Sitten skipataan aina tarvittava määrä sarakkeita/rivejä ja taas piirretään symboleja.
        // 1. Etsi se x-sarake, joka on <= x1, siinä on harvennetun fiksatun datan vasen hilareuna
        int left = ::FindLowIndex(x1, spaceOutSkipFactorX);
        // 2. Etsi se x-sarake, joka on >= x2, siinä on harvennetun fiksatun datan oikea hilareuna
        int right = ::FindHighIndex(x1, x2, gridSizeX, spaceOutSkipFactorX);
        // 3. Etsi se y-sarake, joka on <= y1, siinä on harvennetun fiksatun datan ala hilareuna
        int bottom = ::FindLowIndex(y1, spaceOutSkipFactorY);
        // 4. Etsi se y-sarake, joka on >= y2, siinä on harvennetun fiksatun datan ylä hilareuna
        int top = ::FindHighIndex(y1, y2, gridSizeY, spaceOutSkipFactorY);
        // 5. Laske tuloshilan sarakemäärä
        int columnCount = ((right - left) / spaceOutSkipFactorX) + 1;
        // 6. Laske tuloshilan rivimäärä
        int rowCount = ((top - bottom) / spaceOutSkipFactorY) + 1;
        itsPossibleFixedBaseMacroParamData =
            ::CreateCroppedInfo(fixedBaseData, left, bottom, right, top, columnCount, rowCount);
        itsExtraMacroParamData.IsFixedSpacedOutDataCase(true);
      }
      return itsPossibleFixedBaseMacroParamData != nullptr;
    }
  }
  return false;
}

void NFmiSmartToolModifier::MakePossibleFixedBaseData(const NFmiPoint &spaceOutSkipFactors)
{
  auto fixedBaseData = itsExtraMacroParamData.FixedBaseDataInfo();
  if (fixedBaseData)
  {
    NFmiRect croppedXyRectOut;
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    if (GetPossibleCropGridPoints(
            fixedBaseData, itsUsedMapViewArea, croppedXyRectOut, x1, y1, x2, y2, 1.0))
    {
      if (!MakeFixedBaseDataFromSpacedOutGrid(x1, y1, x2, y2, spaceOutSkipFactors))
      {
        auto columnCount = x2 - x1 + 1;
        auto rowCount = y2 - y1 + 1;
        if (columnCount == fixedBaseData->GridXNumber() && rowCount == fixedBaseData->GridYNumber())
        {
          itsPossibleFixedBaseMacroParamData =
              NFmiInfoOrganizer::CreateNewMacroParamData_checkedInput(
                  columnCount, rowCount, NFmiInfoData::kMacroParam, fixedBaseData->Area());
        }
        else
        {
          itsPossibleFixedBaseMacroParamData =
              ::CreateCroppedInfo(fixedBaseData, x1, y1, x2, y2, columnCount, rowCount);
        }
      }
    }
    else if (!NFmiQueryDataUtil::AreAreasSameKind(fixedBaseData->Area(), itsUsedMapViewArea.get()))
    {
      if (!itsExtraMacroParamData.ResolutionMacroParamData())
      {
        // Jos fixed-datan ja karttanäytön karttaprojektiot olivat eri tyyppisiä ja
        // ei oltu määritelty resolution-based macroParam hilaa, yritetään luoda sellainen käyttäen
        // fixedBaseDatan resoluutiota.
        itsExtraMacroParamData.UseDataForResolutionCalculations(
            itsUsedMapViewArea.get(),
            fixedBaseData,
            itsExtraMacroParamData.WantedFixedBaseData().originalDataString_);
      }
    }
  }
}

const std::vector<NFmiPoint> &NFmiSmartToolModifier::CalculationPoints() const
{
  return itsExtraMacroParamData.CalculationPoints();
}

void NFmiSmartToolModifier::ModifiedLevel(boost::shared_ptr<NFmiLevel> &theLevel)
{
  itsModifiedLevel = theLevel;
}

void NFmiSmartToolModifier::SetGriddingHelper(NFmiGriddingHelperInterface *theGriddingHelper)
{
  itsGriddingHelper = theGriddingHelper;
}

// Tätä on kutsuttava vasta kun macroParam skripti on tulkattu, jotta tiedetään kaikki olosuhteet.
// Esim. Jos käytössä CalculationPoint:eja, ei käytetä harvennettua hilaa, koska data lasketaan
// optimoidusti vain tiettyihin hilapisteisiin.
void NFmiSmartToolModifier::SetPossibleSpacedOutMacroInfo(
    boost::shared_ptr<NFmiFastQueryInfo> &possibleSpacedOutMacroInfo)
{
  if (CalculationPoints().empty())
  {
    itsPossibleSpacedOutMacroInfo = possibleSpacedOutMacroInfo;
  }
}

void NFmiSmartToolModifier::SetUsedMapViewArea(boost::shared_ptr<NFmiArea> &usedMapViewArea)
{
  itsUsedMapViewArea = usedMapViewArea;
}

// 'Editoitu' data (perus pohjadata mm. skripti muuttuja infoille) voi tulla eri tilanteissa
// kahdesta lähteestä:
// 1. Se on oikeasti editoitua dataa
// 2. Se on macroParam laskuissa joku MacroParamData
boost::shared_ptr<NFmiFastQueryInfo> NFmiSmartToolModifier::GetUsedEditedInfo()
{
  if (this->fMacroParamCalculation)
    return UsedMacroParamData();
  else
    return itsInfoOrganizer->FindInfo(NFmiInfoData::kEditable);
}

bool NFmiSmartToolModifier::UseVisualizationOptimazation()
{
  return fUseVisualizationOptimazation;
}
void NFmiSmartToolModifier::UseVisualizationOptimazation(bool newState)
{
  fUseVisualizationOptimazation = newState;
}

// NFmiRect on käänteisessä maailmassa, pitää tehdä oma pikku rect-viritelmä.
struct MRect
{
  MRect(const NFmiRect &theRect)
      : x1(theRect.Left()), y1(theRect.Top()), x2(theRect.Right()), y2(theRect.Bottom())
  {
  }

  double x1;
  double y1;
  double x2;
  double y2;
};

// laskee annettujen suorakulmioiden avulla halutut datan croppauksessa käytetyt xy-pisteet.
// Eli leikkaus pinnan vasen ala ja oikea ylä kulmat.
// Oletus, annetut suorakulmiot leikkaavat.
static void CalcXYCropPoints(const MRect &theDataRect,
                             const MRect &theViewRect,
                             NFmiPoint &theBLXYCropPoint,
                             NFmiPoint &theTRXYCropPoint)
{
  theBLXYCropPoint.X(FmiMax(theDataRect.x1, theViewRect.x1));
  theBLXYCropPoint.Y(FmiMax(theDataRect.y1, theViewRect.y1));

  theTRXYCropPoint.X(FmiMin(theDataRect.x2, theViewRect.x2));
  theTRXYCropPoint.Y(FmiMin(theDataRect.y2, theViewRect.y2));

  // Tämä on ikävää koodia, mutta siivoan jos jaksan, heh hee...
  // käännän y-akselin jälleen
  double tmp = theBLXYCropPoint.Y();
  theBLXYCropPoint.Y(theTRXYCropPoint.Y());
  theTRXYCropPoint.Y(tmp);
}

// Laskee, leikkaavatko annetut suorakulmiot
static bool AreRectsIntersecting(const MRect &theRect1, const MRect &theRect2)
{
  if (FmiMax(theRect1.x1, theRect2.x1) > FmiMin(theRect1.x2, theRect2.x2) ||
      FmiMax(theRect1.y1, theRect2.y1) > FmiMin(theRect1.y2, theRect2.y2))
    return false;
  else
    return true;
}

static const double g_GridCoordinateErrorLimit = 0.000001;

static int GetFlooredGridPointValue(double gridCoordinate)
{
  if (std::fabs(gridCoordinate - static_cast<int>(gridCoordinate)) < g_GridCoordinateErrorLimit)
  {
    // tietyissä tapauksissa halutaan pyöristää lähimpää,
    // kun double virhe saattaa aiheuttaa ongelmia
    return boost::math::iround(gridCoordinate);
  }
  else
  {
    // muuten floor-toiminto
    return static_cast<int>(gridCoordinate);
  }
}

static int GetCeilingGridPointValue(double gridCoordinate)
{
  if (std::fabs(gridCoordinate - static_cast<int>(gridCoordinate)) < g_GridCoordinateErrorLimit)
  {
    // tietyissä tapauksissa halutaan pyöristää lähimpää,
    // kun double virhe saattaa aiheuttaa ongelmia
    return boost::math::iround(gridCoordinate);
  }
  else
  {
    // muuten ceil-toiminto
    return static_cast<int>(std::ceil(gridCoordinate));
  }
}

static bool DoPreliminaryCroppingDataChecks(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                            boost::shared_ptr<NFmiArea> &theMapArea)
{
  if (theInfo && theMapArea)
  {
    if (theInfo->IsGrid())
    {
      if (NFmiQueryDataUtil::AreAreasSameKind(theInfo->Area(), theMapArea.get()))
      {
        return true;
      }
    }
  }
  // Annettua dataa ei voi cropata mielekkäästi annetun alueen avulla
  return false;
}

bool NFmiSmartToolModifier::GetPossibleCropGridPoints(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                                      boost::shared_ptr<NFmiArea> &theMapArea,
                                                      NFmiRect &theCroppedXyRectOut,
                                                      int &x1,
                                                      int &y1,
                                                      int &x2,
                                                      int &y2,
                                                      double acceptedMaxPercentage01)
{
  if (::DoPreliminaryCroppingDataChecks(theInfo, theMapArea))
  {
    // lasketaan ensin xy-pisteet, johon datan croppaus rajoittuu
    NFmiRect zoomInDataAreaXYRect(theInfo->Area()->XYArea(theMapArea.get()));
    if (zoomInDataAreaXYRect.Width() == 0 || zoomInDataAreaXYRect.Height() == 0)
    {
      // En tiedä miten latin-america data vs. pacific-world croppaus tarkastelu
      // pitäisi hoitaa, mutta tämä on quick-fix siihen, syntyvän rectin width on 0.
      return false;
    }
    NFmiPoint blXYCropPoint(zoomInDataAreaXYRect.BottomLeft());
    NFmiPoint trXYCropPoint(zoomInDataAreaXYRect.TopRight());
    if (AreRectsIntersecting(theInfo->Area()->XYArea(), zoomInDataAreaXYRect))
    {
      // jos ei leikannut (ja ne ovat sisäkkäin, koska jos ne ovat pois toistensä päältä, täällä ei
      // oltaisi), laske käytetyt leikkaus pisteet
      CalcXYCropPoints(
          theInfo->Area()->XYArea(), zoomInDataAreaXYRect, blXYCropPoint, trXYCropPoint);
    }

    // zoomaus alueen pitää siis olla kokonaan datan sisällä
    NFmiPoint blGridPoint(theInfo->Grid()->XYToGrid(blXYCropPoint));
    NFmiPoint trGridPoint(theInfo->Grid()->XYToGrid(trXYCropPoint));
    x1 = ::GetFlooredGridPointValue(blGridPoint.X());
    y1 = ::GetFlooredGridPointValue(blGridPoint.Y());
    x2 = ::GetCeilingGridPointValue(trGridPoint.X());
    y2 = ::GetCeilingGridPointValue(trGridPoint.Y());

    double gridCountTotal = theInfo->SizeLocations();
    double columnCount = (x2 - x1 + 1);
    double rowCount = (y2 - y1 + 1);
    double gridCountZoomed = columnCount * rowCount;
    if (columnCount < 2 || rowCount < 2)
    {
      // Pitää tulla vähintään 2x2 hila, muuten ei ole järkeä ja koodissa on vikaa
      return false;
    }
    if (gridCountZoomed > gridCountTotal)
    {
      // Nyt oli jotain vikaa, eihä tässä näin pitäisi käydä, koodissa on vikaa?
      return false;
    }
    if (gridCountZoomed / gridCountTotal <= acceptedMaxPercentage01)
    {
      // Laitetaan joku prosentti raja siihen milloin kannattaa vielä tehdään zoomatun datan piirtoa
      // erikoiskikoin Esim. jos zoomatun alueen hilapisteet ovat alta 90% datan kokonais
      // hilapisteistä, kannattaa optimointi isoviivapiirroissa, mutta muuten ehkä ei. Oletus arvo
      // on 1.0 eli sallii originaali hilan palautuksen (mitä tarvitaan tietyissä tilanteissa).

      // zoomatun alueen rect:i pitää vielä laskea
      NFmiPoint zoomedBottomLeftLatlon(theInfo->Grid()->GridToLatLon(NFmiPoint(x1, y1)));
      NFmiPoint zoomedTopRightLatlon(theInfo->Grid()->GridToLatLon(NFmiPoint(x2, y2)));
      NFmiArea *newZoomedArea = const_cast<NFmiArea *>(theInfo->Area())
                                    ->NewArea(zoomedBottomLeftLatlon, zoomedTopRightLatlon);
      if (newZoomedArea)
      {
        newZoomedArea->SetXYArea(NFmiRect(0, 0, 1, 1));
        theCroppedXyRectOut = newZoomedArea->XYArea(theMapArea.get());
        delete newZoomedArea;
        return true;
      }
    }
  }
  return false;
}
