#pragma once

#include <newbase/NFmiInfoAreaMask.h>

#include <boost/thread.hpp>

class NFmiDataModifier;
class NFmiDataIterator;
class NFmiFastQueryInfo;
class NFmiGriddingHelperInterface;
class NFmiDrawParam;

// tämä maski osaa laskea halutulle asemadatalle hilatut arvot halutulle alueelle
// Jos maskin itsInfo on station-dataa, sen laskut tehdään toisella tavalla kuin 'normaalin'
// hila-datan kanssa
// 1. Se pitää initilisoida kerran joka erillistä aikaa kohden eli lasketaan matriisiin valmiiksi
// kaikki arvot kerralla
// 2. Kun maskin arvoja pyydetään Value-metodissa, ne saadaan valmiiksi lasketusta taulukosta (aika
// initialisointi voi tapahtua myös siellä)
class NFmiStation2GridMask : public NFmiInfoAreaMask
{
  typedef std::map<NFmiMetTime, NFmiDataMatrix<float> > DataCache;

 public:
  using GriddingFunctionCallBackType = std::function<void(NFmiGriddingHelperInterface *,
                                                          const boost::shared_ptr<NFmiArea> &,
                                                          boost::shared_ptr<NFmiDrawParam> &,
                                                          NFmiDataMatrix<float> &,
                                                          const NFmiMetTime &,
                                                          float)>;

  NFmiStation2GridMask(Type theMaskType,
                       NFmiInfoData::Type theDataType,
                       boost::shared_ptr<NFmiFastQueryInfo> &theInfo);
  ~NFmiStation2GridMask(void);
  NFmiStation2GridMask(const NFmiStation2GridMask &theOther);
  NFmiAreaMask *Clone(void) const;

  double Value(const NFmiCalculationParams &theCalculationParams, bool fUseTimeInterpolationAlways);
  void SetGriddingHelpers(NFmiArea *theArea,
                          NFmiGriddingHelperInterface *theGriddingHelper,
                          const NFmiPoint &theStation2GridSize,
                          float theObservationRadiusRelative);
  static void SetGriddingStationDataCallback(
      GriddingFunctionCallBackType theGridStationDataCallback)
  {
    itsGridStationDataCallback = theGridStationDataCallback;
  }

 private:
  void DoGriddingCheck(const NFmiCalculationParams &theCalculationParams);

  //  NFmiDataMatrix<float> itsGriddedStationData; // tässä on asemadatasta lasketut hilatut arvot
  boost::shared_ptr<DataCache> itsGriddedStationData;   // Tämä jaetaan kaikkien kopioiden kesken,
                                                        // jotta multi-thread -koodi saa jaettua
                                                        // työtä
  NFmiDataMatrix<float> *itsCurrentGriddedStationData;  // tähän on laitettu se matriisi, joka
                                                        // sisältää halutun ajan asemadatasta
                                                        // lasketut hilatut arvot
  NFmiMetTime itsLastCalculatedTime;  // tälle ajanhetkelle on station data laskettu (tai puuttuva
                                      // aika), mutta onko se sama kuin itsTime, jos ei ole, pitää
                                      // laskea juuri tälle ajalle

  // Näille muuttujille pitää asettaa arvot erillisellä SetGridHelpers-funktiolla
  boost::shared_ptr<NFmiArea> itsAreaPtr;  // omistaa ja tuhoaa!!
  NFmiGriddingHelperInterface *itsGriddingHelper;
  NFmiPoint itsStation2GridSize;  // tämän kokoiseen hilaan asema data lasketaan
                                  // (itsGriddedStationData -koko)
  // Normaalisti havainto laskuissa ei rajoiteta käytettyjä havaintoja etäisyyden perusteellä.
  // Jos tähän annetaan jotain kFloatMissing:istä poikkeavaa, niin silloin rajoitetaan.
  float itsObservationRadiusRelative;

