#include "NFmiFastInfoUtils.h"

#include "NFmiFastQueryInfo.h"
#include "NFmiFileString.h"
#include "NFmiProducerName.h"

namespace
{
template <typename VectorContainer, typename Operation>
VectorContainer VectorCalculations(const VectorContainer &v1,
                                   const VectorContainer &v2,
                                   Operation oper)
{
  VectorContainer result;
  if (v1.size() == v2.size())
  {
    result.resize(v1.size(), kFloatMissing);
    for (size_t index = 0; index < v1.size(); index++)
    {
      result[index] = oper(v1[index], v2[index]);
    }
  }
  return result;
}

template <typename MatrixContainer, typename Operation>
MatrixContainer MatrixCalculations(const MatrixContainer &m1,
                                   const MatrixContainer &m2,
                                   unsigned int theStartColumnIndex,
                                   Operation oper)
{
  MatrixContainer result;
  if (m1.NX() == m2.NX() && m1.NY() == m2.NY())
  {
    result.Resize(m1.NX(), m2.NY(), kFloatMissing);
    for (size_t yIndex = 0; yIndex < m1.NY(); yIndex++)
    {
      for (size_t xIndex = theStartColumnIndex; xIndex < m1.NX(); xIndex++)
      {
        result[xIndex][yIndex] = oper(m1[xIndex][yIndex], m2[xIndex][yIndex]);
      }
    }
  }
  return result;
}

// Huomasin merkitt�vi� eroja meta wind-vector visualisoinnissa, kun
// wind-vector:ia lasketaan u- ja v-komponenttien avulla. T�ll�in
// neli�inti ja neli�juuri saivat arvot menem��n juuri pikkuisen
// alle tiettyjen rajojen, jollain visualisointi oli erilainen. Nyt
// WS py�ristet��n 1/1-milj. osissa l�himp��n kokonaislukuun, jolloin p��st��n ongelmasta eroon.
float DoWindSpeedFromWindComponentsRoundingFix(float realWS)
{
  const float roundingFactor = 1000000.f;
  float roundingFixerValue = std::round(realWS * roundingFactor) / roundingFactor;
  return roundingFixerValue;
}

bool AreWindComponentsOk(float u, float v) { return u != kFloatMissing && v != kFloatMissing; }

template <typename GetFunction>
float CalcMetaWindVectorValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                              const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage,
                              GetFunction getFunction)
{
  NFmiFastInfoUtils::QueryInfoParamStateRestorer restorer(*theInfo);
  if (metaWindParamUsage.HasWsAndWd())
  {
    theInfo->Param(kFmiWindSpeedMS);
    auto WS = getFunction();
    theInfo->Param(kFmiWindDirection);
    auto WD = getFunction();
    return NFmiFastInfoUtils::CalcWindVectorFromSpeedAndDirection(WS, WD);
  }
  else if (metaWindParamUsage.HasWindComponents())
  {
    theInfo->Param(kFmiWindUMS);
    auto u = getFunction();
    theInfo->Param(kFmiWindVMS);
    auto v = getFunction();
    return NFmiFastInfoUtils::CalcWindVectorFromWindComponents(u, v);
  }

  return kFloatMissing;
}

template <typename GetFunction>
std::pair<float, float> GetMetaWsWdValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage,
    GetFunction getFunction)
{
  if (metaWindParamUsage.HasWindComponents())
  {
    NFmiFastInfoUtils::QueryInfoParamStateRestorer restorer(*theInfo);
    theInfo->Param(kFmiWindUMS);
    auto u = getFunction();
    theInfo->Param(kFmiWindVMS);
    auto v = getFunction();
    float WS = NFmiFastInfoUtils::CalcWS(u, v);
    float WD = NFmiFastInfoUtils::CalcWD(u, v);
    return std::make_pair(WS, WD);
  }

  return std::make_pair(kFloatMissing, kFloatMissing);
}

