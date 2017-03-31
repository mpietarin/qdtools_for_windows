#include "NFmiStation2GridMask.h"
#include "NFmiDrawParam.h"
#include "NFmiGriddingHelperInterface.h"
#include <newbase/NFmiFastInfoUtils.h>
#include <newbase/NFmiFastQueryInfo.h>

// ****************************************************************************
// ****************** NFmiStation2GridMask ************************************
// ****************************************************************************

NFmiStation2GridMask::GriddingFunctionCallBackType NFmiStation2GridMask::itsGridStationDataCallback;

NFmiStation2GridMask::NFmiStation2GridMask(Type theMaskType,
                                           NFmiInfoData::Type theDataType,
                                           boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
    : NFmiInfoAreaMask(
          NFmiCalculationCondition(), theMaskType, theDataType, theInfo, NFmiAreaMask::kNoValue),
      itsGriddedStationData(new DataCache()),
      itsCurrentGriddedStationData(0),
      itsLastCalculatedTime(NFmiMetTime::gMissingTime),
      itsAreaPtr(),
      itsGriddingHelper(0),
      itsStation2GridSize(1, 1),
      itsObservationRadiusRelative(kFloatMissing),
      itsCacheMutex(new MutexType())
{
}

NFmiStation2GridMask::~NFmiStation2GridMask(void)
{
}

NFmiStation2GridMask::NFmiStation2GridMask(const NFmiStation2GridMask &theOther)
    : NFmiInfoAreaMask(theOther),
      itsGriddedStationData(theOther.itsGriddedStationData),
      itsCurrentGriddedStationData(0)  // tämä laitetaan aina 0:ksi
      ,
      itsLastCalculatedTime(theOther.itsLastCalculatedTime),
      itsAreaPtr(theOther.itsAreaPtr.get() ? theOther.itsAreaPtr.get()->Clone() : 0),
      itsGriddingHelper(theOther.itsGriddingHelper),
      itsStation2GridSize(theOther.itsStation2GridSize),
      itsObservationRadiusRelative(theOther.itsObservationRadiusRelative),
      itsCacheMutex(theOther.itsCacheMutex)
{
}

NFmiAreaMask *NFmiStation2GridMask::Clone(void) const
{
  return new NFmiStation2GridMask(*this);
}

double NFmiStation2GridMask::Value(const NFmiCalculationParams &theCalculationParams,
                                   bool /* fUseTimeInterpolationAlways */)
{
  DoGriddingCheck(theCalculationParams);
  if (itsCurrentGriddedStationData)
    return itsCurrentGriddedStationData->GetValue(theCalculationParams.itsLocationIndex,
                                                  kFloatMissing);
  else
    return kFloatMissing;
}

void NFmiStation2GridMask::SetGriddingHelpers(NFmiArea *theArea,
                                              NFmiGriddingHelperInterface *theGriddingHelper,
                                              const NFmiPoint &theStation2GridSize,
                                              float theObservationRadiusRelative)
{
  itsAreaPtr.reset(theArea->Clone());
  itsGriddingHelper = theGriddingHelper;
  itsStation2GridSize = theStation2GridSize;
  itsObservationRadiusRelative = theObservationRadiusRelative;
}

void NFmiStation2GridMask::DoGriddingCheck(const NFmiCalculationParams &theCalculationParams)
{
  if (itsLastCalculatedTime != theCalculationParams.itsTime)
  {
    WriteLock lock(*itsCacheMutex);  // tästä saa edetä vain yksi säie kerrallaan, eli joku joka
                                     // ehtii, laskee cache-matriisin ja asettaa sen
                                     // itsCurrentGriddedStationData:n arvoksi, muut sitten vain
                                     // käyttävät sitä

    // katsotaanko löytyykö valmiiksi laskettua hilaa halutulle ajalle
    DataCache::iterator it = itsGriddedStationData->find(theCalculationParams.itsTime);
    if (it != itsGriddedStationData->end())
      itsCurrentGriddedStationData = &((*it).second);
    else
    {
      // lasketaan halutun ajan hila
      if (itsGriddingHelper && itsAreaPtr.get())
      {
        boost::shared_ptr<NFmiDrawParam> drawParam(
            new NFmiDrawParam(itsDataIdent, itsLevel, 0, itsDataType));
        NFmiDataMatrix<float> griddedData(
            static_cast<NFmiDataMatrix<float>::size_type>(itsStation2GridSize.X()),
            static_cast<NFmiDataMatrix<float>::size_type>(itsStation2GridSize.Y()),
            kFloatMissing);
        if (itsGridStationDataCallback)
          itsGridStationDataCallback(itsGriddingHelper,
                                     itsAreaPtr,
                                     drawParam,
                                     griddedData,
                                     theCalculationParams.itsTime,
                                     itsObservationRadiusRelative);
        std::pair<DataCache::iterator, bool> insertResult = itsGriddedStationData->insert(
            std::make_pair(theCalculationParams.itsTime, griddedData));
        if (insertResult.second)
          itsCurrentGriddedStationData = &((*insertResult.first).second);
        else
          itsCurrentGriddedStationData = 0;
      }
    }
    itsLastCalculatedTime = theCalculationParams.itsTime;
  }
}
// ****************************************************************************
// ****************** NFmiStation2GridMask ************************************
// ****************************************************************************

// ****************************************************************************
// *************** NFmiNearestObsValue2GridMask *******************************
// ****************************************************************************

NFmiNearestObsValue2GridMask::NFmiNearestObsValue2GridMask(
    Type theMaskType,
    NFmiInfoData::Type theDataType,
    boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    int theArgumentCount)
    : NFmiInfoAreaMask(
          NFmiCalculationCondition(), theMaskType, theDataType, theInfo, NFmiAreaMask::kNoValue),
      itsNearestObsValuesData(new DataCache()),
      itsCurrentNearestObsValuesData(0),
      itsLastCalculatedTime(NFmiMetTime::gMissingTime),
      itsAreaPtr(),
      itsGriddingHelper(0),
      itsResultGridSize(1, 1),
      itsCacheMutex(new MutexType())
{
  itsFunctionArgumentCount = theArgumentCount;
}

NFmiNearestObsValue2GridMask::~NFmiNearestObsValue2GridMask(void)
{
}

NFmiNearestObsValue2GridMask::NFmiNearestObsValue2GridMask(
    const NFmiNearestObsValue2GridMask &theOther)
    : NFmiInfoAreaMask(theOther),
      itsNearestObsValuesData(theOther.itsNearestObsValuesData),
      itsCurrentNearestObsValuesData(0),  // tämä laitetaan aina 0:ksi
      itsLastCalculatedTime(theOther.itsLastCalculatedTime),
      itsAreaPtr(theOther.itsAreaPtr.get() ? theOther.itsAreaPtr.get()->Clone() : 0),
      itsGriddingHelper(theOther.itsGriddingHelper),
      itsResultGridSize(theOther.itsResultGridSize),
      itsCacheMutex(theOther.itsCacheMutex)
{
}

NFmiAreaMask *NFmiNearestObsValue2GridMask::Clone(void) const
{
  return new NFmiNearestObsValue2GridMask(*this);
}

double NFmiNearestObsValue2GridMask::Value(const NFmiCalculationParams &theCalculationParams,
                                           bool /* fUseTimeInterpolationAlways */)
{
  DoNearestValueGriddingCheck(theCalculationParams);
  if (itsCurrentNearestObsValuesData)
    return itsCurrentNearestObsValuesData->GetValue(theCalculationParams.itsLocationIndex,
                                                    kFloatMissing);
  else
    return kFloatMissing;
}

void NFmiNearestObsValue2GridMask::SetGriddingHelpers(
    NFmiArea *theArea,
    NFmiGriddingHelperInterface *theGriddingHelper,
    const NFmiPoint &theResultGridSize)
{
  itsAreaPtr.reset(theArea->Clone());
  itsGriddingHelper = theGriddingHelper;
  itsResultGridSize = theResultGridSize;
}

void NFmiNearestObsValue2GridMask::SetArguments(std::vector<float> &theArgumentVector)
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

static NFmiDataMatrix<float> CalcNearestValueMatrix(
    const NFmiLevel &theLevel,
    const NFmiCalculationParams &theCalculationParams,
    const NFmiPoint &theResultGridSize,
    checkedVector<boost::shared_ptr<NFmiFastQueryInfo> > &theInfoVector,
    boost::shared_ptr<NFmiArea> &theAreaPtr,
    float theTimePeekInHours)
{
  // Luodaan tulos matriisi täytettynä puuttuvilla arvoilla
  NFmiDataMatrix<float> nearestValueMatrix(
      static_cast<NFmiDataMatrix<float>::size_type>(theResultGridSize.X()),
      static_cast<NFmiDataMatrix<float>::size_type>(theResultGridSize.Y()),
      kFloatMissing);
  if (theAreaPtr && theInfoVector.size())
  {
    const double kMissingDistanceValue =
        9999999999.;  // etäisyys matriisi alustetaan tällä suurella luvulla, jolloin aina riittää
                      // yksi < -vertailu, kun etsitään lähintä asemaa
    NFmiDataMatrix<double> distanceMatrix(
        nearestValueMatrix.NX(), nearestValueMatrix.NY(), kMissingDistanceValue);
    NFmiGrid grid(theAreaPtr.get(),
                  static_cast<unsigned long>(nearestValueMatrix.NX()),
                  static_cast<unsigned long>(nearestValueMatrix.NY()));
    for (auto infoIter : theInfoVector)
    {
      // data ei saa olla hiladataa, eikä ns. laivadataa (lokaatio muuttuu ajan myötä ja lat/lon
      // arvot ovat erillisiä parametreja)
      if (!infoIter->IsGrid() && !NFmiFastInfoUtils::IsInfoShipTypeData(*infoIter))
      {
        NFmiMetTime wantedTime(theCalculationParams.itsTime);
        if (theTimePeekInHours)
        {  // jos halutaan kurkata ajassa eteen/taakse, tehdään ajan asetus tässä
          long changeByMinutesValue = FmiRound(theTimePeekInHours * 60);
          wantedTime.ChangeByMinutes(changeByMinutesValue);
        }
        if (infoIter->Time(wantedTime))
        {
          for (infoIter->ResetLocation(); infoIter->NextLocation();)
          {
            const NFmiLocation *location = infoIter->Location();
            const NFmiPoint &latlon(location->GetLocation());
            if (theAreaPtr->IsInside(latlon))  // aseman pitää olla kartta-alueen sisällä
            {
              // Katsotaan mitä tuloshilan hilapistettä lähinnä tämä asema oli.
              if (grid.NearestLatLon(latlon.X(), latlon.Y()))
              {
                double distance = infoIter->Location()->Distance(grid.LatLon());
                if (distance < distanceMatrix[grid.CurrentX()][grid.CurrentY()])
                {
                  distanceMatrix[grid.CurrentX()][grid.CurrentY()] = distance;
                  NFmiFastInfoUtils::SetSoundingDataLevel(
                      theLevel, *infoIter);  // Tämä tehdään vain luotaus datalle: tämä level pitää
                                             // asettaa joka pisteelle erikseen, koska vakio
                  // painepinnat eivät ole kaikille luotaus parametreille
                  // samoilla leveleillä
                  nearestValueMatrix[grid.CurrentX()][grid.CurrentY()] =
                      infoIter->FloatValue();  // ei ole väliä onko lähimmän aseman arvo puuttuva
                                               // vai ei, se halutaan aina tulokseen!!!
                }
              }
            }
          }
        }
      }
    }
  }
  return nearestValueMatrix;
}

void NFmiNearestObsValue2GridMask::DoNearestValueGriddingCheck(
    const NFmiCalculationParams &theCalculationParams)
{
  if (itsLastCalculatedTime != theCalculationParams.itsTime)
  {
    WriteLock lock(*itsCacheMutex);  // tästä saa edetä vain yksi säie kerrallaan, eli joku joka
                                     // ehtii, laskee cache-matriisin ja asettaa sen
                                     // itsCurrentGriddedStationData:n arvoksi, muut sitten vain
                                     // käyttävät sitä

    // katsotaanko löytyykö valmiiksi laskettua hilaa halutulle ajalle
    DataCache::iterator it = itsNearestObsValuesData->find(theCalculationParams.itsTime);
    if (it != itsNearestObsValuesData->end())
      itsCurrentNearestObsValuesData = &((*it).second);
    else
    {
      // lasketaan halutun ajan hila
      if (itsGriddingHelper && itsAreaPtr.get())
      {
        // otetaan argumenttina annettu arvo aika hyppäykseen
        float timePeekInHours = itsArgumentVector[0];  // kuinka paljon kurkataan ajassa
                                                       // eteen/taakse kun arvoa haetaan tähän
                                                       // ajanhetkeen

        boost::shared_ptr<NFmiDrawParam> drawParam(
            new NFmiDrawParam(itsDataIdent, itsLevel, 0, itsDataType));
        checkedVector<boost::shared_ptr<NFmiFastQueryInfo> > infoVector;  // tähän haetaan
                                                                          // tarvittavat datat
                                                                          // (synopin tapauksessa
                                                                          // mahdollisesti lista)
        itsGriddingHelper->MakeDrawedInfoVectorForMapView(infoVector, drawParam, itsAreaPtr);
        NFmiDataMatrix<float> nearestValueData = ::CalcNearestValueMatrix(itsLevel,
                                                                          theCalculationParams,
                                                                          itsResultGridSize,
                                                                          infoVector,
                                                                          itsAreaPtr,
                                                                          timePeekInHours);
        std::pair<DataCache::iterator, bool> insertResult = itsNearestObsValuesData->insert(
            std::make_pair(theCalculationParams.itsTime, nearestValueData));
        if (insertResult.second)
          itsCurrentNearestObsValuesData = &((*insertResult.first).second);
        else
          itsCurrentNearestObsValuesData = 0;
      }
    }
    itsLastCalculatedTime = theCalculationParams.itsTime;
  }
}
// ****************************************************************************
// *************** NFmiNearestObsValue2GridMask *******************************
// ****************************************************************************
