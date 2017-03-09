// ======================================================================
/*!
 * \file NFmiQueryDataUtil.cpp
 * \brief Implementation of class NFmiQueryDataUtil
 */
// ======================================================================
/*!
 * \class NFmiQueryDataUtil
 *
 * NFmiQueryDataUtil-luokka käsittelee/muokkaa funktioidensa avulla
 * querydta-otuksia.
 *
 * \todo Refaktoroi käyttäen namespacea ja anonyymiä namespacea
 *       .cpp tiedostossa.
 */
// ======================================================================

// NFmiQueryDataUtil-luokka käsittelee/muokkaa funktioidensa avulla
// querydata-otuksia.

#include "NFmiQueryDataUtil.h"
#include "NFmiCalculationCondition.h"
#include "NFmiCalculator.h"
#include "NFmiCombinedParam.h"
#include "NFmiDataModifierClasses.h"
#include "NFmiDataModifierMode.h"
#include "NFmiFastQueryInfo.h"
#include "NFmiGrid.h"
#include "NFmiInterpolation.h"
#include "NFmiLagrange.h"
#include "NFmiProducerIdLister.h"
#include "NFmiQueryData.h"
#include "NFmiRelativeDataIterator.h"
#include "NFmiRelativeTimeIntegrationIterator.h"
#include "NFmiSmoother.h"
#include "NFmiStreamQueryData.h"
#include "NFmiSuperSmartInfo.h"
#include "NFmiTimeList.h"
#include "NFmiTotalWind.h"
#include "NFmiValueString.h"
#include "NFmiWeatherAndCloudiness.h"
#include "NFmiFileSystem.h"
#include "NFmiFileString.h"
#include "NFmiAzimuthalArea.h"
#include "NFmiLogger.h"

#include <cassert>
#include <fstream>
#include <numeric>
#include <stdexcept>
#include <algorithm>

using namespace std;

GridRecordData::GridRecordData(void)
    : itsOrigTime(),
      itsValidTime(),
      itsParam(),
      itsLevel(),
      itsOrigGrid(),
      itsGrid(),
      fDoProjectionConversion(false),
      itsGridData(),
      itsLatlonCropRect(gMissingCropRect)  // tämä pitää alustaa missing arvoksi aluksi
      ,
      itsGridPointCropOffset(0, 0),
      fParamChanged(false),
      itsOrigParam(0, "no param")
{
}

void GridRecordData::ChangeParam(const NFmiParam &theWantedParam)
{
  fParamChanged = true;
  itsOrigParam = *(itsParam.GetParam());
  itsParam.SetParam(theWantedParam);
}

bool MyGrid::operator<(const MyGrid &theGrid) const
{
  if (itsNX != theGrid.itsNX) return itsNX < theGrid.itsNX;
  if (itsNY != theGrid.itsNY) return itsNY < theGrid.itsNY;
  if (itsArea == 0 && theGrid.itsArea == 0) return false;

  if (itsArea && theGrid.itsArea)
  {
    if (itsArea->ClassId() != theGrid.itsArea->ClassId())
      return itsArea->ClassId() < theGrid.itsArea->ClassId();
    if (itsArea->BottomLeftLatLon() != theGrid.itsArea->BottomLeftLatLon())
      return itsArea->BottomLeftLatLon() < theGrid.itsArea->BottomLeftLatLon();
    if (itsArea->BottomRightLatLon() != theGrid.itsArea->BottomRightLatLon())
      return itsArea->BottomRightLatLon() < theGrid.itsArea->BottomRightLatLon();
    if (itsArea->TopLeftLatLon() != theGrid.itsArea->TopLeftLatLon())
      return itsArea->TopLeftLatLon() < theGrid.itsArea->TopLeftLatLon();
    if (itsArea->TopRightLatLon() != theGrid.itsArea->TopRightLatLon())
      return itsArea->TopRightLatLon() < theGrid.itsArea->TopRightLatLon();

    if (itsArea->ClassId() == kNFmiStereographicArea || itsArea->ClassId() == kNFmiEquiDistArea ||
        itsArea->ClassId() == kNFmiGnomonicArea)
    {
      if (static_cast<const NFmiAzimuthalArea *>(itsArea)->Orientation() !=
          static_cast<const NFmiAzimuthalArea *>(theGrid.itsArea)->Orientation())
        return static_cast<const NFmiAzimuthalArea *>(itsArea)->Orientation() <
               static_cast<const NFmiAzimuthalArea *>(theGrid.itsArea)->Orientation();
    }
  }
  return false;
}

// NFmiThreadCallBacks inplemantations
bool NFmiThreadCallBacks::Stop(void) const
{
  if (itsStopper)
    return itsStopper->Stop();
  else
    return false;
}

void NFmiThreadCallBacks::Stop(bool newValue)
{
  if (itsStopper) return itsStopper->Stop(newValue);
}

void NFmiThreadCallBacks::StepIt(void)
{
  if (itsProgress) itsProgress->StepIt();
}

void NFmiThreadCallBacks::SetRange(int low, int high, int stepCount)
{
  if (itsProgress) itsProgress->SetRange(low, high, stepCount);
}

void NFmiThreadCallBacks::AddRange(int value)
{
  if (itsProgress) itsProgress->AddRange(value);
}

bool NFmiThreadCallBacks::DoPostMessage(unsigned int message, unsigned int wParam, long lParam)
{
  if (itsProgress)
    return itsProgress->DoPostMessage(message, wParam, lParam);
  else
    return false;
}

void NFmiThreadCallBacks::CheckIfStopped(void)
{
  if (itsStopper && itsStopper->Stop()) throw NFmiStopThreadException();
}

bool NFmiThreadCallBacks::WaitUntilInitialized(void)
{
  if (itsProgress)
    return itsProgress->WaitUntilInitialized();
  else
    return true;
}

NFmiTimeIndexCalculator::NFmiTimeIndexCalculator(unsigned long theTimeSize)
    : itsStartTimeIndex(0),
      itsEndTimeIndex(theTimeSize - 1),
      itsCurrentTimeIndex(0),
      fNoMoreWork((theTimeSize > 0) ? false : true)
{
}

NFmiTimeIndexCalculator::NFmiTimeIndexCalculator(unsigned long theStartTimeIndex,
                                                 unsigned long theEndTimeIndex)
    : itsStartTimeIndex(theStartTimeIndex),
      itsEndTimeIndex(theEndTimeIndex),
      itsCurrentTimeIndex(theStartTimeIndex),
      fNoMoreWork((theStartTimeIndex <= theEndTimeIndex) ? false : true)
{
}

bool NFmiTimeIndexCalculator::GetCurrentTimeIndex(unsigned long &theTimeIndexOut)
{
  WriteLock lock(itsMutex);
  if (fNoMoreWork) return false;
  theTimeIndexOut = itsCurrentTimeIndex;
  itsCurrentTimeIndex++;
  if (itsCurrentTimeIndex > itsEndTimeIndex) fNoMoreWork = true;
  return true;
}

NFmiLocationIndexRangeCalculator::NFmiLocationIndexRangeCalculator(unsigned long theLocationSize,
                                                                   unsigned long theChunkSize)
    : itsStartLocationIndex(0),
      itsEndLocationIndex(theLocationSize - 1),
      itsCurrentLocationIndex(0),
      itsChunkSize(theChunkSize),
      fNoMoreWork((theLocationSize > 0) ? false : true)
{
}

NFmiLocationIndexRangeCalculator::NFmiLocationIndexRangeCalculator(
    unsigned long theStartLocationIndex,
    unsigned long theEndLocationIndex,
    unsigned long theChunkSize)
    : itsStartLocationIndex(theStartLocationIndex),
      itsEndLocationIndex(theEndLocationIndex),
      itsCurrentLocationIndex(theStartLocationIndex),
      itsChunkSize(theChunkSize),
      fNoMoreWork((theStartLocationIndex <= theEndLocationIndex) ? false : true)
{
}

bool NFmiLocationIndexRangeCalculator::GetCurrentLocationRange(unsigned long &theStartIndexOut,
                                                               unsigned long &theEndIndexOut)
{
  WriteLock lock(itsMutex);
  if (fNoMoreWork) return false;
  theStartIndexOut = itsCurrentLocationIndex;
  itsCurrentLocationIndex += itsChunkSize;
  if (itsCurrentLocationIndex >= itsEndLocationIndex)
  {
    itsCurrentLocationIndex = itsEndLocationIndex;
    fNoMoreWork = true;
  }
  theEndIndexOut = itsCurrentLocationIndex;
  itsCurrentLocationIndex++;  // viedään indeksi yksi eteenpäin seuraavan threadin alkua varten
  return true;
}

bool GetProducerIdsLister(NFmiQueryInfo *theInfo, NFmiProducerIdLister &theProducerIdsLister)
{
  if (theInfo)
    if (theInfo->FindFirstKey(NFmiQueryDataUtil::GetOfficialQueryDataProdIdsKey()))
      return theProducerIdsLister.IntepretProducerIdString(string(theInfo->GetCurrentKeyValue()));
  return false;
}

void CopyProducerIds(NFmiFastQueryInfo &theDestInfo, NFmiFastQueryInfo &theSourceInfo)
{
  if (theSourceInfo.FindFirstKey(NFmiQueryDataUtil::GetOfficialQueryDataProdIdsKey()))
    theDestInfo.AddKey(NFmiQueryDataUtil::GetOfficialQueryDataProdIdsKey(),
                       theSourceInfo.GetCurrentKeyValue(),
                       true);
}

void CopyProducerIds(NFmiQueryData *theDestData, NFmiQueryData *theSourceData)
{
  if (theDestData && theSourceData)
  {
    NFmiFastQueryInfo destInfo(theDestData);
    NFmiFastQueryInfo sourceInfo(theSourceData);
    CopyProducerIds(destInfo, sourceInfo);
  }
}

void AddProducerIds(NFmiFastQueryInfo &theDestInfo, NFmiFastQueryInfo &theSourceInfo)
{
  NFmiProducerIdLister sourceIds;
  if (GetProducerIdsLister(&theSourceInfo, sourceIds))
  {
    NFmiProducerIdLister destIds(theDestInfo.TimeDescriptor(), -1);
    for (theDestInfo.ResetTime(); theDestInfo.NextTime();)
    {
      destIds.ProducerId(theDestInfo.TimeIndex(), sourceIds.ProducerId(theDestInfo.Time(), true));
      destIds.ModelOriginTime(theDestInfo.TimeIndex(),
                              sourceIds.ModelOriginTime(theDestInfo.Time(), true));
    }
    theDestInfo.SetCurrentKeyValue(destIds.MakeProducerIdString());
  }
}

//! esim. tuulen suunta on ympyrä arvoinen eli arvo menee kehässä ylärajalta alarajalle.
static bool IsParamCircularValued(const NFmiParam *theParam)
{
  // circular toimii siis vain nyt tuulen suunnalle, lisää muita jos tarpeen
  if (theParam->GetIdent() == kFmiWindDirection) return true;
  return false;
}

NFmiQueryDataUtil::LimitChecker::LimitChecker(float theLowerLimit,
                                              float theUpperLimit,
                                              bool theCircularValue)
    : itsLowerLimit(theLowerLimit),
      itsUpperLimit(theUpperLimit),
      fCircularValue(theCircularValue),
      itsLimitDifference()
{
  Update();
}

float NFmiQueryDataUtil::LimitChecker::GetInsideLimitsValue(float theValue) const
{
  if (theValue == kFloatMissing) return theValue;
  if (!fCircularValue)
  {
    if (itsLowerLimit != kFloatMissing && theValue < itsLowerLimit)
      return itsLowerLimit;
    else if (itsUpperLimit != kFloatMissing && theValue > itsUpperLimit)
      return itsUpperLimit;
  }
  else
  {
    if (theValue < itsLowerLimit)
    {  // ei ota huomioon jos arvo menee yli ylä ja alarajan erotuksen. Tuli ongelmia toteuttaa se
      // ja oletan, että tapaus on harvinainen (lagrange tuskin pystyy tuottamaan niin
      // isoja/pieniä
      // lukuja)
      float tmp = itsUpperLimit - (itsLowerLimit - theValue);
      return tmp;
    }
    else if (theValue > itsUpperLimit)
    {
      float tmp = itsLowerLimit + (theValue - itsUpperLimit);
      return tmp;
    }
  }
  return theValue;
}