template <typename GetFunction>
std::pair<float, float> CalcMetaWindComponentsValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage,
    GetFunction getFunction)
{
  if (metaWindParamUsage.HasWsAndWd())
  {
    NFmiFastInfoUtils::QueryInfoParamStateRestorer restorer(*theInfo);
    theInfo->Param(kFmiWindSpeedMS);
    auto WS = getFunction();
    theInfo->Param(kFmiWindDirection);
    auto WD = getFunction();
    float u = NFmiFastInfoUtils::CalcU(WS, WD);
    float v = NFmiFastInfoUtils::CalcV(WS, WD);
    return std::make_pair(u, v);
  }

  return std::make_pair(kFloatMissing, kFloatMissing);
}

float GetMetaWindVectorValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindVectorValue(
      theInfo, metaWindParamUsage, [&]() { return theInfo->FloatValue(); });
}

float GetMetaWindVectorValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             const NFmiMetTime &theTime,
                             const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindVectorValue(
      theInfo, metaWindParamUsage, [&]() { return theInfo->InterpolatedValue(theTime); });
}

float GetMetaWindVectorValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             const NFmiPoint &theLatlon,
                             const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindVectorValue(
      theInfo, metaWindParamUsage, [&]() { return theInfo->InterpolatedValue(theLatlon); });
}

float GetMetaWindVectorValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             const NFmiMetTime &theTime,
                             const NFmiPoint &theLatlon,
                             const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindVectorValue(theInfo,
                                   metaWindParamUsage,
                                   [&]()
                                   { return theInfo->InterpolatedValue(theLatlon, theTime); });
}

std::pair<float, float> GetMetaWsWdValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::GetMetaWsWdValues(theInfo, metaWindParamUsage, [&]() { return theInfo->FloatValue(); });
}

std::pair<float, float> GetMetaWsWdValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiMetTime &theTime,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::GetMetaWsWdValues(
      theInfo, metaWindParamUsage, [&]() { return theInfo->InterpolatedValue(theTime); });
}

std::pair<float, float> GetMetaWsWdValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiPoint &theLatlon,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::GetMetaWsWdValues(
      theInfo, metaWindParamUsage, [&]() { return theInfo->InterpolatedValue(theLatlon); });
}

std::pair<float, float> GetMetaWsWdValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiMetTime &theTime,
    const NFmiPoint &theLatlon,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::GetMetaWsWdValues(theInfo,
                             metaWindParamUsage,
                             [&]() { return theInfo->InterpolatedValue(theLatlon, theTime); });
}

std::pair<float, float> GetMetaWindComponentsValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindComponentsValues(
      theInfo, metaWindParamUsage, [&]() { return theInfo->FloatValue(); });
}

std::pair<float, float> GetMetaWindComponentsValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiMetTime &theTime,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindComponentsValues(
      theInfo, metaWindParamUsage, [&]() { return theInfo->InterpolatedValue(theTime); });
}

std::pair<float, float> GetMetaWindComponentsValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiPoint &theLatlon,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindComponentsValues(
      theInfo, metaWindParamUsage, [&]() { return theInfo->InterpolatedValue(theLatlon); });
}

std::pair<float, float> GetMetaWindComponentsValues(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiMetTime &theTime,
    const NFmiPoint &theLatlon,
    const NFmiFastInfoUtils::MetaWindParamUsage &metaWindParamUsage)
{
  return ::CalcMetaWindComponentsValues(theInfo,
                                        metaWindParamUsage,
                                        [&]()
                                        { return theInfo->InterpolatedValue(theLatlon, theTime); });
}
}  // namespace

