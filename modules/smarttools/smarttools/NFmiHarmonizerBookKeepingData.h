#ifndef NFMIHARMONIZERBOOKKEEPINGDATA_H
#define NFMIHARMONIZERBOOKKEEPINGDATA_H

#include <newbase/NFmiMetTime.h>
#include <newbase/NFmiParamBag.h>
#include <set>

// smartinfon pitää pitää kirjaa harmonisaattoriin liittyvistä asioista
// mitkä parametrit ja ajat ovat olleet milläkin hetkellä muokatttuina
class NFmiHarmonizerBookKeepingData
{
 public:
  NFmiHarmonizerBookKeepingData(void);
  NFmiHarmonizerBookKeepingData(const std::set<NFmiMetTime> &theHarmonizerTimesSet,
                                bool foobar,
                                const NFmiParamBag &theHarmonizerParams);
  ~NFmiHarmonizerBookKeepingData(void);

  NFmiParamBag &HarmonizerParams(void) { return itsHarmonizerParams; }
  void HarmonizerParams(const NFmiParamBag &theParamBag) { itsHarmonizerParams = theParamBag; }
  void InsertTime(const NFmiMetTime &theTime);
  bool HarmonizeAllTimes(void) const { return fHarmonizeAllTimes; }
  void HarmonizeAllTimes(bool newValue) { fHarmonizeAllTimes = newValue; }
  std::set<NFmiMetTime> &HarmonizerTimesSet(void) { return itsHarmonizerTimesSet; }
 private:
  std::set<NFmiMetTime>
      itsHarmonizerTimesSet;  // tänne laitetaan kaikki muokatut ajat jotta voidaan
                              // harmonisoinnin yhteydessä tehdä timedesc, jonka avulla ajetaan
                              // harmonisaattori skripti
  bool fHarmonizeAllTimes;  // apuna edellisen set:in lisäksi, että jos käydään läpi koko data
  NFmiParamBag
      itsHarmonizerParams;  // tähän merkitään parametrit, joita on muokattu eri työkaluilla.
};

#endif  // NFMIHARMONIZERBOOKKEEPINGDATA_H
