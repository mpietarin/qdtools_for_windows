#pragma once
// ======================================================================
/*!
 * \file NFmiIgnoreStationsData.h
 * \brief Class keeps list of observation stations that should be ignored
 * in certain situations like drawing on map (due errourness observations).
 */
// ======================================================================

#include <string>
#include <list>

class NFmiLocation;

class NFmiIgnoreStation
{
 public:
  NFmiIgnoreStation(void);
  std::string MakeStationString(void);
  bool IsRange(void) const;
  bool GetIdValues(const std::string &theStationIdstr);

  unsigned long itsId;   // halutun aseman id (wmo tai muu vastaava)
  unsigned long itsId2;  // jos t�ss� on arvo, kyse on rangesta joka kattaa kaikki asemat v�lilt�

  std::string itsName;  // haetaan id:t� vastaava nimi, jos l�ytyy
  unsigned long
      itsLastLocationIndex;  // t�lt� kohdalta asema l�ytyi viimeksi jostain datasta (optimointia)
  bool fEnabled;             // onko t�m� nyt k�yt�ss� vai ei
};

class NFmiIgnoreStationsData
{
 public:
  NFmiIgnoreStationsData(void);

  void InitFromSettings(const std::string &theBaseNameSpace);
  void StoreToSettings(void);
  void Clear(void);
  void Add(const NFmiIgnoreStation &theStation);
  void Remove(unsigned long theId);
  bool IgnoreStationsDialogOn(void) const { return fIgnoreStationsDialogOn; }
  void IgnoreStationsDialogOn(bool newValue) { fIgnoreStationsDialogOn = newValue; }
  bool UseListWithContourDraw(void) const { return fUseListWithContourDraw; }
  void UseListWithContourDraw(bool newValue) { fUseListWithContourDraw = newValue; }
  bool UseListWithSymbolDraw(void) const { return fUseListWithSymbolDraw; }
  void UseListWithSymbolDraw(bool newValue) { fUseListWithSymbolDraw = newValue; }
  std::list<NFmiIgnoreStation> &StationList(void) { return itsStationList; }
  bool IsStationBlocked(const NFmiLocation &theLocation, bool theSymbolCase);
  bool IsIdInList(unsigned long theStationId);

 private:
  std::string MakeStationListString(void);
  void AddStationsFromString(const std::string &theStationListStr);

  bool fIgnoreStationsDialogOn;  // onko dialogi n�kyviss� vai ei
  bool fUseListWithContourDraw;
  bool fUseListWithSymbolDraw;
  std::list<NFmiIgnoreStation> itsStationList;

  std::string itsBaseNameSpace;
};