namespace NFmiFastInfoUtils
{
bool IsInfoShipTypeData(NFmiFastQueryInfo &theInfo)
{
  if (theInfo.IsGrid() == false)
  {
    FmiProducerName prodId = static_cast<FmiProducerName>(theInfo.Producer()->GetIdent());
    if (prodId == kFmiSHIP || prodId == kFmiBUOY) return true;
  }
  return false;
}

// synop-dataa ei tarvitsee asettaa mihink��n, mutta
// mm. luotaus plotin yhteydess� pit�� etsi� oikea korkeus/level
void SetSoundingDataLevel(const NFmiLevel &theWantedSoundingPressureLevel, NFmiFastQueryInfo &info)
{
  if (theWantedSoundingPressureLevel.LevelType() == kFmiSoundingLevel)
  {  // sounding plotin yhteydess� pit�� asettaa level haluttuun korkeuteen
    float levelValue = theWantedSoundingPressureLevel.LevelValue();
    FmiParameterName parName = static_cast<FmiParameterName>(info.Param().GetParamIdent());
    unsigned long parIndex = info.ParamIndex();
    bool subParaUsed = info.IsSubParamUsed();
    if (info.Param(kFmiPressure))
    {
      for (info.ResetLevel(); info.NextLevel();)
        if (info.FloatValue() == levelValue) break;
    }
    if (subParaUsed)
      info.Param(parName);  // pakko vet�� t�m� hitaalla tavalla jostain syyst�
    else
      info.ParamIndex(parIndex);
  }
}

std::string GetTotalDataFilePath(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  NFmiFileString totalFilePath(theInfo->DataFilePattern());
  totalFilePath.FileName(theInfo->DataFileName());
  return std::string(totalFilePath);
}

bool IsYearLongData(const boost::shared_ptr<NFmiFastQueryInfo> &info)
{
  const auto &timeDescriptor = info->TimeDescriptor();
  auto timeDiffInDays = timeDescriptor.LastTime().DifferenceInDays(timeDescriptor.FirstTime());
  if (timeDiffInDays >= 364 && timeDiffInDays <= 366)
    return true;
  else
    return false;
}

bool IsModelClimatologyData(const boost::shared_ptr<NFmiFastQueryInfo> &info)
{
  if (info && info->DataType() == NFmiInfoData::kClimatologyData)
  {
    if (info->IsGrid())
    {
      if (IsYearLongData(info))
      {
        return true;
      }
    }
  }
  return false;
}

NFmiMetTime GetUsedTimeIfModelClimatologyData(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                              const NFmiMetTime &theTime)
{
  if (theInfo)
  {
    if (NFmiFastInfoUtils::IsModelClimatologyData(theInfo))
    {
      // For year long climatology data, used time must be fixed to data's own year
      auto usedTime(theTime);
      usedTime.SetYear(theInfo->TimeDescriptor().FirstTime().GetYear());
      return usedTime;
    }
    else if (theInfo->DataType() == NFmiInfoData::kStationary)
    {
      // Stationaarisissa datoissa (esim. topograafiset datat) on vain 1. aika, joka on fiksattu kun
      // data on tehty
      return theInfo->TimeDescriptor().FirstTime();
    }
  }

  return theTime;
}

bool IsMovingSoundingData(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  if (theInfo && !theInfo->IsGrid())
  {
    if (theInfo->SizeLevels() > 7) return theInfo->HasLatlonInfoInData();
  }
  return false;
}

// Lightning eli salama tyyppi datalla on yksi feikki asema ja paljon aikoja, jos useita salamoita
// tai vastaavia havaintoja on havaittu. Datassa pit�� olla latitude ja longitude parametrit,
// jotta jokaisella satunnaiselle havainnolle saadaan kulloiseenkin aikaan oikea lokaatio.
bool IsLightningTypeData(boost::shared_ptr<NFmiFastQueryInfo> &info)
{
  if (info && !info->IsGrid())
  {
    if (info->SizeLocations() == 1 && info->HasLatlonInfoInData())
    {
      return true;
    }
  }
  return false;
}

bool FindTimeIndicesForGivenTimeRange(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                      const NFmiMetTime &theStartTime,
                                      long minuteRange,
                                      unsigned long &timeIndex1,
                                      unsigned long &timeIndex2)
{
  theInfo->FindNearestTime(theStartTime, kForward);
  timeIndex1 = theInfo->TimeIndex();
  NFmiMetTime endTime(theStartTime);
  endTime.ChangeByMinutes(minuteRange);
  theInfo->FindNearestTime(endTime, kBackward);
  timeIndex2 = theInfo->TimeIndex();

  if (timeIndex1 == gMissingIndex || timeIndex2 == gMissingIndex)
    return false;
  else if (timeIndex1 == timeIndex2)
  {
    // pit�� testata erikoistapaus, koska TimeToNearestStep-palauttaa aina jotain, jos on dataa
    theInfo->TimeIndex(timeIndex1);
    const auto &foundTime = theInfo->Time();
    if (foundTime > endTime || foundTime < theStartTime)
    {
      // jos l�ydetty aika on alku ja loppu ajan ulkopuolella ei piirret� salamaa
      return false;
    }
  }

  // Viel� 2. erikoistapaus: Kyse timeList datasta, jossa loppuajalle l�ytyy useita per�kk�isi�
  // aikoja, t�ll�in FindNearestTime palauttaa 1. l�yt�m�ns� halutun ajan. Huom! t�ss� k�ytet��n
  // FindNearestTime metodia, koska se k�ytt�� timelist tapauksissa binary search hakua, Jos t�m�n
  // funktion tekisi k�ym��n l�pi kaikki ajat j�rjestyksess�, tulisi siit� tietyille datoille
  // tuskallisen hidas.
  theInfo->TimeIndex(timeIndex2);
  for (auto timeIndex = timeIndex2 + 1; timeIndex < theInfo->SizeTimes(); timeIndex++)
  {
    theInfo->TimeIndex(timeIndex);
    if (theInfo->Time() == endTime)
    {
      timeIndex2 = timeIndex;
    }
    else
    {
      break;
    }
  }

  return true;
}

bool FindMovingSoundingDataTime(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                const NFmiMetTime &theTime,
                                NFmiLocation &theLocation)
{
  theInfo->FirstLocation();  // liikkuvissa luotauksissa vain yksi dummy paikka, laitetaan se p��lle
  NFmiMetTime timeStart(theTime);
  timeStart.ChangeByMinutes(-30);
  unsigned long timeIndex1 = 0;
  unsigned long timeIndex2 = 0;
  if (FindTimeIndicesForGivenTimeRange(theInfo, timeStart, 60, timeIndex1, timeIndex2) == false)
    return false;

  float lat = 0;
  float lon = 0;
  theInfo->Param(kFmiLatitude);
  unsigned long latIndex = theInfo->ParamIndex();
  theInfo->Param(kFmiLongitude);
  unsigned long lonIndex = theInfo->ParamIndex();
  double minDistance = 99999999;
  unsigned long minDistTimeInd = static_cast<unsigned long>(-1);
  for (unsigned long i = timeIndex1; i <= timeIndex2; i++)
  {
    theInfo->TimeIndex(i);

    for (theInfo->ResetLevel(); theInfo->NextLevel();)
    {
      theInfo->ParamIndex(latIndex);
      lat = theInfo->FloatValue();
      theInfo->ParamIndex(lonIndex);
      lon = theInfo->FloatValue();

      if (lat != kFloatMissing && lon != kFloatMissing)
      {
        NFmiLocation loc(NFmiPoint(lon, lat));
        double currDist = theLocation.Distance(loc);
        if (currDist < minDistance)
        {
          minDistance = currDist;
          minDistTimeInd = i;
        }
      }
    }
  }
  if (minDistance < 1000 * 1000)  // jos liikkuva luotaus paikka l�ytyi v�hint�in 1000 km s�teelt�
                                  // hiiren klikkauspaikasta, otetaan kyseinen luotaus piirtoon
  {
    theInfo->TimeIndex(minDistTimeInd);
    // pit�� lis�ksi asettaa locationiksi luotauksen alkupiste
    theInfo->FirstLevel();
    theInfo->ParamIndex(latIndex);
    lat = theInfo->FloatValue();
    theInfo->ParamIndex(lonIndex);
    lon = theInfo->FloatValue();
    theLocation.SetLatitude(lat);
    theLocation.SetLongitude(lon);

    return true;
  }

  return false;
}

QueryInfoParamStateRestorer::QueryInfoParamStateRestorer(NFmiQueryInfo &info)
    : info_(info),
      paramId_(info.ParamIndex() != gMissingIndex
                   ? static_cast<FmiParameterName>(info.Param().GetParamIdent())
                   : kFmiBadParameter)
{
}

QueryInfoParamStateRestorer::~QueryInfoParamStateRestorer()
{
  // Parametrin asetuksella nollataan/asetetaan mahd. combinedParam jutut
  info_.Param(paramId_);
}

QueryInfoTotalStateRestorer::QueryInfoTotalStateRestorer(NFmiQueryInfo &info)
    : QueryInfoParamStateRestorer(info),
      locationIndex_(info.LocationIndex()),
      timeIndex_(info.TimeIndex()),
      levelIndex_(info.LevelIndex())
{
}

QueryInfoTotalStateRestorer::~QueryInfoTotalStateRestorer()
{
  info_.LocationIndex(locationIndex_);
  info_.TimeIndex(timeIndex_);
  info_.LevelIndex(levelIndex_);
}

MetaWindParamUsage::MetaWindParamUsage() = default;

MetaWindParamUsage::MetaWindParamUsage(bool hasTotalWind,
                                       bool hasWindVectorParam,
                                       bool hasWsAndWd,
                                       bool hasWindComponents)
    : fHasTotalWind(hasTotalWind),
      fHasWindVectorParam(hasWindVectorParam),
      fHasWsAndWd(hasWsAndWd),
      fHasWindComponents(hasWindComponents)
{
}

bool MetaWindParamUsage::ParamNeedsMetaCalculations(unsigned long paramId) const
{
  if (fHasTotalWind) return false;

  switch (paramId)
  {
    case kFmiWindDirection:
    case kFmiWindSpeedMS:
      return MakeMetaWsAndWdParams();
    case kFmiWindVectorMS:
      return MakeMetaWindVectorParam();
    case kFmiWindUMS:
    case kFmiWindVMS:
      return MakeMetaWindComponents();
    default:
      return false;
  }
}

bool MetaWindParamUsage::NoWindMetaParamsNeeded() const
{
  return fHasTotalWind || (fHasWindVectorParam && fHasWsAndWd && fHasWindComponents);
}

bool MetaWindParamUsage::MakeMetaWindVectorParam() const
{
  return (!fHasWindVectorParam) && (HasWsAndWd() || HasWindComponents());
}

bool MetaWindParamUsage::MakeMetaWsAndWdParams() const
{
  return (!HasWsAndWd() && HasWindComponents());
}

bool MetaWindParamUsage::MakeMetaWindComponents() const
{
  return (HasWsAndWd() && !HasWindComponents());
}

bool MetaWindParamUsage::IsStreamlinePossible() const
{
  return fHasTotalWind || HasWsAndWd() || HasWindComponents();
}

MetaWindParamUsage CheckMetaWindParamUsage(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  if (theInfo)
    return CheckMetaWindParamUsage(*theInfo);
  else
    return MetaWindParamUsage();
}

MetaWindParamUsage CheckMetaWindParamUsage(NFmiQueryInfo &theInfo)
{
  QueryInfoParamStateRestorer restorer(theInfo);
  MetaWindParamUsage metaWindParamUsage;
  if (theInfo.Param(kFmiTotalWindMS)) metaWindParamUsage.fHasTotalWind = true;

  if (theInfo.Param(kFmiWindVectorMS)) metaWindParamUsage.fHasWindVectorParam = true;

  if (theInfo.Param(kFmiWindDirection) && theInfo.Param(kFmiWindSpeedMS))
    metaWindParamUsage.fHasWsAndWd = true;

  if (theInfo.Param(kFmiWindUMS) && theInfo.Param(kFmiWindVMS))
    metaWindParamUsage.fHasWindComponents = true;

  return metaWindParamUsage;
}

std::vector<std::unique_ptr<NFmiDataIdent>> MakePossibleWindMetaParams(
    NFmiQueryInfo &theInfo, bool allowStreamlineParameter)
{
  static const NFmiParam windDirectionBaseParam(kFmiWindDirection,
                                                "Wind direction (meta)",
                                                kFloatMissing,
                                                kFloatMissing,
                                                kFloatMissing,
                                                kFloatMissing,
                                                "%.1f",
                                                kLinearly);
  static const NFmiParam windSpeedBaseParam(kFmiWindSpeedMS,
                                            "Wind speed (meta)",
                                            kFloatMissing,
                                            kFloatMissing,
                                            kFloatMissing,
                                            kFloatMissing,
                                            "%.1f",
                                            kLinearly);
  static const NFmiParam windVectorBaseParam(kFmiWindVectorMS,
                                             "Wind vector (meta)",
                                             kFloatMissing,
                                             kFloatMissing,
                                             kFloatMissing,
                                             kFloatMissing,
                                             "%.1f",
                                             kLinearly);
  static const NFmiParam windUBaseParam(kFmiWindUMS,
                                        "Wind u component (meta)",
                                        kFloatMissing,
                                        kFloatMissing,
                                        kFloatMissing,
                                        kFloatMissing,
                                        "%.1f",
                                        kLinearly);
  static const NFmiParam windVBaseParam(kFmiWindVMS,
                                        "Wind v component (meta)",
                                        kFloatMissing,
                                        kFloatMissing,
                                        kFloatMissing,
                                        kFloatMissing,
                                        "%.1f",
                                        kLinearly);
  static const NFmiParam streamlineBaseParam(NFmiInfoData::kFmiSpStreamline, "Streamline (meta)");

  std::vector<std::unique_ptr<NFmiDataIdent>> metaParams;
  auto metaWindParamUsage = NFmiFastInfoUtils::CheckMetaWindParamUsage(theInfo);
  const auto &producer = *theInfo.Producer();
  if (metaWindParamUsage.MakeMetaWindComponents())
  {
    metaParams.push_back(std::make_unique<NFmiDataIdent>(windUBaseParam, producer));
    metaParams.push_back(std::make_unique<NFmiDataIdent>(windVBaseParam, producer));
  }
  if (metaWindParamUsage.MakeMetaWsAndWdParams())
  {
    metaParams.push_back(std::make_unique<NFmiDataIdent>(windDirectionBaseParam, producer));
    metaParams.push_back(std::make_unique<NFmiDataIdent>(windSpeedBaseParam, producer));
  }
  if (metaWindParamUsage.MakeMetaWindVectorParam())
  {
    metaParams.push_back(std::make_unique<NFmiDataIdent>(windVectorBaseParam, producer));
  }
  if (allowStreamlineParameter && metaWindParamUsage.IsStreamlinePossible())
  {
    if (theInfo.IsGrid())
    {
      metaParams.push_back(std::make_unique<NFmiDataIdent>(streamlineBaseParam, producer));
    }
  }

  return metaParams;
}

float GetMetaWindValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                       const MetaWindParamUsage &metaWindParamUsage,
                       unsigned long wantedParamId)
{
  switch (wantedParamId)
  {
    case kFmiWindDirection:
      return ::GetMetaWsWdValues(theInfo, metaWindParamUsage).second;
    case kFmiWindSpeedMS:
      return ::GetMetaWsWdValues(theInfo, metaWindParamUsage).first;
    case kFmiWindVectorMS:
      return ::GetMetaWindVectorValue(theInfo, metaWindParamUsage);
    case kFmiWindUMS:
      return ::GetMetaWindComponentsValues(theInfo, metaWindParamUsage).first;
    case kFmiWindVMS:
      return ::GetMetaWindComponentsValues(theInfo, metaWindParamUsage).second;
    default:
      return kFloatMissing;
  }
}

float GetMetaWindValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                       const NFmiMetTime &theTime,
                       const MetaWindParamUsage &metaWindParamUsage,
                       unsigned long wantedParamId)
{
  switch (wantedParamId)
  {
    case kFmiWindDirection:
      return ::GetMetaWsWdValues(theInfo, theTime, metaWindParamUsage).second;
    case kFmiWindSpeedMS:
      return ::GetMetaWsWdValues(theInfo, theTime, metaWindParamUsage).first;
    case kFmiWindVectorMS:
      return ::GetMetaWindVectorValue(theInfo, theTime, metaWindParamUsage);
    case kFmiWindUMS:
      return ::GetMetaWindComponentsValues(theInfo, theTime, metaWindParamUsage).first;
    case kFmiWindVMS:
      return ::GetMetaWindComponentsValues(theInfo, theTime, metaWindParamUsage).second;
    default:
      return kFloatMissing;
  }
}

float GetMetaWindValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                       const NFmiPoint &theLatlon,
                       const MetaWindParamUsage &metaWindParamUsage,
                       unsigned long wantedParamId)
{
  switch (wantedParamId)
  {
    case kFmiWindDirection:
      return ::GetMetaWsWdValues(theInfo, theLatlon, metaWindParamUsage).second;
    case kFmiWindSpeedMS:
      return ::GetMetaWsWdValues(theInfo, theLatlon, metaWindParamUsage).first;
    case kFmiWindVectorMS:
      return ::GetMetaWindVectorValue(theInfo, theLatlon, metaWindParamUsage);
    case kFmiWindUMS:
      return ::GetMetaWindComponentsValues(theInfo, theLatlon, metaWindParamUsage).first;
    case kFmiWindVMS:
      return ::GetMetaWindComponentsValues(theInfo, theLatlon, metaWindParamUsage).second;
    default:
      return kFloatMissing;
  }
}