  // Kun itsCurrentGriddedStationData -muuttujaa lasketaan tai asetetaan, sen saa tehdä kullekin
  // ajalle vain kerran. Tämä lukko systeemi takaa sen.
  typedef boost::shared_mutex MutexType;
  typedef boost::shared_lock<MutexType>
      ReadLock;  // Read-lockia ei oikeasti tarvita, mutta laitan sen tähän, jos joskus tarvitaankin
  typedef boost::unique_lock<MutexType> WriteLock;
  // TÄMÄ jaetaan kaikkien kopioiden kesken, jotta multi-thread -koodi saa jaettua työtä
  boost::shared_ptr<MutexType> itsCacheMutex;
  // Callback funktio asemadatan griddaus funktioon
  static GriddingFunctionCallBackType itsGridStationDataCallback;
};

// NFmiNearestObsValue2GridMask -luokka laskee havainto datasta sellaisen
// hilan, mihin sijoitetaan kuhunkin hilapisteeseen vain sitä lähimmän aseman
// arvon (oli se puuttuvaa tai ei). Eli kaikkiin hilapisteisiin ei tule arvoa.
// Tämän avulla on tarkoitus voida tehdä 'asemapiste' -laskuja ja visualisoida
// niitä kartalla vain teksti muodossa. Tällöin teksti tulee näkyviin kartalla
// lähellä kunkin aseman omaa pistettä ja muut hilapisteet ovat puuttuvaa joten
// siihen ei tule näkyviin mitään.
class NFmiNearestObsValue2GridMask : public NFmiInfoAreaMask
{
  typedef std::map<NFmiMetTime, NFmiDataMatrix<float> > DataCache;

 public:
  NFmiNearestObsValue2GridMask(Type theMaskType,
                               NFmiInfoData::Type theDataType,
                               boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                               int theArgumentCount);
  ~NFmiNearestObsValue2GridMask(void);
  NFmiNearestObsValue2GridMask(const NFmiNearestObsValue2GridMask &theOther);
  NFmiAreaMask *Clone(void) const;

  double Value(const NFmiCalculationParams &theCalculationParams, bool fUseTimeInterpolationAlways);
  void SetGriddingHelpers(NFmiArea *theArea,
                          NFmiGriddingHelperInterface *theGriddingHelper,
                          const NFmiPoint &theResultGridSize);
  void SetArguments(std::vector<float> &theArgumentVector);

 private:
  void DoNearestValueGriddingCheck(const NFmiCalculationParams &theCalculationParams);

  boost::shared_ptr<DataCache> itsNearestObsValuesData;  // Tämä jaetaan kaikkien kopioiden kesken,
                                                         // jotta multi-thread -koodi saa jaettua
                                                         // työtä
  NFmiDataMatrix<float> *itsCurrentNearestObsValuesData;  // tähän on laitettu se matriisi, joka
                                                          // sisältää halutun ajan asemadatasta
                                                          // lasketut hilatut arvot
  NFmiMetTime itsLastCalculatedTime;  // tälle ajanhetkelle on station data laskettu (tai puuttuva
                                      // aika), mutta onko se sama kuin itsTime, jos ei ole, pitää
                                      // laskea juuri tälle ajalle

  // Näille muuttujille pitää asettaa arvot erillisellä SetGridHelpers-funktiolla
  boost::shared_ptr<NFmiArea> itsAreaPtr;  // omistaa ja tuhoaa!!
  NFmiGriddingHelperInterface *itsGriddingHelper;
  NFmiPoint itsResultGridSize;  // tämän kokoiseen hilaan asema data lasketaan
                                // (itsNearestObsValuesData -hilakoko)

  std::vector<float> itsArgumentVector;  // tähän lasketaan lennossa laskuissa tarvittavat
                                         // argumentit (1. aikahyppy)

  // Kun itsCurrentNearestObsValuesData -muuttujaa lasketaan tai asetetaan, sen saa tehdä kullekin
  // ajalle vain kerran. Tämä lukko systeemi takaa sen.
  typedef boost::shared_mutex MutexType;
  typedef boost::shared_lock<MutexType>
      ReadLock;  // Read-lockia ei oikeasti tarvita, mutta laitan sen tähän, jos joskus tarvitaankin
  typedef boost::unique_lock<MutexType> WriteLock;
  boost::shared_ptr<MutexType> itsCacheMutex;  // TÄMÄ jaetaan kaikkien kopioiden kesken, jotta
                                               // multi-thread -koodi saa jaettua työtä
};