void NFmiQueryDataUtil::LimitChecker::Update(void)
{
  if (fCircularValue)
  {
    if (itsLowerLimit != kFloatMissing && itsUpperLimit != kFloatMissing)
    {
      itsLimitDifference = itsUpperLimit - itsLowerLimit;
      if (itsLimitDifference) return;
      throw runtime_error("Kiertävä arvoisen parametrin max ja min rajat olivat samat.");
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * Luo uusi info jossa parametrina annettu hila (ja alue)
 *
 * \param theSourceInfo Undocumented
 * \param theWantedGridFormat Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryInfo *CreateInfo(const NFmiQueryInfo *theSourceInfo, NFmiGrid *theWantedGridFormat)
{
  NFmiQueryInfo *info = 0;
  if (theWantedGridFormat)
  {
    NFmiHPlaceDescriptor hPlaceDesc(*theWantedGridFormat);
    info = new NFmiQueryInfo(theSourceInfo->ParamDescriptor(),
                             theSourceInfo->TimeDescriptor(),
                             hPlaceDesc,
                             theSourceInfo->VPlaceDescriptor());
    info->InfoVersion(
        theSourceInfo
            ->InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
  }
  return info;
}

// ----------------------------------------------------------------------
/*!
 * HUOM!!! datan rakenteen pitää olla muuten sama paitsi arean ja hilan osalta!!!!!!
 *
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool GridInfo2NewGridInfo(NFmiFastQueryInfo &theSourceInfo, NFmiFastQueryInfo &theDestInfo)
{
  if (theSourceInfo.IsGrid() && theDestInfo.IsGrid())
  {
    theSourceInfo.First();
    theDestInfo.First();
    for (theDestInfo.ResetParam(), theSourceInfo.ResetParam();
         theDestInfo.NextParam() && theSourceInfo.NextParam();)
      for (theDestInfo.ResetLocation(); theDestInfo.NextLocation();)
      {
        NFmiPoint latlon = theDestInfo.LatLon();
        for (theDestInfo.ResetLevel(), theSourceInfo.ResetLevel();
             theDestInfo.NextLevel() && theSourceInfo.NextLevel();)
          for (theDestInfo.ResetTime(), theSourceInfo.ResetTime();
               theDestInfo.NextTime() && theSourceInfo.NextTime();)
          {
            float value = theSourceInfo.InterpolatedValue(latlon);
            theDestInfo.FloatValue(value);
          }
      }
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * Konvertoi hilamuotoisen QDatan toiseen haluttuun hilaan
 * HUOM!!! datan rakenteen pitää olla muuten sama paitsi arean ja hilan osalta!!!!!!
 *
 * \param theSourceData Undocumented
 * \param theWantedGridFormat Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::GridQD2NewGridQD(NFmiQueryData *theSourceData,
                                                   NFmiGrid *theWantedGridFormat)
{
  NFmiQueryData *newData = 0;
  if (theSourceData && theWantedGridFormat)
  {
    NFmiQueryInfo *newInnerInfo = CreateInfo(theSourceData->Info(), theWantedGridFormat);
    if (newInnerInfo)
    {
      newData = CreateEmptyData(*newInnerInfo);
      if (newData)
      {
        NFmiFastQueryInfo sourceInfo(theSourceData);
        NFmiFastQueryInfo destInfo(newData);
        bool status = GridInfo2NewGridInfo(sourceInfo, destInfo);
        // interpolointi ei onnistunut??!!??
        if (!status) throw runtime_error("NFmiQueryDataUtil::GridQD2NewGridQD failed");
      }
    }
  }
  return newData;
}

// optimoidaan ja tutkitaan mitä paikkoja todella tarvitaan
// eli mitkä info2:n paikat ovat interpoloitavissa info1:sta.
static void GetMatchingLocations(NFmiFastQueryInfo &theInfo1,
                                 NFmiFastQueryInfo &theInfo2,
                                 checkedVector<unsigned long> &theLocationIndexies,
                                 checkedVector<NFmiPoint> &theLatlons)
{
  if (theInfo1.IsGrid() && theInfo2.IsGrid() && theInfo1.Area())
  {
    theInfo1.First();
    theInfo2.First();
    NFmiPoint latlon;
    const NFmiArea *area = theInfo1.Area();
    for (theInfo2.FirstLocation(); theInfo2.NextLocation();)
    {
      latlon = theInfo2.LatLon();
      if (area->IsInside(latlon))
      {  // jos
        theLocationIndexies.push_back(theInfo2.LocationIndex());
        theLatlons.push_back(latlon);
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * Kirjoittaa area/grid 1:stä area/grid 2:een päälle, jos löytyy dataa
 * Parametrien ei tarvitse olla samoja mutta leveleiden pitää olla samoja.
 * Aikojen ei vielä tarvitse olla täysin samoja, alku ja loppu ajat voivat
 * vaihdella. Aika-askeleen pituus pitää olla sama ja aikojen pitää olla
 * yhteisissä ajoissaan synkronissa (eli jos 6h resoluutio, tuntien pitää
 * menne molemmisssa 0,6,12,...  eikä esim. niin, että toisessa 0, 6, 12..
 * ja toisessa 2, 8, 14, ...).
 *
 * HUOM! Ei kirjoita data2:en päälle vaan luo uuden qdatan joka palautetaan.
 *
 * \param areaData1 Undocumented
 * \param areaData2 Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::Area1QDOverArea2QD(NFmiQueryData *areaData1,
                                                     NFmiQueryData *areaData2)
{
  NFmiQueryData *newData = 0;
  if (areaData1 && areaData2)
  {
    newData = new NFmiQueryData(*areaData2);
    NFmiFastQueryInfo area1Info(areaData1);
    NFmiFastQueryInfo area2Info(newData);

    // optimoidaan ja tutkitaan mitä paikkoja todella tarvitaan eli mitkä
    // data2:n paikat ovat interpoloitavissa data1:sta

    // tähän kerätään ne data1:n paikka-indeksit, jotkä todella tarvitaan
    checkedVector<unsigned long> locationIndexies;

    // ja tähän niiden paikkojen lotlon-pisteet
    checkedVector<NFmiPoint> latlons;

    GetMatchingLocations(area1Info, area2Info, locationIndexies, latlons);
    for (area2Info.ResetParam(); area2Info.NextParam();)
    {
      if (area1Info.Param(FmiParameterName(area2Info.Param().GetParamIdent())))
      {
        for (area2Info.ResetTime();
             area2Info.NextTime();)  // aikojen ei täydy olla vielä samoja eri qdatoissa
        {
          if (area1Info.Time(area2Info.Time()))
          {
            for (area1Info.ResetLevel(), area2Info.ResetLevel();
                 area1Info.NextLevel() && area2Info.NextLevel();)
            {
              size_t ssize = locationIndexies.size();
              for (size_t i = 0; i < ssize; i++)
              {
                float value = area1Info.InterpolatedValue(latlons[i]);
                if (!(value == kFloatMissing || value == kTCombinedWeatherFloatMissing))
                {
                  area2Info.LocationIndex(locationIndexies[i]);
                  area2Info.FloatValue(value);
                }
              }
            }
          }
        }
      }
    }
  }
  return newData;
}

// ----------------------------------------------------------------------
/*!
 * \param srcInfo Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::CreateEmptyData(NFmiQueryInfo &srcInfo)
{
  if (srcInfo.Size() == 0)
    throw std::runtime_error(
        "Error in NFmiQueryDataUtil::CreateEmptyData: given srcInfo size was 0, one or several "
        "descriptor(s) was empty?");
  NFmiQueryData *data = new NFmiQueryData(srcInfo);
  data->Init();
  return data;
}

// ----------------------------------------------------------------------
/*!
 * \brief Create a memory mapped querydata for writing
 *
 * \param srcInfo Header data
 * \param theFilename Filename for the mmapped data
 * \param fInitialize True, if data is to be initialized to missing values
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::CreateEmptyData(NFmiQueryInfo &srcInfo,
                                                  const std::string &theFilename,
                                                  bool fInitialize)
{
  if (srcInfo.Size() == 0)
    throw std::runtime_error("Attempt to create empty querydata as a memory mapped file");
  NFmiQueryData *data = new NFmiQueryData(srcInfo);

  std::ostringstream header;
  header << srcInfo;

  // Initialize with given info as a string to given filename
  data->Init(header.str(), theFilename, fInitialize);
  return data;
}

// ----------------------------------------------------------------------
/*!
 * Luo uuden QDatan, jossa on vain halutut parametrit. Voidaan käyttää
 * myös irroittamaan yhdistelmä parametrien aliparametreja.
 *
 * \param theSourceData Undocumented
 * \param theWantedParams Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::ExtractParams(NFmiQueryData *theSourceData,
                                                const NFmiParamBag &theWantedParams)
{
  NFmiQueryInfo *dataInfo = theSourceData->Info();
  NFmiParamDescriptor parDesc(theWantedParams);
  NFmiQueryInfo tmpInfo(parDesc,
                        dataInfo->TimeDescriptor(),
                        dataInfo->HPlaceDescriptor(),
                        dataInfo->VPlaceDescriptor());
  tmpInfo.InfoVersion(
      dataInfo->InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
  NFmiQueryData *destData = CreateEmptyData(tmpInfo);
  if (destData)
  {
    NFmiFastQueryInfo sourceInfo(theSourceData);
    NFmiFastQueryInfo destInfo(destData);
    for (destInfo.ResetParam(); destInfo.NextParam();)
    {
      if (!sourceInfo.Param(*destInfo.Param().GetParam()))  // pyytää infolta Param().GetParam(),
                                                            // jolloin palautetaan NFmiParam,
                                                            // jolloin etsittäessä parametria
                                                            // tarkastellaan vain param-id:tä
        continue;  // tähän ei oikeasti saisi mennä!!!!!!! assertin paikka?
      for (destInfo.ResetLocation(), sourceInfo.ResetLocation();
           destInfo.NextLocation() && sourceInfo.NextLocation();)
      {
        for (destInfo.ResetLevel(), sourceInfo.ResetLevel();
             destInfo.NextLevel() && sourceInfo.NextLevel();)
        {
          for (destInfo.ResetTime(), sourceInfo.ResetTime();
               destInfo.NextTime() && sourceInfo.NextTime();)
          {
            float value = sourceInfo.FloatValue();
            destInfo.FloatValue(value);
          }
        }
      }
    }
    CopyProducerIds(destInfo, sourceInfo);
  }
  return destData;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 */
// ----------------------------------------------------------------------

void ExtractLocationsFromLocationData(NFmiFastQueryInfo &theSourceInfo,
                                      NFmiFastQueryInfo &theDestInfo)
{
  for (theDestInfo.ResetLocation(); theDestInfo.NextLocation();)
  {
    if (!theSourceInfo.Location(*theDestInfo.Location())) continue;
    for (theDestInfo.ResetParam(), theSourceInfo.ResetParam();
         theDestInfo.NextParam() && theSourceInfo.NextParam();)
    {
      for (theDestInfo.ResetLevel(), theSourceInfo.ResetLevel();
           theDestInfo.NextLevel() && theSourceInfo.NextLevel();)
      {
        for (theDestInfo.ResetTime(), theSourceInfo.ResetTime();
             theDestInfo.NextTime() && theSourceInfo.NextTime();)
        {
          float value = theSourceInfo.FloatValue();
          theDestInfo.FloatValue(value);
        }
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 *
 */
// ----------------------------------------------------------------------
template <typename T>
struct IsMissingValue
{
  IsMissingValue(T theMissValue) : itsMissValue(theMissValue){};
  bool operator()(const T &theValue) { return itsMissValue == theValue; }
  T itsMissValue;
};

// ----------------------------------------------------------------------
/*!
 * \param theInfo Undocumented
 * \param theLatlon Undocumented
 * \param theInterpolationMethod Undocumented
 * \param theParamInterpMethod Undocumented
 * \param theLimitChecker Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float LocationInterpolationValue(NFmiFastQueryInfo &theInfo,
                                 const NFmiPoint &theLatlon,
                                 FmiInterpolationMethod theInterpolationMethod,
                                 FmiInterpolationMethod theParamInterpMethod,
                                 NFmiQueryDataUtil::LimitChecker &theLimitChecker)
{
  if (theInterpolationMethod != kLagrange ||
      !(theParamInterpMethod == kLinearly || theParamInterpMethod == kLagrange))
    return theInfo.InterpolatedValue(theLatlon);
  else
  {
    NFmiLagrange lagrange;
    static const int maxSize = 4;
    checkedVector<double> valueVec(maxSize * maxSize);
    static const double tVec[maxSize] = {-1, 0, 1, 2};  // arvojen t sijainnit
    static const double sVec[maxSize] = {-1, 0, 1, 2};  // arvojen s sijainnit
    float value = kFloatMissing;
    const NFmiGrid &grid = *theInfo.Grid();
    unsigned long gridXNumber = grid.XNumber();
    unsigned long gridYNumber = grid.YNumber();
    NFmiPoint gridPoint(theInfo.Grid()->LatLonToGrid(theLatlon));
    unsigned long zeroPointIndex =
        static_cast<int>(gridPoint.Y()) * gridXNumber +
        static_cast<int>(gridPoint.X());  // nollapisteindeksi tarkoittaa sen hilapisteen indeksiä,
                                          // joka saadaan kun griPoint leikataan alas lähimpään
                                          // kokonais hilapisteeseen
    if (!theInfo.LocationIndex(zeroPointIndex)) return value;
    if ((gridPoint.X() > 1) && (gridPoint.Y() > 1) && (gridPoint.X() < gridXNumber - 2) &&
        (gridPoint.Y() < gridYNumber - 2))
    {
      for (int j = -1; j < 3; j++)
      {
        for (int i = -1; i < 3; i++)
        {
          int ind = (j + 1) * maxSize + (i + 1);
          valueVec[ind] = theInfo.PeekLocationValue(i, j);
        }
      }
      if (std::find_if(valueVec.begin(), valueVec.end(), ::IsMissingValue<double>(kFloatMissing)) !=
          valueVec.end())
        return kFloatMissing;  // jos yksikin lähtöarvoista oli puuttuva, palauta puuttuva
      else
      {
        lagrange.Init(sVec, tVec, &valueVec[0], maxSize, maxSize);
        value = static_cast<float>(
            lagrange.Interpolate(fmod(gridPoint.X(), 1), fmod(gridPoint.Y(), 1)));
        return theLimitChecker.GetInsideLimitsValue(value);
      }
    }
    else  // hilan reunoilla tehdään lineaarinen interpolointi, en jaksa tehdä erikoistapauksia
          // reunoille ja kulmiin.
      return theInfo.InterpolatedValue(theLatlon);
    //	  return value;
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 * \param theInterpolationMethod Undocumented
 */
// ----------------------------------------------------------------------

void ExtractLocationsFromGridData(NFmiFastQueryInfo &theSourceInfo,
                                  NFmiFastQueryInfo &theDestInfo,
                                  FmiInterpolationMethod theInterpolationMethod)
{
  for (theDestInfo.ResetParam(), theSourceInfo.ResetParam();
       theDestInfo.NextParam() && theSourceInfo.NextParam();)
  {
    const NFmiParam *tmpParam = theDestInfo.Param().GetParam();
    NFmiQueryDataUtil::LimitChecker checkerWithLagrangeInterpolation(
        static_cast<float>(tmpParam->MinValue()),
        static_cast<float>(tmpParam->MaxValue()),
        IsParamCircularValued(tmpParam));

    FmiInterpolationMethod paramInterpMethod =
        theDestInfo.Param().GetParam()->InterpolationMethod();
    for (theDestInfo.ResetLocation(); theDestInfo.NextLocation();)
    {
      if (!theSourceInfo.Location(*theDestInfo.Location())) continue;
      for (theDestInfo.ResetLevel(), theSourceInfo.ResetLevel();
           theDestInfo.NextLevel() && theSourceInfo.NextLevel();)
      {
        for (theDestInfo.ResetTime(), theSourceInfo.ResetTime();
             theDestInfo.NextTime() && theSourceInfo.NextTime();)
        {
          float value = LocationInterpolationValue(theSourceInfo,
                                                   theDestInfo.LatLon(),
                                                   theInterpolationMethod,
                                                   paramInterpMethod,
                                                   checkerWithLagrangeInterpolation);
          theDestInfo.FloatValue(value);
        }
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * Rakentaa muuten saman datan kuin sourceData, paitsi erottaa sourcesta
 * halutut locationit. Jos sourceData on jo location dataa, irrotetaan
 * sieltä vain halutut asemat, mutta jos source on hilamuotoista dataa,
 * interpoloidaan arvot haluttuihin pisteisiin.
 *
 * \param theSourceData Undocumented
 * \param theWantedLocations Undocumented
 * \param theInterpolationMethod Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::ExtractLocations(NFmiQueryData *theSourceData,
                                                   const NFmiLocationBag &theWantedLocations,
                                                   FmiInterpolationMethod theInterpolationMethod)
{
  NFmiQueryInfo *dataInfo = theSourceData->Info();
  NFmiHPlaceDescriptor locDesc(theWantedLocations);
  NFmiQueryInfo tmpInfo(dataInfo->ParamDescriptor(),
                        dataInfo->TimeDescriptor(),
                        locDesc,
                        dataInfo->VPlaceDescriptor());
  tmpInfo.InfoVersion(
      dataInfo->InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
  NFmiQueryData *destData = CreateEmptyData(tmpInfo);
  if (destData)
  {
    NFmiFastQueryInfo sourceInfo(theSourceData);
    NFmiFastQueryInfo destInfo(destData);
    if (sourceInfo.IsGrid())
      ExtractLocationsFromGridData(sourceInfo, destInfo, theInterpolationMethod);
    else
      ExtractLocationsFromLocationData(sourceInfo, destInfo);

    CopyProducerIds(destInfo, sourceInfo);
  }
  return destData;
}

// ----------------------------------------------------------------------
/*!
 * Luo annetuista datoista uuden datan siten että parametrit ovat yhdistetty.
 * Jos samoja parametreja, valitaan toinen data lähteeksi, mutta loppu tulokseen
 * tulee tietty parametri vain kerran.
 * Ajat otetaan data1:stä. Alkuja loppuaikojen ei tarvitse olla siis samoja.
 * Oletuksia: levelit, hila, area ja aika-askel ovat samoja ja ajat synkronissa
 * (eli käy läpi samoja tunteja 0, 3, 6, ... eikä toinen menee 1, 4, 7,... ja
 * toinen 2, 5, 8,...)
 *
 * \param theSourceData1 Undocumented
 * \param theSourceData2 Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::CombineParams(NFmiQueryData *theSourceData1,
                                                NFmiQueryData *theSourceData2)
{
  NFmiQueryInfo *dataInfo1 = theSourceData1->Info();
  NFmiParamDescriptor parDesc1(dataInfo1->ParamDescriptor());
  NFmiParamDescriptor parDesc2(theSourceData2->Info()->ParamDescriptor());
  NFmiParamDescriptor newParDesc(parDesc1.Combine(parDesc2));
  NFmiQueryInfo tmpInfo(newParDesc,
                        dataInfo1->TimeDescriptor(),
                        dataInfo1->HPlaceDescriptor(),
                        dataInfo1->VPlaceDescriptor());
  tmpInfo.InfoVersion(
      dataInfo1
          ->InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
  NFmiQueryData *destData = CreateEmptyData(tmpInfo);
  if (destData)
  {
    NFmiFastQueryInfo sourceInfo1(theSourceData1);
    NFmiFastQueryInfo sourceInfo2(theSourceData2);
    NFmiFastQueryInfo destInfo(destData);
    bool useInfo1 = false;
    bool useInfo2 = false;
    for (destInfo.ResetParam(); destInfo.NextParam();)
    {
      if (sourceInfo1.Param(*destInfo.Param().GetParam()))  // pyytää infolta Param().GetParam(),
                                                            // jolloin palautetaan NFmiParam,
                                                            // jolloin etsittäessä parametria
                                                            // tarkastellaan vain param-id:tä
        useInfo1 = true;
      if (sourceInfo2.Param(*destInfo.Param().GetParam()))  // pyytää infolta Param().GetParam(),
                                                            // jolloin palautetaan NFmiParam,
                                                            // jolloin etsittäessä parametria
                                                            // tarkastellaan vain param-id:tä
        useInfo2 = true;
      if (!(useInfo1 || useInfo2))  // jos kummastakaan ei löytynyt parametria ei tehdä mitään
        continue;                   // tähän ei oikeasti saisi mennä!!!!!!! assertin paikka?

      for (destInfo.ResetTime(); destInfo.NextTime();)
      {
        bool timeFound = false;
        if (useInfo1 && sourceInfo1.Time(destInfo.Time())) timeFound = true;
        if (useInfo2 && sourceInfo2.Time(destInfo.Time())) timeFound = true;
        if (!timeFound ||
            !(useInfo1 || useInfo2))  // jos kummastakaan ei löytynyt parametria ei tehdä mitään
          continue;                   // tähän ei oikeasti saisi mennä!!!!!!! assertin paikka?

        for (destInfo.ResetLocation(), sourceInfo1.ResetLocation(), sourceInfo2.ResetLocation();
             destInfo.NextLocation() && sourceInfo1.NextLocation() && sourceInfo2.NextLocation();)
        {
          for (destInfo.ResetLevel(), sourceInfo1.ResetLevel(), sourceInfo2.ResetLevel();
               destInfo.NextLevel() && sourceInfo1.NextLevel() && sourceInfo2.NextLevel();)
          {
            float value = kFloatMissing;
            float value1 = sourceInfo1.FloatValue();
            float value2 = sourceInfo2.FloatValue();
            if (useInfo2 &&
                value2 != kFloatMissing)  // kakkosdata on priorisoitu korkeammalle (persa halusi)
              value = value2;
            else if (useInfo1 && value1 != kFloatMissing)
              value = value1;

            destInfo.FloatValue(value);
          }
        }
      }
    }
  }
  return destData;
}

// ----------------------------------------------------------------------
/*!
 * Saa qdatan1:n ja qdata2:n ja luo niistä uuden datan siten että locationbagit
 * on yhdistetty. Jos samoja locationeja, valitaan qdata2:n lähteeksi, ja
 * lopputulokseen tulee tietty locationi vain kerran. Ajat otetaan qdata1:stä.
 * Alku- ja loppuaikojen ei tarvitse olla siis samoja. Oletuksia: levelit,
 * parametrit ja aika-askel ovat samoja ja ajat synkronissa (eli käy läpi samoja
 * tunteja 0, 3, 6, ... eikä toinen menee 1, 4, 7,... ja toinen 2, 5, 8,...).
 *
 * \param theSourceData1 Undocumented
 * \param theSourceData2 Undocumented
 * \param thePriorisedDataNumber Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::CombineLocations(NFmiQueryData *theSourceData1,
                                                   NFmiQueryData *theSourceData2,
                                                   int thePriorisedDataNumber)
{
  NFmiQueryInfo *dataInfo1 = theSourceData1->Info();
  NFmiHPlaceDescriptor hplaceDesc1(dataInfo1->HPlaceDescriptor());
  NFmiHPlaceDescriptor hplaceDesc2(theSourceData2->Info()->HPlaceDescriptor());
  NFmiHPlaceDescriptor newHPlaceDesc(hplaceDesc1.Combine(hplaceDesc2));
  NFmiQueryInfo tmpInfo(dataInfo1->ParamDescriptor(),
                        dataInfo1->TimeDescriptor(),
                        newHPlaceDesc,
                        dataInfo1->VPlaceDescriptor());
  tmpInfo.InfoVersion(
      dataInfo1
          ->InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
  NFmiQueryData *destData = CreateEmptyData(tmpInfo);
  if (destData)
  {
    NFmiFastQueryInfo sourceInfo1(theSourceData1);
    NFmiFastQueryInfo sourceInfo2(theSourceData2);
    NFmiFastQueryInfo destInfo(destData);
    for (destInfo.ResetLocation(); destInfo.NextLocation();)
    {
      bool useInfo1 = false;
      bool useInfo2 = false;
      // TODO Muuta latlon-hauksi?!?!
      if (sourceInfo1.Location(*destInfo.Location())) useInfo1 = true;
      if (sourceInfo2.Location(*destInfo.Location())) useInfo2 = true;
      if (!(useInfo1 || useInfo2))  // jos kummastakaan ei löytynyt parametria ei tehdä mitään
        continue;                   // tähän ei oikeasti saisi mennä!!!!!!! assertin paikka?

      //				for(destInfo.ResetParam(), sourceInfo1.ResetParam(),
      // sourceInfo2.ResetParam(); destInfo.NextParam() && sourceInfo1.NextParam() &&
      // sourceInfo2.NextParam();)
      for (destInfo.ResetParam(); destInfo.NextParam();)
      {
        // asetetaan sourceinfot osoittamaan oikeaan parametriin, jos sellainen löytyy
        sourceInfo1.Param(static_cast<FmiParameterName>(destInfo.Param().GetParamIdent()));
        sourceInfo2.Param(static_cast<FmiParameterName>(destInfo.Param().GetParamIdent()));

        for (destInfo.ResetTime(), sourceInfo1.ResetTime();
             destInfo.NextTime() && sourceInfo1.NextTime();)
        {  // source1:stä voidaan juoksuttaa samalla aika loopilla koska timedesc on otettu
           // sieltä,
           // mutta source2:lta pitää aina kysyä!
          bool useInfo2Time = true;
          if (!sourceInfo2.Time(destInfo.Time())) useInfo2Time = false;
          if (!(useInfo1 ||
                (useInfo2 &&
                 useInfo2Time)))  // jos kummastakaan ei löytynyt parametria ei tehdä mitään
            continue;             // tähän ei oikeasti saisi mennä!!!!!!! assertin paikka?

          for (destInfo.ResetLevel(), sourceInfo1.ResetLevel(), sourceInfo2.ResetLevel();
               destInfo.NextLevel() && sourceInfo1.NextLevel() && sourceInfo2.NextLevel();)
          {
            float value1 = sourceInfo1.FloatValue();
            float value2 = sourceInfo2.FloatValue();
            float value = (thePriorisedDataNumber == 2) ? value2 : value1;
            if (value ==
                kFloatMissing)  // jos priorisoitu data oli puuttuvaa, annetaan sitten se toinen
              value = (thePriorisedDataNumber == 2) ? value1 : value2;

            destInfo.FloatValue(value);
          }
        }
      }
    }
  }
  return destData;
}

// ----------------------------------------------------------------------
/*!
 *  Irroittaa halutut ajat querydatasta
 *  HUOM!! irroittaa vain saman aikaresoluution dataa.
 *  timebagin pitää olla samaa resoluutiota datan kanssa
 *
 * \param theSourceData Undocumented
 * \param theWantedTimeDesc Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::ExtractTimes(const NFmiQueryData *theSourceData,
                                               const NFmiTimeDescriptor &theWantedTimeDesc)
{
  NFmiQueryInfo *tmpInfoPtr = theSourceData->Info();
  NFmiQueryInfo dataInfo(tmpInfoPtr->ParamDescriptor(),
                         theWantedTimeDesc,
                         tmpInfoPtr->HPlaceDescriptor(),
                         tmpInfoPtr->VPlaceDescriptor(),
                         tmpInfoPtr->InfoVersion());
  //  dataInfo.InfoVersion(tmpInfoPtr->InfoVersion()); // pidetään infoversio samana, muuten menee
  //  interpoloinnit pieleen!
  NFmiQueryData *destData = CreateEmptyData(dataInfo);
  if (destData)
  {
    NFmiFastQueryInfo sourceInfo(const_cast<NFmiQueryData *>(theSourceData));
    NFmiFastQueryInfo destInfo(destData);
    for (destInfo.ResetTime(); destInfo.NextTime();)
    {
      if (sourceInfo.Time(destInfo.Time()))
      {
        for (destInfo.ResetParam(), sourceInfo.ResetParam();
             destInfo.NextParam() && sourceInfo.NextParam();)
        {
          for (destInfo.ResetLocation(), sourceInfo.ResetLocation();
               destInfo.NextLocation() && sourceInfo.NextLocation();)
          {
            for (destInfo.ResetLevel(), sourceInfo.ResetLevel();
                 destInfo.NextLevel() && sourceInfo.NextLevel();)
            {
              float value = sourceInfo.FloatValue();
              destInfo.FloatValue(value);
            }
          }
        }
      }
    }
    if (destData)  // lisataan tuottaja id tieto, jos lahdedatasta sita loytyy
      AddProducerIds(destInfo, sourceInfo);
  }
  return destData;
}

// ----------------------------------------------------------------------
/*!
 *  täyttää kaiken minkä voi sourcesta dest:iin jos destissä on puuttuvaa(!)
 *  Oletus infot muuten samanlaisia, paitsi alku ja loppuajat voivat poiketa.
 *  Palauttaa true jos täyttö oli kattava (tai dataa oli jo joka paikassa muuten) ja
 *  false jos joku paikka jäi täyttämättä.
 *
 * \param theDestination Undocumented
 * \param theSource Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool FillSimilarDataWithDifferentTimeEnds(NFmiFastQueryInfo *theDestination,
                                          NFmiFastQueryInfo *theSource)
{
  bool fullFill = false;
  if (theDestination && theSource)
  {
    float value = kFloatMissing;
    float sourceValue = kFloatMissing;
    fullFill = true;
    for (theDestination->ResetTime(); theDestination->NextTime();)
    {
      theSource->Time(theDestination->Time());
      for (theDestination->ResetParam(), theSource->ResetParam();
           theDestination->NextParam() && theSource->NextParam();)
      {
        for (theDestination->ResetLocation(), theSource->ResetLocation();
             theDestination->NextLocation() && theSource->NextLocation();)
        {
          for (theDestination->ResetLevel(), theSource->ResetLevel();
               theDestination->NextLevel() && theSource->NextLevel();)
          {
            value = theDestination->FloatValue();
            if (value == kFloatMissing || value == kTCombinedWeatherFloatMissing)
            {
              sourceValue = theSource->FloatValue();  // palauttaa puuttuvaa jos aika ei ollut
                                                      // kohdallaan vain fastqueryinfossa!!!
              if (sourceValue == kFloatMissing || sourceValue == kTCombinedWeatherFloatMissing)
                fullFill = false;  // yhdenkin kerran kun menee tänne, tiedetään että jokin osa
                                   // datasta puuttuu vielä
              else
                theDestination->FloatValue(sourceValue);
            }
          }
        }
      }
    }
  }
  return fullFill;
}

// ----------------------------------------------------------------------
/*!
 *  Ottaa kaiken minkä voi sourcesta dest:iin, jos destissä on puuttuvaa(!)
 *  Oletus infot muuten samanlaisia, paitsi alku ja loppuajat  voivat poiketa.
 *  Sourcen aikaresoluutio on pienempi destinationin.
 *  Palauttaa true jos täyttö oli kattava (tai dataa oli jo joka paikassa muuten) ja
 *  false jos joku paikka jäi täyttämättä.
 *  Jos weatherandcloudiness otus ja siinä sademäärä, lasketaan tarvittava sadesumma talteen
 *  13.6.2002/Marko Lisäsin myös aikainterpolointiin maxSearchRange-parametrin,
 *  jolla rajoitetaan halutessa interpolointia ja extrapolointia. Eli jos arvot
 *  löytyvät liian kaukaa ajallisesti, laitetaan puuttuvaa arvoa tulokseksi.
 *  Oletus arvo on 360 minuuttia jos parametrin arvoksi annetaan 0, ei ajallista rajaa ole.
 *  Lisäksi muutin toiminnon niin, että käytetään aina queryinfon-aikainterpolointia.
 *
 * \param theDestination Undocumented
 * \param theSource Undocumented
 * \param theTimeResolutionRatio Undocumented
 * \param theMaxTimeSearchRangeInMinutes Undocumented
 * \return Undocumented
 *
 * \todo Poista kayttamaton theTimeResolutionRatio parametri
 */
// ----------------------------------------------------------------------

bool InterpolateSimilarDataToLargerTimeResolution(NFmiFastQueryInfo *theDestination,
                                                  NFmiFastQueryInfo *theSource,
                                                  float /* theTimeResolutionRatio */,
                                                  int theMaxTimeSearchRangeInMinutes)
{
  bool fullFill = false;
  if (theDestination && theSource)
  {
    float value = kFloatMissing;
    float sourceValue = kFloatMissing;
    fullFill = true;
    NFmiDataModifierMode modeModifier;
    for (theDestination->ResetTime(); theDestination->NextTime();)
    {
      NFmiMetTime t(theDestination->Time());
      theSource->Time(t);
      for (theDestination->ResetParam(), theSource->ResetParam();
           theDestination->NextParam() && theSource->NextParam();)
      {
        for (theDestination->ResetLocation(), theSource->ResetLocation();
             theDestination->NextLocation() && theSource->NextLocation();)
        {
          for (theDestination->ResetLevel(), theSource->ResetLevel();
               theDestination->NextLevel() && theSource->NextLevel();)
          {
            value = theDestination->FloatValue();
            if (value == kFloatMissing || value == kTCombinedWeatherFloatMissing)
            {
              sourceValue = theSource->InterpolatedValue(t, theMaxTimeSearchRangeInMinutes);
              if (sourceValue == kFloatMissing || sourceValue == kTCombinedWeatherFloatMissing)
                fullFill = false;  // yhdenkin kerran kun menee tänne, tiedetään että jokin osa
                                   // datasta puuttuu vielä
              else
                theDestination->FloatValue(sourceValue);
            }
          }  // for(location)-looppi
        }    // for(param)-looppi
      }      // for(level)-looppi
    }        // for(time)-looppi
  }
  return fullFill;
}

// ----------------------------------------------------------------------
/*!
 * \param theValueVec Undocumented
 * \param theTimeVec Undocumented
 * \param theInterpolatedTimePlace Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float GetLagrangeValue(checkedVector<double> &theValueVec,
                       checkedVector<double> &theTimeVec,
                       double theInterpolatedTimePlace)
{
  NFmiLagrange lagrange;
  if (std::find_if(theValueVec.begin(),
                   theValueVec.end(),
                   ::IsMissingValue<double>(kFloatMissing)) != theValueVec.end())
    return kFloatMissing;  // jos yksikin lähtöarvoista oli puuttuva, palauta puuttuva
  else
  {
    lagrange.Init(&theTimeVec[0], &theValueVec[0], static_cast<int>(theValueVec.size()));
    return static_cast<float>(lagrange.Interpolate(theInterpolatedTimePlace));
  }
}

// ----------------------------------------------------------------------
/*!
 * TheInterpolatedTimePlace on interpoloitavan ajan paikka 0 - 1 avaruudessa
 * niiden kahden aikapisteen välissä, jotka ovat lähinnä interpoloitavaa
 * pistettä lähdedatassa.
 *
 * \param theInfo Undocumented
 * \param theTime Undocumented
 * \param theInterpolationMethod Undocumented
 * \param theMaxTimeSearchRangeInMinutes Undocumented
 * \param theOneStepBeforeTimeIndex Undocumented
 * \param theInterpolatedTimePlace Undocumented
 * \param theParamInterpMethod Undocumented
 * \param theLimitChecker Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float TimeInterpolationValue(NFmiFastQueryInfo &theInfo,
                             const NFmiMetTime &theTime,
                             FmiInterpolationMethod theInterpolationMethod,
                             int theMaxTimeSearchRangeInMinutes,
                             unsigned long theOneStepBeforeTimeIndex,
                             double theInterpolatedTimePlace,
                             FmiInterpolationMethod theParamInterpMethod,
                             NFmiQueryDataUtil::LimitChecker &theLimitChecker)
{
  if (theInterpolationMethod != kLagrange ||
      !(theParamInterpMethod == kLinearly || theParamInterpMethod == kLagrange))
    return theInfo.InterpolatedValue(theTime, theMaxTimeSearchRangeInMinutes);
  else
  {
    float value = kFloatMissing;
    unsigned long oldTimeIndex = theInfo.TimeIndex();
    theInfo.TimeIndex(theOneStepBeforeTimeIndex);
    if (theOneStepBeforeTimeIndex ==
        0)  // case: annettu index timebagin alku rajalla, eli tehdään interpolointi 3:lla arvolla
    {
      checkedVector<double> valueVec(3);
      checkedVector<double> timeVec(
          3);  // yllä olevien arvojen suhteelliset paikat aika-avaruudessa (0 - 1)
      timeVec[0] = 0;
      timeVec[1] = 1;
      timeVec[2] = 2;
      valueVec[0] = theInfo.PeekTimeValue(0);
      valueVec[1] = theInfo.PeekTimeValue(1);
      valueVec[2] = theInfo.PeekTimeValue(2);
      value = GetLagrangeValue(valueVec, timeVec, theInterpolatedTimePlace);
    }
    else if (theOneStepBeforeTimeIndex == theInfo.SizeTimes() - 2)  // case: annettu index timebagin
                                                                    // loppu rajalla, eli tehdään
                                                                    // interpolointi 3:lla arvolla
    {
      checkedVector<double> valueVec(3);
      checkedVector<double> timeVec(
          3);  // yllä olevien arvojen suhteelliset paikat aika-avaruudessa (0 - 1)
      timeVec[0] = -1;
      timeVec[1] = 0;
      timeVec[2] = 1;
      valueVec[0] = theInfo.PeekTimeValue(-1);
      valueVec[1] = theInfo.PeekTimeValue(0);
      valueVec[2] = theInfo.PeekTimeValue(1);
      value = GetLagrangeValue(valueVec, timeVec, theInterpolatedTimePlace);
    }
    else if (theOneStepBeforeTimeIndex >
             theInfo.SizeTimes() - 2)  // ei voi interpoloida, annettu indeksi väärin?
      value = kFloatMissing;
    else  // muuten tehdään normaali 4:n pisteen interpolointi
    {
      checkedVector<double> valueVec(4);
      checkedVector<double> timeVec(
          4);  // yllä olevien arvojen suhteelliset paikat aika-avaruudessa (0 - 1)
      timeVec[0] = -1;
      timeVec[1] = 0;
      timeVec[2] = 1;
      timeVec[3] = 2;
      valueVec[0] = theInfo.PeekTimeValue(-1);
      valueVec[1] = theInfo.PeekTimeValue(0);
      valueVec[2] = theInfo.PeekTimeValue(1);
      valueVec[3] = theInfo.PeekTimeValue(2);
      value = GetLagrangeValue(valueVec, timeVec, theInterpolatedTimePlace);
    }
    theInfo.TimeIndex(oldTimeIndex);  // palautetaan varmuuden vuoksi originaali index (vaikka
                                      // minusta se on turhaa)
    return theLimitChecker.GetInsideLimitsValue(value);
  }
}

// ----------------------------------------------------------------------
/*!
 *  Laskee muutuva aikaresoluutioiselle datalle aikainterpoloinnin.
 *	Käyttää lagrangea jos on niin haluttu tai se on mahdollista.
 *
 * \param theInfo Undocumented
 * \param theTime Undocumented
 * \param theInterpolationMethod Undocumented
 * \param theMaxTimeSearchRangeInMinutes Undocumented
 * \param theOneStepBeforeTimeIndex Undocumented
 * \param theInterpolatedTimePlace Undocumented
 * \param theParamInterpMethod Undocumented
 * \param theLimitChecker Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float TimeInterpolationValueWCTR(NFmiFastQueryInfo &theInfo,
                                 const NFmiMetTime &theTime,
                                 FmiInterpolationMethod theInterpolationMethod,
                                 int theMaxTimeSearchRangeInMinutes,
                                 unsigned long theOneStepBeforeTimeIndex,
                                 double theInterpolatedTimePlace,
                                 FmiInterpolationMethod theParamInterpMethod,
                                 NFmiQueryDataUtil::LimitChecker &theLimitChecker,
                                 checkedVector<double> &theTimeFactors)  // lagrange timeFactorit on
                                                                         // laskettu kerran joka
                                                                         // aika askeleelle
{
  if (theInterpolationMethod != kLagrange ||
      !(theParamInterpMethod == kLinearly || theParamInterpMethod == kLagrange))
    return theInfo.InterpolatedValue(theTime, theMaxTimeSearchRangeInMinutes);
  else
  {
    //	  checkedVector<double> timeVec(4); // yllä olevien arvojen suhteelliset paikat
    // aika-avaruudessa (0 - 1)
    float value = kFloatMissing;
    unsigned long oldTimeIndex = theInfo.TimeIndex();
    theInfo.TimeIndex(theOneStepBeforeTimeIndex);
    if (theOneStepBeforeTimeIndex ==
        0)  // case: annettu index timebagin alku rajalla, eli tehdään interpolointi 3:lla arvolla
    {
      checkedVector<double> valueVec(3);
      valueVec[0] = theInfo.PeekTimeValue(0);
      valueVec[1] = theInfo.PeekTimeValue(1);
      valueVec[2] = theInfo.PeekTimeValue(2);
      value = GetLagrangeValue(valueVec, theTimeFactors, theInterpolatedTimePlace);
    }
    else if (theOneStepBeforeTimeIndex == theInfo.SizeTimes() - 2)  // case: annettu index timebagin
                                                                    // loppu rajalla, eli tehdään
                                                                    // interpolointi 3:lla arvolla
    {
      checkedVector<double> valueVec(3);
      valueVec[0] = theInfo.PeekTimeValue(-1);
      valueVec[1] = theInfo.PeekTimeValue(0);
      valueVec[2] = theInfo.PeekTimeValue(1);
      value = GetLagrangeValue(valueVec, theTimeFactors, theInterpolatedTimePlace);
    }
    else if (theOneStepBeforeTimeIndex >
             theInfo.SizeTimes() - 2)  // ei voi interpoloida, annettu indeksi väärin?
      value = kFloatMissing;
    else  // muuten tehdään normaali 4:n pisteen interpolointi
    {
      checkedVector<double> valueVec(4);
      valueVec[0] = theInfo.PeekTimeValue(-1);
      valueVec[1] = theInfo.PeekTimeValue(0);
      valueVec[2] = theInfo.PeekTimeValue(1);
      valueVec[3] = theInfo.PeekTimeValue(2);
      value = GetLagrangeValue(valueVec, theTimeFactors, theInterpolatedTimePlace);
    }
    theInfo.TimeIndex(oldTimeIndex);  // palautetaan varmuuden vuoksi originaali index (vaikka
                                      // minusta se on turhaa)
    return theLimitChecker.GetInsideLimitsValue(value);
  }
}

// ----------------------------------------------------------------------
/*!
 *  Interpoloi kaiken minkä voi sourcesta dest:iin.
 *  Oletus infot muuten samanlaisia, paitsi alku ja loppuajat  voivat poiketa.
 *  Sourcen aikaresoluutio on suurempi destinationin.
 *  Palauttaa true jos täyttö oli kattava (tai dataa oli jo joka paikassa muuten) ja
 *  false jos joku paikka jäi täyttämättä.
 *  13.6.2002/Marko Lisäsin myös aikainterpolointiin maxSearchRange-parametrin,
 *  jolla rajoitetaan halutessa interpolointia ja extrapolointia. Eli jos arvot
 *  löytyvät liian kaukaa ajallisesti, laitetaan puuttuvaa arvoa tulokseksi.
 *  Oletus arvo on 360 minuuttia jos parametrin arvoksi annetaan 0, ei ajallista rajaa ole.
 *
 * \param theDestination Undocumented
 * \param theSource Undocumented
 * \param theTimeResolutionRatio Undocumented
 * \param theMaxTimeSearchRangeInMinutes Undocumented
 * \param theInterpolationMethod Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool InterpolateSimilarDataToSmallerTimeResolution(NFmiFastQueryInfo *theDestination,
                                                   NFmiFastQueryInfo *theSource,
                                                   float /* theTimeResolutionRatio */,
                                                   int theMaxTimeSearchRangeInMinutes,
                                                   FmiInterpolationMethod theInterpolationMethod)
{
  bool fullFill = false;
  if (theDestination && theSource)
  {
    float sourceValue = kFloatMissing;
    bool useLagrange = theInterpolationMethod == kLagrange;
    unsigned long lagrangeTimeIndex = 0;  // tähän laitetaan interpoloitavaa aikaa lähin edeltävän
                                          // ajan aikaindeksi, joka löytyy lähdedatasta
    double lagrangeTimePosition = 0;      // tähän laitetaan interpoloitavan ajan paikka 0 - 1
    // avaruudessa niiden kahden aikapisteen välissä, jotka ovat
    // lähinnä interpoloitavaa pistettä lähdedatassa.
    fullFill = true;
    for (theDestination->ResetTime(); theDestination->NextTime();)
    {
      NFmiMetTime t(theDestination->Time());
      bool timeStatus = theSource->Time(t);
      if (useLagrange && (!timeStatus))
      {  // tehdään lagrange intepolointia varten tarvittavat laskut
        theSource->TimeToNearestStep(t, kBackward, theMaxTimeSearchRangeInMinutes);
        lagrangeTimeIndex = theSource->TimeIndex();
        NFmiMetTime time1(theSource->Time());
        theSource->NextTime();
        NFmiMetTime time2(theSource->Time());
        lagrangeTimePosition = static_cast<double>(t.DifferenceInMinutes(time1)) /
                               static_cast<double>(time2.DifferenceInMinutes(time1));
      }
      for (theDestination->ResetParam(), theSource->ResetParam();
           theDestination->NextParam() && theSource->NextParam();)
      {
        const NFmiParam *tmpParam = theDestination->Param().GetParam();
        NFmiQueryDataUtil::LimitChecker checkerWithLagrangeInterpolation(
            static_cast<float>(tmpParam->MinValue()),
            static_cast<float>(tmpParam->MaxValue()),
            IsParamCircularValued(tmpParam));
        FmiInterpolationMethod paramInterpMethod =
            theDestination->Param().GetParam()->InterpolationMethod();
        for (theDestination->ResetLocation(), theSource->ResetLocation();
             theDestination->NextLocation() && theSource->NextLocation();)
        {
          for (theDestination->ResetLevel(), theSource->ResetLevel();
               theDestination->NextLevel() && theSource->NextLevel();)
          {
            sourceValue = theSource->FloatValue();  // palauttaa puuttuvaa jos aika ei ollut
                                                    // kohdallaan vain fastqueryinfossa!!!
            if (timeStatus &&
                !(sourceValue == kFloatMissing || sourceValue == kTCombinedWeatherFloatMissing))
            {
              theDestination->FloatValue(sourceValue);
            }
            else  // pitää interpoloida
            {
              float tempV = TimeInterpolationValue(*theSource,
                                                   t,
                                                   theInterpolationMethod,
                                                   theMaxTimeSearchRangeInMinutes,
                                                   lagrangeTimeIndex,
                                                   lagrangeTimePosition,
                                                   paramInterpMethod,
                                                   checkerWithLagrangeInterpolation);
              theDestination->FloatValue(tempV);
            }
          }
        }
      }
    }
  }
  return fullFill;
}

// luo vaihtuva aikaresoluutioisesta (WCTR=With Changing Time Resolution) datasta tasaväliaikaista
// dataa
bool MakeSimilarTimeBagDataFromWCTRData(NFmiFastQueryInfo &theDestination,
                                        NFmiFastQueryInfo &theSource,
                                        int theMaxTimeSearchRangeInMinutes,
                                        FmiInterpolationMethod theInterpolationMethod)
{
  if (theSource.InfoVersion() != theDestination.InfoVersion()) return false;

  theDestination.First();
  int timeSize = theDestination.TimeDescriptor().Size();

  bool status = true;
  NFmiMetTime time1, time2;
  NFmiDataModifierSum sumModifier;
  NFmiDataModifierMode modeModifier;
  bool useLagrange = theInterpolationMethod == kLagrange;
  unsigned long lagrangeTimeIndex = 0;  // tähän laitetaan interpoloitavaa aikaa lähin edeltävän
                                        // ajan aikaindeksi, joka löytyy lähdedatasta
  double lagrangeTimePosition = 0;  // tähän laitetaan interpoloitavan ajan paikka 0 - 1 avaruudessa
                                    // niiden kahden aikapisteen välissä, jotka ovat lähinnä
                                    // interpoloitavaa pistettä lähdedatassa.
  checkedVector<double> timeFactors;  // tähän lasketaan joka aika-askeleella mahdolliset lagrange
                                      // laskuissa tarvittavat aikakertoimet

  int i = 0;
  for (theDestination.ResetTime(); theDestination.NextTime() && i < timeSize;
       i++)  // juoksutetaan ajat läpi
  {
    if (i == 0)
      time1 = time2 = theDestination.Time();
    else
    {
      time1 = time2;
      time1.SetTimeStep(1);
      time2 = theDestination.Time();
      time1.ChangeByMinutes(1);
    }

    bool timeFound = theSource.Time(theDestination.Time());
    NFmiMetTime currentTime(theDestination.Time());
    if (!timeFound) theSource.TimeToNearestStep(currentTime, kForward);

    if (useLagrange && (!timeFound))
    {  // tehdään lagrange intepolointia varten tarvittavat laskut
      theSource.TimeToNearestStep(currentTime, kBackward, theMaxTimeSearchRangeInMinutes);
      lagrangeTimeIndex = theSource.TimeIndex();
      NFmiMetTime timeLagrange1(theSource.Time());
      theSource.NextTime();
      lagrangeTimePosition = currentTime.DifferenceInMinutes(timeLagrange1);

      theSource.TimeIndex(lagrangeTimeIndex);  // source pitää asettaa vielä oikeaan aikaan, että
                                               // timeFactorit voidaan laskea
      if (lagrangeTimeIndex == 0)  // case: annettu index timebagin alku rajalla, eli tehdään
                                   // interpolointi 3:lla arvolla
      {
        timeFactors.resize(3);
        NFmiMetTime time_0(theSource.Time());
        theSource.NextTime();
        NFmiMetTime time_1(theSource.Time());
        theSource.NextTime();
        NFmiMetTime time_2(theSource.Time());
        timeFactors[0] = 0;
        timeFactors[1] = time_1.DifferenceInMinutes(time_0);
        timeFactors[2] = time_2.DifferenceInMinutes(time_0);
      }
      else if (lagrangeTimeIndex == theSource.SizeTimes() - 2)  // case: annettu index timebagin
                                                                // loppu rajalla, eli tehdään
                                                                // interpolointi 3:lla arvolla
      {
        timeFactors.resize(3);
        NFmiMetTime time_0(theSource.Time());
        theSource.PreviousTime();
        NFmiMetTime time_m1(theSource.Time());
        theSource.NextTime();
        theSource.NextTime();
        NFmiMetTime time_1(theSource.Time());
        timeFactors[0] = time_m1.DifferenceInMinutes(time_0);
        timeFactors[1] = 0;
        timeFactors[2] = time_1.DifferenceInMinutes(time_0);
      }
      else  // muuten tehdään normaali 4:n pisteen interpolointi
      {
        timeFactors.resize(4);
        NFmiMetTime time_0(theSource.Time());
        theSource.PreviousTime();
        NFmiMetTime time_m1(theSource.Time());
        theSource.NextTime();
        theSource.NextTime();
        NFmiMetTime time_1(theSource.Time());
        theSource.NextTime();
        NFmiMetTime time_2(theSource.Time());
        timeFactors[0] = time_m1.DifferenceInMinutes(time_0);
        timeFactors[1] = 0;
        timeFactors[2] = time_1.DifferenceInMinutes(time_0);
        timeFactors[3] = time_2.DifferenceInMinutes(time_0);
      }
    }

    for (theDestination.ResetParam(); theDestination.NextParam();)  // juoksutetaan parametrit läpi
    {
      const NFmiParam *tmpParam = theDestination.Param().GetParam();
      NFmiQueryDataUtil::LimitChecker checkerWithLagrangeInterpolation(
          static_cast<float>(tmpParam->MinValue()),
          static_cast<float>(tmpParam->MaxValue()),
          IsParamCircularValued(tmpParam));
      FmiInterpolationMethod paramInterpMethod =
          theDestination.Param().GetParam()->InterpolationMethod();
      if (theSource.Param(*theDestination.Param().GetParam()))
      {
        for (theDestination.ResetLocation(), theSource.ResetLocation();
             theDestination.NextLocation() && theSource.NextLocation();)
        {
          for (theDestination.ResetLevel(), theSource.ResetLevel();
               theDestination.NextLevel() && theSource.NextLevel();)
          {
            if (timeFound)
              theDestination.FloatValue(theSource.FloatValue());
            else
            {
              float tempV = TimeInterpolationValueWCTR(theSource,
                                                       currentTime,
                                                       theInterpolationMethod,
                                                       theMaxTimeSearchRangeInMinutes,
                                                       lagrangeTimeIndex,
                                                       lagrangeTimePosition,
                                                       paramInterpMethod,
                                                       checkerWithLagrangeInterpolation,
                                                       timeFactors);
              theDestination.FloatValue(tempV);
            }
          }
        }
      }
      //				else // HUOM! ei vakava virhe (joten kommentissa)
      //					status = false; // parametrilista ei ollut
      // täydellinen sourcedatassa (pitää varautua moiseen!)
    }
    //		else // HUOM! ei vakava virhe (joten kommentissa)
    //			status = false; // ei saisi tulla tähän!!!!
  }
  return status;
}

// ----------------------------------------------------------------------
/*!
 * Tämä funktio tekee pienen siirron ja muokkauksen annettuun timebagiin
 * riippuen referenssi bagistä. Eli mahdollinen siirto tunti juoksutuksesta
 * (3h resoluutio) 1, 4, 7, 10, ... vaikka seuraavanlaiseen  0, 3, 6, 9, ...
 * HUOM!!! TimeBAgeilla pitää olla sama resoluutio!!!
 *
 * \param theTimeBag Undocumented
 * \param thePossibleReferenceTimeBag Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiTimeBag MakeReferenceTimeBagCheck(const NFmiTimeBag &theTimeBag,
                                      NFmiTimeBag *thePossibleReferenceTimeBag)
{
  NFmiMetTime startTime(theTimeBag.FirstTime());
  NFmiMetTime startRefTime(thePossibleReferenceTimeBag->FirstTime());
  int resolutionInMinutes = theTimeBag.Resolution();
  int shiftInMinutes = abs(startTime.DifferenceInMinutes(startRefTime) % resolutionInMinutes);
  if (shiftInMinutes == 0)
    return theTimeBag;
  else
  {
    startTime.ChangeByMinutes(shiftInMinutes);
    NFmiMetTime endTime(theTimeBag.LastTime());
    endTime.ChangeByMinutes(-(resolutionInMinutes - shiftInMinutes));
    if (startTime > endTime) endTime = startTime;
    NFmiTimeBag tmp(startTime, endTime, resolutionInMinutes);
    return tmp;
  }
}

// ----------------------------------------------------------------------
/*!
 * Jos timebag pointteri on 0, otetaan timebagi smartista ja resoluutio
 * annetusta parametrista, mutta jos timebag pointteri osoittaa johonkin,
 * otetaan se suoraan käyttöön. thePossibleReferenceTimeBag:ia käytetään
 * asettamaan tulosdatan timebagi oikeaan aika-jaksotukseen. Eli
 * jos halutaan esim., että tunnit juoksevat 3 tunnin
 * välein 1, 4, 7, 10, ... tai 0, 3, 6, 9, ...
 *
 * \param theSourceInfo Undocumented
 * \param theResolutionInMinutes Undocumented
 * \param theStartTimeResolutionInMinutes Undocumented
 * \param theWantedTimeBag Undocumented
 * \param thePossibleReferenceTimeBag Undocumented
 * \param theMaxTimeSearchRangeInMinutes Undocumented
 * \param theInterpolationMethod Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *CreateSimilarDataWithTimeInterpolation(NFmiFastQueryInfo *theSourceInfo,
                                                      long theResolutionInMinutes,
                                                      int theStartTimeResolutionInMinutes,
                                                      NFmiTimeBag *theWantedTimeBag,
                                                      NFmiTimeBag *thePossibleReferenceTimeBag,
                                                      int theMaxTimeSearchRangeInMinutes,
                                                      FmiInterpolationMethod theInterpolationMethod)
{
  NFmiQueryData *newData = 0;
  if (theSourceInfo)
  {
    theSourceInfo->FirstTime();  // Varmistetaan että ollaan 1. aika-askeleessa, jotta
    // aikaresoluutio on aina sama samalle datalle (eikä satunnainen,
    // jos vaihtuva resoluutioinen data ja aika-indeksi on joku muu
    // kuin 0)
    bool sourceDataIsWCTR =
        theSourceInfo->TimeDescriptor().UseTimeList();  // WCTR = With Changing Time Resolution
    float timeResolutionRatio =
        float(theWantedTimeBag ? static_cast<long>(theWantedTimeBag->Resolution())
                               : theResolutionInMinutes) /
        theSourceInfo->TimeResolution();
    if (timeResolutionRatio == 1 && !theWantedTimeBag && !sourceDataIsWCTR)
      return theSourceInfo->RefQueryData()->Clone();

    // luodaan uusi info, jolla aika-askel parametrina saadun timeperiodin mukainen
    NFmiTimePerioid timePerioid(theResolutionInMinutes);
    NFmiTimeBag timeBag;
    if (theWantedTimeBag)
      timeBag = *theWantedTimeBag;
    else
    {
      NFmiMetTime startTime(theSourceInfo->TimeDescriptor().FirstTime());
      NFmiMetTime endTime(theSourceInfo->TimeDescriptor().LastTime());
      if (theStartTimeResolutionInMinutes != kFloatMissing)
      {
        startTime.SetTimeStep(static_cast<short>(theStartTimeResolutionInMinutes));
        // startTime.PreviousMetTime();
        endTime.SetTimeStep(static_cast<short>(theStartTimeResolutionInMinutes));
        // endTime.NextMetTime();
      }
      NFmiTimeBag tmpTimeBag(startTime, endTime, timePerioid);
      timeBag = tmpTimeBag;
    }

    if (thePossibleReferenceTimeBag)
      timeBag = MakeReferenceTimeBagCheck(timeBag, thePossibleReferenceTimeBag);

    NFmiTimeDescriptor timeDescriptor(theSourceInfo->TimeDescriptor().OriginTime(),
                                      timeBag,
                                      kUTC,
                                      theSourceInfo->TimeDescriptor().IsInterpolation());

    // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
    NFmiQueryInfo newInnerInfo(theSourceInfo->ParamDescriptor(),
                               timeDescriptor,
                               theSourceInfo->HPlaceDescriptor(),
                               theSourceInfo->VPlaceDescriptor(),
                               theSourceInfo->InfoVersion());
    newInnerInfo.Header(theSourceInfo->Header());
    newData = new NFmiQueryData(newInnerInfo);
    newData->Init();

    NFmiFastQueryInfo newFastInfo(newData);

    if (sourceDataIsWCTR)
    {
      MakeSimilarTimeBagDataFromWCTRData(
          newFastInfo, *theSourceInfo, theMaxTimeSearchRangeInMinutes, theInterpolationMethod);
    }
    else
    {
      if (timeResolutionRatio == 1)
        FillSimilarDataWithDifferentTimeEnds(&newFastInfo, theSourceInfo);
      else if (timeResolutionRatio > 1)
        InterpolateSimilarDataToLargerTimeResolution(
            &newFastInfo, theSourceInfo, timeResolutionRatio, theMaxTimeSearchRangeInMinutes);
      else
        InterpolateSimilarDataToSmallerTimeResolution(&newFastInfo,
                                                      theSourceInfo,
                                                      timeResolutionRatio,
                                                      theMaxTimeSearchRangeInMinutes,
                                                      theInterpolationMethod);
    }
  }
  return newData;
}

// ----------------------------------------------------------------------
/*!
 * Parametrit,levelit ja hila/asemat pitää olla samat!
 * Pitää olla timeBag (ei timelist, tulee muuttumaan?)
 * Interpolationmethod uutena parametrina. Ei oteta datasta
 * ja niiden parametreista, vaan annetaan erillisenä parametrina
 * (tiettyjä parametreja mm. yhdisteläparametreja, ei voi hoitaa lagrange
 * metodilla ja ne tehdään lineaarisesti tai nearest metodilla.).
 *
 * \param theSourceData Undocumented
 * \param theTimeResolutionInMinutes Undocumented
 * \param theStartTimeResolutionInMinutes Undocumented
 * \param thePossibleReferenceTimeBag Undocumented
 * \param theMaxTimeSearchRangeInMinutes Undocumented
 * \param theInterpolationMethod Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::InterpolateTimes(NFmiQueryData *theSourceData,
                                                   int theTimeResolutionInMinutes,
                                                   int theStartTimeResolutionInMinutes,
                                                   NFmiTimeBag *thePossibleReferenceTimeBag,
                                                   int theMaxTimeSearchRangeInMinutes,
                                                   FmiInterpolationMethod theInterpolationMethod)
{
  NFmiFastQueryInfo sourceInfo(theSourceData);
  NFmiTimeBag *wantedTimeBag = 0;  // tulevaisuudessa erilainen käyttö?
  NFmiQueryData *data = CreateSimilarDataWithTimeInterpolation(&sourceInfo,
                                                               theTimeResolutionInMinutes,
                                                               theStartTimeResolutionInMinutes,
                                                               wantedTimeBag,
                                                               thePossibleReferenceTimeBag,
                                                               theMaxTimeSearchRangeInMinutes,
                                                               theInterpolationMethod);
  if (data)
  {
    NFmiFastQueryInfo newDataInfo(data);
    AddProducerIds(newDataInfo, sourceInfo);
  }

  return data;
}

// ----------------------------------------------------------------------
/*!
 * Luodaan uusi TimeDescriptori: alkuperäiseltä datalta otetaan alku-
 * ja loppuajat, aikaresoluutio integerinä komentoriviltä
 *
 * \param theSourceData Undocumented
 * \param theTimeResolutionInMinutes Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiTimeDescriptor NFmiQueryDataUtil::NewTimeDescriptor(NFmiQueryData *theSourceData,
                                                        int theTimeResolutionInMinutes)
{
  const NFmiTimeBag &originTimeBag = theSourceData->Info()->ValidTimes();

  // luodaan uusi timeBagi ja	timeDescriptori
  NFmiTimeBag newTimeBag(
      originTimeBag.FirstTime(), originTimeBag.LastTime(), theTimeResolutionInMinutes);
  NFmiTimeDescriptor newTimeDescriptor(theSourceData->Info()->OriginTime(), newTimeBag);

  return newTimeDescriptor;
}

// ----------------------------------------------------------------------
/*!
 *  Kirjoitetaan lyhyemmän aikavälin data pidemmän aikavälin datan alun päälle
 *  Oletus: datoilla sama aikaresoluutio (+ Parametrit,levelit ja hila/asemat pitää olla samat!)
 *
 * \param theLongerInfo Undocumented
 * \param theShorterInfo Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiQueryDataUtil::OverWriteTimesFromFile(NFmiFastQueryInfo &theLongerInfo,
                                               NFmiFastQueryInfo &theShorterInfo)
{
  theLongerInfo.Reset();
  theShorterInfo.Reset();
  float value = kFloatMissing;

  // Etsitään pienin yhteinen aika
  do
  {
    if (!theShorterInfo.NextTime()) break;
  } while (!theLongerInfo.Time(theShorterInfo.Time()));

  // Siirretään ajassa yksi pykälä taaksepäin, että ei jää yhtään aikaa väliin
  // Entä, jos ensimmäinen aika mätsää --> siirtääkö resettiin?
  theShorterInfo.PreviousTime();
  theLongerInfo.PreviousTime();

  // Kirjoitetaan lyhyemmän datan tiedot pidemmän päälle
  for (; theShorterInfo.NextTime() &&
         theLongerInfo.NextTime();)  // käydään läpi niin kauan kuin lyhyemmässä datassa on aikoja
  {
    for (theLongerInfo.ResetParam(), theShorterInfo.ResetParam();
         theLongerInfo.NextParam() && theShorterInfo.NextParam();)
    {
      for (theLongerInfo.ResetLocation(), theShorterInfo.ResetLocation();
           theLongerInfo.NextLocation() && theShorterInfo.NextLocation();)
      {
        for (theLongerInfo.ResetLevel(), theShorterInfo.ResetLevel();
             theLongerInfo.NextLevel() && theShorterInfo.NextLevel();)
        {
          value = theShorterInfo.FloatValue();
          if (value == kFloatMissing || value == kTCombinedWeatherFloatMissing)
            ;  // ei tehdä mitään jos puuttuvaa
          else
            theLongerInfo.FloatValue(value);

          // HUOM! muutin puuttuva arvo käsittelyä siten, että vain ei puuttuvat
          // arvot sijoitetaan kohdedataan, mutta toimintaa ei lopeteta kuitenkaan.

          // VAARA!!!! Marko joutui tekemään pikavirityksen!!!
          // Jos löytyy yksikin puuttuva arvo, lopetetaan funktion suoritus.
          // Tämä sen takia, että ECMWF-jutussa tulee lopussa joka toisessa aika-askeleessa
          // puuttuvaa ja siitä eteenpäin datan syötön on loputtava!!
          // Tämä pitää kuitenkin hoitaa joskus tyylikkäämmin.
          /*
                                            if(value == kFloatMissing)
                                                  return true;

                                            theLongerInfo.FloatValue(value);
          */
        }
      }
    }
  }
  return true;
}

// ----------------------------------------------------------------------
/*!
 * Funktio luo qdata:sta uuden qdatan, jossa on uusi haluttu hila.
 * Uudessa qdatassa on muut systeemit otettu alkuperäisesta qdatasta
 * (time, param ja level).
 *
 * \param theSourceData Undocumented
 * \param theWantedGrid Undocumented
 * \param theInterpolationMethod
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::Interpolate2OtherGrid(NFmiQueryData *theSourceData,
                                                        const NFmiGrid *theWantedGrid,
                                                        NFmiLogger *theLogger)
{
  NFmiQueryData *newData = 0;
  if (theSourceData && theSourceData->IsGrid() && theWantedGrid)
  {
    NFmiHPlaceDescriptor newHPlace(*theWantedGrid,
                                   theSourceData->Info()->HPlaceDescriptor().SelectedType());
    NFmiQueryInfo innerInfo(theSourceData->Info()->ParamDescriptor(),
                            theSourceData->Info()->TimeDescriptor(),
                            newHPlace,
                            theSourceData->Info()->VPlaceDescriptor(),
                            theSourceData->InfoVersion());
    newData = CreateEmptyData(innerInfo);
    FillGridDataFullMT(theSourceData, newData, gMissingIndex, gMissingIndex, theLogger);
  }
  CopyProducerIds(theSourceData, newData);
  return newData;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 * \param theModifierId Undocumented
 * \param theAreaFactor Undocumented
 * \param theRange Undocumented
 * \todo Not implemented
 */
// ----------------------------------------------------------------------

void SpaceOutDataWithSmoother(NFmiFastQueryInfo & /* theSourceInfo */,
                              NFmiFastQueryInfo & /* theDestInfo */,
                              int /* theModifierId */,
                              double /* theAreaFactor */,
                              double /* theRange */)
{
}

// ----------------------------------------------------------------------
/*!
 * Laskee saadusta datasta uuden (haluttuun hilaväliin). Käyttää annettua
 * datamodifieria (esim. avg) laskeakseen uuteen hilaa/dataan arvoja.
 * Datamodifier saa laskuunsa datat aina annetusta datanhakulaatikosta.
 * Jos sinulla on esim. jokin data 200x300 hilassa ja haluat laskea siitä
 * uudet arvot esim. 30x50 hilaan laskien samalla tasoitusta (keskiarvoa)
 * vaikka 5x7 hilalaatikko-alueen yli: anna originaali data, pointti
 * jossa arvot 30 ja 50, NFmiDataModifierAvg-pointteri ja recti, jossa
 * arvot l=-2, t=3, r=2, b=-3. Paluuarvona tulee NFmiQueryData-otus,
 * jossa on tulos (tai 0-pointteri, jos virhe). Parametrit, Levelit ja
 * Aikasysteemit pysyvät samoina.
 *
 * \param theSourceInfo Undocumented
 * \param newGridSize Undocumented
 * \param theModifier Undocumented
 * \param theModifierId Undocumented
 * \param theAreaFactor Undocumented
 * \param theRange Undocumented
 * \param theDataGatheringRange Undocumented
 * \param theMaskCondition Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::SpaceOutData(NFmiFastQueryInfo &theSourceInfo,
                                               const NFmiPoint &newGridSize,
                                               NFmiDataModifier *theModifier,
                                               int theModifierId,
                                               double theAreaFactor,
                                               double theRange,
                                               const NFmiRect &theDataGatheringRange,
                                               const NFmiCalculationCondition &theMaskCondition)
{
  NFmiQueryData *newData = 0;
  if (theSourceInfo.IsGrid())
  {
    NFmiGrid newGrid(theSourceInfo.Grid()->Area(),
                     static_cast<unsigned long>(newGridSize.X()),
                     static_cast<unsigned long>(newGridSize.Y()));
    NFmiHPlaceDescriptor newHPlace(newGrid);
    NFmiQueryInfo newInnerInfo(theSourceInfo.ParamDescriptor(),
                               theSourceInfo.TimeDescriptor(),
                               newHPlace,
                               theSourceInfo.VPlaceDescriptor());
    newInnerInfo.InfoVersion(
        theSourceInfo
            .InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
    newData = CreateEmptyData(newInnerInfo);
    if (newData)
    {
      NFmiFastQueryInfo destInfo(newData);
      theSourceInfo.First();
      destInfo.First();
      if (theModifierId == 101 || theModifierId == 102)
        SpaceOutDataWithSmoother(theSourceInfo, destInfo, theModifierId, theAreaFactor, theRange);
      else if (theModifier)
      {
        int bottom = static_cast<int>(
            theDataGatheringRange.Bottom());  // otettu ulos silmukoista optimointia varten
        int top = static_cast<int>(
            theDataGatheringRange.Top());  // otettu ulos silmukoista optimointia varten
        int left = static_cast<int>(
            theDataGatheringRange.Left());  // otettu ulos silmukoista optimointia varten
        int right = static_cast<int>(
            theDataGatheringRange.Right());  // otettu ulos silmukoista optimointia varten
        float value = kFloatMissing;
        bool useMask = (theMaskCondition.Condition() != kFmiNoMaskOperation &&
                        theMaskCondition.Condition() != kFmiMaskAll);
        for (destInfo.ResetLocation(); destInfo.NextLocation();)
        {
          if (theSourceInfo.NearestPoint(destInfo.LatLon()))
          {
            for (theSourceInfo.ResetParam(), destInfo.ResetParam();
                 theSourceInfo.NextParam() && destInfo.NextParam();)
            {
              bool combinedParam =
                  theSourceInfo.Param().GetParam()->InterpolationMethod() == kByCombinedParam;
              for (theSourceInfo.ResetLevel(), destInfo.ResetLevel();
                   theSourceInfo.NextLevel() && destInfo.NextLevel();)
              {
                for (theSourceInfo.ResetTime(), destInfo.ResetTime();
                     theSourceInfo.NextTime() && destInfo.NextTime();)
                {
                  if (combinedParam)
                  {  // ********* tämä haara vielä toteuttamatta, eli yhdistelmä parametreja ei
                     // vielä hoideta!!! *******
                    value = kFloatMissing;
                  }
                  else
                  {
                    theModifier->Clear();
                    if (useMask)
                    {
                      bool maskOn = theMaskCondition.IsMasked(theSourceInfo.FloatValue());
                      for (int j = top; j <= bottom; j++)
                      {
                        for (int i = left; i <= right; i++)
                        {
                          float tmp = theSourceInfo.PeekLocationValue(i, j);
                          if (maskOn && theMaskCondition.IsMasked(tmp))
                            theModifier->Calculate(tmp);
                          else if (!maskOn && !theMaskCondition.IsMasked(tmp))
                            theModifier->Calculate(tmp);
                        }
                      }
                      value = theModifier->CalculationResult();
                    }
                    else
                    {
                      for (int j = top; j <= bottom; j++)
                      {
                        for (int i = left; i <= right; i++)
                          theModifier->Calculate(theSourceInfo.PeekLocationValue(i, j));
                      }
                      value = theModifier->CalculationResult();
                    }
                  }
                  destInfo.FloatValue(value);
                }
              }
            }
          }
        }
      }
    }
  }
  return newData;
}

static NFmiTimeDescriptor MakeWantedTimeDescriptor(const NFmiTimeDescriptor &theTimeDescriptor1,
                                                   const NFmiTimeDescriptor &theTimeDescriptor2,
                                                   int theStartTimeFunction,
                                                   int theEndTimeFunction,
                                                   int theOriginTimeFunction,
                                                   bool fMakeChangingTimeResolution)
{
  bool is1TimeBag = !theTimeDescriptor1.UseTimeList();
  bool is2TimeBag = !theTimeDescriptor2.UseTimeList();
  if (is1TimeBag ^ is2TimeBag)
    throw runtime_error(
        "NFmiQueryDataUtil::MakeWantedTimeDescriptor: Ei osaa yhdistää timeList- ja "
        "timeBag-dataa.");

  NFmiMetTime origTime(theOriginTimeFunction == 1 ? theTimeDescriptor1.OriginTime()
                                                  : theTimeDescriptor2.OriginTime());
  if (fMakeChangingTimeResolution && is1TimeBag && is2TimeBag)
  {
    NFmiTimeList times1(*theTimeDescriptor1.ValidTimeBag());
    NFmiTimeList times2(*theTimeDescriptor2.ValidTimeBag());
    NFmiTimeList timesTotal = times1.Combine(times2, theStartTimeFunction, theEndTimeFunction);
    return NFmiTimeDescriptor(origTime, timesTotal);
  }
  else
  {
    NFmiTimeDescriptor timeDesc(
        theTimeDescriptor1.Combine(theTimeDescriptor2, theStartTimeFunction, theEndTimeFunction));
    timeDesc.OriginTime(origTime);
    return timeDesc;
  }
}

// ----------------------------------------------------------------------
/*!
 * Ohjelma luo kahdesta annetusta datasta uuden qdatan, jossa on alku ja
 * loppu aika valittu kahdesta annetusta qdatasta, niin että uuden aikaväli
 * on maksimaalinen (kahdesta qdatasta valitaan pienin aika alkuajaksi ja
 * suurin aika loppuajaksi). Datoista numero 1 on priorisoitu korkeammalle
 * eli siitä otetaan arvot, jos sama aika löytyy molemmista. Tulosdataan jää
 * ajallinen aukko, jos uuden datan timebagissä on aikoja, joita ei löydy
 * kummastakaan lähdedatasta. Tuotosta voi rajoittaa siten, että
 * theMaxTimeBagSizeInNewData määrää suurimman uuden timebagin koon, jos
 * uudesta timebagistä tulee suurempi kuin tämä luku on, palautetaan 0-pointteri.
 * Kahden datan aikaresoluutioiden pitää olla samat ja niiden pitää olla
 * synkronissa (eli tunti ajat menevät molemmissa 0, 3, 6, 9, ... eikä toisessa
 * esim. 1, 4, 7, 10,... ja  toisessa 0, 3, 6, 9, ...) Muuten kahdessa annetussa
 * qdatassa pitää olla samat leveli, paramit ja paikat (hila/asemat). Uudessa
 * qdatassa on muut systeemit otettu alkuperäisesta qdatasta (hila/asemat,
 * param ja level).
 *
 * \param theSourceInfo1 Dominoiva data.
 * \param theSourceInfo2 tällä täytetään ne arvot, mitä ei saata ykkös datasta.
 * \param theMaxTimeBagSizeInNewData Ei tehdä tämän suurempaa timebagia (vahinkojen varalle).
 * \param theStartTimeFunction Mistä alkuaika otetaan, jos 0, otetaan pienempi ajoista, jos
 *							   1, otetaan data1:stä alkuaika, jos 2,
 *data2:sta
 *alkuaika.
 * \param theEndTimeFunction Mistä loppu aika otetaan. Jos 0, otetaan suurempi, muutoin sama kuin
 *yllä.
 * \return Palauttaa nyt luodun querydatan pointterin, jos jokin meni pieleen, 0-pointteri
 *palautetaan.
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::CombineTimes(NFmiFastQueryInfo &theSourceInfo1,
                                               NFmiFastQueryInfo &theSourceInfo2,
                                               int theMaxTimeBagSizeInNewData,
                                               int theStartTimeFunction,
                                               int theEndTimeFunction,
                                               int theOriginTimeFunction,
                                               bool fMakeChangingTimeResolution)
{
  NFmiQueryData *destData = 0;

  NFmiTimeDescriptor newTimeDesc = ::MakeWantedTimeDescriptor(theSourceInfo1.TimeDescriptor(),
                                                              theSourceInfo2.TimeDescriptor(),
                                                              theStartTimeFunction,
                                                              theEndTimeFunction,
                                                              theOriginTimeFunction,
                                                              fMakeChangingTimeResolution);
  if (newTimeDesc.Resolution() == 0)
    throw runtime_error(
        "NFmiQueryDataUtil::CombineTimes: Aikaresoluutioksi tuli 0, tarkista onko alkuperäisissä "
        "datoissa aikaresoluutiota.");

  unsigned long oldSize = newTimeDesc.Size();
  newTimeDesc.PruneTimes(theMaxTimeBagSizeInNewData, true);
  if (oldSize != newTimeDesc.Size())
    cerr << "Warning: max allowed time steps is " << theMaxTimeBagSizeInNewData
         << " and now there would have been " << newTimeDesc.Size()
         << ".\nUse m-option to increase max time steps." << endl;
  //  if(newTimeDesc.Size() < static_cast<unsigned int>(theMaxTimeBagSizeInNewData))
  {
    NFmiQueryInfo tmpInfo(theSourceInfo1.ParamDescriptor(),
                          newTimeDesc,
                          theSourceInfo1.HPlaceDescriptor(),
                          theSourceInfo1.VPlaceDescriptor());
    tmpInfo.InfoVersion(
        theSourceInfo1
            .InfoVersion());  // pidetään infoversio samana, muuten menee interpoloinnit pieleen!
    destData = CreateEmptyData(tmpInfo);
    if (destData)
    {
      NFmiFastQueryInfo destInfo(destData);
      for (destInfo.ResetTime(); destInfo.NextTime();)
      {
        bool useInfo1 = true;
        bool useInfo2 = true;
        if (!theSourceInfo1.Time(destInfo.Time())) useInfo1 = false;
        if (!theSourceInfo2.Time(destInfo.Time())) useInfo2 = false;
        if (!(useInfo1 || useInfo2))  // jos kummastakaan ei löytynyt parametria ei tehdä mitään
          continue;                   // tähän ei oikeasti saisi mennä!!!!!!! assertin paikka?

        for (destInfo.ResetParam(), theSourceInfo1.ResetParam(), theSourceInfo2.ResetParam();
             destInfo.NextParam() && theSourceInfo1.NextParam() && theSourceInfo2.NextParam();)
        {
          for (destInfo.ResetLocation(),
               theSourceInfo1.ResetLocation(),
               theSourceInfo2.ResetLocation();
               destInfo.NextLocation() && theSourceInfo1.NextLocation() &&
               theSourceInfo2.NextLocation();)
          {
            for (destInfo.ResetLevel(), theSourceInfo1.ResetLevel(), theSourceInfo2.ResetLevel();
                 destInfo.NextLevel() && theSourceInfo1.NextLevel() && theSourceInfo2.NextLevel();)
            {
              float value = kFloatMissing;
              if (useInfo1) value = theSourceInfo1.FloatValue();

              if (value == kFloatMissing && useInfo2) value = theSourceInfo2.FloatValue();

              destInfo.FloatValue(value);
            }
          }
        }
      }
    }
  }
  return destData;
}

NFmiQueryDataUtil::SignificantSoundingLevels NFmiQueryDataUtil::GetSignificantSoundingLevelIndices(
    NFmiFastQueryInfo &theInfo)
{
  if (theInfo.Param(kFmiVerticalSoundingSignificance))
  {
    std::unique_ptr<SoundingLevelContainer> indexVector(new SoundingLevelContainer());
    for (theInfo.ResetLevel(); theInfo.NextLevel();)
    {
      auto value = theInfo.FloatValue();
      if (value != kFloatMissing && value > 0) indexVector->push_back(theInfo.LevelIndex());
    }
    if (indexVector->size()) return indexVector;
  }
  return SignificantSoundingLevels();
}

void NFmiQueryDataUtil::DestroyGridRecordData(GridDataVector &theGridRecordDatas)
{
  checkedVector<GridRecordData *>::iterator it = theGridRecordDatas.begin();
  checkedVector<GridRecordData *>::iterator endIter = theGridRecordDatas.end();
  for (; it != endIter; ++it)
    delete *it;
}

static void FillWithGribData(GridRecordData &theGridData, NFmiFastQueryInfo &theInfo)
{
  if (theInfo.Grid())
  {
    theInfo.First();  // HUOM! toimii siis vain 1. ajalle, paramille ja levelille
    theGridData.itsOrigTime = theInfo.OriginTime();
    theGridData.itsValidTime = theInfo.ValidTime();
    theGridData.itsParam = theInfo.Param();
    theGridData.itsLevel = *theInfo.Level();

    theGridData.itsGrid =
        MyGrid(theInfo.Area(), theInfo.Grid()->XNumber(), theInfo.Grid()->YNumber());
    theGridData.itsGridData.Resize(theInfo.Grid()->XNumber(), theInfo.Grid()->YNumber());
    theInfo.Values(theGridData.itsGridData);
  }
}

static const NFmiLevel &FindFirstLevel(float theLevelValue,
                                       NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  size_t gribCount = theGridRecordDatas.size();
  for (size_t i = 0; i < gribCount; i++)
    if (static_cast<unsigned int>(theLevelValue) == theGridRecordDatas[i]->itsLevel.LevelValue())
      return theGridRecordDatas[i]->itsLevel;
  throw runtime_error("Error in FindFirstLevel-function.");
}

static const NFmiDataIdent &FindFirstParam(int theParId,
                                           NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  size_t gribCount = theGridRecordDatas.size();
  for (size_t i = 0; i < gribCount; i++)
    if (static_cast<unsigned int>(theParId) == theGridRecordDatas[i]->itsParam.GetParamIdent())
      return theGridRecordDatas[i]->itsParam;
  throw runtime_error("Error in FindFirstParam-function.");
}

static NFmiVPlaceDescriptor GetVPlaceDesc(NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  // set:in avulla selvitetaan kuinka monta erilaista level valueta loytyy,
  // ei oteta huomioon, etta voi olla eri tyyppisia leveleita
  set<float> levelValues;
  size_t gribCount = theGridRecordDatas.size();
  for (size_t i = 0; i < gribCount; i++)
    levelValues.insert(theGridRecordDatas[i]->itsLevel.LevelValue());

  NFmiLevelBag levelBag;
  set<float>::iterator it = levelValues.begin();
  for (; it != levelValues.end(); ++it)
    levelBag.AddLevel(NFmiLevel(FindFirstLevel(*it, theGridRecordDatas)));

  return NFmiVPlaceDescriptor(levelBag);
}

static NFmiParamDescriptor GetParamDesc(NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  set<int> parIds;  // set:in avulla selvitetaan kuinka monta erilaista identtia loytyy
  size_t gribCount = theGridRecordDatas.size();
  for (size_t i = 0; i < gribCount; i++)
    parIds.insert(theGridRecordDatas[i]->itsParam.GetParamIdent());

  NFmiParamBag parBag;
  set<int>::iterator it = parIds.begin();
  for (; it != parIds.end(); ++it)
    parBag.Add(FindFirstParam(*it, theGridRecordDatas));

  return NFmiParamDescriptor(parBag);
}

static bool ConvertTimeList2TimeBag(NFmiTimeList &theTimeList, NFmiTimeBag &theTimeBag)
{                                       // tutkitaan onko mahdollista tehda listasta bagi
                                        // eli ajat ovat peräkkäisiä ja tasavälisiä
  if (theTimeList.NumberOfItems() > 1)  // ei  tehdä yhdestä ajasta bagiä vaikka se on mahdollista
  {
    theTimeList.First();
    theTimeList.Next();
    int resolution = theTimeList.CurrentResolution();
    for (; theTimeList.Next();)
    {
      if (resolution != theTimeList.CurrentResolution())
        return false;  // jos yhdenkin aikavälin resoluutio poikkeaa, ei voida tehdä bagia
    }
    theTimeBag = NFmiTimeBag(theTimeList.FirstTime(), theTimeList.LastTime(), resolution);
    return true;
  }
  return false;
}

static NFmiHPlaceDescriptor GetHPlaceDesc(NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  // Oletus, vain yhdenlainen gridi descriptori loytyy,
  // muuten lahtee poikkeus

  MyGrid &tmpGrid = theGridRecordDatas[0]->itsGrid;
  size_t gribCount = theGridRecordDatas.size();
  for (size_t i = 1; i < gribCount; i++)
    if (!(tmpGrid == theGridRecordDatas[i]->itsGrid))
      throw runtime_error("Kaikki grid-määritykset eivät olleet samanlaisia! Lopetetaan...");

  NFmiGrid grid(tmpGrid.itsArea, tmpGrid.itsNX, tmpGrid.itsNY);

  return NFmiHPlaceDescriptor(grid);
}

static NFmiTimeDescriptor GetTimeDesc(NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  // set:in avulla selvitetaan kuinka monta erilaista timea loytyy.
  set<NFmiMetTime> timesSet;
  size_t gribCount = theGridRecordDatas.size();
  for (size_t i = 0; i < gribCount; i++)
    timesSet.insert(theGridRecordDatas[i]->itsValidTime);

  // Tehdaan aluksi timelist, koska se on helpompi,
  // myohemmin voi miettia saisiko aikaan timebagin.
  NFmiTimeList timeList;
  set<NFmiMetTime>::iterator it = timesSet.begin();
  for (; it != timesSet.end(); ++it)
    timeList.Add(new NFmiMetTime(*it));

  NFmiTimeBag timeBag;
  bool fUseTimeBag = ConvertTimeList2TimeBag(timeList, timeBag);  // jos mahd.

  // Oletus kaikki origintimet ovat samoja, en tutki niita nyt yhtaan.
  if (fUseTimeBag)
    return NFmiTimeDescriptor(theGridRecordDatas[0]->itsOrigTime, timeBag);
  else
    return NFmiTimeDescriptor(theGridRecordDatas[0]->itsOrigTime, timeList);
}

static void FillQDataWithGribRecords(NFmiQueryData &theQData,
                                     NFmiQueryDataUtil::GridDataVector &theGridRecordDatas)
{
  NFmiFastQueryInfo info(&theQData);
  size_t gribCount = theGridRecordDatas.size();
  GridRecordData *tmp = 0;
  for (size_t i = 0; i < gribCount; i++)
  {
    tmp = theGridRecordDatas[i];
    if (info.Time(tmp->itsValidTime) && info.Level(tmp->itsLevel) && info.Param(tmp->itsParam))
    {
      if (!info.SetValues(tmp->itsGridData))
        throw runtime_error("qdatan täyttö gribi datalla epäonnistui, lopetetaan...");
    }
    else
      throw runtime_error(
          "qdatan täyttö ohjelma ei saanut asetettua aikaa/paikkaa/leveliä, lopetetaan...");
  }
}

static NFmiQueryData *CreateQueryData(NFmiQueryDataUtil::GridDataVector &theGridRecordDatas,
                                      int theMaxTimesInNewData)
{
  NFmiQueryData *qdata = 0;
  size_t gridCount = theGridRecordDatas.size();
  if (gridCount > 0)
  {
    NFmiParamDescriptor params(GetParamDesc(theGridRecordDatas));
    NFmiVPlaceDescriptor levels(GetVPlaceDesc(theGridRecordDatas));
    NFmiTimeDescriptor times(GetTimeDesc(theGridRecordDatas));
    if (times.Size() > static_cast<unsigned int>(theMaxTimesInNewData))
      times.PruneTimes(theMaxTimesInNewData);
    NFmiHPlaceDescriptor hplace(GetHPlaceDesc(theGridRecordDatas));
    NFmiQueryInfo innerInfo(params, times, hplace, levels);
    //		CheckInfoSize(innerInfo, theMaxQDataSizeInBytes);
    qdata = NFmiQueryDataUtil::CreateEmptyData(innerInfo);
    FillQDataWithGribRecords(*qdata, theGridRecordDatas);
  }
  return qdata;
}

// ----------------------------------------------------------------------
/*!
 * Ohjelma luo tiedosto nimilistan datoista yhden uuden querydatan
 * johon on otettu jokaisen annetun tiedoston datat.
 * Ainakin aluksi funktio on yksinkertaistettu siten, että jokaisesta
 * datasta otetaan mukaan vain 1. aika, parametri ja leveli. Koska
 * Sellaista tarkoitusta varten funktio on alunperin tehty.
 *
 * \param theFileNames Lista tiedostoja, jotka on tarkoitus yhdistää.
 * \param theMaxTimesInNewData Ei tehdä tämän suurempaa timebagia/listaa (vahinkojen varalle).
 * \return Palauttaa nyt luodun querydatan pointterin, jos jokin meni pieleen, 0-pointteri
 * palautetaan.
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::CombineTimes(checkedVector<std::string> &theFileNames,
                                               int theMaxTimesInNewData)
{
  NFmiQueryData *data = 0;
  NFmiStreamQueryData sData;
  GridDataVector gridRecordDatas;
  checkedVector<string>::iterator it = theFileNames.begin();
  checkedVector<string>::iterator endIt = theFileNames.end();
  for (; it != endIt; ++it)
  {
    if (sData.ReadData(*it))
    {
      GridRecordData *tmpData = new GridRecordData;
      FillWithGribData(*tmpData, *sData.QueryInfoIter());
      gridRecordDatas.push_back(
          tmpData);  // taman voisi optimoida, luomalla aluksi niin iso vektori kuin tarvitaan
    }
  }  // for-loop

  data = CreateQueryData(gridRecordDatas, theMaxTimesInNewData);
  DestroyGridRecordData(gridRecordDatas);
  return data;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 */
// ----------------------------------------------------------------------

void WeatherAndCloudinessFromHessaa(NFmiFastQueryInfo &theSourceInfo,
                                    NFmiFastQueryInfo &theDestInfo)
{
  double destInfoVersion = theDestInfo.InfoVersion();
  NFmiWeatherAndCloudiness weather(destInfoVersion);
  if (theSourceInfo.Param(kFmiWeatherSymbol3) && theDestInfo.Param(kFmiWeatherAndCloudiness))
  {
    for (theSourceInfo.ResetLocation(), theDestInfo.ResetLocation();
         theSourceInfo.NextLocation() && theDestInfo.NextLocation();)
    {
      for (theSourceInfo.ResetLevel(), theDestInfo.ResetLevel();
           theSourceInfo.NextLevel() && theDestInfo.NextLevel();)
      {
        float f1 = theSourceInfo.FloatValue();
        if (f1 != kFloatMissing)
          weather = NFmiWeatherAndCloudiness(
              static_cast<unsigned long>(f1), kFmiHessaaWeather, kFloatMissing, destInfoVersion);
        else
          weather = NFmiWeatherAndCloudiness(destInfoVersion);

        float temp = kFloatMissing;
        if (theSourceInfo.Param(kFmiTemperature)) temp = theSourceInfo.FloatValue();
        theSourceInfo.Param(kFmiWeatherSymbol3);
        weather.Temperature(temp);

        theDestInfo.FloatValue(weather.TransformedFloatValue());
      }
    }
  }
}

static void SetPrecipitationFormValue(
    NFmiFastQueryInfo &theSourceInfo,
    NFmiWeatherAndCloudiness &theWeather,
    const std::vector<unsigned long> &thePrecipFormParamIndexVector)
{
  float precipFormValue = kFloatMissing;
  for (size_t i = 0; i < thePrecipFormParamIndexVector.size(); i++)
  {
    theSourceInfo.ParamIndex(
        thePrecipFormParamIndexVector[i]);  // asetetaan hetkeksi precipForm parametri päälle
    if (theSourceInfo.FloatValue() != kFloatMissing)
    {
      precipFormValue = theSourceInfo.FloatValue();
      break;
    }
  }

  if (precipFormValue != kFloatMissing) theWeather.SubValue(precipFormValue, kFmiPrecipitationForm);
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 */
// ----------------------------------------------------------------------

void WeatherAndCloudinessFromManyParams(NFmiFastQueryInfo &theSourceInfo,
                                        NFmiFastQueryInfo &theDestInfo,
                                        bool fAllowLessParamsWhenCreatingWeather,
                                        bool fDoaccuratePrecip,
                                        const std::vector<int> &thePrecipFormParIds)
{
  double destInfoVersion = theDestInfo.InfoVersion();
  NFmiWeatherAndCloudiness weather(destInfoVersion);
  float clouds = kFloatMissing;
  float hSade = kFloatMissing;
  float fog = kFloatMissing;

  float temperature = kFloatMissing;
  theSourceInfo.Param(kFmiTemperature);
  unsigned long temperatureIndex = theSourceInfo.ParamIndex();

  float totalClouds = kFloatMissing;
  float precipitation = kFloatMissing;
  theSourceInfo.Param(kFmiPrecipitationRate);
  unsigned long precipitationIndex = theSourceInfo.ParamIndex();
  float precipitation1h = kFloatMissing;
  theSourceInfo.Param(kFmiPrecipitation1h);
  unsigned long precipitation1hIndex = theSourceInfo.ParamIndex();
  float precipitation3h = kFloatMissing;
  theSourceInfo.Param(kFmiPrecipitation3h);
  unsigned long precipitation3hIndex = theSourceInfo.ParamIndex();
  float precipitation6h = kFloatMissing;
  theSourceInfo.Param(kFmiPrecipitation6h);
  unsigned long precipitation6hIndex = theSourceInfo.ParamIndex();

  theSourceInfo.Param(kFmiLowCloudCover);
  unsigned long clParIndex = theSourceInfo.ParamIndex();
  theSourceInfo.Param(kFmiMediumCloudCover);
  unsigned long cmParIndex = theSourceInfo.ParamIndex();
  theSourceInfo.Param(kFmiHighCloudCover);
  unsigned long chParIndex = theSourceInfo.ParamIndex();

  theSourceInfo.Param(kFmiHumidity);
  unsigned long rhIndex = theSourceInfo.ParamIndex();

  theDestInfo.Param(kFmiWeatherAndCloudiness);
  unsigned long destWeatherAndcloudinessParIndex = theDestInfo.ParamIndex();

  theDestInfo.Param(kFmiPrecipitationRate);
  unsigned long destAccurePrecipParIndex = theDestInfo.ParamIndex();

  std::vector<unsigned long> precipFormParamIndexVector;
  for (size_t i = 0; i < thePrecipFormParIds.size(); i++)
  {
    theSourceInfo.Param(static_cast<FmiParameterName>(std::abs(thePrecipFormParIds[i])));
    precipFormParamIndexVector.push_back(theSourceInfo.ParamIndex());
  }

  // käydään läpi infon Paramit kFmiWeatherSymbol1, kFmiCloudSymbol, kFmiFogSymbol ja
  // kFmiTotalCloudCover
  if (theDestInfo.ParamIndex(destWeatherAndcloudinessParIndex))
  {
    for (theSourceInfo.ResetLocation(), theDestInfo.ResetLocation();
         theSourceInfo.NextLocation() && theDestInfo.NextLocation();)
    {
      for (theSourceInfo.ResetLevel(), theDestInfo.ResetLevel();
           theSourceInfo.NextLevel() && theDestInfo.NextLevel();)
      {
        theDestInfo.ParamIndex(destWeatherAndcloudinessParIndex);
        hSade = kFloatMissing;
        // float precipDividor = 1;
        if (theSourceInfo.Param(kFmiWeatherSymbol1))
        {
          hSade = theSourceInfo.FloatValue();
          if (hSade == kFloatMissing)
          {
            if (theSourceInfo.Param(kFmiWeatherSymbol2))
            {
              hSade = theSourceInfo.FloatValue();
            }
          }
        }

        precipitation = kFloatMissing;
        precipitation1h = kFloatMissing;
        precipitation3h = kFloatMissing;
        precipitation6h = kFloatMissing;
        if (theSourceInfo.ParamIndex(precipitationIndex))
        {
          precipitation = theSourceInfo.FloatValue();
        }
        if (theSourceInfo.ParamIndex(precipitation1hIndex))
        {
          precipitation1h = theSourceInfo.FloatValue();
        }
        if (theSourceInfo.ParamIndex(precipitation3hIndex))
        {
          float tmp = theSourceInfo.FloatValue();
          precipitation3h =
              (tmp == kFloatMissing)
                  ? kFloatMissing
                  : tmp / 3.f;  // nykyään halutaan tuntisateita siksi jaetaan tuntien määrällä
        }
        if (theSourceInfo.ParamIndex(precipitation6hIndex))
        {
          float tmp = theSourceInfo.FloatValue();
          precipitation6h =
              (tmp == kFloatMissing)
                  ? kFloatMissing
                  : tmp / 6.f;  // nykyään halutaan tuntisateita siksi jaetaan tuntien määrällä
        }

        if (precipitation1h != kFloatMissing)  // kFmiPrecipitation1h on ykkös prioriteetilla
          precipitation = precipitation1h;
        else if (precipitation == kFloatMissing)  // kFmiPrecipitationRate on kakkos prioriteetilla
        {
          if (precipitation3h != kFloatMissing)  // 3. priority 3h sateelle
          {
            precipitation = precipitation3h;
            // precipDividor = 3;
          }
          else if (precipitation6h != kFloatMissing)  // 4. priority 6h sateelle
          {
            precipitation = precipitation6h;
            // precipDividor = 6;
          }
        }

        if (theSourceInfo.Param(kFmiCloudSymbol)) clouds = theSourceInfo.FloatValue();
        if (theSourceInfo.Param(kFmiFogSymbol)) fog = theSourceInfo.FloatValue();
        if (theSourceInfo.Param(kFmiTotalCloudCover)) totalClouds = theSourceInfo.FloatValue();
        if (theSourceInfo.Param(kFmiTemperature)) temperature = theSourceInfo.FloatValue();

        bool doHSade1Fix = false;
        if (hSade == kFloatMissing && precipitation != kFloatMissing) doHSade1Fix = true;
        bool doHSade1ZeroOnRainFix = false;
        if (hSade == 0 && precipitation > 0) doHSade1ZeroOnRainFix = true;

        if (fAllowLessParamsWhenCreatingWeather || doHSade1Fix || doHSade1ZeroOnRainFix)
        {  // nyt pitää säveltää muutamat parametrit, jos on sallittu tehdä w&C parametri
           // vähemmistä
          // aineksista
          if (clouds == kFloatMissing && totalClouds != kFloatMissing)
          {
            const int defaultCloudType = 302;  // tämä on alapilveä pilvityyppiä 2 (kumpupilvi?)
            if (totalClouds <= 30)
              clouds = 1000 + defaultCloudType;
            else if (totalClouds <= 70)
              clouds = 2000 + defaultCloudType;
            else
              clouds = 3000 + defaultCloudType;
          }

          // lasketaan tarvittaessa sateenolomuoto tutkaryhmän käyttämästa T ja RH kaavasta
          if (hSade == kFloatMissing || doHSade1ZeroOnRainFix)
          {
            if (precipitation <= 0)
              hSade = 0;
            else if ((precipitation != kFloatMissing && precipitation > 0) || doHSade1ZeroOnRainFix)
            {
              hSade = 62;  // alustetaan sade vesisateeksi
              theSourceInfo.ParamIndex(temperatureIndex);
              temperature = theSourceInfo.FloatValue();
              theSourceInfo.ParamIndex(rhIndex);
              float rh = theSourceInfo.FloatValue();
              if (temperature != kFloatMissing && rh != kFloatMissing)
              {
                float xFactor = static_cast<float>(
                    1.0 - 1.0 / (1.0 + ::exp(22.0 - 2.7 * temperature - 0.2 * rh)));
                if (xFactor >= 0.8f)
                  hSade = 72;  // 72 = lumisadetta
                else if (xFactor >= 0.2f && xFactor < 0.8f)
                  hSade = 68;  // 68 = räntää
              }
            }
          }
          fog = 0;
        }
        if (hSade == kFloatMissing && clouds != kFloatMissing && totalClouds != kFloatMissing)
          hSade = 0;  // pika fixi, en korjaa weatherandcloudiness konstruktoria, vaan korjaan
                      // parametreja tässä
        weather = NFmiWeatherAndCloudiness(clouds,
                                           hSade,
                                           temperature,
                                           totalClouds,
                                           fog,
                                           precipitation,
                                           destInfoVersion);  //, true);

        // asetetaan vielä mahdolliset ala-, keski- ja yläpilvet
        if (clParIndex != gMissingIndex)
        {
          theSourceInfo.ParamIndex(clParIndex);
          weather.SubValue(theSourceInfo.FloatValue(), kFmiLowCloudCover);
        }
        if (cmParIndex != gMissingIndex)
        {
          theSourceInfo.ParamIndex(cmParIndex);
          weather.SubValue(theSourceInfo.FloatValue(), kFmiMediumCloudCover);
        }
        if (chParIndex != gMissingIndex)
        {
          theSourceInfo.ParamIndex(chParIndex);
          weather.SubValue(theSourceInfo.FloatValue(), kFmiHighCloudCover);
        }
        if (precipFormParamIndexVector
                .size())  // tässä on optimoitu mahd. sateen olomuoto arvojen hakua
          ::SetPrecipitationFormValue(theSourceInfo, weather, precipFormParamIndexVector);

        theDestInfo.FloatValue(weather.TransformedFloatValue());

        if (fDoaccuratePrecip && precipitation != kFloatMissing &&
            theDestInfo.ParamIndex(destAccurePrecipParIndex))
        {
          theDestInfo.FloatValue(precipitation);
        }
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 */
// ----------------------------------------------------------------------

void TotalWindFromWindVector(NFmiFastQueryInfo &theSourceInfo,
                             NFmiFastQueryInfo &theDestInfo,
                             FmiParameterName theWindGustParId)
{
  double destInfoVersion = theDestInfo.InfoVersion();
  bool windGustFound = theSourceInfo.Param(theWindGustParId);
  unsigned long gustParamIndex = theSourceInfo.ParamIndex();
  unsigned long windVectorParamIndex = 0;
  float windGustValue = kFloatMissing;

  NFmiTotalWind wind(destInfoVersion);
  if (theSourceInfo.Param(kFmiWindVectorMS) && theDestInfo.Param(kFmiTotalWindMS))
  {
    windVectorParamIndex = theSourceInfo.ParamIndex();
    for (theSourceInfo.ResetLocation(), theDestInfo.ResetLocation();
         theSourceInfo.NextLocation() && theDestInfo.NextLocation();)
    {
      for (theSourceInfo.ResetLevel(), theDestInfo.ResetLevel();
           theSourceInfo.NextLevel() && theDestInfo.NextLevel();)
      {
        float f1 = theSourceInfo.FloatValue();
        if (windGustFound)  // tässä on optimoitu mahd. puuska arvojen hakua
        {
          theSourceInfo.ParamIndex(gustParamIndex);  // asetetaan hetkeksi puuska parametri päälle
          windGustValue = theSourceInfo.FloatValue();
          theSourceInfo.ParamIndex(
              windVectorParamIndex);  // laitetaan sitten tuulivektori takaisin päälle
        }

        wind = NFmiTotalWind(f1, kFmiWindVector, destInfoVersion, windGustValue);
        theDestInfo.FloatValue(wind.TransformedFloatValue());
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 */
// ----------------------------------------------------------------------

void TotalWindFromWindSpeedAndDirection(NFmiFastQueryInfo &theSourceInfo,
                                        NFmiFastQueryInfo &theDestInfo,
                                        FmiParameterName theWindGustParId)
{
  double destInfoVersion = theDestInfo.InfoVersion();
  NFmiTotalWind wind(destInfoVersion);
  bool windGustFound = theSourceInfo.Param(theWindGustParId);
  unsigned long gustParamIndex = theSourceInfo.ParamIndex();
  theSourceInfo.Param(
      kFmiWindSpeedMS);  // oletus, että tämä löytyy, muuten tätä funktiota ei kutsuta
  unsigned long WSParamIndex = theSourceInfo.ParamIndex();
  theSourceInfo.Param(
      kFmiWindDirection);  // oletus, että tämä löytyy, muuten tätä funktiota ei kutsuta
  unsigned long WDParamIndex = theSourceInfo.ParamIndex();
  float windGustValue = kFloatMissing;

  if (theSourceInfo.Param(kFmiWindSpeedMS) && theDestInfo.Param(kFmiTotalWindMS))
  {
    for (theSourceInfo.ResetLocation(), theDestInfo.ResetLocation();
         theSourceInfo.NextLocation() && theDestInfo.NextLocation();)
    {
      for (theSourceInfo.ResetLevel(), theDestInfo.ResetLevel();
           theSourceInfo.NextLevel() && theDestInfo.NextLevel();)
      {
        theSourceInfo.ParamIndex(WSParamIndex);
        float f1 = theSourceInfo.FloatValue();
        theSourceInfo.ParamIndex(WDParamIndex);
        float f2 = theSourceInfo.FloatValue();

        if (windGustFound)  // tässä on optimoitu mahd. puuska arvojen hakua
        {
          theSourceInfo.ParamIndex(gustParamIndex);  // asetetaan hetkeksi puuska parametri päälle
          windGustValue = theSourceInfo.FloatValue();
        }

        wind = NFmiTotalWind(f2, f1, kFmiDirectionAndSpeed, destInfoVersion, windGustValue);
        theDestInfo.FloatValue(wind.TransformedFloatValue());
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theDestInfo Undocumented
 */
// ----------------------------------------------------------------------

void TotalWindFromUandVcomponents(NFmiFastQueryInfo &theSourceInfo,
                                  NFmiFastQueryInfo &theDestInfo,
                                  FmiParameterName theWindGustParId)
{
  double destInfoVersion = theDestInfo.InfoVersion();
  NFmiTotalWind wind(destInfoVersion);
  bool windGustFound = theSourceInfo.Param(theWindGustParId);
  unsigned long gustParamIndex = theSourceInfo.ParamIndex();
  theSourceInfo.Param(kFmiWindUMS);  // oletus, että tämä löytyy, muuten tätä funktiota ei kutsuta
  unsigned long uParamIndex = theSourceInfo.ParamIndex();
  theSourceInfo.Param(kFmiWindVMS);  // oletus, että tämä löytyy, muuten tätä funktiota ei kutsuta
  unsigned long vParamIndex = theSourceInfo.ParamIndex();
  float windGustValue = kFloatMissing;

  // U- and V-component directions depend on the grid projection. The azimuth
  // may change at each grid point separately, but is the same for both the
  // source and destination infos

  const NFmiArea *area = theSourceInfo.Area();

  if (theSourceInfo.Param(kFmiWindUMS) && theSourceInfo.Param(kFmiWindVMS) &&
      theDestInfo.Param(kFmiTotalWindMS))
  {
    for (theSourceInfo.ResetLocation(), theDestInfo.ResetLocation();
         theSourceInfo.NextLocation() && theDestInfo.NextLocation();)
    {
      for (theSourceInfo.ResetLevel(), theDestInfo.ResetLevel();
           theSourceInfo.NextLevel() && theDestInfo.NextLevel();)
      {
        theSourceInfo.ParamIndex(uParamIndex);
        float u = theSourceInfo.FloatValue();
        theSourceInfo.ParamIndex(vParamIndex);
        float v = theSourceInfo.FloatValue();
        if (windGustFound)  // tässä on optimoitu mahd. puuska arvojen hakua
        {
          theSourceInfo.ParamIndex(gustParamIndex);  // asetetaan hetkeksi puuska parametri päälle
          windGustValue = theSourceInfo.FloatValue();
        }

        // Rotate the components so that they are in normal latlon coordinates, since we needs to
        // construct
        // the speed and direction in them. Note that NFmiTotalWind::CalcU and CalcV are wrong in
        // not
        // taking the TrueNorthAzimuth as input

        if (u == kFloatMissing || v == kFloatMissing)
          wind = NFmiTotalWind(
              kFloatMissing, kFloatMissing, kFmiUVComponents, destInfoVersion, windGustValue);
        else
        {
          double azimuth = (!area ? 0.0 : area->TrueNorthAzimuth(theSourceInfo.LatLon()).ToRad());

          float uu = static_cast<float>(u * cos(azimuth) + v * sin(azimuth));
          float vv = static_cast<float>(v * cos(azimuth) - u * sin(azimuth));
          wind = NFmiTotalWind(uu, vv, kFmiUVComponents, destInfoVersion, windGustValue);
        }

        theDestInfo.FloatValue(wind.TransformedFloatValue());
      }
    }
  }
}

void FillCombineParamsOneTime(NFmiFastQueryInfo &theSourceInfo,
                              NFmiFastQueryInfo &theDestInfo,
                              const CombinedParamStruct &theCombinedParamStruct,
                              FmiParameterName theWindGustParId,
                              bool fAllowLessParamsWhenCreatingWeather,
                              bool fDoaccuratePrecip,
                              const std::vector<int> &thePrecipFormParIds)
{
  for (theDestInfo.ResetParam(); theDestInfo.NextParam();)
  {
    if (theSourceInfo.Param(*theDestInfo.Param().GetParam()))
    {
      for (theDestInfo.ResetLocation(), theSourceInfo.ResetLocation();
           theDestInfo.NextLocation() && theSourceInfo.NextLocation();)
        for (theDestInfo.ResetLevel(), theSourceInfo.ResetLevel();
             theDestInfo.NextLevel() && theSourceInfo.NextLevel();)
          theDestInfo.FloatValue(theSourceInfo.FloatValue());
    }
  }
  if (theCombinedParamStruct.weather1)
    WeatherAndCloudinessFromManyParams(theSourceInfo,
                                       theDestInfo,
                                       fAllowLessParamsWhenCreatingWeather,
                                       fDoaccuratePrecip,
                                       thePrecipFormParIds);
  else if (theCombinedParamStruct.weather2)
    WeatherAndCloudinessFromHessaa(theSourceInfo, theDestInfo);

  if (theCombinedParamStruct.wind1)
    TotalWindFromWindVector(theSourceInfo, theDestInfo, theWindGustParId);
  else if (theCombinedParamStruct.wind2)
    TotalWindFromWindSpeedAndDirection(theSourceInfo, theDestInfo, theWindGustParId);
  else if (theCombinedParamStruct.wind3)
    TotalWindFromUandVcomponents(theSourceInfo, theDestInfo, theWindGustParId);
}

void MakeCombineParamsOneThread(NFmiFastQueryInfo &theSourceInfo,
                                NFmiFastQueryInfo &theDestInfo,
                                const CombinedParamStruct &theCombinedParamStruct,
                                FmiParameterName theWindGustParId,
                                bool fAllowLessParamsWhenCreatingWeather,
                                NFmiTimeIndexCalculator &theTimeIndexCalculator,
                                bool fDoaccuratePrecip,
                                const std::vector<int> &thePrecipFormParIds)
{
  unsigned long workedTimeIndex = 0;
  for (; theTimeIndexCalculator.GetCurrentTimeIndex(workedTimeIndex);)
  {
    if (theSourceInfo.TimeIndex(workedTimeIndex) && theDestInfo.TimeIndex(workedTimeIndex))
    {
      FillCombineParamsOneTime(theSourceInfo,
                               theDestInfo,
                               theCombinedParamStruct,
                               theWindGustParId,
                               fAllowLessParamsWhenCreatingWeather,
                               fDoaccuratePrecip,
                               thePrecipFormParIds);
    }
  }
}

static bool WantedWeatherAndCloudinessParamsFound(NFmiFastQueryInfo &theInfo,
                                                  bool fAllowLessParamsWhenCreatingWeather)
{
  if (fAllowLessParamsWhenCreatingWeather)
  {
    if (theInfo.Param(kFmiTotalCloudCover) &&
        (theInfo.Param(kFmiPrecipitationRate) || theInfo.Param(kFmiPrecipitation3h) ||
         theInfo.Param(kFmiPrecipitation6h)))
      return true;
  }
  else
  {
    if (theInfo.Param(kFmiWeatherSymbol1) && theInfo.Param(kFmiTotalCloudCover) &&
        theInfo.Param(kFmiCloudSymbol) && theInfo.Param(kFmiFogSymbol))
      return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \param theWeather1 Undocumented
 * \param theWeather2 Undocumented
 * \param theWind1 Undocumented
 * \param theWind2 Undocumented
 * \param theWind3 Undocumented
 * \param fKeepCloudSymbolParameter Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiParamBag CheckAndMakeCombinedParamBag(NFmiFastQueryInfo &theSourceInfo,
                                          bool *theWeather1,
                                          bool *theWeather2,
                                          bool *theWind1,
                                          bool *theWind2,
                                          bool *theWind3,
                                          bool fKeepCloudSymbolParameter,
                                          bool fDoTotalWind,
                                          bool fDoWeatherAndCloudiness,
                                          FmiParameterName theWindGustParId,
                                          const std::vector<int> &thePrecipFormParIds,
                                          bool fAllowLessParamsWhenCreatingWeather,
                                          bool fDoaccuratePrecip)
{
  *theWeather1 = false;
  *theWeather2 = false;
  *theWind1 = false;
  *theWind2 = false;
  *theWind3 = false;
  NFmiParamBag bag(theSourceInfo.ParamBag());

  if (fDoTotalWind)
  {
    if (theSourceInfo.Param(kFmiWindVectorMS))
    {
      *theWind1 = true;

      // poista seuraavat parametrit uudesta parambagistä, koska niillä rakennetaan totalwind
      if (bag.SetCurrent(kFmiWindVectorMS)) bag.Remove();

      NFmiTotalWind tmp;
      NFmiDataIdent *saaParam = tmp.CreateParam(*theSourceInfo.Producer());
      bag.Add(*saaParam);
      delete saaParam;
    }
    else if (theSourceInfo.Param(kFmiWindDirection) && theSourceInfo.Param(kFmiWindSpeedMS))
    {
      *theWind2 = true;

      // poista seuraavat parametrit uudesta parambagistä, koska niillä rakennetaan totalwind
      if (bag.SetCurrent(kFmiWindDirection)) bag.Remove();
      if (bag.SetCurrent(kFmiWindSpeedMS)) bag.Remove();

      NFmiTotalWind tmp;
      NFmiDataIdent *saaParam = tmp.CreateParam(*theSourceInfo.Producer());
      bag.Add(*saaParam);
      delete saaParam;
    }
    else if (theSourceInfo.Param(kFmiWindUMS) && theSourceInfo.Param(kFmiWindVMS))
    {
      *theWind3 = true;

      // poista seuraavat parametrit uudesta parambagistä, koska niillä rakennetaan totalwind
      if (bag.SetCurrent(kFmiWindUMS)) bag.Remove();
      if (bag.SetCurrent(kFmiWindVMS)) bag.Remove();

      NFmiTotalWind tmp;
      NFmiDataIdent *saaParam = tmp.CreateParam(*theSourceInfo.Producer());
      bag.Add(*saaParam);
      delete saaParam;
    }

    // katsotaan vielä windGust tilanne
    if (theWindGustParId != kFmiBadParameter && (*theWind1 || *theWind2 || *theWind3))
    {
      if (theSourceInfo.Param(theWindGustParId))
      {
        // poista windgust parametri uudesta parambagistä, koska sillä rakennetaan totalwindia
        if (bag.SetCurrent(theWindGustParId)) bag.Remove();
      }
    }
  }
  if (fDoWeatherAndCloudiness)
  {
    if (WantedWeatherAndCloudinessParamsFound(theSourceInfo, fAllowLessParamsWhenCreatingWeather))
    {
      *theWeather1 = true;

      // poista seuraavat parametrit uudesta parambagistä, koska niillä rakennetaan
      // weatherandcloudiness
      if (bag.SetCurrent(kFmiWeatherSymbol1)) bag.Remove();
      if (bag.SetCurrent(kFmiTotalCloudCover)) bag.Remove();
      if ((!fKeepCloudSymbolParameter) && bag.SetCurrent(kFmiCloudSymbol)) bag.Remove();
      if (bag.SetCurrent(kFmiFogSymbol)) bag.Remove();

      // nämä poistetaan myös, koska niitä käytetään rakennettaessa w&c:tä
      if (bag.SetCurrent(kFmiWeatherSymbol2)) bag.Remove();
      if (bag.SetCurrent(kFmiPrecipitationRate)) bag.Remove();
      if (bag.SetCurrent(kFmiPrecipitation1h)) bag.Remove();
      if (bag.SetCurrent(kFmiPrecipitation3h)) bag.Remove();
      if (bag.SetCurrent(kFmiPrecipitation6h)) bag.Remove();
      // nämä poistetaan myös, koska niitä käytetään rakennettaessa w&c:tä
      if (bag.SetCurrent(kFmiLowCloudCover)) bag.Remove();
      if (bag.SetCurrent(kFmiMediumCloudCover)) bag.Remove();
      if (bag.SetCurrent(kFmiHighCloudCover)) bag.Remove();

      NFmiWeatherAndCloudiness tmp;  // tässä ei ole infoversiosta väliä!
      NFmiDataIdent *saaParam = tmp.CreateParam(*theSourceInfo.Producer());
      bag.Add(*saaParam);
      delete saaParam;
      if (fDoaccuratePrecip)
      {
        NFmiDataIdent accuratePrecip(
            NFmiParam(
                kFmiPrecipitationRate, "exact rr1h", 0, kFloatMissing, 1, 0, "%.2f", kLinearly),
            *theSourceInfo.Producer());
        bag.Add(accuratePrecip);
      }
    }
    else if (theSourceInfo.Param(kFmiWeatherSymbol3))
    {
      *theWeather2 = true;

      // poista seuraavat parametrit uudesta parambagistä, koska niillä rakennetaan
      // weatherandcloudiness
      if (bag.SetCurrent(kFmiWeatherSymbol3)) bag.Remove();

      NFmiWeatherAndCloudiness tmp;  // tässä ei ole infoversiosta väliä!
      NFmiDataIdent *saaParam = tmp.CreateParam(*theSourceInfo.Producer());
      bag.Add(*saaParam);
      delete saaParam;
    }

    // katsotaan vielä precipitation form parametrin tilanne, onko niitä vektorissa ja
    // poistetaanko
    // niitä (positiiviset arvot) vai ei (negatiiviset arvot)
    if (thePrecipFormParIds.size() && (*theWeather1 || *theWeather2))
    {
      for (size_t i = 0; i < thePrecipFormParIds.size(); i++)
      {
        if (thePrecipFormParIds[i] > 0)
        {
          FmiParameterName precipFormParId = static_cast<FmiParameterName>(thePrecipFormParIds[i]);
          if (theSourceInfo.Param(precipFormParId))
          {
            // poista precipForm parametri uudesta parambagistä, koska sillä rakennetaan Weatheria
            if (bag.SetCurrent(precipFormParId)) bag.Remove();
          }
        }
      }
    }
  }

  return bag;
}

// ----------------------------------------------------------------------
/*!
 * Funktio saa parametrina queryinfon ja luo uuden qdatan, jossa on
 * mahdollisesti luotu uudet combinedparamas parametrit mukana (totalwind
 * + weatherAndCloudiness). Nämä on mukana sillä ehdolla, että niihin löytyy
 * rakennuspalikat (esim. totalwind:iä varten pitää löytyä joko tuulen suunta
 * ja nopeus tai windvector (tai u ja v komponentit?)). Jos rakennus palikat
 * löytyvät, filtteri tekee uuden datan niin, että se lisää parametri listaan
 * combineparamin ja poistaa listasta combineparamiin tarvittavat
 * rakennuspalikat (ettei toisteta dataa).
 *
 * Poikkeus, halutessa kFmiCloudSymbol voidaan pitää parametrilistassa.
 * Uudessa qdatassa on muut systeemit otettu alkuperäisesta datasta
 * (hila/asemat, time ja level).
 *
 * infoVersion 6 on pienin tunnettu versio numero, mikä sallitaan.
 *
 * \param theSourceInfo Undocumented
 * \param theWantedInfoVersion Undocumented
 * \param fKeepCloudSymbolParameter Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::MakeCombineParams(NFmiFastQueryInfo &theSourceInfo,
                                                    double theWantedInfoVersion,
                                                    bool fKeepCloudSymbolParameter,
                                                    bool fDoTotalWind,
                                                    bool fDoWeatherAndCloudiness,
                                                    FmiParameterName theWindGustParId,
                                                    bool fAllowLessParamsWhenCreatingWeather,
                                                    int theMaxUsedThreadCount,
                                                    bool fDoaccuratePrecip,
                                                    bool fForceTimeBag)
{
  std::vector<int> thePrecipFormParIds;
  return MakeCombineParams(theSourceInfo,
                           theWantedInfoVersion,
                           fKeepCloudSymbolParameter,
                           fDoTotalWind,
                           fDoWeatherAndCloudiness,
                           theWindGustParId,
                           thePrecipFormParIds,
                           fAllowLessParamsWhenCreatingWeather,
                           theMaxUsedThreadCount,
                           fDoaccuratePrecip,
                           fForceTimeBag);
}

NFmiQueryData *NFmiQueryDataUtil::MakeCombineParams(NFmiFastQueryInfo &theSourceInfo,
                                                    double theWantedInfoVersion,
                                                    bool fKeepCloudSymbolParameter,
                                                    bool fDoTotalWind,
                                                    bool fDoWeatherAndCloudiness,
                                                    FmiParameterName theWindGustParId,
                                                    const std::vector<int> &thePrecipFormParIds,
                                                    bool fAllowLessParamsWhenCreatingWeather,
                                                    int theMaxUsedThreadCount,
                                                    bool fDoaccuratePrecip,
                                                    bool fForceTimeBag)
{
  if (theWantedInfoVersion < 6.)
    theWantedInfoVersion =
        6.;  // 6 on pienin tunnustettu versio numero (muuten tulos data on turha kirjoittaa!)

  NFmiQueryData *destData = 0;
  CombinedParamStruct combinedParamStruct;
  NFmiParamBag newParBag(CheckAndMakeCombinedParamBag(theSourceInfo,
                                                      &combinedParamStruct.weather1,
                                                      &combinedParamStruct.weather2,
                                                      &combinedParamStruct.wind1,
                                                      &combinedParamStruct.wind2,
                                                      &combinedParamStruct.wind3,
                                                      fKeepCloudSymbolParameter,
                                                      fDoTotalWind,
                                                      fDoWeatherAndCloudiness,
                                                      theWindGustParId,
                                                      thePrecipFormParIds,
                                                      fAllowLessParamsWhenCreatingWeather,
                                                      fDoaccuratePrecip));
  NFmiParamDescriptor newParDesc(newParBag);

  // Force timebag if so requested
  NFmiTimeDescriptor tdesc = theSourceInfo.TimeDescriptor();
  if (fForceTimeBag && tdesc.ValidTimeList() != NULL)
  {
    NFmiTimeBag tbag;
    if (!ConvertTimeList2TimeBag(*tdesc.ValidTimeList(), tbag))
      throw std::runtime_error("Unable to force a time bag version of the given querydata");
    tdesc = NFmiTimeDescriptor(tdesc.OriginTime(), tbag);
  }

  NFmiQueryInfo tmpInfo(
      newParDesc, tdesc, theSourceInfo.HPlaceDescriptor(), theSourceInfo.VPlaceDescriptor());
  tmpInfo.InfoVersion(theWantedInfoVersion);  // asetetaan haluttu infoversion
  destData = CreateEmptyData(tmpInfo);
  if (destData)
  {
    NFmiFastQueryInfo destInfo(destData);

    unsigned int usedThreadCount = boost::thread::hardware_concurrency();
    if (theMaxUsedThreadCount > 0)
      usedThreadCount = std::min(static_cast<unsigned int>(theMaxUsedThreadCount), usedThreadCount);
    if (usedThreadCount > destInfo.SizeTimes()) usedThreadCount = destInfo.SizeTimes();

    NFmiPoint dummyPoint =
        destInfo.LatLon();  // Varmistetaan että NFmiQueryDatan itsLatLonCache on alustettu!!
    dummyPoint =
        theSourceInfo.LatLon();  // Varmistetaan että NFmiQueryDatan itsLatLonCache on alustettu!!

    if (usedThreadCount <= 1)
    {
      for (theSourceInfo.ResetTime(), destInfo.ResetTime();
           theSourceInfo.NextTime() && destInfo.NextTime();)
      {
        FillCombineParamsOneTime(theSourceInfo,
                                 destInfo,
                                 combinedParamStruct,
                                 theWindGustParId,
                                 fAllowLessParamsWhenCreatingWeather,
                                 fDoaccuratePrecip,
                                 thePrecipFormParIds);
      }
    }
    else
    {
      // pakko luoda dynaamisesti eri threadeille tarvittavat kopiot source ja target datoista
      std::vector<boost::shared_ptr<NFmiFastQueryInfo> > targetInfos(usedThreadCount);
      std::vector<boost::shared_ptr<NFmiFastQueryInfo> > sourceInfos(usedThreadCount);
      for (unsigned int i = 0; i < usedThreadCount; i++)
      {
        targetInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(destInfo));
        sourceInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(theSourceInfo));
      }

      unsigned long usedStartTimeIndex = 0;
      unsigned long usedEndTimeIndex = destInfo.SizeTimes() - 1;

      NFmiTimeIndexCalculator timeIndexCalculator(usedStartTimeIndex, usedEndTimeIndex);
      boost::thread_group calcParts;
      for (unsigned int i = 0; i < usedThreadCount; i++)
        calcParts.add_thread(new boost::thread(MakeCombineParamsOneThread,
                                               *(sourceInfos[i].get()),
                                               *(targetInfos[i].get()),
                                               combinedParamStruct,
                                               theWindGustParId,
                                               fAllowLessParamsWhenCreatingWeather,
                                               boost::ref(timeIndexCalculator),
                                               fDoaccuratePrecip,
                                               thePrecipFormParIds));
      calcParts.join_all();  // odotetaan että threadit lopettavat
    }
  }
  return destData;
}

// ********************************************************************************
// ***************  NowcastFilter osuus on nyt täällä!  ***************************
// ********************************************************************************

// ----------------------------------------------------------------------
/*!
 * Luetaan fqd-tiedostot data1 ja data2 ja liitetään ne yhteen ja
 * talletetaan output-tiedostoon. Käytetään NowCastFilter:issä.
 *
 * Tietokannassa oleva viralliene Kepa-data on pohja. Siihen halutaan liimata
 * päälle viimeisin aluetoimistosta tuleva ShortRange-data. Virallisen datan
 * uusi alkuaika tulee olemaan sama kuin ShortRange-datassa. Aluetoimistossa
 * oletetaan olevan minimissään tietty määrä dataa, mikä liimataan uuden datan
 * alkuun. 12 tuntia on minimi tällä hetkellä. Aluetoimiston metku voi kuitenkin
 * lähettäessään sanoa että dataa tulee enemmänkin aina 48 tuntiin asti. Jos on
 * ilmoitettu enemmän kuin  12 tuntia, otetaan sen mukaan enemmän dataa
 * ShortRange-datasta.
 *
 * \param theData1FileName Undocumented
 * \param theData2FileName Undocumented
 * \param theOutputFileName Undocumented
 * \param theTimeRangeForPureData Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiQueryDataUtil::DoNowCastFiltering(const NFmiString &theData1FileName,
                                           const NFmiString &theData2FileName,
                                           const NFmiString &theOutputFileName,
                                           int theTimeRangeForPureData)
{
  NFmiStreamQueryData data1;  // kepa-data
  NFmiStreamQueryData data2;  // shortrange-data

  if (!data1.ReadData(theData1FileName)) return false;

  if (!data2.ReadData(theData2FileName)) return false;

  if (data1.QueryInfoIter() && data2.QueryInfoIter())
  {
    NFmiFastQueryInfo *info1 = data1.QueryInfoIter();  // kepa
    //	  double kepaDataVersionNumber = info1->InfoVersion();
    NFmiFastQueryInfo *info2 = data2.QueryInfoIter();  // shortrange
    //	  double shortRangeDataVersionNumber = info2->InfoVersion();
    /*
              if(kepaDataVersionNumber != shortRangeDataVersionNumber)
              {
                      if(kepaDataVersionNumber == 6. && shortRangeDataVersionNumber == 7.)
                      {
                      }
                      else if(kepaDataVersionNumber == 7. && shortRangeDataVersionNumber == 6.)
                      {
                      }
                      else
                      {
                              cerr << "Yhdistäminen ei onnistu, koska datoissa on oudot versiot:
       kepa = " << kepaDataVersionNumber << " ja shortRange = " << shortRangeDataVersionNumber <<
       endl;
                              cerr << "Lopetetaan ohjelma." << endl;
                              return false
                      }
              }
    */
    NFmiQueryData *newQueryData =
        NFmiQueryDataUtil::DoNowCastFiltering(*info1, *info2, theTimeRangeForPureData);

    // Tulos tiedostoon
    if (newQueryData)
    {
      NFmiString realOutFileName(theOutputFileName);  // otetaan toistaiseksi käyttöön haluttu
                                                      // outputFileName koska jakelija ei osaa
      // hoitaa muuttunutta tiedoston nimeä vielä
      ofstream out(realOutFileName, ios::binary);
      if (out)
      {
        out << *newQueryData;  // 31.5.2001/Marko
        out.close();
      }
      delete newQueryData;
      return true;
    }
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 *  Kopioi tavaran data/info1:stä data/info2:een.
 *
 *  Oletuksia: Levelit ja locationit samoja. Paramit voivat olla erilaisia.
 *  Ajat luultavasti erilaisia, sitä varten funktio on tehty, että lyhyt
 *  shortrange data heitetään pitkän kepadatan päälle.  Ajoissa aikaresoluution
 *  pitää olla sama ja ajat alkavat samasta hetkestä että niitä voidaan
 *  juoksuttaa haluttuun pisteeseen asti (theWantedHoursFromStart).
 *
 * \param theInfo1 Undocumented
 * \param theInfo2 Undocumented
 * \param theWantedHoursFromStart Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool CopyData1OverData2(NFmiFastQueryInfo &theInfo1,
                        NFmiFastQueryInfo &theInfo2,
                        int theWantedHoursFromStart)
{
  int wantedTimeSteps = int(theWantedHoursFromStart / (theInfo1.ValidTimes().Resolution() / 60.));
  float tmpValue = kFloatMissing;
  for (theInfo2.ResetParam(); theInfo2.NextParam();)
  {
    if (!theInfo1.Param(*theInfo2.Param().GetParam()))
      continue;  // jos parametria ei löydy, jatketaan seuraavalla
    int timeCounter = 0;
    for (theInfo2.ResetTime(), theInfo1.ResetTime(); theInfo2.NextTime() && theInfo1.NextTime();
         timeCounter++)
    {
      if (timeCounter >= wantedTimeSteps)  // esim. jos halutaan 15 tuntia alusta, ehto ottaa mukaan
                                           // ajat 0-15 eli periaatteessa 16 tunnin datan, mutta se
                                           // on sama kuin 15 tuntia alusta eteenpäin.
        continue;                          // kopioidaan dataa vain haluttu aikajakso alusta
      for (theInfo2.ResetLocation(), theInfo1.ResetLocation();
           theInfo2.NextLocation() && theInfo1.NextLocation();)
      {
        for (theInfo2.ResetLevel(), theInfo1.ResetLevel();
             theInfo2.NextLevel() && theInfo1.NextLevel();)
        {
          tmpValue = theInfo1.FloatValue();
          if (!(tmpValue == kFloatMissing ||
                tmpValue == kTCombinedWeatherFloatMissing))  // edellinen ehto oli aina true!
            theInfo2.FloatValue(tmpValue);  // asetetaan data vain jos ei puuttuvaa!!
        }
      }
    }
  }
  return true;
}

// ----------------------------------------------------------------------
/*!
 * \param theInfo1 Undocumented
 * \param theInfo2 Undocumented
 * \param theTimeRangeForPureData Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::DoNowCastFiltering(NFmiFastQueryInfo &theInfo1,
                                                     NFmiFastQueryInfo &theInfo2,
                                                     int theTimeRangeForPureData)
{
  theInfo1.First();
  theInfo2.First();

  // 31.5.2001/Marko Uusi versio tarkistaa datasta, löytyykö avainta SHORTRANGE_DATAHOURS2DB.
  // Tällöin aluetoimisto on tehnyt editointia halutun määrän ja sen verran täytetään
  // tietokantaan dataa.

  NFmiString key("SHORTRANGE_DATAHOURS2DB");
  if (theInfo2.FindFirstKey(key))
  {
    NFmiValueString keyValueStr(theInfo2.GetCurrentKeyValue());
    if (keyValueStr.IsNumeric())
    {
      theTimeRangeForPureData =
          long(keyValueStr) + 1;  // todellisuudessa pituuteen lisätään yksi tunti

      int maxHours =
          theInfo2.ValidTimes().LastTime().DifferenceInHours(theInfo2.ValidTimes().FirstTime());
      if (theTimeRangeForPureData > maxHours) theTimeRangeForPureData = maxHours;
    }
  }

  // Penan lisäykset
  NFmiTimePerioid period(theInfo1.TimeResolution());
  NFmiMetTime originTime = theInfo2.OriginTime();
  NFmiMetTime firstTime = theInfo2.ValidTime();
  NFmiMetTime lastTime = theInfo1.ValidTimes().LastTime();
  NFmiTimeBag timeBag(firstTime, lastTime, period);
  FmiTimeLocalzation localization = kUTC;
  bool interpolation = false;

  NFmiTimeDescriptor timeDescriptor(originTime, timeBag, localization, interpolation);

  NFmiQueryData *newQueryData = NFmiQueryDataUtil::ExtractTimes(
      theInfo1.RefQueryData(),
      timeDescriptor);  // tämä on nopeampi ja toimii varmemmin kuin CreateSubQueryData

  NFmiFastQueryInfo newFastInfo(newQueryData);
  newFastInfo.First();

  CopyData1OverData2(theInfo2, newFastInfo, theTimeRangeForPureData);

  NFmiProducerIdLister shortProdIds(theInfo2);
  NFmiProducerIdLister longProdIds(theInfo1);
  NFmiProducerIdLister newProdIds;
  //  newProdIds.GenerateNowCastCombination(shortProdIds, theTimeRangeForPureData, longProdIds);
  if (newProdIds.MakeProducerIdString() != string(""))
    newFastInfo.AddKey(NFmiQueryDataUtil::GetOfficialQueryDataProdIdsKey(),
                       newProdIds.MakeProducerIdString(),
                       true);

  return newQueryData;
}

// ----------------------------------------------------------------------
/*!
 * Oletus qinfoista: ovat kopiot samasta datasta, eli samat paramit,
 * levelit, ajat ja paikat.
 *
 * \param theSourceInfoV6 Undocumented
 * \param theDestInfoV7 Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool FqdV6ToV7WeatherAndCloudinessModification(NFmiFastQueryInfo &theSourceInfoV6,
                                               NFmiFastQueryInfo &theDestInfoV7)
{
  if (theDestInfoV7.Param(
          kFmiWeatherAndCloudiness))  // tarkistetaan vielä onko mitään konvertoitavaa
  {
    for (theSourceInfoV6.ResetLocation(), theDestInfoV7.ResetLocation();
         theSourceInfoV6.NextLocation() && theDestInfoV7.NextLocation();)
    {
      for (theSourceInfoV6.ResetLevel(), theDestInfoV7.ResetLevel();
           theSourceInfoV6.NextLevel() && theDestInfoV7.NextLevel();)
      {
        for (theSourceInfoV6.ResetTime(), theDestInfoV7.ResetTime();
             theSourceInfoV6.NextTime() && theDestInfoV7.NextTime();)
        {
          // jouduin laittamaan tähän totalclouds quickfixin, koska muuten
          // kun asetellaan yläpilviä, totalcloudiness lasketaan ja se voi
          // muuttaa haluttua dataa (muutos pitää tehdä, koska metkut eivät
          // editoi muuta kuin kok. pilvisyyttä)
          theSourceInfoV6.Param(kFmiTotalCloudCover);
          float totalClouds = theSourceInfoV6.FloatValue();

          // asetetaan ensin v6:sta ylä pilvet v7:aan
          theSourceInfoV6.Param(kFmiHighCloudCover);
          float highClouds = theSourceInfoV6.FloatValue();
          theDestInfoV7.Param(kFmiHighCloudCover);
          theDestInfoV7.FloatValue(highClouds);

          // sitten asetetaan sademäärä v6:sta v7:ään
          theSourceInfoV6.Param(kFmiPrecipitation1h);
          float precipitation = theSourceInfoV6.FloatValue();
          theDestInfoV7.Param(kFmiPrecipitation1h);
          theDestInfoV7.FloatValue(precipitation);

          // lopuksi asetetaan totalcloudiness siihen mihin se alunperin oli
          theDestInfoV7.Param(kFmiTotalCloudCover);
          theDestInfoV7.FloatValue(totalClouds);
        }
      }
    }

    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfoV7 Undocumented
 * \param theDestInfoV6 Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool FqdV7ToV6WeatherAndCloudinessModification(NFmiFastQueryInfo &theSourceInfoV7,
                                               NFmiFastQueryInfo &theDestInfoV6)
{
  if (theDestInfoV6.Param(
          kFmiWeatherAndCloudiness))  // tarkistetaan vielä onko mitään konvertoitavaa
  {
    for (theSourceInfoV7.ResetLocation(), theDestInfoV6.ResetLocation();
         theSourceInfoV7.NextLocation() && theDestInfoV6.NextLocation();)
    {
      for (theSourceInfoV7.ResetLevel(), theDestInfoV6.ResetLevel();
           theSourceInfoV7.NextLevel() && theDestInfoV6.NextLevel();)
      {
        for (theSourceInfoV7.ResetTime(), theDestInfoV6.ResetTime();
             theSourceInfoV7.NextTime() && theDestInfoV6.NextTime();)
        {
          // jouduin laittamaan tähän totalclouds quickfixin, koska muuten
          // kun asetellaan yläpilviä, totalcloudiness lasketaan ja se voi
          // muuttaa haluttua dataa (muutos pitää tehdä, koska metkut eivät
          // editoi muuta kuin kok. pilvisyyttä)
          theSourceInfoV7.Param(kFmiTotalCloudCover);
          float totalClouds = theSourceInfoV7.FloatValue();

          // asetetaan ensin v7:sta ylä pilvet v6:aan
          theSourceInfoV7.Param(kFmiHighCloudCover);
          float highClouds = theSourceInfoV7.FloatValue();
          theDestInfoV6.Param(kFmiHighCloudCover);
          theDestInfoV6.FloatValue(highClouds);

          // sitten asetetaan sademäärä v7:sta v6:een
          theSourceInfoV7.Param(kFmiPrecipitation1h);
          float precipitation = theSourceInfoV7.FloatValue();
          theDestInfoV6.Param(kFmiPrecipitation1h);
          theDestInfoV6.FloatValue(precipitation);

          // lopuksi asetetaan totalcloudiness siihen mihin se alunperin oli
          theDestInfoV6.Param(kFmiTotalCloudCover);
          theDestInfoV6.FloatValue(totalClouds);
        }
      }
    }

    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfoV6 Undocumented
 * \param theDestInfoV7 Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool FqdV6ToV7TotalWindModification(NFmiFastQueryInfo &theSourceInfoV6,
                                    NFmiFastQueryInfo &theDestInfoV7)
{
  if (theDestInfoV7.Param(kFmiTotalWindMS))  // tarkistetaan vielä onko mitään konvertoitavaa
  {
    for (theSourceInfoV6.ResetLocation(), theDestInfoV7.ResetLocation();
         theSourceInfoV6.NextLocation() && theDestInfoV7.NextLocation();)
    {
      for (theSourceInfoV6.ResetLevel(), theDestInfoV7.ResetLevel();
           theSourceInfoV6.NextLevel() && theDestInfoV7.NextLevel();)
      {
        for (theSourceInfoV6.ResetTime(), theDestInfoV7.ResetTime();
             theSourceInfoV6.NextTime() && theDestInfoV7.NextTime();)
        {
          // asetetaan WS V6:sta V7 dataan
          theSourceInfoV6.Param(kFmiWindSpeedMS);
          float spd = theSourceInfoV6.FloatValue();
          theDestInfoV7.Param(kFmiWindSpeedMS);
          theDestInfoV7.FloatValue(spd);

          // sitten asetetaan gustIndex V6:sta V7:aan
          theSourceInfoV6.Param(kFmiWindGust);
          float gustIndex = theSourceInfoV6.FloatValue();
          theDestInfoV7.Param(kFmiWindGust);
          theDestInfoV7.FloatValue(gustIndex);
        }
      }
    }

    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfoV7 Undocumented
 * \param theDestInfoV6 Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool FqdV7ToV6TotalWindModification(NFmiFastQueryInfo &theSourceInfoV7,
                                    NFmiFastQueryInfo &theDestInfoV6)
{
  if (theDestInfoV6.Param(kFmiTotalWindMS))  // tarkistetaan vielä onko mitään konvertoitavaa
  {
    for (theSourceInfoV7.ResetLevel(), theDestInfoV6.ResetLevel();
         theSourceInfoV7.NextLevel() && theDestInfoV6.NextLevel();)
    {
      for (theSourceInfoV7.ResetLocation(), theDestInfoV6.ResetLocation();
           theSourceInfoV7.NextLocation() && theDestInfoV6.NextLocation();)
      {
        for (theSourceInfoV7.ResetTime(), theDestInfoV6.ResetTime();
             theSourceInfoV7.NextTime() && theDestInfoV6.NextTime();)
        {
          // asetetaan ensin WS V7 datasta V6:een
          theSourceInfoV7.Param(kFmiWindSpeedMS);
          float spd = theSourceInfoV7.FloatValue();
          theDestInfoV6.Param(kFmiWindSpeedMS);
          theDestInfoV6.FloatValue(spd);

          // sitten asetetaan gustIndex V7:sta V6:een
          theSourceInfoV7.Param(kFmiWindGust);
          float gustIndex = theSourceInfoV7.FloatValue();
          theDestInfoV6.Param(kFmiWindGust);
          theDestInfoV6.FloatValue(gustIndex);
        }
      }
    }

    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * Suorittavat datan konversiot 6:sta 7:aan tai päin vastoin
 *
 * \param theSourceInfo Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::FqdV6ToV7(NFmiFastQueryInfo &theSourceInfo)
{
  NFmiQueryData *data = 0;
  if (theSourceInfo.RefQueryData())
  {
    data = new NFmiQueryData(*theSourceInfo.RefQueryData());  // tehdään ensin kopio datasta
    data->InfoVersion(7);  // vaihdetaan vielä destination datan versio 7:ksi
    NFmiFastQueryInfo destInfo(data);
    FqdV6ToV7WeatherAndCloudinessModification(
        theSourceInfo, destInfo);  // tehdään tarpeelliset weatherandCloudiness-muutokset
    FqdV6ToV7TotalWindModification(theSourceInfo,
                                   destInfo);  // tehdään tarpeelliset totalwind-muutokset
  }
  return data;
}

// ----------------------------------------------------------------------
/*!
 * \param theSourceInfo Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::FqdV7ToV6(NFmiFastQueryInfo &theSourceInfo)
{
  NFmiQueryData *data = 0;
  if (theSourceInfo.RefQueryData())
  {
    data = new NFmiQueryData(*theSourceInfo.RefQueryData());  // tehdään ensin kopio datasta
    data->InfoVersion(6);  // vaihdetaan vielä destination datan versio 6:ksi
    NFmiFastQueryInfo destInfo(data);
    FqdV7ToV6WeatherAndCloudinessModification(
        theSourceInfo, destInfo);  // tehdään tarpeelliset weatherandCloudiness-muutokset
    FqdV7ToV6TotalWindModification(theSourceInfo,
                                   destInfo);  // tehdään tarpeelliset totalwind-muutokset
  }
  return data;
}

// ----------------------------------------------------------------------
/*!
 * Tämä funktio luo uuden datan jossa sama rakenne kuin annetussa datassa
 * (params, times, levels, jne.). Filtteri laskee sourceDatasta tulosDataan
 * arvot käyttäen haluttua NFmiSmoother-luokan tasoitus filtteriä. Esim.
 * laskee +-3 tunnin tasoituksen kaikkialle.
 * Offsetit pyöristetään lähimpiin sourceDatan aika-askeleisiin.
 *
 * \param theSourceData Undocumented
 * \param theTimeOffsetStartInMinutes Undocumented
 * \param theTimeOffsetEndInMinutes Undocumented
 * \param theSmootherMethod ks. NFmiSmoother-konstruktori.
 * \param theFactor kertoo tasoituksen voimakkuuden 1 on voimakas ja
 *  suuremmat luvut heikentävät tasoitusta.
 * \return Metodissa luotu NFmiQueryData olio, jota on tasoitettu.
 */
// ----------------------------------------------------------------------

NFmiQueryData *DoTimeFilteringWithSmoother(NFmiQueryData *theSourceData,
                                           int theTimeOffsetStartInMinutes,
                                           int theTimeOffsetEndInMinutes,
                                           int theSmootherMethod,
                                           double theFactor)
{
  NFmiQueryData *data = 0;
  if (theSourceData)
  {
    data = theSourceData->Clone();
    NFmiFastQueryInfo destInfo(data);
    NFmiFastQueryInfo sourceInfo(theSourceData);
    int stepNumber =
        static_cast<int>(round((theTimeOffsetEndInMinutes - theTimeOffsetStartInMinutes) /
                               sourceInfo.TimeResolution())) +
        1;

    // HUOM! ei osaa hoitaa hommaa oikein jos datassa on aikalista jossa erilaiset aikavälit
    int sizeTimes = sourceInfo.SizeTimes();
    checkedVector<float> relTimeLocation(
        sizeTimes);  // x on aika eli eri aika-askelten suhteelliset sijainnit (0, 1, 2, 3, ...)
                     //		std::accumulate(relTimeLocation.begin(), relTimeLocation.end(), 0); // 0
    // tarkoittaa
    // että
    // täytetään vektori luvuilla jotka alkavat 0:sta ja kasvavat aina yhdellä
    for (int j = 0; j < sizeTimes; j++)
      relTimeLocation[j] = static_cast<float>(j);  // tarkoittaa että täytetään vektori luvuilla
    // jotka alkavat 0:sta ja kasvavat aina yhdellä

    checkedVector<float> values(sizeTimes);
    checkedVector<float> result;
    NFmiSmoother smoother(static_cast<NFmiSmoother::NFmiSmootherMethod>(theSmootherMethod),
                          static_cast<int>(theFactor),
                          static_cast<float>(stepNumber / 2));  // NFmiSmoother tasoittaa dataa
                                                                // laskien mukaan arvoja +n:n ja
                                                                // -n:n aika-askeleen yli
    // laskettavasta kohdasta joten jaan
    // stepit kahdella.

    for (destInfo.ResetParam(), sourceInfo.ResetParam();
         destInfo.NextParam() && sourceInfo.NextParam();)
    {
      for (destInfo.ResetLocation(), sourceInfo.ResetLocation();
           destInfo.NextLocation() && sourceInfo.NextLocation();)
      {
        for (destInfo.ResetLevel(), sourceInfo.ResetLevel();
             destInfo.NextLevel() && sourceInfo.NextLevel();)
        {
          int i = 0;
          for (sourceInfo.ResetTime(); sourceInfo.NextTime(); i++)
            values[i] = sourceInfo.FloatValue();  // täytetään ensin aikasarjavektori datalla

          result = smoother.Smoothen(relTimeLocation, values);
          i = 0;
          for (destInfo.ResetTime(); destInfo.NextTime(); i++)
            destInfo.FloatValue(result[i]);  // täytetään tulosdata tasoitetulla aikasarjalla
        }
      }
    }
  }
  return data;
}

// ----------------------------------------------------------------------
/*!
 * \param theFilterId Undocumented
 * \return Undocumented
 * \todo Return an boost::shared_ptr instead
 */
// ----------------------------------------------------------------------

NFmiDataModifier *CreateModifier(int theFilterId)
{
  switch (theFilterId)
  {
    case 0:
      return new NFmiDataModifierAvg;
    case 1:
      return new NFmiDataModifierMax;
    case 2:
      return new NFmiDataModifierMin;
    case 5:
      return new NFmiDataModifierSum;
    case 4:  // jouduin laittamaan id:ksi 4:n, koska editorin filtteröinti funktion valinta on
             // tehty
             // niin köpösti ja siellä tämän on pakko olla 4
      return new NFmiDataModifierMedian;
    default:
      return 0;
  }
}

// ----------------------------------------------------------------------
/*!
 *  Tämä ohjelma lukee stdin:in annetun qdatan, luo uuden datan jossa sama
 *  rakenne. Filtteri laskee sourceDatasta tulosDataan arvot halutulla
 *  suhteellisella aikavälillä arvot halutulla filtterillä. Esim. laskee
 *  +-3 tunnin keskiarvot kaikkialle eli tasoittaa dataa jonkin verran. \par
 *  Käyttö: \par
 *  type sourceData | doTimeFiltering.exe filterId relTimeOffsetStartInMinutes
 *relTimeOffsetEndInMinutes > tulosData \par
 *
 *  Offsetit pyöristetään lähimpiin sourceDatan aika-askeleisiin. Filtteri
 *  funktiot: 0 = avg, 1 = max, 2 = min (EI VIELÄ KÄYTÖSSÄ! 3 = wavg), 4=NFmiSmoother tasoitus,
 *  5 = sum filtterit. Jos haluat tasoittaa tuntidataa laskien keskiarvon yli 7
 *  tunnin (+-3h), anna arvot seuraavasti: \par
 *  type sourceData | doTimeFiltering.exe 0 -180 180 [lisä_argumentti1=3] [lisä_argumentti2=2] ...
 *>
 *tulosData \par
 *  Lisä argumentteja saa antaa käytetystä funktiosta riippuen. \par
 *  NFmiSmoother tasoituksen lisä argumentit:
 *			1=smoother menetelmä (2=kFmiSmootherNeighbourhood,
 *3=kFmiSmootherPseudoGaussian)
 *			2=smootherFactor (1=voimakas ja mitä suurempi luku sitä heikompi)
 *
 * \param theSourceData Undocumented
 * \param theFilterId Undocumented
 * \param theTimeOffsetStartInMinutes Undocumented
 * \param theTimeOffsetEndInMinutes Undocumented
 * \param theAdditionalParam1 Mahdollinen käytettyyn filter-funktioon liittyvä parametri.
 * \param theAdditionalParam2 Mahdollinen käytettyyn filter-funktioon liittyvä parametri.
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::DoTimeFiltering(NFmiQueryData *theSourceData,
                                                  int theFilterId,
                                                  int theTimeOffsetStartInMinutes,
                                                  int theTimeOffsetEndInMinutes,
                                                  double theAdditionalParam1,
                                                  double theAdditionalParam2)
{
  if (theFilterId ==
      4)  // 4= tehdään operaatiot NFmiSmootherluokan avulla esim. pseudogaussian-tasoitus jne.
    return DoTimeFilteringWithSmoother(theSourceData,
                                       theTimeOffsetStartInMinutes,
                                       theTimeOffsetEndInMinutes,
                                       static_cast<int>(theAdditionalParam1),
                                       theAdditionalParam2);

  NFmiQueryData *data = 0;
  if (theSourceData)
  {
    data = theSourceData->Clone();
    NFmiDataModifier *modifier = CreateModifier(theFilterId);
    boost::shared_ptr<NFmiDataModifier> modifierPtr(
        modifier);  // tuhoaa automaattisesti modifier:in kun scoopista poistutaan
    if (modifier)
    {
      NFmiFastQueryInfo destInfo(data);
      NFmiSuperSmartInfo sourceInfo(theSourceData);
      int stepNumber =
          static_cast<int>(round((theTimeOffsetEndInMinutes - theTimeOffsetStartInMinutes) /
                                 static_cast<int>(sourceInfo.TimeResolution()))) +
          1;
      int startShift = theTimeOffsetStartInMinutes / static_cast<int>(sourceInfo.TimeResolution());
      NFmiRelativeTimeIntegrationIterator2 timeIterator(&sourceInfo, stepNumber, startShift);
      NFmiCalculator calculator(&timeIterator, modifier);
      sourceInfo.TimeIntegrationCalculator(&calculator);

      for (destInfo.ResetParam(), sourceInfo.ResetParam();
           destInfo.NextParam() && sourceInfo.NextParam();)
      {
        for (destInfo.ResetLocation(), sourceInfo.ResetLocation();
             destInfo.NextLocation() && sourceInfo.NextLocation();)
        {
          for (destInfo.ResetLevel(), sourceInfo.ResetLevel();
               destInfo.NextLevel() && sourceInfo.NextLevel();)
          {
            for (destInfo.ResetTime(), sourceInfo.ResetTime();
                 destInfo.NextTime() && sourceInfo.NextTime();)
            {
              float value = sourceInfo.FloatValue(false, false, true, false);
              destInfo.FloatValue(value);
            }
          }
        }
      }
    }
  }
  return data;
}

// ----------------------------------------------------------------------
/*!
 *  Tämä ohjelma lukee stdin:in annetun qdatan, luo uuden datan jossa sama
 *  rakenne. Filtteri laskee sourceDatasta tulosDataan arvot halutuilla
 *  alueille älillä arvot halutulla filtterillä. Esim. laskee
 *  keskiarvot tietyn kokoisen ympyrän sisältä kaikkialle eli tasoittaa dataa
 *  jonkin verran. \par
 *  Käyttö: \par
 *  type sourceData | doAreaFiltering.exe filterId areaType [lisä_argumentti1=1]
 * [lisä_argumentti2=1] > tulosData \par
 *
 *  Lisäargumentteja annetaan riippuen käytetystä areaType:stä:
 *  0 = all  (Ei ole toteutettu!)    (laske arvo läpi kaikkien (hila)pisteiden), ei lisä
 * argumentteja. \par
 *  1 = ympyrä  (Ei ole toteutettu!) (laske arvo tietynkokoisen ympyrän sisältä), lisä_argumentti1
 * on säde kilometreissa. \par
 *  2 = hilalaatikko (laske arvo tietynkokoisen hilalaatikon sisältä), lisä_argumentti1 on
 *  hilamäärä x-suunnassa (keskitettynä laskettavaan pisteeseen) ja lisä_argumentti2 on
 *  vastaava luku y-suunnassa. \par
 *
 *  Filtteri funktiot: 0 = avg, 1 = max, 2 = min (3 = wavg EI VIELÄ KÄYTÖSSÄ!), (4=NFmiSmoother
 * tasoitus
 *  EI VIELÄ KÄYTÖSSÄ!), 5 = sum. Jos haluat tasoittaa dataa laskien keskiarvon yli 5x7
 * hilalaatikon
 * yli,
 *  anna arvot seuraavasti: \par
 *  type sourceData | doAreaFiltering.exe 0 2 5 7 > tulosData \par
 *
 * \param theSourceData Undocumented
 * \param theFilterId Undocumented
 * \param theAreaType Undocumented
 * \param theAdditionalParam1 Mahdollinen käytettyyn filter-funktioon liittyvä parametri.
 * \param theAdditionalParam2 Mahdollinen käytettyyn filter-funktioon liittyvä parametri.
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::DoAreaFiltering(NFmiQueryData *theSourceData,
                                                  int theFilterId,
                                                  int theAreaType,
                                                  double theAdditionalParam1,
                                                  double theAdditionalParam2)
{
  if (theAreaType != 2) return 0;
  NFmiQueryData *data = 0;
  if (theSourceData)
  {
    data = theSourceData->Clone();
    NFmiDataModifier *modifier = CreateModifier(theFilterId);
    boost::shared_ptr<NFmiDataModifier> modifierPtr(
        modifier);  // tuhoaa automaattisesti modifier:in kun scoopista poistutaan
    if (modifier)
    {
      NFmiFastQueryInfo destInfo(data);
      NFmiSuperSmartInfo sourceInfo(theSourceData);
      int xCount = static_cast<int>(theAdditionalParam1 / 2);
      int yCount = static_cast<int>(theAdditionalParam2 / 2);
      NFmiRelativeDataIterator areaIterator(&sourceInfo, xCount, yCount);
      NFmiCalculator calculator(&areaIterator, modifier);
      sourceInfo.SetCalculator(&calculator);

      for (destInfo.ResetParam(), sourceInfo.ResetParam();
           destInfo.NextParam() && sourceInfo.NextParam();)
      {
        for (destInfo.ResetLocation(), sourceInfo.ResetLocation();
             destInfo.NextLocation() && sourceInfo.NextLocation();)
        {
          for (destInfo.ResetLevel(), sourceInfo.ResetLevel();
               destInfo.NextLevel() && sourceInfo.NextLevel();)
          {
            for (destInfo.ResetTime(), sourceInfo.ResetTime();
                 destInfo.NextTime() && sourceInfo.NextTime();)
            {
              float value = sourceInfo.FloatValue(false, true, false, false);
              destInfo.FloatValue(value);
            }
          }
        }
      }
    }
  }
  return data;
}

// ======================================================================

const NFmiString &NFmiQueryDataUtil::GetOfficialQueryDataProdIdsKey(void)
{
  static const NFmiString officialKey("ProdIds");
  return officialKey;
}

static void CheckCropLimits(int nx, int ny, int theLeft, int theTop, int theRight, int theBottom)
{
  // Tarkastellaan, että annetut croppi rajat olivat järkeviä.
  if (theLeft < 0 || theLeft >= nx)
    throw runtime_error("cropin vasen-reuna oli yli originaali hilan rajojen.");
  if (theRight < 0 || theRight >= nx)
    throw runtime_error("cropin oikea-reuna oli yli originaali hilan rajojen.");
  if (theBottom < 0 || theBottom >= ny)
    throw runtime_error("cropin ala-reuna oli yli originaali hilan rajojen.");
  if (theTop < 0 || theTop >= ny)
    throw runtime_error("cropin ylä-reuna oli yli originaali hilan rajojen.");
  if (theLeft > theRight) throw runtime_error("cropin vasen-reuna oli yli oikean reunan.");
  if (theBottom > theTop) throw runtime_error("cropin ala-reuna oli yli ylä-reunan.");
}

static NFmiArea *CreateCropArea(const NFmiGrid *theGrid,
                                const NFmiPoint &theBLXY,
                                const NFmiPoint &theTRXY)
{
  NFmiArea *area = 0;
  NFmiPoint blLatlon(theGrid->GridToLatLon(theBLXY));
  NFmiPoint trLatlon(theGrid->GridToLatLon(theTRXY));
  area = theGrid->Area()->CreateNewArea(blLatlon, trLatlon);
  return area;
}

// sitten päästään itse croppaukseen!!!!
static void CropData(NFmiFastQueryInfo &theSource,
                     NFmiFastQueryInfo &theResult,
                     int theLeft,
                     int theBottom)
{
  for (theResult.ResetParam(), theSource.ResetParam();
       theResult.NextParam() && theSource.NextParam();)
  {
    for (theResult.ResetLevel(), theSource.ResetLevel();
         theResult.NextLevel() && theSource.NextLevel();)
    {
      for (theResult.ResetTime(), theSource.ResetTime();
           theResult.NextTime() && theSource.NextTime();)
      {
        theResult.Bottom();
        theSource.Bottom();
        theSource.MoveUp(theBottom);  // liikutetaan sourcea haluttuun cropin alkupisteeseen
        do
        {
          theResult.Left();
          theSource.Left();
          theSource.MoveRight(theLeft);  // liikutetaan sourcea haluttuun cropin alkupisteeseen
          do
          {
            theResult.FloatValue(theSource.FloatValue());
          } while (theResult.MoveRight() && theSource.MoveRight());
        } while (theResult.MoveUp() && theSource.MoveUp());
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * Tämä funktio luo uuden datan joka on cropattu halutulla tavalla
 * annetusta datasta. Uudessa datassa on muuten sama rakenne
 * (params, times, levels).
 *
 * Haluttu croppi alue annetaan hila-indeksi laatikkona. Kyseiseen
 * laatikkoon myös lasketaan uusi area.
 * Croppaus aluetta ei voi 'venyttää' ali eikä yli alkuperäisen rajojen.
 * Jos joku haluaa tälläisen efektin, pitää käyttää vaikka
 * Interpolate2OtherArea-filtteriä.
 *
 * Hila piste indeksi alkavat 0:sta ja päättyvät hilaruudukon koon size-1:een.
 *
 * \param theInfo data, josta halutaan croppi tehdä.
 * \param theLeft Cropin vasen reuna hilapiste indeksinä (left < right).
 * \param theTop Cropin ylä reuna hilapiste indeksinä (bottom < top).
 * \param theRight Cropin oikea reuna hilapiste indeksinä.
 * \param theBottom Cropin ala reuna hilapiste indeksinä.
 * \return Metodissa luotu NFmiQueryData olio, tai 0-pointteri jos jokin meni pieleen.
 */
// ----------------------------------------------------------------------

NFmiQueryData *NFmiQueryDataUtil::QDCrop(
    NFmiFastQueryInfo &theInfo, int theLeft, int theTop, int theRight, int theBottom)
{
  NFmiQueryData *data = 0;
  if (theInfo.Grid())
  {
    int nx = theInfo.Grid()->XNumber();
    int ny = theInfo.Grid()->YNumber();
    if (theRight == kShortMissing)  // tämä oli defaultti arvo
      theRight = nx - 1;
    if (theTop == kShortMissing)  // tämä oli defaultti arvo
      theTop = ny - 1;
    CheckCropLimits(nx, ny, theLeft, theTop, theRight, theBottom);
    NFmiPoint blXY(theLeft, theBottom);
    NFmiPoint trXY(theRight, theTop);
    NFmiArea *area = CreateCropArea(theInfo.Grid(), blXY, trXY);
    boost::shared_ptr<NFmiArea> areaPtr(area);
    if (area)
    {
      NFmiGrid grid(area, theRight - theLeft + 1, theTop - theBottom + 1);
      NFmiHPlaceDescriptor hPlace(grid);
      NFmiQueryInfo innerInfo(
          theInfo.ParamDescriptor(), theInfo.TimeDescriptor(), hPlace, theInfo.VPlaceDescriptor());
      data = NFmiQueryDataUtil::CreateEmptyData(innerInfo);
      if (data)
      {
        NFmiFastQueryInfo tulosInfo(data);
        CropData(theInfo, tulosInfo, theLeft, theBottom);
      }
    }
  }
  return data;
}

// tämä on ulkopuolisen callback-olion tarkistus. Eli jos toisesta säikeestä on
// tullut lopetus käsky, tämä heittää poikkeuksen, jolloin datan rakentelu loppuu siihen.
// Kaikki dynaamiset datat pitää siis tehdä niin että ne osaavat tuhota itsensä automaattisesti.
void NFmiQueryDataUtil::CheckIfStopped(NFmiStopFunctor *theStopFunctor)  // tämä versio on pelkälle
                                                                         // tausta threadille, jolle
                                                                         // on annettu vain
                                                                         // NFmiStopFunctor-otus
{
  if (theStopFunctor && theStopFunctor->Stop()) throw NFmiStopThreadException();
}
void NFmiQueryDataUtil::CheckIfStopped(NFmiThreadCallBacks *theThreadCallBacks)  // tämä versio on
                                                                                 // GUI ohjelman
                                                                                 // funktiolle,
                                                                                 // jossa tulee
                                                                                 // näkyviin
                                                                                 // progressbar ja
                                                                                 // cancel
                                                                                 // mahdollisuus
{
  if (theThreadCallBacks && theThreadCallBacks->Stop()) throw NFmiStopThreadException();
}

// Tällä juoksutetaan mahdollisen ulkopuolisen callback-olion edistymistä.
void NFmiQueryDataUtil::DoStepIt(NFmiThreadCallBacks *theThreadCallBacks)
{
  if (theThreadCallBacks) theThreadCallBacks->StepIt();
}

// Tällä asetetaan mahdollisen ulkopuolisen callback-olion edistymisrajat ja steppi.
void NFmiQueryDataUtil::SetRange(NFmiThreadCallBacks *theThreadCallBacks,
                                 int low,
                                 int high,
                                 int stepCount)
{
  if (theThreadCallBacks) theThreadCallBacks->SetRange(low, high, stepCount);
}

// Tällä lisätään mahdollisen ulkopuolisen callback-olion edistymisrajaan loppuarvoa.
void NFmiQueryDataUtil::AddRange(NFmiThreadCallBacks *theThreadCallBacks, int value)
{
  if (theThreadCallBacks) theThreadCallBacks->AddRange(value);
}

// Oletuksia: theFilesIn on tiedostojen uutuus järjestyksessä.
// Jos theBaseQData:ssa on data, haetaan vain sen viimeistä dataa uudemmat queryDatat,
// eli loopin voi lopettaa 1. missä ajat vanhempia (tiedostojen aika järjestys)
static void ReadQueryDataFiles(boost::shared_ptr<NFmiQueryData> theBaseQData,
                               const std::string &theDirName,
                               std::vector<std::string> &theFilesIn,
                               std::vector<boost::shared_ptr<NFmiQueryData> > &theQDataVectorOut,
                               NFmiStopFunctor *theStopFunctor,
                               bool fDoRebuildCheck)
{
  bool doTimeCheck = false;
  NFmiMetTime lastBaseTime;
  if (fDoRebuildCheck == false && theBaseQData)
  {
    doTimeCheck = true;
    lastBaseTime = theBaseQData->Info()->TimeDescriptor().LastTime();
  }

  for (size_t i = 0; i < theFilesIn.size(); i++)
  {
    NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);
    std::string totalFileName = theDirName + theFilesIn[i];
    if (NFmiFileSystem::FileReadable(totalFileName))
    {
      boost::shared_ptr<NFmiQueryData> qDataPtr;
      try
      {
        qDataPtr = boost::shared_ptr<NFmiQueryData>(new NFmiQueryData(totalFileName, true));
      }
      catch (...)
      {
        // pakkko tehdä try-catch -blokki korruptoituneiden tiedostojen varalle
      }

      if (qDataPtr)
      {
        if (doTimeCheck)
        {
          if (qDataPtr->Info()->TimeDescriptor().FirstTime() <= lastBaseTime) break;
        }
        theQDataVectorOut.push_back(qDataPtr);
      }
    }
  }
}

static void MakeFastInfos(std::vector<boost::shared_ptr<NFmiQueryData> > &theQDataVectorIn,
                          std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorOut)
{
  for (size_t i = 0; i < theQDataVectorIn.size(); i++)
  {
    NFmiFastQueryInfo *fInfo = new NFmiFastQueryInfo(theQDataVectorIn[i].get());
    if (fInfo) theFInfoVectorOut.push_back(boost::shared_ptr<NFmiFastQueryInfo>(fInfo));
  }
}

// Tekee uniikki listan validTimeista, jotka on annetussa fastInfo vektorissa. sorttaa ne
// laskevaan
// järjestykseen (uusimmat ensin) ja leikkaa tarvittaessa listan koon haluttuun määrään
// theMaxTimeStepsInData-parametrin mukaan. Jos se on <= 0, tällöin otetaan kaiiki ajat mukaan.
static std::vector<NFmiMetTime> MakeValidTimesList(
    std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorIn, int theMaxTimeStepsInData)
{
  std::set<NFmiMetTime> uniqueValidTimes;
  for (size_t i = 0; i < theFInfoVectorIn.size(); i++)
  {
    NFmiFastQueryInfo *fInfo = theFInfoVectorIn[i].get();
    if (fInfo)
    {
      for (fInfo->ResetTime(); fInfo->NextTime();)
      {
        uniqueValidTimes.insert(fInfo->Time());
      }
    }
  }

  if (uniqueValidTimes.empty()) return std::vector<NFmiMetTime>();

  std::list<NFmiMetTime> selectedTimes(uniqueValidTimes.begin(), uniqueValidTimes.end());
  selectedTimes.reverse();  // käännetään järjestys, että uusimmat on ensin
  if (theMaxTimeStepsInData > 0 && static_cast<int>(selectedTimes.size()) > theMaxTimeStepsInData)
    selectedTimes.resize(theMaxTimeStepsInData);
  selectedTimes.reverse();  // käännetään taas järjestys, että lopullinen määrä aikoja nousevaan
                            // järjestykseen
  return std::vector<NFmiMetTime>(selectedTimes.begin(), selectedTimes.end());
}

static NFmiTimeList MakeTimeList(std::vector<NFmiMetTime> &theValidTimesIn)
{
  NFmiTimeList timeList;
  for (size_t i = 0; i < theValidTimesIn.size(); i++)
    timeList.Add(new NFmiMetTime(theValidTimesIn[i]), false, false);

  return timeList;
}

namespace std
{
template <>
struct less<NFmiDataIdent>
{
  bool operator()(const NFmiDataIdent &lhs, const NFmiDataIdent &rhs)
  {
    return lhs.GetParamIdent() < rhs.GetParamIdent();
  }
};
}

static NFmiParamDescriptor MakeParamDesc(
    std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorIn)
{
  std::set<NFmiDataIdent> paramSet;
  for (size_t i = 0; i < theFInfoVectorIn.size(); i++)
  {
    for (theFInfoVectorIn[i]->ResetParam(); theFInfoVectorIn[i]->NextParam();)
      paramSet.insert(theFInfoVectorIn[i]->Param());
  }
  if (paramSet.empty())
    throw std::runtime_error("Error in MakeParamDesc, no parameters were found.");

  std::vector<NFmiDataIdent> paramVec(paramSet.begin(), paramSet.end());

  NFmiParamBag parBag(&paramVec[0], static_cast<unsigned long>(paramVec.size()));
  NFmiParamDescriptor parDesc(parBag);
  return parDesc;
}

namespace std
{
template <>
struct less<NFmiLevel>
{
  bool operator()(const NFmiLevel &lhs, const NFmiLevel &rhs)
  {
    return lhs.LevelValue() < rhs.LevelValue();
  }
};
}

static NFmiVPlaceDescriptor MakeVPlaceDesc(
    std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorIn)
{
  std::set<NFmiLevel> levelSet;
  for (size_t i = 0; i < theFInfoVectorIn.size(); i++)
  {
    for (theFInfoVectorIn[i]->ResetLevel(); theFInfoVectorIn[i]->NextLevel();)
      levelSet.insert(*(theFInfoVectorIn[i]->Level()));
  }
  if (levelSet.empty()) throw std::runtime_error("Error in MakeVPlaceDesc, no levels were found.");

  std::vector<NFmiLevel> levelVec(levelSet.begin(), levelSet.end());

  NFmiLevelBag levBag(&levelVec[0], static_cast<unsigned long>(levelVec.size()));

  NFmiVPlaceDescriptor levDesc(levBag);
  return levDesc;
}

// Tässä rakennetaan yhdistelmä dataa varten meta-info.
// Jos fFirstInfoDefines = true, otetaan 1. fInfosta (tuorein)  hila+alue, levelit ja parametri
// lista.
// Jos se oli false, rakennetaan level ja parametri listat kaikista annetuista infoista, hila
// otetaan aina 1. infosta.
// Validtime-vektorista tehdään timeDescriptori.
// HUOM! Tämä toimii nyt vain hiladatoille. Muuttuu tulevaisuudessa...
static NFmiQueryInfo MakeCombinedDatasMetaInfo(
    std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorIn,
    std::vector<NFmiMetTime> &theValidTimesIn,
    bool fFirstInfoDefines)
{
  if (theFInfoVectorIn.size() == 0)
    throw std::runtime_error(
        "Error in MakeCombinedDatasMetaInfo, given fastInfo-vector was empty.");
  NFmiFastQueryInfo *firstInfo = theFInfoVectorIn[0].get();
  if (firstInfo == 0)
    throw std::runtime_error("Error in MakeCombinedDatasMetaInfo, 1. fastInfo was 0-pointer.");
  NFmiParamDescriptor parDesc;
  if (fFirstInfoDefines)
    parDesc = firstInfo->ParamDescriptor();
  else
    parDesc = ::MakeParamDesc(theFInfoVectorIn);

  NFmiMetTime origTime = firstInfo->OriginTime();
  NFmiTimeDescriptor timeDesc(origTime, ::MakeTimeList(theValidTimesIn));
  if (firstInfo->Grid() == 0)
    throw std::runtime_error("Error in MakeCombinedDatasMetaInfo, 1. fastInfo's has no grid.");
  NFmiHPlaceDescriptor hPlaceDesc = firstInfo->HPlaceDescriptor();
  NFmiVPlaceDescriptor vPlaceDesc;
  if (fFirstInfoDefines)
    vPlaceDesc = firstInfo->VPlaceDescriptor();
  else
    vPlaceDesc = ::MakeVPlaceDesc(theFInfoVectorIn);

  NFmiQueryInfo qInfo(parDesc, timeDesc, hPlaceDesc, vPlaceDesc);
  return qInfo;
}

static NFmiFastQueryInfo *FindWantedInfo(
    const NFmiMetTime &theTime,
    std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorIn)
{
  for (size_t i = 0; i < theFInfoVectorIn.size(); i++)
  {
    if (theFInfoVectorIn[i]->Time(theTime)) return theFInfoVectorIn[i].get();
  }
  return 0;  // tänne ei pitäisi mennä, pitäisikö heittää poikkeus?
}

static void FillDataToCurrentTime(
    NFmiFastQueryInfo &theFilledInfo,
    std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVectorIn,
    NFmiStopFunctor *theStopFunctor)
{
  NFmiFastQueryInfo *sourceInfo = ::FindWantedInfo(theFilledInfo.Time(), theFInfoVectorIn);
  if (sourceInfo)  // aina pitäisi löytyä sourceInfo, mutta tarkistetaan kuitenkin
  {
    if (theFilledInfo.Grid() && sourceInfo->Grid() &&
        *(theFilledInfo.Grid()) == *(sourceInfo->Grid()))
    {
      for (theFilledInfo.ResetLevel(); theFilledInfo.NextLevel();)
      {
        if (sourceInfo->Level(*theFilledInfo.Level()))
        {
          for (theFilledInfo.ResetParam(); theFilledInfo.NextParam();)
          {
            if (sourceInfo->Param(theFilledInfo.Param()))
            {
              NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);  // tarkistetaan lopetus ehtoa
                                                                  // aina yhden kentän täytön
                                                                  // välein...
              // oletus vielä nyt että hpalceDesc:it samanlaisia
              NFmiDataMatrix<float> values;
              sourceInfo->Values(values);
              theFilledInfo.SetValues(values);
            }
          }
        }
      }
    }
  }
}

std::string NFmiQueryDataUtil::GetFileFilterDirectory(const std::string &theFileFilter)
{
  NFmiFileString fileStr(theFileFilter);
  NFmiString str;
  str += fileStr.Device();
  str += fileStr.Path();

  return static_cast<char *>(str);
}

static void CombineTimeStepDatas(NFmiQueryData &theData,
                                 std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVector,
                                 NFmiStopFunctor *theStopFunctor)
{
  NFmiFastQueryInfo combFastInfo(&theData);
  combFastInfo.LastTime();
  do
  {
    NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);
    ::FillDataToCurrentTime(combFastInfo, theFInfoVector, theStopFunctor);
  } while (combFastInfo.PreviousTime());
}

static void CombineSliceDatas(NFmiQueryData &theData,
                              std::vector<boost::shared_ptr<NFmiFastQueryInfo> > &theFInfoVector,
                              NFmiStopFunctor *theStopFunctor)
{
  NFmiFastQueryInfo combFastInfo(&theData);
  // käydään kaikki fInfot läpi ja katsotaan mihin rakoihin data saadaan tungettua
  for (size_t i = 0; i < theFInfoVector.size(); i++)
  {
    NFmiFastQueryInfo &fInfo = *(theFInfoVector[i].get());

    // ainakin aluksi pitää olla hila dataa ja hilojenkin samanlaisia, TODO tämän voisi muuttaa
    // tulevaisuudessa
    if (fInfo.Grid() && combFastInfo.Grid() && *(fInfo.Grid()) == *(combFastInfo.Grid()))
    {
      // luultavasti fInfo on vain yksi slice (= yksi aika,level,param), mutta joudumme
      // käymään sen läpi varmuuden vuoksi kokonaan ja katsomaan jokaisen slicen kohdalla meneekö
      // se
      // tulos dataan
      for (fInfo.ResetTime(); fInfo.NextTime();)
      {
        NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);
        if (combFastInfo.Time(fInfo.Time()))
        {
          for (fInfo.ResetLevel(); fInfo.NextLevel();)
          {
            if (combFastInfo.Level(*fInfo.Level()))
            {
              for (fInfo.ResetParam(); fInfo.NextParam();)
              {
                if (combFastInfo.Param(
                        static_cast<FmiParameterName>(fInfo.Param().GetParamIdent())))
                {
                  NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);
                  // oletus vielä nyt että hpalceDesc:it samanlaisia
                  NFmiDataMatrix<float> values;
                  fInfo.Values(values);
                  combFastInfo.SetValues(values);
                }
              }
            }
          }
        }
      }
    }
  }
}

// fFirstInfoDefines määrää sekä rakennetaanko 'metaInfo' ensimmäisen infon avulla,
// että miten lopullinen data täytetään.
// Jos fDoTimeStepCombine on true, on tarkoitus liittää samanlaisista aika-askel-datoista
// yksi pitkä aikajakso. Mutta jos se on false, on tarkoitus vain liittää joukko
// 'sliceja' yhdeksi isoksi dataksi eli viipaleita 3D-dataksi.
// Jos theBaseQData sisältää dataa, käytetään se ensin pohjaksi uudelle datalle.
NFmiQueryData *NFmiQueryDataUtil::CombineQueryDatas(
    bool fDoRebuild,
    boost::shared_ptr<NFmiQueryData> &theBaseQData,
    std::vector<boost::shared_ptr<NFmiQueryData> > &theQDataVector,
    bool fDoTimeStepCombine,
    int theMaxTimeStepsInData,
    NFmiStopFunctor *theStopFunctor)
{
  std::vector<boost::shared_ptr<NFmiFastQueryInfo> > fInfoVector;
  ::MakeFastInfos(theQDataVector, fInfoVector);
  if (fDoRebuild == false && theBaseQData)
    fInfoVector.push_back(
        boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(theBaseQData.get())));

  std::vector<NFmiMetTime> foundValidTimes =
      ::MakeValidTimesList(fInfoVector, theMaxTimeStepsInData);
  if (foundValidTimes.size() > 0)
  {
    NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);
    NFmiQueryInfo combinedDataMetaInfo =
        ::MakeCombinedDatasMetaInfo(fInfoVector, foundValidTimes, fDoTimeStepCombine);
    NFmiQueryDataUtil::CheckIfStopped(theStopFunctor);
    std::unique_ptr<NFmiQueryData> data(CreateEmptyData(combinedDataMetaInfo));
    if (data)
    {
      if (fDoTimeStepCombine)
        ::CombineTimeStepDatas(*data, fInfoVector, theStopFunctor);
      else
        ::CombineSliceDatas(*data, fInfoVector, theStopFunctor);
      return data.release();
    }
  }
  return 0;
}

static boost::shared_ptr<NFmiQueryData> GetNewestQueryData(const std::string &theBaseDataFileFilter)
{
  boost::shared_ptr<NFmiQueryData> qDataPtr;
  std::string fileName = NFmiFileSystem::NewestPatternFileName(theBaseDataFileFilter);
  if (fileName.empty() == false)
  {
    if (NFmiFileSystem::FileReadable(fileName))
    {
      NFmiQueryData *qData = new NFmiQueryData(fileName, true);
      if (qData) qDataPtr = boost::shared_ptr<NFmiQueryData>(qData);
    }
  }
  return qDataPtr;
}

// Luo queryDatan annetun filefilterin avulla. Niistä tiedostoista mitä tulee annetulla
// filefilterillä
// luodaan yksi kokooma qData. theMaxTimeStepsInData-parametrilla voidaan rajoittaa luodun datan
// koko
// Siten että lopulliseen dataan otetaan vain niin monta uusinta aika-askelta kun on sanottu. Jos
// sen arvo on 0, otetaan kaikki aika-askeleet lopulliseen dataan.
// HUOM! vaatimuksia datoille, ensimmäisestä datasta (=uusimmasta) otetaan pohjat
// (hila,parametrit,levelit):
// 1. Datoissa pitää olla sama hila ja alue, eli sama NFmiGrid.
// 2. Ei saa olla asemadataa (Tämä pitää muuttaa vielä)
// 3. Pitää olla samat parametrit (Tämä pitää muuttaa vielä)
// 4. Pitää olla samat levelit (Tämä pitää muuttaa vielä)
// 5. Muutos: annettua theBaseDataFileFilter:istä etsitään uusin tiedosto.
//   jos se löytyy, otetaan siitä pohjaksi niin paljon dataa kuin mahdollista ja luetaan
//   theFileFilter
//  datoja vain niin paljon kuin tarpeen eli vain data, missä on uudempaa dataa kuin base-datassa.
//  Tämä siksi että näin tarvitsee käydä levypalvelimella mahdollisimman vähän kun base-data on
//  omalla
//  kovalevyllä ja sen kanssa ei ole verkko ongelmia.
NFmiQueryData *NFmiQueryDataUtil::CombineQueryDatas(bool fDoRebuildCheck,
                                                    const std::string &theBaseDataFileFilter,
                                                    const std::string &theFileFilter,
                                                    bool fDoTimeStepCombine,
                                                    int theMaxTimeStepsInData,
                                                    NFmiStopFunctor *theStopFunctor)
{
  boost::shared_ptr<NFmiQueryData> baseQData;
  try
  {
    baseQData = ::GetNewestQueryData(theBaseDataFileFilter);
  }
  catch (...)
  {
    // ei tehdä mitään ylimääräistä poikkeuksen tapahtuessa (esim. korruptoitunut pohja-data
    // tiedosto)
  }

  std::list<std::string> fileList = NFmiFileSystem::PatternFiles(theFileFilter);
  fileList.sort();     // sortataan
  fileList.reverse();  // käännetään järjestys, jolloin jos käytetty YYYYMMDDHHmmss aikaleimaa
                       // tiedoston alussa, tulee uusimmat tiedostot alkuun

  // Oletus, jokaisessa tiedostossa on yksi aika, ja tiedostot ovat oikeassa aikajärjestyksessä.
  // Tehdään tiedosto-lista vektoriksi.
  std::vector<std::string> files(fileList.begin(), fileList.end());

  std::string dirName =
      GetFileFilterDirectory(theFileFilter);  // fileFilteristä pitää ottaa hakemisto irti, koska
                                              // PatternFiles-funktio palautta vain tiedostojen
                                              // nimet, ei polkua mukana
  std::vector<boost::shared_ptr<NFmiQueryData> > qDataVector;
  ::ReadQueryDataFiles(baseQData, dirName, files, qDataVector, theStopFunctor, fDoRebuildCheck);
  return CombineQueryDatas(fDoRebuildCheck,
                           baseQData,
                           qDataVector,
                           fDoTimeStepCombine,
                           theMaxTimeStepsInData,
                           theStopFunctor);
}

static void FillGridDataInThread(NFmiFastQueryInfo &theSourceInfo,
                                 NFmiFastQueryInfo &theTargetInfo,
                                 NFmiDataMatrix<NFmiLocationCache> &theLocationCacheMatrix,
                                 checkedVector<NFmiTimeCache> &theTimeCacheVector,
                                 unsigned long theStartTimeIndex,
                                 unsigned long theEndTimeIndex,
                                 int theThreadNumber,
                                 NFmiLogger *theLogger)
{
  if (theLogger)
  {
    std::string logStr("FillGridDataInThread - thread no: ");
    logStr += NFmiStringTools::Convert(theThreadNumber);
    logStr += " started with start-time-index: ";
    logStr += NFmiStringTools::Convert(theStartTimeIndex);
    logStr += " and end-time-index: ";
    logStr += NFmiStringTools::Convert(theEndTimeIndex);
    theLogger->LogMessage(logStr, NFmiLogger::kDebugInfo);
  }
  if (theStartTimeIndex == gMissingIndex || theEndTimeIndex == gMissingIndex) return;

  NFmiDataMatrix<float> gridValues;
  bool doGroundData =
      (theSourceInfo.SizeLevels() == 1) &&
      (theTargetInfo.SizeLevels() ==
       1);  // jos molemmissa datoissa vain yksi leveli, se voidaan jättää tarkastamatta
  bool doLocationInterpolation =
      (NFmiQueryDataUtil::AreGridsEqual(theSourceInfo.Grid(), theTargetInfo.Grid()) == false);

  unsigned long targetXSize = theTargetInfo.GridXNumber();
  for (theTargetInfo.ResetParam(); theTargetInfo.NextParam();)
  {
    if (theLogger)
    {
      std::string logStr("FillGridDataInThread - thread no: ");
      logStr += NFmiStringTools::Convert(theThreadNumber);
      logStr += " starting param: ";
      logStr += NFmiStringTools::Convert(theTargetInfo.ParamIndex() + 1);
      logStr += "/";
      logStr += NFmiStringTools::Convert(theTargetInfo.SizeParams());
      if (theTargetInfo.ParamIndex() == 0) logStr += " (outer loop)";
      theLogger->LogMessage(logStr, NFmiLogger::kDebugInfo);
    }

    if (theSourceInfo.Param(static_cast<FmiParameterName>(theTargetInfo.Param().GetParamIdent())))
    {
      for (theTargetInfo.ResetLevel(); theTargetInfo.NextLevel();)
      {
        if (doGroundData || theSourceInfo.Level(*theTargetInfo.Level()))
        {
          for (unsigned long i = theStartTimeIndex; i <= theEndTimeIndex; i++)
          {
            if (theTargetInfo.TimeIndex(i) == false) continue;
            NFmiMetTime targetTime = theTargetInfo.Time();
            bool doTimeInterpolation =
                false;  // jos aikaa ei löydy suoraan, tarvittaessa tehdään aikainterpolaatio
            if (theSourceInfo.Time(theTargetInfo.Time()) ||
                (doTimeInterpolation =
                     theSourceInfo.TimeDescriptor().IsInside(theTargetInfo.Time())) == true)
            {
              NFmiTimeCache &timeCache = theTimeCacheVector[theTargetInfo.TimeIndex()];
              if (doLocationInterpolation == false)
              {
                if (doTimeInterpolation)
                  theSourceInfo.Values(gridValues, targetTime);
                else
                  theSourceInfo.Values(gridValues);

                theTargetInfo.SetValues(gridValues);
              }
              else
              {  // interpoloidaan paikan suhteen
                for (theTargetInfo.ResetLocation(); theTargetInfo.NextLocation();)
                {
                  float value = kFloatMissing;
                  //									float value2
                  //=
                  // kFloatMissing;
                  NFmiLocationCache &locCache =
                      theLocationCacheMatrix[theTargetInfo.LocationIndex() %
                                             targetXSize][theTargetInfo.LocationIndex() /
                                                          targetXSize];
                  if (doLocationInterpolation && doTimeInterpolation)
                    value = theSourceInfo.CachedInterpolation(locCache, timeCache);
                  else if (doLocationInterpolation)
                  {
                    value = theSourceInfo.CachedInterpolation(locCache);
                    /*
                                                                                                    value2 = theSourceInfo.InterpolatedValue(theTargetInfo.LatLon());
                                                                                                    if(::IsEqualEnough(value, value2, 0.00000001f) == false)
                                                                                                    {
                                                                                                            value = theSourceInfo.CachedInterpolation(locCache);
                                                                                                            value2 = theSourceInfo.InterpolatedValue(theTargetInfo.LatLon());
                                                                                                    }
                    */
                  }
                  theTargetInfo.FloatValue(value);
                }
              }
            }
          }
        }
      }
    }
  }
}

// Kun joku rutiini jaetaan kolmeen työ-threadi osioon ajan-suhteen, tämä laskee
// kullekin threadille alkuaika ja loppuaika indeksit.
void CalcTimeIndexiesForThreeThreads(unsigned long theStartInd,
                                     unsigned long theEndInd,
                                     unsigned long &theStartInd1,
                                     unsigned long &theEndInd1,
                                     unsigned long &theStartInd2,
                                     unsigned long &theEndInd2,
                                     unsigned long &theStartInd3,
                                     unsigned long &theEndInd3)
{
  if (theStartInd > theEndInd)
    throw std::runtime_error(
        "Error in NFmiQueryDataUtil::CalcTimeIndexiesForThreeThreads: time-indexies given wrong "
        "way?");

  unsigned long totalSize =
      theEndInd - theStartInd +
      1;  // pitää lisätä lukuun yksi, jos alku ja loppu indeksi ovat samoja, on lukumäärä 1 jne.
  unsigned long partSize = totalSize / 3;

  if (totalSize == 0)
  {
    theStartInd1 = theEndInd1 = theStartInd2 = theEndInd2 = theStartInd3 = theEndInd3 =
        gMissingIndex;
    return;
  }
  else if (totalSize == 1)
  {
    theStartInd1 = theEndInd1 = theStartInd;
    theStartInd2 = theEndInd2 = theStartInd3 = theEndInd3 = gMissingIndex;
    return;
  }
  else if (totalSize == 2)
  {
    theStartInd1 = theEndInd1 = theStartInd;
    theStartInd2 = theEndInd2 = theStartInd + 1;
    theStartInd3 = theEndInd3 = gMissingIndex;
    return;
  }
  else if (totalSize % 3 == 0)
  {
    theStartInd1 = theStartInd;
    theStartInd2 = theStartInd + partSize;
    theStartInd3 = theStartInd + partSize * 2;
  }
  else if (totalSize % 3 == 1)
  {
    theStartInd1 = theStartInd;
    theStartInd2 = theStartInd + partSize;
    theStartInd3 =
        theStartInd + partSize * 2;  // viimeinen threadi saa tehdä yhden ylimääräisen aika-askeleen
  }
  else if (totalSize % 3 == 2)
  {
    theStartInd1 = theStartInd;
    theStartInd2 =
        theStartInd + partSize;  // toinen threadi saa tehdä yhden ylimääräisen aika-askeleen
    theStartInd3 = theStartInd + partSize * 2 +
                   1;  // viimeinen threadi saa tehdä yhden ylimääräisen aika-askeleen
  }
  theEndInd1 = theStartInd2 - 1;
  theEndInd2 = theStartInd3 - 1;
  theEndInd3 = theEndInd;
}

//#define BOOST_DISABLE_THREADS 1

// Funktio täyttää lähdedatasta mahdollisimman hyvin kohde-dataa.
// Tarkistaa aikoja, parametreja ja leveleitä.
// Datojen pitää olla vielä hilamuotoisia, tarvittaessa tehdään interpolaatiota
// ajan ja paikan suhteen.
// Yritetty optimoida joka suhteessa.
// Kokeiltu (1) latlon interpolaation kanssa että jos täyttää matriisiin datan ja lopuksi
// SetValues
// target:ille, niin se onkin hitaampaa!
void NFmiQueryDataUtil::FillGridData(NFmiQueryData *theSource,
                                     NFmiQueryData *theTarget,
                                     unsigned long theStartTimeIndex,
                                     unsigned long theEndTimeIndex,
                                     NFmiLogger *theLogger,
                                     bool fUseOnlyOneThread)
{
  if (theSource && theTarget && theSource->IsGrid() && theTarget->IsGrid())
  {
    NFmiFastQueryInfo source1(theSource);
    NFmiFastQueryInfo target1(theTarget);
    bool doLocationInterpolation =
        (NFmiQueryDataUtil::AreGridsEqual(source1.Grid(), target1.Grid()) == false);
    NFmiDataMatrix<NFmiLocationCache> locationCacheMatrix;
    if (doLocationInterpolation) source1.CalcLatlonCachePoints(target1, locationCacheMatrix);
    checkedVector<NFmiTimeCache> timeCacheVector;
    source1.CalcTimeCache(target1, timeCacheVector);

    unsigned long usedStartTimeIndex = theStartTimeIndex;
    unsigned long usedEndTimeIndex = theEndTimeIndex;
    if (usedStartTimeIndex == gMissingIndex) usedStartTimeIndex = 0;
    if (usedEndTimeIndex == gMissingIndex) usedEndTimeIndex = target1.SizeTimes() - 1;

#ifdef UNIX
#define DISABLE_THIS_THREAD 1
#else
#ifdef BOOST_DISABLE_THREADS
#define DISABLE_THIS_THREAD 1
#endif
#endif

#ifdef DISABLE_THIS_THREAD

    ::FillGridDataInThread(source1,
                           target1,
                           locationCacheMatrix,
                           timeCacheVector,
                           usedStartTimeIndex,
                           usedEndTimeIndex,
                           1,
                           theLogger);

#else   // DISABLE_THIS_THREAD

    if (fUseOnlyOneThread || target1.SizeTimes() < 3)
    {  // jos aikoja oli alle kolme, tai ei haluta jakaa työtä, lasketaan data yhdessä säikeessä
      ::FillGridDataInThread(source1,
                             target1,
                             locationCacheMatrix,
                             timeCacheVector,
                             usedStartTimeIndex,
                             usedEndTimeIndex,
                             1,
                             theLogger);
    }
    else
    {
      unsigned long timeStart1 = gMissingIndex;
      unsigned long timeStart2 = gMissingIndex;
      unsigned long timeStart3 = gMissingIndex;
      unsigned long timeEnd1 = gMissingIndex;
      unsigned long timeEnd2 = gMissingIndex;
      unsigned long timeEnd3 = gMissingIndex;
      // jaetaan aika indeksi kolmeen osaa ja käynnistetään kolme worker threadia, jotka laskevat
      // eri osia datasta
      CalcTimeIndexiesForThreeThreads(usedStartTimeIndex,
                                      usedEndTimeIndex,
                                      timeStart1,
                                      timeEnd1,
                                      timeStart2,
                                      timeEnd2,
                                      timeStart3,
                                      timeEnd3);

      NFmiFastQueryInfo source2(theSource);
      NFmiFastQueryInfo target2(theTarget);
      NFmiFastQueryInfo source3(theSource);
      NFmiFastQueryInfo target3(theTarget);

      // HUOM! multi threaddauksessa otettava huomioon!!!!
      // NFmiQueryData-luokassa on itsLatLonCache, joka on samalla käytössä kaikilla
      // erillisillä Info-iteraattoreilla. Tämä on ns. lazy-init dataa, joka initialisoidaan
      // kerran
      // tarvittaessa. Nyt samoihin datoihin tehdään kolme info-iteraattoria ja tässä kutsumalla
      // yhdelle niistä
      // LatLon-metodia, varmistetaan että itsLatLonCache on initialisoitu, ennen kuin mennään
      // suorittamaan kolmea threadia yhtäaikaa. Tämä oli kaato bugi, kun kolme thtreadia kutsui
      // Latlon:ia
      // ja cache-data ei oltu vielä initialisoitu, jolloin sitä initialisoitiin kolme kertaa
      // päällekkäin.
      // Ongelma oli varsinkin isommille datoille (esim. mbe), joille kyseisen cachen rakentaminen
      // kestää kauemmin.
      NFmiPoint dummyPoint =
          target3.LatLon();  // Varmistetaan että NFmiQueryDatan itsLatLonCache on alustettu!!
      dummyPoint =
          source3.LatLon();  // Varmistetaan että NFmiQueryDatan itsLatLonCache on alustettu!!

      boost::thread_group calcParts;
      calcParts.add_thread(new boost::thread(::FillGridDataInThread,
                                             source3,
                                             target3,
                                             locationCacheMatrix,
                                             timeCacheVector,
                                             timeStart3,
                                             timeEnd3,
                                             3,
                                             theLogger));
      calcParts.add_thread(new boost::thread(::FillGridDataInThread,
                                             source2,
                                             target2,
                                             locationCacheMatrix,
                                             timeCacheVector,
                                             timeStart2,
                                             timeEnd2,
                                             2,
                                             theLogger));
      calcParts.add_thread(new boost::thread(::FillGridDataInThread,
                                             source1,
                                             target1,
                                             locationCacheMatrix,
                                             timeCacheVector,
                                             timeStart1,
                                             timeEnd1,
                                             1,
                                             theLogger));
      calcParts.join_all();  // odotetaan että threadit lopettavat
    }
#endif  // BOOST_DISABLE_THREADS
  }
}

static void FillSingleTimeGridDataInThread(
    NFmiFastQueryInfo &theSourceInfo,
    NFmiFastQueryInfo &theTargetInfo,
    bool fDoLocationInterpolation,
    const NFmiDataMatrix<NFmiLocationCache> &theLocationCacheMatrix,
    const checkedVector<NFmiTimeCache> &theTimeCacheVector,
    NFmiTimeIndexCalculator &theTimeIndexCalculator,
    int theThreadNumber,
    NFmiLogger *theDebugLogger)
{
  NFmiDataMatrix<float> gridValues;
  bool doGroundData =
      (theSourceInfo.SizeLevels() == 1) &&
      (theTargetInfo.SizeLevels() ==
       1);  // jos molemmissa datoissa vain yksi leveli, se voidaan jättää tarkastamatta
  unsigned long targetXSize = theTargetInfo.GridXNumber();
  unsigned long workedTimeIndex = 0;
  for (; theTimeIndexCalculator.GetCurrentTimeIndex(workedTimeIndex);)
  {
    if (theDebugLogger)
    {
      std::string logStr("FillSingleTimeGridDataInThread - thread no: ");
      logStr += NFmiStringTools::Convert(theThreadNumber);
      logStr += " started timeIndex: ";
      logStr += NFmiStringTools::Convert(workedTimeIndex);
      theDebugLogger->LogMessage(logStr, NFmiLogger::kDebugInfo);
    }

    if (theTargetInfo.TimeIndex(workedTimeIndex) == false) continue;
    NFmiMetTime targetTime = theTargetInfo.Time();
    bool doTimeInterpolation =
        false;  // jos aikaa ei löydy suoraan, tarvittaessa tehdään aikainterpolaatio
    if (theSourceInfo.Time(theTargetInfo.Time()) ||
        (doTimeInterpolation = theSourceInfo.TimeDescriptor().IsInside(theTargetInfo.Time())) ==
            true)
    {
      const NFmiTimeCache &timeCache = theTimeCacheVector[theTargetInfo.TimeIndex()];
      for (theTargetInfo.ResetParam(); theTargetInfo.NextParam();)
      {
        if (theSourceInfo.Param(
                static_cast<FmiParameterName>(theTargetInfo.Param().GetParamIdent())))
        {
          for (theTargetInfo.ResetLevel(); theTargetInfo.NextLevel();)
          {
            if (doGroundData || theSourceInfo.Level(*theTargetInfo.Level()))
            {
              if (fDoLocationInterpolation == false)
              {
                if (doTimeInterpolation)
                  theSourceInfo.Values(gridValues, targetTime);
                else
                  theSourceInfo.Values(gridValues);

                theTargetInfo.SetValues(gridValues);
              }
              else
              {  // interpoloidaan paikan suhteen
                for (theTargetInfo.ResetLocation(); theTargetInfo.NextLocation();)
                {
                  float value = kFloatMissing;
                  const NFmiLocationCache &locCache =
                      theLocationCacheMatrix[theTargetInfo.LocationIndex() %
                                             targetXSize][theTargetInfo.LocationIndex() /
                                                          targetXSize];
                  if (fDoLocationInterpolation && doTimeInterpolation)
                    value = theSourceInfo.CachedInterpolation(locCache, timeCache);
                  else if (fDoLocationInterpolation)
                    value = theSourceInfo.CachedInterpolation(locCache);
                  theTargetInfo.FloatValue(value);
                }
              }
            }
          }
        }
      }
    }
  }
}

// Tämä on FillGridData-funktion viritetty versio, joka tekee työtä koneen kaikilla
// kone-threadeilla.
// Luodaan tarvittavat threadit jotka sitten pyytävät aina seuraavan käsiteltävän ajan indeksin.
// Työstö tapahtuu siis iksi aika kerrallaa omissa threadeissaan.
// Optimoitu myös niin että lasketaan valmiiksi kaikki tarvittavat location ja time interpolaatio
// cachet.
void NFmiQueryDataUtil::FillGridDataFullMT(NFmiQueryData *theSource,
                                           NFmiQueryData *theTarget,
                                           unsigned long theStartTimeIndex,
                                           unsigned long theEndTimeIndex,
                                           NFmiLogger *theDebugLogger)
{
  if (theSource && theTarget && theSource->IsGrid() && theTarget->IsGrid())
  {
    NFmiFastQueryInfo source1(theSource);
    NFmiFastQueryInfo target1(theTarget);
    bool doLocationInterpolation =
        (NFmiQueryDataUtil::AreGridsEqual(source1.Grid(), target1.Grid()) == false);
    NFmiDataMatrix<NFmiLocationCache> locationCacheMatrix;
    if (doLocationInterpolation) source1.CalcLatlonCachePoints(target1, locationCacheMatrix);
    checkedVector<NFmiTimeCache> timeCacheVector;
    source1.CalcTimeCache(target1, timeCacheVector);

    unsigned long usedStartTimeIndex = theStartTimeIndex;
    unsigned long usedEndTimeIndex = theEndTimeIndex;
    if (usedStartTimeIndex == gMissingIndex) usedStartTimeIndex = 0;
    if (usedEndTimeIndex == gMissingIndex) usedEndTimeIndex = target1.SizeTimes() - 1;

    unsigned int usedThreadCount = boost::thread::hardware_concurrency();
#ifdef UNIX
    // Using all CPUs with the algorithm below leads to severe cache
    // trashing and poor performance for all programs running simultaneously,
    // since time is the innermost data element in the 4D data cube.
    usedThreadCount /= 2;
#endif

    if (usedThreadCount > target1.SizeTimes()) usedThreadCount = target1.SizeTimes();

    if (usedThreadCount <= 1)
    {  // käytetään vanhaa simppelimpää funktiota kun threadauksesta ei ole hyötyä
      ::FillGridDataInThread(source1,
                             target1,
                             locationCacheMatrix,
                             timeCacheVector,
                             usedStartTimeIndex,
                             usedEndTimeIndex,
                             1,
                             theDebugLogger);
    }
    else
    {
      // HUOM! multi threaddauksessa otettava huomioon!!!!
      // NFmiQueryData-luokassa on itsLatLonCache, joka on samalla käytössä kaikilla
      // erillisillä Info-iteraattoreilla. Tämä on ns. lazy-init dataa, joka initialisoidaan
      // kerran
      // tarvittaessa. Nyt samoihin datoihin tehdään kolme info-iteraattoria ja tässä kutsumalla
      // yhdelle niistä
      // LatLon-metodia, varmistetaan että itsLatLonCache on initialisoitu, ennen kuin mennään
      // suorittamaan kolmea threadia yhtäaikaa. Tämä oli kaato bugi, kun kolme thtreadia kutsui
      // Latlon:ia
      // ja cache-data ei oltu vielä initialisoitu, jolloin sitä initialisoitiin kolme kertaa
      // päällekkäin.
      // Ongelma oli varsinkin isommille datoille (esim. mbe), joille kyseisen cachen rakentaminen
      // kestää kauemmin.
      NFmiPoint dummyPoint =
          target1.LatLon();  // Varmistetaan että NFmiQueryDatan itsLatLonCache on alustettu!!
      dummyPoint =
          source1.LatLon();  // Varmistetaan että NFmiQueryDatan itsLatLonCache on alustettu!!

      // pakko luoda dynaamisesti eri threadeille tarvittavat kopiot source ja target datoista
      std::vector<boost::shared_ptr<NFmiFastQueryInfo> > targetInfos(usedThreadCount);
      std::vector<boost::shared_ptr<NFmiFastQueryInfo> > sourceInfos(usedThreadCount);
      for (unsigned int i = 0; i < usedThreadCount; i++)
      {
        targetInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(target1));
        sourceInfos[i] = boost::shared_ptr<NFmiFastQueryInfo>(new NFmiFastQueryInfo(source1));
      }

      NFmiTimeIndexCalculator timeIndexCalculator(usedStartTimeIndex, usedEndTimeIndex);
      boost::thread_group calcParts;
      for (unsigned int i = 0; i < usedThreadCount; i++)
        calcParts.add_thread(new boost::thread(::FillSingleTimeGridDataInThread,
                                               *(sourceInfos[i].get()),
                                               *(targetInfos[i].get()),
                                               doLocationInterpolation,
                                               locationCacheMatrix,
                                               timeCacheVector,
                                               boost::ref(timeIndexCalculator),
                                               i,
                                               theDebugLogger));
      calcParts.join_all();  // odotetaan että threadit lopettavat
    }
  }
}

bool NFmiQueryDataUtil::AreGridsEqual(const NFmiGrid *theGrid1, const NFmiGrid *theGrid2)
{
  if (theGrid1 && theGrid2)
  {
    if (AreAreasEqual(theGrid1->Area(), theGrid2->Area()))
    {
      if (theGrid1->XNumber() == theGrid2->XNumber() && theGrid1->YNumber() == theGrid2->YNumber())
        return true;
    }
  }
  return false;
}

// tutkii onko area-tyypit samoja, ovatko nurkka pisteet samoja ja
// vielä erikoistapauksena onko orientaatio sama
bool NFmiQueryDataUtil::AreAreasEqual(const NFmiArea *theArea1, const NFmiArea *theArea2)
{
  if (theArea1 && theArea2)
  {
    if (theArea1->ClassId() == theArea2->ClassId())
    {
      // pitääkö pisteille antaa liikkuma varaa epätarkkuuden takia?
      if (theArea1->BottomLeftLatLon() == theArea2->BottomLeftLatLon() &&
          theArea1->TopRightLatLon() == theArea2->TopRightLatLon())
      {
        if (theArea1->ClassId() == kNFmiStereographicArea ||
            theArea1->ClassId() == kNFmiEquiDistArea || theArea1->ClassId() == kNFmiGnomonicArea)
        {
          if (static_cast<const NFmiAzimuthalArea *>(theArea1)->Orientation() ==
              static_cast<const NFmiAzimuthalArea *>(theArea2)->Orientation())
            return true;
          else
            return false;
        }
        else
          return true;
      }
    }
  }
  return false;
}

bool NFmiQueryDataUtil::AreAreasSameKind(const NFmiArea *theArea1, const NFmiArea *theArea2)
{
  if (theArea1 && theArea2)
  {
    if (theArea1->ClassId() == kNFmiStereographicArea || theArea1->ClassId() == kNFmiEquiDistArea ||
        theArea1->ClassId() == kNFmiGnomonicArea)
    {
      if (static_cast<const NFmiAzimuthalArea *>(theArea1)->Orientation() ==
          static_cast<const NFmiAzimuthalArea *>(theArea2)->Orientation())
        return true;
      else
        return false;
    }
    else if (theArea1->ClassId() == theArea2->ClassId())
      return true;
  }
  return false;
}

NFmiQueryData *NFmiQueryDataUtil::ReadNewestData(const std::string &theFileFilter)
{
  std::string tmpFileName;
  std::time_t timeOfFile = NFmiFileSystem::FindFile(theFileFilter, true, &tmpFileName);
  if (timeOfFile == 0) return 0;
  std::string foundFileName = NFmiFileSystem::PathFromPattern(theFileFilter);
  if (foundFileName.empty() == false) foundFileName += kFmiDirectorySeparator;
  foundFileName += tmpFileName;
  NFmiQueryData *data = new NFmiQueryData(foundFileName, true);
  return data;
}

// Halutaan laskea eri tehtäviä varten optimaalinen threadien käyttö.
// Esim. soundingIndex laskut tehdään haluttaessa niin että otetaan käyttöön n kpl
// työthreadeja jotka laskevat kerrallaa yksittäisen aika-askeleen.
// Jos käytössä olisi esim. 8 threadia ja aika-askelia olisi 17 tapahtuisi
// normaalisti seuraavaa:
// 8 threadia laskisisvat ensin 8 ensimmäistä aika-askelta, sitten 8 seuraavaa
// aika-askelta ja lopuksi yksi threadi laskisi viimeistä aika-askelta.
// Tämä on tuhlausta, koska työskentely kestäisi 3-työskentely yksikköä, mutta
// kaksi ensimmäistä veisi CPU:n rehut 100% ja lopuksi 1/8 osan.
// Jos työhön valjastettaisiin 6 threadia, työn kesto olisi sama 3 yksikköä
// mutta CPU:ta käytettäisiin 6/8, 6/8 ja 5/8 työskentelyn aikana, jolloin
// CPU:lla ei olisi yli kuormitettu turhaa kahden ensimmäisen työskentely syklin aikana.
int NFmiQueryDataUtil::CalcOptimalThreadCount(int maxAvailableThreads, int separateTaskCount)
{
  if (maxAvailableThreads <= 1)
    return 1;  // Esim. jos käyttäjä on pyytänyt maxthread-1::lla arvoa ja CPU:ssa on vain 1
               // threadi
               // käytössä
  if (maxAvailableThreads >= separateTaskCount) return separateTaskCount;
  if (maxAvailableThreads == 2) return 2;  // turha tälle oikeastaan laskea mitään

  double ratio = static_cast<double>(separateTaskCount) / maxAvailableThreads;
  int wantedIntegerPart = static_cast<int>(ratio);
  if (ratio == wantedIntegerPart)
    return maxAvailableThreads;  // jos jakosuhteeksi tuli kokonaisluku, käytetään kaikkia
                                 // annettuja
                                 // threadeja

  // Jos ei löytynyt tasalukuja, pitää iteroida semmoinen ratio, jolla saadaan mahdollisimman iso
  // kokonaisluku,
  // jossa on mahdollisimman iso murto-osa siis esim. 6.92 (6.92 on parempi kuin vaikka 6.34)
  // double maxRatio = ratio;
  int maxRatioThreadcount = maxAvailableThreads;
  for (int threadCount = maxAvailableThreads - 1; threadCount > 1; threadCount--)
  {
    double ratio2 = static_cast<double>(separateTaskCount) / threadCount;
    int wantedIntegerPart2 = static_cast<int>(ratio2);
    if (wantedIntegerPart2 == wantedIntegerPart && ratio2 > ratio)
    {
      // maxRatio = ratio2;
      maxRatioThreadcount = threadCount;
    }
    if (wantedIntegerPart2 == wantedIntegerPart + 1 && ratio2 == wantedIntegerPart2)
    {  // löydettiin jakosuhde, joka käyttää vajaata threadi määrää täysillä esim. maxThread = 4,
      // taskcount = 6, tällöin optimaali on 3 threadia
      // maxRatio = ratio2;
      maxRatioThreadcount = threadCount;
      break;
    }
    if (wantedIntegerPart2 > wantedIntegerPart) break;
  }
  return maxRatioThreadcount;
}