float GetMetaWindValue(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                       const NFmiMetTime &theTime,
                       const NFmiPoint &theLatlon,
                       const MetaWindParamUsage &metaWindParamUsage,
                       unsigned long wantedParamId)
{
  switch (wantedParamId)
  {
    case kFmiWindDirection:
      return ::GetMetaWsWdValues(theInfo, theTime, theLatlon, metaWindParamUsage).second;
    case kFmiWindSpeedMS:
      return ::GetMetaWsWdValues(theInfo, theTime, theLatlon, metaWindParamUsage).first;
    case kFmiWindVectorMS:
      return ::GetMetaWindVectorValue(theInfo, theTime, theLatlon, metaWindParamUsage);
    case kFmiWindUMS:
      return ::GetMetaWindComponentsValues(theInfo, theTime, theLatlon, metaWindParamUsage).first;
    case kFmiWindVMS:
      return ::GetMetaWindComponentsValues(theInfo, theTime, theLatlon, metaWindParamUsage).second;
    default:
      return kFloatMissing;
  }
}

void CalcDequeWindSpeedAndDirectionFromComponents(const std::deque<float> &u,
                                                  const std::deque<float> &v,
                                                  std::deque<float> &wsOut,
                                                  std::deque<float> &wdOut)
{
  wsOut = ::VectorCalculations(u, v, CalcWS);
  wdOut = ::VectorCalculations(u, v, CalcWD);
}

