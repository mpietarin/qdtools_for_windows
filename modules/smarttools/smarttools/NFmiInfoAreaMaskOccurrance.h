#pragma once

#include <newbase/NFmiInfoAreaMask.h>

class NFmiDrawParam;

class _FMI_DLL NFmiInfoAreaMaskOccurrance : public NFmiInfoAreaMaskProbFunc
{
 public:
  ~NFmiInfoAreaMaskOccurrance(void);
  NFmiInfoAreaMaskOccurrance(const NFmiCalculationCondition &theOperation,
                             Type theMaskType,
                             NFmiInfoData::Type theDataType,
                             const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             NFmiAreaMask::FunctionType thePrimaryFunc,
                             NFmiAreaMask::FunctionType theSecondaryFunc,
                             int theArgumentCount,
                             const boost::shared_ptr<NFmiArea> &theCalculationArea,
                             bool synopXCase);
  NFmiInfoAreaMaskOccurrance(const NFmiInfoAreaMaskOccurrance &theOther);
  void Initialize(void);  // Tätä kutsutaan konstruktorin jälkeen, tässä alustetaan tietyille
                          // datoille mm. käytetyt aikaindeksit ja käytetyt locaaion indeksit
  NFmiAreaMask *Clone(void) const;

  static void SetMultiSourceDataGetterCallback(
      const std::function<void(checkedVector<boost::shared_ptr<NFmiFastQueryInfo>> &,
                               boost::shared_ptr<NFmiDrawParam> &,
                               const boost::shared_ptr<NFmiArea> &)> &theCallbackFunction);

  // tätä kaytetaan smarttool-modifierin yhteydessä
  double Value(const NFmiCalculationParams &theCalculationParams, bool fUseTimeInterpolationAlways);

 protected:
  bool IsGridData() const;
  bool IsKnownMultiSourceData();  // nyt synop ja salama datat ovat tälläisiä
  void DoCalculations(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                      const NFmiCalculationParams &theCalculationParams,
                      const NFmiLocation &theLocation,
                      const std::vector<unsigned long> &theLocationIndexCache,
                      int &theOccurranceCountInOut);
  void DoCalculateCurrentLocation(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                  const NFmiLocation &theLocation,
                                  bool theIsStationLocationsStoredInData,
                                  int &theOccurranceCountInOut);
  void InitializeLocationIndexCaches();
  std::vector<unsigned long> CalcLocationIndexCache(boost::shared_ptr<NFmiFastQueryInfo> &theInfo);

  bool fSynopXCase;  // halutaanko vain normaali asemat (true), ei liikkuvia asemia (laivat, poijut)
  bool fUseMultiSourceData;
  boost::shared_ptr<NFmiArea> itsCalculationArea;  // Joitain laskuja optimoidaan ja niillä
                                                   // lähdedatasta laskut rajataan laskettavan
                                                   // kartta-alueen sisälle
  checkedVector<boost::shared_ptr<NFmiFastQueryInfo>> itsInfoVector;  // Tähän laitetaan laskuissa
  // käytettävät datat, joko se
  // joko on jo emoluokassa
  // oleva itsInfo, tai
  // multisource tapauksissa
  // joukko datoja

  // Jokaiselle käytössä olevalle datalle lasketaan locationIndex cache, eli kaikki ne pisteet
  // kustakin datasta,
  // joita käytetään laskuissa. Jos jollekin datalle on tyhjä vektori, lasketaan siitä kaikki. Jos
  // jostain datasta
  // ei käytetä yhtään pistettä, on siihen kuuluvassa vektorissa vain yksi luku (gMissingIndex).
  // Tämä alustetaan Initialize -metodissa.
  std::vector<std::vector<unsigned long>> itsCalculatedLocationIndexies;

  static std::function<void(checkedVector<boost::shared_ptr<NFmiFastQueryInfo>> &,
                            boost::shared_ptr<NFmiDrawParam> &,
                            const boost::shared_ptr<NFmiArea> &)> itsMultiSourceDataGetter;

 private:
  NFmiInfoAreaMaskProbFunc &operator=(const NFmiInfoAreaMaskProbFunc &theMask);
};