void CalcDequeWindComponentsFromSpeedAndDirection(const std::deque<float> &ws,
                                                  const std::deque<float> &wd,
                                                  std::deque<float> &uOut,
                                                  std::deque<float> &vOut)
{
  uOut = ::VectorCalculations(ws, wd, CalcU);
  vOut = ::VectorCalculations(ws, wd, CalcV);
}

void CalcMatrixWindComponentsFromSpeedAndDirection(const NFmiDataMatrix<float> &ws,
                                                   const NFmiDataMatrix<float> &wd,
                                                   NFmiDataMatrix<float> &uOut,
                                                   NFmiDataMatrix<float> &vOut)
{
  uOut = ::MatrixCalculations(ws, wd, 0, CalcU);
  vOut = ::MatrixCalculations(ws, wd, 0, CalcV);
}

void CalcDequeWindVectorFromSpeedAndDirection(const std::deque<float> &ws,
                                              const std::deque<float> &wd,
                                              std::deque<float> &windVectorOut)
{
  windVectorOut = ::VectorCalculations(ws, wd, CalcWindVectorFromSpeedAndDirection);
}

void CalcMatrixWindVectorFromSpeedAndDirection(const NFmiDataMatrix<float> &ws,
                                               const NFmiDataMatrix<float> &wd,
                                               NFmiDataMatrix<float> &windVectorOut,
                                               unsigned int theStartColumnIndex)
{
  windVectorOut =
      ::MatrixCalculations(ws, wd, theStartColumnIndex, CalcWindVectorFromSpeedAndDirection);
}

void CalcMatrixWindVectorFromWindComponents(const NFmiDataMatrix<float> &u,
                                            const NFmiDataMatrix<float> &v,
                                            NFmiDataMatrix<float> &windVectorOut,
                                            unsigned int theStartColumnIndex)
{
  windVectorOut = ::MatrixCalculations(u, v, theStartColumnIndex, CalcWindVectorFromWindComponents);
}

void CalcMatrixWsFromWindComponents(const NFmiDataMatrix<float> &u,
                                    const NFmiDataMatrix<float> &v,
                                    NFmiDataMatrix<float> &wsOut,
                                    unsigned int theStartColumnIndex)
{
  wsOut = ::MatrixCalculations(u, v, theStartColumnIndex, CalcWS);
}

void CalcMatrixWdFromWindComponents(const NFmiDataMatrix<float> &u,
                                    const NFmiDataMatrix<float> &v,
                                    NFmiDataMatrix<float> &wdOut,
                                    unsigned int theStartColumnIndex)
{
  wdOut = ::MatrixCalculations(u, v, theStartColumnIndex, CalcWD);
}

void CalcMatrixUcomponentFromSpeedAndDirection(const NFmiDataMatrix<float> &ws,
                                               const NFmiDataMatrix<float> &wd,
                                               NFmiDataMatrix<float> &uOut,
                                               unsigned int theStartColumnIndex)
{
  uOut = ::MatrixCalculations(ws, wd, theStartColumnIndex, CalcU);
}

void CalcMatrixVcomponentFromSpeedAndDirection(const NFmiDataMatrix<float> &ws,
                                               const NFmiDataMatrix<float> &wd,
                                               NFmiDataMatrix<float> &vOut,
                                               unsigned int theStartColumnIndex)
{
  vOut = ::MatrixCalculations(ws, wd, theStartColumnIndex, CalcV);
}

bool SetInfoToGridPoint(boost::shared_ptr<NFmiFastQueryInfo> &info,
                        unsigned long gridPointX,
                        unsigned long gridPointY)
{
  // Pit�� tarkistaa erikseen ett� hilaindeksit eiv�t ole hilakoon ulkopuolella, koska unsigned long
  // yli/alivuotojen (tuplana) takia voikin tulla hyv�ksytt�vi� indekseja
  if (gridPointX >= info->GridXNumber() || gridPointY >= info->GridYNumber()) return false;
  auto locationIndex = info->Grid()->DataIndex(gridPointX, gridPointY);
  return info->LocationIndex(locationIndex);
}

float CalcWS(float u, float v)
{
  if (!AreWindComponentsOk(u, v)) return kFloatMissing;
  float WS = std::sqrt(u * u + v * v);
  WS = ::DoWindSpeedFromWindComponentsRoundingFix(WS);
  return WS;
}

float CalcWD(float u, float v)
{
  NFmiWindDirection windDirection(u, v);
  return static_cast<float>(windDirection.Value());
}

float CalcU(float WS, float WD)
{
  if (!AreWindComponentsOk(WS, WD)) return kFloatMissing;
  // jos tuulensuunta on vaihtelevaa (999), palautetaan 0 arvo (voisi olla my�s puuttuvaa)
  if (WD == 999) return 0;
  // huom! tuulen suunta pit�� ensin k��nt�� 180 astetta ja sitten
  // muuttaa radiaaneiksi kulma/360 * 2*pii
  float value = WS * sin(((fmod(180.f + WD, 360.f) / 360.f) * (2.f * static_cast<float>(kPii))));
  return value;
}

float CalcV(float WS, float WD)
{
  if (!AreWindComponentsOk(WS, WD)) return kFloatMissing;
  // jos tuulensuunta on vaihtelevaa (999), palautetaan 0 arvo (voisi olla my�s puuttuvaa)
  if (WD == 999) return 0;
  // Huom! tuulen suunta pit�� ensin k��nt�� 180 astetta ja sitten
  // muuttaa radiaaneiksi kulma/360 * 2*pii
  float value = WS * cos(((fmod(180.f + WD, 360.f) / 360.f) * (2.f * static_cast<float>(kPii))));
  return value;
}

float CalcWindVectorFromWindComponents(float u, float v)
{
  return CalcWindVectorFromSpeedAndDirection(CalcWS(u, v), CalcWD(u, v));
}

float CalcWindVectorFromSpeedAndDirection(float WS, float WD)
{
  if (WS != kFloatMissing && WD != kFloatMissing)
  {
    return std::round(WS) * 100 + std::round(WD / 10.f);
  }
  return kFloatMissing;
}

}  // namespace NFmiFastInfoUtils
