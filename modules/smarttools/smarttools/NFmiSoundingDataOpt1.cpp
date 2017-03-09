// ======================================================================
/*!
 * \file NFmiSoundingDataOpt1.cpp
 *
 * Apuluokka laskemaan ja tutkimaan luotaus dataa. Osaa täyttää itsensä
 * mm. mallipinta QueryDatasta (infosta).
 */
// ======================================================================

#include "NFmiSoundingDataOpt1.h"
#include "NFmiSoundingFunctions.h"
#include <newbase/NFmiAngle.h>
#include <newbase/NFmiDataModifierAvg.h>
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiInterpolation.h>
#include <newbase/NFmiValueString.h>
#include <newbase/NFmiQueryDataUtil.h>

#include <fstream>

// hakee lähimmän sopivan painepinnan, mistä löytyy halutuille parametreille arvot
// Mutta ei sallita muokkausta ennen 1. validia leveliä!
bool NFmiSoundingDataOpt1::GetTandTdValuesFromNearestPressureLevel(double P,
                                                                   double &theFoundP,
                                                                   double &theT,
                                                                   double &theTd)
{
  if (P != kFloatMissing)
  {
    std::deque<float> &pV = GetParamData(kFmiPressure);
    std::deque<float> &tV = GetParamData(kFmiTemperature);
    std::deque<float> &tdV = GetParamData(kFmiDewPoint);
    if (pV.size() > 0)  // oletus että parV on saman kokoinen kuin pV -vektori
    {
      double minDiffP = 999999;
      theFoundP = 999999;
      theT = kFloatMissing;
      theTd = kFloatMissing;
      bool foundLevel = false;
      for (int i = 0; i < static_cast<int>(pV.size()); i++)
      {
        if (i < 0)
          return false;  // jos 'tyhjä' luotaus, on tässä aluksi -1 indeksinä
        if (pV[i] != kFloatMissing)
        {
          double pDiff = ::fabs(pV[i] - P);
          if (pDiff < minDiffP)
          {
            theFoundP = pV[i];
            minDiffP = pDiff;
            theT = tV[i];
            theTd = tdV[i];
            foundLevel = true;
          }
        }
      }
      if (foundLevel)
        return true;

      theFoundP = kFloatMissing;  // 'nollataan' tämä vielä varmuuden vuoksi
    }
  }
  return false;
}

// hakee lähimmän sopivan painepinnan, mistä löytyy halutulle parametrille ei-puuttuva arvo
bool NFmiSoundingDataOpt1::SetValueToPressureLevel(float P,
                                                   float theParamValue,
                                                   FmiParameterName theId)
{
  if (P != kFloatMissing)
  {
    std::deque<float> &pV = GetParamData(kFmiPressure);
    std::deque<float> &parV = GetParamData(theId);
    if (pV.size() > 0)  // oletus että parV on saman kokoinen kuin pV -vektori
    {
      std::deque<float>::iterator it = std::find(pV.begin(), pV.end(), P);
      if (it != pV.end())
      {
        int index = static_cast<int>(std::distance(pV.begin(), it));
        parV[index] = theParamValue;
        return true;
      }
    }
  }
  return false;
}

void NFmiSoundingDataOpt1::SetTandTdSurfaceValues(float T, float Td)
{
  std::deque<float> &tV = GetParamData(kFmiTemperature);
  std::deque<float> &tdV = GetParamData(kFmiDewPoint);
  tV[0] = T;
  tdV[0] = Td;
}

// paluttaa paine arvon halutulle metri korkeudelle
float NFmiSoundingDataOpt1::GetPressureAtHeight(double H)
{
  if (H == kFloatMissing)
    return kFloatMissing;

  double maxDiffInH = 100;  // jos ei voi interpoloida, pitää löydetyn arvon olla vähintäin näin
                            // lähellä, että hyväksytään
  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &hV = GetParamData(kFmiGeomHeight);
  float value = kFloatMissing;
  if (hV.size() > 0 && pV.size() == hV.size())
  {
    unsigned int ssize = static_cast<unsigned int>(pV.size());
    float lastP = kFloatMissing;
    float lastH = kFloatMissing;
    float currentP = kFloatMissing;
    float currentH = kFloatMissing;
    bool goneOverWantedHeight = false;
    for (unsigned int i = 0; i < ssize; i++)
    {
      currentP = pV[i];
      currentH = hV[i];
      if (currentH != kFloatMissing)
      {
        if (currentH > H)
        {
          if (currentP != kFloatMissing)
          {
            goneOverWantedHeight = true;
            break;
          }
        }
        if (currentP != kFloatMissing)
        {
          lastP = currentP;
          lastH = currentH;
          if (currentH == H)
            return currentP;  // jos oli tarkka osuma, turha jatkaa
        }
      }
    }
    if (goneOverWantedHeight && lastP != kFloatMissing && currentP != kFloatMissing &&
        lastH != kFloatMissing && currentH != kFloatMissing)
    {  // interpoloidaan arvo kun löytyi kaikki arvot
      value = static_cast<float>(NFmiInterpolation::Linear(H, currentH, lastH, currentP, lastP));
    }
    else if (lastP != kFloatMissing && lastH != kFloatMissing && ::fabs(lastH - H) < maxDiffInH)
      value = lastP;
    else if (currentP != kFloatMissing && currentH != kFloatMissing &&
             ::fabs(currentH - H) < maxDiffInH)
      value = currentP;
  }
  return value;
}

// Luotausten muokkausta varten pitää tietään onko ensimmäinen luotaus vaihtunut (paikka, aika,
// origin-aika).
// Jos ei ole ja luotausta on modifioitu, piirretään modifioitu, muuten nollataan 'modifioitu'
// luotaus ja täytetään se tällä uudella datalla.
bool NFmiSoundingDataOpt1::IsSameSounding(const NFmiSoundingDataOpt1 &theOtherSounding)
{
  if (Location() == theOtherSounding.Location())
    if (Time() == theOtherSounding.Time())
      if (OriginTime() == theOtherSounding.OriginTime())
        return true;
  return false;
}

// hakee ne arvot h, u ja v parametreista, mitkä ovat samalta korkeudelta ja mitkä
// eivät ole puuttuvia. Haetaan alhaalta ylös päin arvoja
bool NFmiSoundingDataOpt1::GetLowestNonMissingValues(float &H, float &U, float &V)
{
  std::deque<float> &hV = GetParamData(kFmiGeomHeight);
  std::deque<float> &uV = GetParamData(kFmiWindUMS);
  std::deque<float> &vV = GetParamData(kFmiWindVMS);
  if (hV.size() > 0 && hV.size() == uV.size() && hV.size() == vV.size())
  {
    for (int i = 0; i < static_cast<int>(hV.size()); i++)
    {
      if (hV[i] != kFloatMissing && uV[i] != kFloatMissing && vV[i] != kFloatMissing)
      {
        H = hV[i];
        U = uV[i];
        V = vV[i];
        return true;
      }
    }
  }
  return false;
}

// laskee halutun parametrin arvon haluttuun metri korkeuteen
float NFmiSoundingDataOpt1::GetValueAtHeight(FmiParameterName theId, float H)
{
  float P = GetPressureAtHeight(H);
  if (P == kFloatMissing)
    return kFloatMissing;

  return GetValueAtPressure(theId, P);
}

// Hakee halutun parametrin arvon halutulta painekorkeudelta.
float NFmiSoundingDataOpt1::GetValueAtPressure(FmiParameterName theId, float P)
{
  if (P == kFloatMissing)
    return kFloatMissing;

  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &paramV = GetParamData(theId);
  float value = kFloatMissing;
  if (paramV.size() > 0 && pV.size() == paramV.size())
  {
    unsigned int ssize = static_cast<unsigned int>(pV.size());
    float lastP = kFloatMissing;
    float lastValue = kFloatMissing;
    float currentP = kFloatMissing;
    float currentValue = kFloatMissing;
    for (unsigned int i = 0; i < ssize; i++)
    {
      currentP = pV[i];
      currentValue = paramV[i];
      if (currentP != kFloatMissing)
      {
        if (currentP < P)
        {
          if (currentValue != kFloatMissing)
            break;
        }
        if (currentValue != kFloatMissing)
        {
          lastP = currentP;
          lastValue = currentValue;
          if (currentP == P)
            return currentValue;  // jos oli tarkka osuma, turha jatkaa
        }
      }
    }
    float maxPDiff =
        10.;  // suurin sallittu ero, että arvo hyväksytään, jos pyydetty paine on 'asteikon ulkona'
    if (lastP != kFloatMissing && currentP != kFloatMissing && lastValue != kFloatMissing &&
        currentValue != kFloatMissing)
    {  // interpoloidaan arvo kun löytyi kaikki arvot
      if (theId == kFmiWindVectorMS)
        value = NFmiSoundingFunctions::CalcLogInterpolatedWindWectorValue(
            lastP, currentP, P, lastValue, currentValue);
      else
        value = NFmiSoundingFunctions::CalcLogInterpolatedValue(
            lastP, currentP, P, lastValue, currentValue);
    }
    else if (lastP != kFloatMissing && lastValue != kFloatMissing)
    {
      if (::fabs(lastP - P) < maxPDiff)
        value = lastValue;
    }
    else if (currentP != kFloatMissing && currentValue != kFloatMissing)
    {
      if (::fabs(currentP - P) < maxPDiff)
        value = currentValue;
    }
  }
  return value;
}

// Laskee u ja v komponenttien keskiarvot halutulla välillä
// Huom! z korkeudet interpoloidaan, koska havaituissa luotauksissa niitä ei ole aina ja varsinkaan
// samoissa
// väleissä kuin tuulia
bool NFmiSoundingDataOpt1::CalcAvgWindComponentValues(double fromZ,
                                                      double toZ,
                                                      double &u,
                                                      double &v)
{
  u = kFloatMissing;
  v = kFloatMissing;
  std::deque<float> &pV = GetParamData(kFmiPressure);
  if (pV.size() > 0)
  {
    double hStep = 100;  // käydään dataa läpi 100 metrin välein

    NFmiDataModifierAvg uAvg;
    NFmiDataModifierAvg vAvg;
    for (double h = fromZ; h < toZ + hStep / 2.; h += hStep)  // käydää layeria 100 metrin välein
    {
      double pressure = GetPressureAtHeight(h);
      float tmpU = GetValueAtPressure(kFmiWindUMS, static_cast<float>(pressure));
      float tmpV = GetValueAtPressure(kFmiWindVMS, static_cast<float>(pressure));
      if (tmpU != kFloatMissing && tmpV != kFloatMissing)
      {
        uAvg.Calculate(tmpU);
        vAvg.Calculate(tmpV);
      }
    }
    u = uAvg.CalculationResult();
    v = vAvg.CalculationResult();
    return (u != kFloatMissing && v != kFloatMissing);
  }
  return false;
}

float NFmiSoundingDataOpt1::FindPressureWhereHighestValue(FmiParameterName theId,
                                                          float theMaxP,
                                                          float theMinP)
{
  float maxValuePressure = kFloatMissing;
  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &paramV = GetParamData(theId);
  if (pV.size() > 0)
  {
    float maxValue = -99999999.f;
    for (unsigned int i = 0; i < pV.size(); i++)
    {
      float tmpP = pV[i];
      float tmpParam = paramV[i];
      if (tmpP != kFloatMissing && tmpParam != kFloatMissing)
      {
        if (tmpP <= theMaxP && tmpP >= theMinP)  // eli ollaanko haluttujen pintojen välissä
        {
          if (tmpParam > maxValue)
          {
            maxValue = tmpParam;
            maxValuePressure = tmpP;
          }
        }

        if (tmpP < theMinP)  // jos oltiin jo korkeammalla kuin minimi paine, voidaan lopettaa
          break;
      }
    }
  }
  return maxValuePressure;
}

// Stabiilisuus indeksien Most Unstable tapausta etsittäessä etsitään maksimi ThetaE:tä.
// Hakua rajoitetaan pinnan ja jonkin max korkeuden väliin.
// Joskus käy niin että maanpinnan lähellä on lokaali maksimi ja juuri ennen maksimi korkeutta tulee
// globaali maksimi.
// Tämä globaali maksimi ei kuitenkaan ole se maksimi mitä halutaan vaan jos pintaa lähempänä
// olevassa kerroksessa olevan
// lokaalin maksimin arvo on tarpeeksi lähellä globaalia maksimia, otetaan sen kerroksen arvot
// käyttöön.
struct ThetaEValues
{
  ThetaEValues() : P(kFloatMissing), ThetaE(kFloatMissing), T(kFloatMissing), Td(kFloatMissing) {}
  ThetaEValues(float P, float T, float Td, double ThetaE) : P(P), ThetaE(ThetaE), T(T), Td(Td) {}
  float P;
  double ThetaE;
  float T;
  float Td;
};

// Tällä operaattorilla sortataan lasketut theta kerrokset
#ifdef THIS_IS_NOT_USED_ANYWHERE
static bool operator<(const ThetaEValues &theta1, const ThetaEValues &theta2)
{
  if (theta1.P != kFloatMissing && theta2.P != kFloatMissing && theta1.ThetaE != kFloatMissing &&
      theta2.ThetaE != kFloatMissing)
  {                                     // Jos ei puuttuvia arvoja, alkaa kiinnnostavat vertailut
    if (theta1.ThetaE > theta2.ThetaE)  // Kumpi theta arvo on suurempi
      return false;
    else if (theta1.ThetaE == theta2.ThetaE && theta1.P < theta2.P)  // Jos Thetat on samoja,
                                                                     // kiinnostaa se kummpi niistä
                                                                     // on lähempänä maanpintaa
      return false;
  }
  return true;
}
#endif

// Tällä operaattorilla sortataan lasketut theta kerrokset
#ifdef THIS_IS_NOT_USED_ANYWHERE
static bool operator>(const ThetaEValues &theta1, const ThetaEValues &theta2)
{
  if (theta1.P != kFloatMissing && theta2.P != kFloatMissing)
  {
    if (theta1.P > theta2.P)
      return true;
  }
  return false;
}
#endif

#ifdef THIS_IS_NOT_USED_ANYWHERE
static bool isCloseToUpperLevelLimit(const ThetaEValues &theta, double topPressureLevel)
{
  if (std::fabs((theta.P - topPressureLevel) / topPressureLevel) < 0.3)
    return true;
  else
    return false;
}
#endif

#ifdef THIS_IS_NOT_USED_ANYWHERE
static bool isGlobalAndLocalThetasCloseEnough(double globalMaxTheta, double localThetaE)
{
  if (std::fabs((globalMaxTheta - localThetaE) / globalMaxTheta) < 0.15)
    return true;
  else
    return false;
}
#endif

// Jos globaali maksimi on lähempänä maanpintaa eli kauempana annetusta yläkerroksen rajasta,
// annetaan se.
// Jos globaali maksimi lähellä yläkerroksen rajaa, katsotaan löytyykö alemmista kerroksista lokaali
// maksimi,
// joka on tarpeeksi lähellä globaalin maksimin arvoa ja palautetaan se.

#ifdef THIS_IS_NOT_USED_ANYWHERE
static ThetaEValues getBestMUThetaValues(std::set<ThetaEValues> &thetas, double topPressureLevel)
{
  if (thetas.size() == 0)
    return ThetaEValues();
  else if (thetas.size() == 1)
    return *thetas.begin();
  else
  {
    // Käydään containeria läpi lopusta alkuun, koska datat ovat siinä nousevassa järjestyksessä
    std::set<ThetaEValues>::reverse_iterator iter = thetas.rbegin();
    if (!::isCloseToUpperLevelLimit(*iter, topPressureLevel))
      return *iter;  // maksimi theta oli etäällä ylärajasta, voidaan palauttaa se
    double globalMaxTheta = iter->ThetaE;
    for (; ++iter != thetas.rend();)
    {
      if (!::isCloseToUpperLevelLimit(*iter, topPressureLevel))
      {
        if (::isGlobalAndLocalThetasCloseEnough(globalMaxTheta, iter->ThetaE))
          return *iter;  // Palautetaan lokaali maksimi
        else
          return *thetas.rbegin();  // Koska 1. lokaali maanpintaa lähempänä ollut theta maksimi ei
                                    // ollut tarpeeksi lähellä globaalia maksimia, palautetaan
                                    // globaali maksimi
      }
    }
    return *thetas.rbegin();  // Koska ei löytynyt hyviä lokaaleja maksimeja alemmalta tasolta,
                              // palautetaan globaali maksimi
  }
}
#endif

// Etsitään maanpinnasta ylöspäin lokaali maksimeja. Jos lokaali maksimi on tarpeeksi lähellä
// totaali maksimia, palautetaan sen arvo.
// Muuten palautetaan globaali maksimi.

#ifdef THIS_IS_NOT_USED_ANYWHERE
static ThetaEValues getBestMUThetaValues2(std::vector<ThetaEValues> &thetas,
                                          double topPressureLevel)
{
  if (thetas.size() == 0)
    return ThetaEValues();
  else if (thetas.size() == 1)
    return *thetas.begin();
  else
  {
    // Etsitään globaali maksimi
    ThetaEValues globalMaxTheta = *std::max_element(thetas.begin(), thetas.end());

    // Onko vector:in 0. elementti lokaali maksimi on erikoistapaus
    if (thetas[0].ThetaE > thetas[1].ThetaE &&
        ::isGlobalAndLocalThetasCloseEnough(globalMaxTheta.ThetaE, thetas[0].ThetaE))
      return thetas[0];  // Maanpintaa lähin arvo oli lokaali maksimi ja se oli tarpeeksi lähellä
                         // globaalia maksimi thetaa
    else
    {
      for (size_t i = 1; i < thetas.size() - 1; i++)
      {
        if (thetas[i - 1].ThetaE < thetas[i].ThetaE && thetas[i].ThetaE > thetas[i + 1].ThetaE &&
            ::isGlobalAndLocalThetasCloseEnough(globalMaxTheta.ThetaE, thetas[i].ThetaE))
          return thetas[i];  // Jos i:s theta oli suurempi kuin viereiset arvot (lokaali maksimi) ja
                             // se oli tarpeeksi lähellä globaalia maksimi thetaa
      }
    }
    return globalMaxTheta;  // Jos tullaan tänne, palautetaan sitten globaali maksimi eli tämän
                            // pitää olla sama kuin vectorin viimeisen elementin
  }
}
#endif

#ifdef THIS_IS_NOT_USED_ANYWHERE
static void setThetaValues(
    const ThetaEValues &theta, double &T, double &Td, double &P, double &theMaxThetaE)
{
  theMaxThetaE = theta.ThetaE;
  T = theta.T;
  Td = theta.Td;
  P = theta.P;
}
#endif

// Käy läpi luotausta ja etsi sen kerroksen arvot, jolta löytyy suurin theta-E ja
// palauta sen kerroksen T, Td ja P ja laskettu max Theta-e.
// Etsitään arvoja jos pinta on alle theMinP-tason (siis alle tuon tason fyysisesti).
// HUOM! theMinP ei voi olla kFloatMissing, jos haluat että kaikki levelit käydään läpi laita sen
// arvoksi 0.
bool NFmiSoundingDataOpt1::FindHighestThetaE(
    double &T, double &Td, double &P, double &theMaxThetaE, double theMinP)
{
  T = kFloatMissing;
  Td = kFloatMissing;
  P = kFloatMissing;
  theMaxThetaE = kFloatMissing;
  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &tV = GetParamData(kFmiTemperature);
  std::deque<float> &tdV = GetParamData(kFmiDewPoint);
  if (pV.size() > 0)
  {
    theMaxThetaE = -99999999;
    for (unsigned int i = 0; i < pV.size(); i++)
    {
      float tmpP = pV[i];
      float tmpT = tV[i];
      float tmpTd = tdV[i];
      if (tmpP != kFloatMissing && tmpT != kFloatMissing && tmpTd != kFloatMissing)
      {
        if (tmpP >= theMinP)  // eli ollaanko lähempana maanpintaa kuin raja paine pinta on
        {
          double thetaE = NFmiSoundingFunctions::CalcThetaE(tmpT, tmpTd, tmpP);
          if (thetaE != kFloatMissing && thetaE > theMaxThetaE)
          {
            theMaxThetaE = thetaE;
            T = tmpT;
            Td = tmpTd;
            P = tmpP;
          }
        }
        else  // jos oltiin korkeammalla, voidaan lopettaa
          break;
      }
    }
  }
  return theMaxThetaE != kFloatMissing;
}
/*
bool NFmiSoundingDataOpt1::FindHighestThetaE2(double &T, double &Td, double &P, double
&theMaxThetaE, double theMinP)
{
        T = kFloatMissing;
        Td = kFloatMissing;
        P = kFloatMissing;
        theMaxThetaE = kFloatMissing;
        std::deque<float>&pV = GetParamData(kFmiPressure);
        std::deque<float>&tV = GetParamData(kFmiTemperature);
        std::deque<float>&tdV = GetParamData(kFmiDewPoint);
        if(pV.size() > 0)
        {
        std::set<ThetaEValues> thetas;
        std::vector<ThetaEValues> thetas2;
                theMaxThetaE = -99999999;
                for(unsigned int i=0; i<pV.size(); i++)
                {
                        float tmpP = pV[i];
                        float tmpT = tV[i];
                        float tmpTd = tdV[i];
                        if(tmpP != kFloatMissing && tmpT != kFloatMissing && tmpTd != kFloatMissing)
                        {
                                if(tmpP >= theMinP) // eli ollaanko lähempana maanpintaa kuin raja
paine pinta on
                                {
                                        double thetaE = NFmiSoundingFunctions::CalcThetaE(tmpT,
tmpTd, tmpP);
                    thetas.insert(ThetaEValues(tmpP, tmpT, tmpTd, thetaE));
                    thetas2.push_back(ThetaEValues(tmpP, tmpT, tmpTd, thetaE));
                                }
                                else // jos oltiin korkeammalla, voidaan lopettaa
                                        break;
                        }
                }
//        ::setThetaValues(::getBestMUThetaValues(thetas, theMinP), T, Td, P, theMaxThetaE);
        ::setThetaValues(::getBestMUThetaValues2(thetas2, theMinP), T, Td, P, theMaxThetaE);
        }
        return theMaxThetaE != kFloatMissing;
}
*/

// Tämä on Pieter Groenemeijerin ehdottama tapa laskea LCL-laskuihin tarvittavia T, Td ja P arvoja
// yli halutun layerin.
// Laskee keskiarvot T:lle, Td:lle ja P:lle haluttujen korkeuksien välille.
// Eli laskee keskiarvon lämpötilalle potentiaali lömpötilojen avulla.
// Laskee kastepisteen keskiarvon mixing valueiden avulla.
// Näille lasketaan keskiarvot laskemalla halutun layerin läpi 1 hPa askelissa, ettei epämääräiset
// näyte valit painota mitenkaan laskuja.
// Paineelle otetaan suoraan pohja kerroksen arvo.
// Oletus fromZ ja toZ eivät ole puuttuvia.
bool NFmiSoundingDataOpt1::CalcLCLAvgValues(
    double fromZ, double toZ, double &T, double &Td, double &P, bool fUsePotTandMix)
{
  T = kFloatMissing;
  Td = kFloatMissing;
  P = kFloatMissing;
  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &tV = GetParamData(kFmiTemperature);
  std::deque<float> &tdV = GetParamData(kFmiDewPoint);
  if (pV.size() > 0)
  {
    for (unsigned int i = 0; i < pV.size(); i++)
      if (pV[i] != kFloatMissing && tV[i] != kFloatMissing &&
          tdV[i] != kFloatMissing)  // etsitään 1. ei puuttuva paine arvo eli alin paine arvo (missä
                                    // myös T ja Td arvot)
      {
        P = pV[i];
        break;
      }
    if (P == kFloatMissing)
      return false;
    int startP = static_cast<int>(round(GetPressureAtHeight(fromZ)));
    int endP = static_cast<int>(round(GetPressureAtHeight(toZ)));
    if (startP == kFloatMissing || endP == kFloatMissing || startP <= endP)
      return false;
    NFmiDataModifierAvg avgT;   // riippuen moodista tässä lasketaan T tai Tpot keskiarvoa
    NFmiDataModifierAvg avgTd;  // riippuen moodista tässä lasketaan Td tai w keskiarvoa
    for (int pressure = startP; pressure > endP;
         pressure--)  // käydää layeria yhden hPa:n välein läpi
    {
      float temperature = GetValueAtPressure(kFmiTemperature, static_cast<float>(pressure));
      float dewpoint = GetValueAtPressure(kFmiDewPoint, static_cast<float>(pressure));
      if (temperature != kFloatMissing && dewpoint != kFloatMissing)
      {
        if (fUsePotTandMix)
        {
          avgT.Calculate(static_cast<float>(NFmiSoundingFunctions::T2tpot(temperature, pressure)));
          avgTd.Calculate(static_cast<float>(
              NFmiSoundingFunctions::CalcMixingRatio(temperature, dewpoint, pressure)));
        }
        else
        {
          avgT.Calculate(temperature);
          avgTd.Calculate(dewpoint);
        }
      }
    }
    if (avgT.CalculationResult() != kFloatMissing && avgTd.CalculationResult() != kFloatMissing)
    {
      if (fUsePotTandMix)
      {
        T = NFmiSoundingFunctions::Tpot2t(avgT.CalculationResult(), P);
        Td = NFmiSoundingFunctions::CalcDewPoint(T, avgTd.CalculationResult(), P);
      }
      else
      {
        T = avgT.CalculationResult();
        Td = avgTd.CalculationResult();
      }
      return (P != kFloatMissing && T != kFloatMissing && Td != kFloatMissing);
    }
  }
  return false;
}

// Tämä hakee annettua painearvoa lähimmät arvot, jotka löytyvät kaikille halutuille parametreille.
// Palauttaa true, jos löytyy dataa ja false jos ei löydy.
// OLETUS: maanpinta arvot ovat vektorin alussa, pitäisi tarkistaa??
bool NFmiSoundingDataOpt1::GetValuesStartingLookingFromPressureLevel(double &T,
                                                                     double &Td,
                                                                     double &P)
{
  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &tV = GetParamData(kFmiTemperature);
  std::deque<float> &tdV = GetParamData(kFmiDewPoint);
  if (pV.size() > 0)
  {
    for (unsigned int i = 0; i < pV.size(); i++)
    {
      if (pV[i] != kFloatMissing)  // && tV[i] != kFloatMissing && tdV[i] != kFloatMissing)
      {
        if (P >= pV[i] && tV[i] != kFloatMissing && tdV[i] != kFloatMissing)
        {
          T = tV[i];
          Td = tdV[i];
          P = pV[i];
          return true;
        }
      }
    }
  }
  return false;
}

// oletuksia paljon:
// theInfo on validi, aika ja paikka on jo asetettu
bool NFmiSoundingDataOpt1::FillParamData(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    FmiParameterName theId,
    NFmiQueryDataUtil::SignificantSoundingLevels &theSoungingLevels)
{
  try
  {
    std::deque<float> &data = GetResizedParamData(theInfo, theId, theSoungingLevels);
    bool paramFound = LookForFilledParamFromInfo(theInfo, theId);
    if (paramFound)
    {
      if (theSoungingLevels)
        FillParamDataFromSignificantLevels(theInfo, data, theSoungingLevels);
      else
        FillParamDataNormally(theInfo, data);
      DoAfterFillChecks(theInfo, data, theId);
      return true;
    }
  }
  catch (std::exception & /* e */)
  {
  }
  return false;
}

void NFmiSoundingDataOpt1::FillParamDataNormally(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo, std::deque<float> &data)
{
  int i = 0;
  for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
    data[i] = theInfo->FloatValue();
}

void NFmiSoundingDataOpt1::FillParamDataFromSignificantLevels(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    std::deque<float> &data,
    NFmiQueryDataUtil::SignificantSoundingLevels &significantLevels)
{
  int i = 0;
  for (auto levelIndex : *significantLevels)
  {
    if (theInfo->LevelIndex(levelIndex))
    {
      data[i++] = theInfo->FloatValue();
    }
  }
}

std::deque<float> &NFmiSoundingDataOpt1::GetResizedParamData(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    FmiParameterName theId,
    NFmiQueryDataUtil::SignificantSoundingLevels &theSoungingLevels)
{
  std::deque<float> &data = GetParamData(theId);
  if (theSoungingLevels)
    data.resize(theSoungingLevels->size(), kFloatMissing);
  else
    data.resize(theInfo->SizeLevels(), kFloatMissing);
  return data;
}

// Asettaa fastInfon osoittamaan oikeaa parametria.
// Kastepiste parametri on erikoistapaus.
bool NFmiSoundingDataOpt1::LookForFilledParamFromInfo(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo, FmiParameterName theId)
{
  bool paramFound = theInfo->Param(theId);
  // Kastepiste parametri voi tulla luotausten yhteydessä tällä
  // parametrilla ja mallidatan yhteydessä toisella
  if (paramFound == false && theId == kFmiDewPoint)
    paramFound = theInfo->Param(kFmiDewPoint2M);
  return paramFound;
}

void NFmiSoundingDataOpt1::DoAfterFillChecks(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                             std::deque<float> &data,
                                             FmiParameterName theId)
{
  // jos ei nousevassa järjestyksessä, käännetään vektorissa olevat arvot
  if (theInfo->HeightParamIsRising() == false)
    std::reverse(data.begin(), data.end());
  if (theId == kFmiPressure)
    fPressureDataAvailable = true;
  if (theId == kFmiGeomHeight || theId == kFmiGeopHeight || theId == kFmiFlAltitude)
    fHeightDataAvailable = true;
}

// Oletus, tässä info on jo parametrissa ja ajassa kohdallaan.
bool NFmiSoundingDataOpt1::FastFillParamData(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                             FmiParameterName theId)
{
  bool status = false;
  std::deque<float> &data = GetParamData(theId);
  data.resize(theInfo->SizeLevels(), kFloatMissing);  // alustetaan vektori puuttuvalla
  if (theInfo->Param(theId))
  {
    int i = 0;
    for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
      data[i] = theInfo->FloatValue();
    status = true;
  }
  else if (theId == kFmiDewPoint && theInfo->Param(kFmiHumidity))
  {
    unsigned int RHindex = theInfo->ParamIndex();
    if (!theInfo->Param(kFmiTemperature))
      return false;
    unsigned int Tindex = theInfo->ParamIndex();
    float T = 0;
    float RH = 0;
    int i = 0;
    for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
    {
      theInfo->ParamIndex(Tindex);
      T = theInfo->FloatValue();
      theInfo->ParamIndex(RHindex);
      RH = theInfo->FloatValue();  // varmuuden vuoksi kaikki interpoloinnit päälle, se funktio
                                   // tarkistaa tarvitseeko sitä tehdä
      data[i] = static_cast<float>(NFmiSoundingFunctions::CalcDP(T, RH));
    }
    status = true;
  }
  else if (theId == kFmiPressure)
  {  // jos halutaan paine dataa ja parametria ei ollut datassa, oliko kyseessa painepinta data,
     // jolloin paine pitää irroittaa level-tiedosta
    if (theInfo->FirstLevel())
    {
      if (theInfo->Level()->LevelType() == kFmiPressureLevel)
      {
        int i = 0;
        for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
          data[i] = static_cast<float>(theInfo->Level()->LevelValue());
        status = true;
      }
    }
  }

  ::ReverseSoundingData(theInfo, data);

  return status;
}

// Katsotaan saadaanko täytettyä korkeus data heith-levleiden avulla
bool NFmiSoundingDataOpt1::FillHeightDataFromLevels(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  bool status = false;
  std::deque<float> &data = GetParamData(kFmiGeopHeight);
  data.resize(theInfo->SizeLevels(), kFloatMissing);  // alustetaan vektori puuttuvalla
  if (theInfo->FirstLevel())
  {
    if (theInfo->Level()->LevelType() == kFmiHeight)
    {
      int i = 0;
      for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
        data[i] = static_cast<float>(theInfo->Level()->LevelValue());
      status = true;
      ::ReverseSoundingData(theInfo, data);
    }
  }
  return status;
}

bool NFmiSoundingDataOpt1::FillParamData(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                         FmiParameterName theId,
                                         const NFmiMetTime &theTime,
                                         const NFmiPoint &theLatlon)
{
  bool status = false;
  std::deque<float> &data = GetParamData(theId);
  data.resize(theInfo->SizeLevels(), kFloatMissing);  // alustetaan vektori puuttuvalla
  if (theInfo->Param(theId))
  {
    int i = 0;
    for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
      data[i] = theInfo->InterpolatedValue(theLatlon, theTime);  // varmuuden vuoksi kaikki
                                                                 // interpoloinnit päälle, se
                                                                 // funktio tarkistaa tarvitseeko
                                                                 // sitä tehdä
    status = true;
  }
  else if (theId == kFmiDewPoint && theInfo->Param(kFmiHumidity))
  {
    unsigned int RHindex = theInfo->ParamIndex();
    if (!theInfo->Param(kFmiTemperature))
      return false;
    unsigned int Tindex = theInfo->ParamIndex();
    float T = 0;
    float RH = 0;
    int i = 0;
    for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
    {
      theInfo->ParamIndex(Tindex);
      T = theInfo->InterpolatedValue(theLatlon, theTime);  // varmuuden vuoksi kaikki interpoloinnit
                                                           // päälle, se funktio tarkistaa
                                                           // tarvitseeko sitä tehdä
      theInfo->ParamIndex(RHindex);
      RH = theInfo->InterpolatedValue(theLatlon, theTime);  // varmuuden vuoksi kaikki
                                                            // interpoloinnit päälle, se funktio
                                                            // tarkistaa tarvitseeko sitä tehdä
      data[i] = static_cast<float>(NFmiSoundingFunctions::CalcDP(T, RH));
    }
    status = true;
  }
  else if (theId == kFmiPressure)
  {  // jos halutaan paine dataa ja parametria ei ollut datassa, oliko kyseessa painepinta data,
     // jolloin paine pitää irroittaa level-tiedosta
    if (theInfo->FirstLevel())
    {
      if (theInfo->Level()->LevelType() == kFmiPressureLevel)
      {
        int i = 0;
        for (theInfo->ResetLevel(); theInfo->NextLevel(); i++)
          data[i] = static_cast<float>(theInfo->Level()->LevelValue());
        status = true;
      }
    }
  }

  ::ReverseSoundingData(theInfo, data);

  return status;
}

unsigned long NFmiSoundingDataOpt1::GetHighestNonMissingValueLevelIndex(FmiParameterName theParaId)
{
  std::deque<float> &vec = GetParamData(theParaId);
  std::deque<float>::size_type ssize = vec.size();
  unsigned long index = 0;
  for (unsigned long i = 0; i < ssize; i++)
    if (vec[i] != kFloatMissing)
      index = i;
  return index;
}

unsigned long NFmiSoundingDataOpt1::GetLowestNonMissingValueLevelIndex(FmiParameterName theParaId)
{
  std::deque<float> &vec = GetParamData(theParaId);
  std::deque<float>::size_type ssize = vec.size();
  for (unsigned long i = 0; i < ssize; i++)
    if (vec[i] != kFloatMissing)
      return i;
  return gMissingIndex;
}

// Tarkistaa onko annetun parametrin alin ei puuttuva indeksi liian korkea, jotta data olisi hyvää
// (= ei liian puutteellista).
// Jos data on liian puutteellista, palautetaan true, muuten false.
bool NFmiSoundingDataOpt1::CheckForMissingLowLevelData(FmiParameterName theParaId,
                                                       unsigned long theMissingIndexLimit)
{
  unsigned long index = GetLowestNonMissingValueLevelIndex(theParaId);
  if (index == gMissingIndex || index > theMissingIndexLimit)
    return true;
  else
    return false;
}

bool NFmiSoundingDataOpt1::IsDataGood()
{
  double T = kFloatMissing, Td = kFloatMissing, P = kFloatMissing;
  if (!GetValuesNeededInLCLCalculations(kLCLCalcSurface, T, Td, P))
    return false;

  unsigned long missingIndexLimit =
      FmiRound(itsTemperatureData.size() * 0.2);  // jos viidesosa tärkeän parametrin ala alaosasta
                                                  // puuttuu, oletetaan että data on liian
                                                  // puutteellista ja kelvotonta
  if (CheckForMissingLowLevelData(kFmiTemperature, missingIndexLimit))
    return false;
  if (CheckForMissingLowLevelData(kFmiDewPoint, missingIndexLimit))
    return false;

  return true;
}

// tämä leikkaa Fill.. -metodeissa laskettuja data vektoreita niin että pelkät puuttuvat kerrokset
// otetaan pois
void NFmiSoundingDataOpt1::CutEmptyData(void)
{
  std::vector<FmiParameterName> itsSoundingParameters;
  itsSoundingParameters.push_back(kFmiPressure);
  itsSoundingParameters.push_back(kFmiTemperature);
  itsSoundingParameters.push_back(kFmiDewPoint);
  itsSoundingParameters.push_back(kFmiWindSpeedMS);
  itsSoundingParameters.push_back(kFmiWindDirection);
  itsSoundingParameters.push_back(kFmiGeomHeight);

  unsigned int greatestNonMissingLevelIndex = 0;
  unsigned int maxLevelIndex =
      static_cast<unsigned int>(GetParamData(itsSoundingParameters[0]).size());
  for (unsigned int i = 0; i < itsSoundingParameters.size(); i++)
  {
    unsigned int currentIndex = GetHighestNonMissingValueLevelIndex(itsSoundingParameters[i]);
    if (currentIndex > greatestNonMissingLevelIndex)
      greatestNonMissingLevelIndex = currentIndex;
    if (greatestNonMissingLevelIndex >= maxLevelIndex)
      return;  // ei tarvitse leikata, kun dataa löytyy korkeimmaltakin leveliltä
  }

  // tässä pitää käydä läpi kaikki data vektorit!!!! Oikeasti nämä datavektori pitäisi laittaa omaan
  // vektoriin että sitä voitaisiin iteroida oikein!
  itsTemperatureData.resize(greatestNonMissingLevelIndex);
  itsDewPointData.resize(greatestNonMissingLevelIndex);
  itsHumidityData.resize(greatestNonMissingLevelIndex);
  itsPressureData.resize(greatestNonMissingLevelIndex);
  itsGeomHeightData.resize(greatestNonMissingLevelIndex);
  itsWindSpeedData.resize(greatestNonMissingLevelIndex);
  itsWindDirectionData.resize(greatestNonMissingLevelIndex);
  itsWindComponentUData.resize(greatestNonMissingLevelIndex);
  itsWindComponentVData.resize(greatestNonMissingLevelIndex);
  itsWindVectorData.resize(greatestNonMissingLevelIndex);
  itsTotalCloudinessData.resize(greatestNonMissingLevelIndex);
}

static bool FindTimeIndexies(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             const NFmiMetTime &theStartTime,
                             long minuteRange,
                             unsigned long &timeIndex1,
                             unsigned long &timeIndex2)
{
  theInfo->FindNearestTime(theStartTime, kBackward);
  timeIndex1 = theInfo->TimeIndex();
  NFmiMetTime endTime(theStartTime);
  endTime.ChangeByMinutes(minuteRange);
  theInfo->FindNearestTime(endTime, kBackward);
  timeIndex2 = theInfo->TimeIndex();

  if (timeIndex1 == timeIndex2)  // pitää testata erikois tapaus, koska TimeToNearestStep-palauttaa
                                 // aina jotain, jos on dataa
  {
    theInfo->TimeIndex(timeIndex1);
    NFmiMetTime foundTime(theInfo->Time());
    if (foundTime > endTime || foundTime < theStartTime)  // jos löydetty aika on alku ja loppu ajan
                                                          // ulkopuolella ei piirretä salamaa
      return false;
  }
  return true;
}

static bool FindAmdarSoundingTime(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                  const NFmiMetTime &theTime,
                                  NFmiLocation &theLocation)
{
  theInfo->FirstLocation();  // amdareissa vain yksi dummy paikka, laitetaan se päälle
  NFmiMetTime timeStart(theTime);
  timeStart.ChangeByMinutes(-30);
  unsigned long timeIndex1 = 0;
  unsigned long timeIndex2 = 0;
  if (::FindTimeIndexies(theInfo, timeStart, 60, timeIndex1, timeIndex2) == false)
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
  if (minDistance < 1000 * 1000)  // jos lento löytyi vähintäin 1000 km säteeltä hiiren
                                  // klikkauspaikasta, otetaan kyseinen amdar piirtoon
  {
    theInfo->TimeIndex(minDistTimeInd);
    // pitää lisäksi asettaa locationiksi luotauksen alkupiste
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

bool NFmiSoundingDataOpt1::IsMovingSounding(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  return theInfo->HasLatlonInfoInData();
}

// Tälle anntaan asema dataa ja ei tehdä minkäänlaisia interpolointeja.
bool NFmiSoundingDataOpt1::FillSoundingData(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                                            const NFmiMetTime &theTime,
                                            const NFmiMetTime &theOriginTime,
                                            const NFmiLocation &theLocation,
                                            int useStationIdOnly)
{
  NFmiMetTime usedTime = theTime;
  NFmiLocation usedLocation(theLocation);
  ClearDatas();
  if (theInfo && !theInfo->IsGrid())
  {
    fObservationData = true;
    theInfo->FirstLevel();
    bool isMovingSounding = IsMovingSounding(theInfo);
    bool timeOk = false;
    if (isMovingSounding)
    {
      timeOk = ::FindAmdarSoundingTime(theInfo, usedTime, usedLocation);
      usedTime = theInfo->Time();
    }
    else
      timeOk = theInfo->Time(usedTime);
    if (timeOk)
    {
      bool stationOk = false;
      if (isMovingSounding)
        stationOk = true;  // asemaa ei etsitä, jokainen lento liittyy tiettyyn aikaa
      else
        stationOk = (useStationIdOnly ? theInfo->Location(usedLocation.GetIdent())
                                      : theInfo->Location(usedLocation));
      if (stationOk)
      {
        itsLocation = usedLocation;
        itsTime = usedTime;
        itsOriginTime = theOriginTime;
        auto significantLevelIndices =
            NFmiQueryDataUtil::GetSignificantSoundingLevelIndices(*theInfo);

        FillParamData(theInfo, kFmiTemperature, significantLevelIndices);
        FillParamData(theInfo, kFmiDewPoint, significantLevelIndices);
        FillParamData(theInfo, kFmiPressure, significantLevelIndices);
        if (!FillParamData(theInfo, kFmiGeomHeight, significantLevelIndices))
          if (!FillParamData(theInfo, kFmiGeopHeight, significantLevelIndices))
            if (!FillParamData(theInfo,
                               kFmiFlAltitude,
                               significantLevelIndices))  // eri datoissa on geom ja geop heightia,
                                                          // kokeillaan molempia tarvittaessa
              FillHeightDataFromLevels(theInfo);
        FillParamData(theInfo, kFmiWindSpeedMS, significantLevelIndices);
        FillParamData(theInfo, kFmiWindDirection, significantLevelIndices);
        FillParamData(theInfo, kFmiWindUMS, significantLevelIndices);
        FillParamData(theInfo, kFmiWindVMS, significantLevelIndices);
        FillParamData(theInfo, kFmiWindVectorMS, significantLevelIndices);
        CalculateHumidityData();
        InitZeroHeight();
        SetVerticalParamStatus();
        return true;
      }
    }
  }
  return false;
}

// Tälle annetaan hiladataa, ja interpolointi tehdään tarvittaessa ajassa ja paikassa.
bool NFmiSoundingDataOpt1::FillSoundingData(
    const boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
    const NFmiMetTime &theTime,
    const NFmiMetTime &theOriginTime,
    const NFmiPoint &theLatlon,
    const NFmiString &theName,
    const boost::shared_ptr<NFmiFastQueryInfo> &theGroundDataInfo,
    bool useFastFill)
{
  ClearDatas();
  if (theInfo && theInfo->IsGrid())
  {
    fObservationData = false;
    itsLocation = NFmiLocation(theLatlon);
    itsLocation.SetName(theName);
    itsTime = theTime;
    itsOriginTime = theOriginTime;

    if (useFastFill)
    {
      FastFillParamData(theInfo, kFmiTemperature);
      FastFillParamData(theInfo, kFmiDewPoint);
      FastFillParamData(theInfo, kFmiPressure);
      if (!FastFillParamData(theInfo, kFmiGeomHeight))
        if (!FastFillParamData(theInfo, kFmiGeopHeight))  // eri datoissa on geom ja geop heightia,
                                                          // kokeillaan molempia tarvittaessa
          FillHeightDataFromLevels(theInfo);
      FastFillParamData(theInfo, kFmiWindSpeedMS);
      FastFillParamData(theInfo, kFmiWindDirection);
      FastFillParamData(theInfo, kFmiWindUMS);
      FastFillParamData(theInfo, kFmiWindVMS);
      FastFillParamData(theInfo, kFmiWindVectorMS);
      FastFillParamData(theInfo, kFmiTotalCloudCover);
    }
    else
    {
      FillParamData(theInfo, kFmiTemperature, theTime, theLatlon);
      FillParamData(theInfo, kFmiDewPoint, theTime, theLatlon);
      FillParamData(theInfo, kFmiPressure, theTime, theLatlon);
      if (!FillParamData(theInfo, kFmiGeomHeight, theTime, theLatlon))
        if (!FillParamData(theInfo, kFmiGeopHeight, theTime, theLatlon))  // eri datoissa on geom ja
                                                                          // geop heightia,
                                                                          // kokeillaan molempia
                                                                          // tarvittaessa
          FillHeightDataFromLevels(theInfo);
      FillParamData(theInfo, kFmiWindSpeedMS, theTime, theLatlon);
      FillParamData(theInfo, kFmiWindDirection, theTime, theLatlon);
      FillParamData(theInfo, kFmiWindUMS, theTime, theLatlon);
      FillParamData(theInfo, kFmiWindVMS, theTime, theLatlon);
      FillParamData(theInfo, kFmiWindVectorMS, theTime, theLatlon);
      FillParamData(theInfo, kFmiTotalCloudCover, theTime, theLatlon);
    }

    CalculateHumidityData();
    InitZeroHeight();
    FixPressureDataSoundingWithGroundData(theGroundDataInfo);
    SetVerticalParamStatus();
    return true;
  }
  return false;
}

static bool AnyGoodValues(const std::deque<float> &values)
{
  for (size_t i = 0; i < values.size(); i++)
  {
    if (values[i] != kFloatMissing)
      return true;
  }
  return false;
}

// Tarkistaa onko pressure taulussa yhtaan ei puuttuvaa arvoa ja asettaa fPressureDataAvailable
// -lipun päälle.
// Tarkistaa onko height taulussa yhtaan ei puuttuvaa arvoa ja asettaa fHeightDataAvailable -lipun
// päälle.
void NFmiSoundingDataOpt1::SetVerticalParamStatus(void)
{
  std::deque<float> &pVec = GetParamData(kFmiPressure);
  if (::AnyGoodValues(pVec))
    fPressureDataAvailable = true;
  std::deque<float> &hVec = GetParamData(kFmiGeomHeight);
  if (::AnyGoodValues(hVec))
    fHeightDataAvailable = true;
}

// theVec is vector that is to be erased from the start.
// theCutIndex is the index that ends the cutting operation. If 0 then nothing is done.
template <class vectorContainer>
static void CutStartOfVector(vectorContainer &theVec, int theCutIndex)
{
  if (theVec.size() > 0 && theCutIndex > 0 &&
      static_cast<unsigned long>(theCutIndex) < theVec.size() - 1)
    theVec.erase(theVec.begin(), theVec.begin() + theCutIndex);
}

// Jos kyseessä on painepinta dataa, mistä löytyy myös siihen liittyvä pinta data, jossa on
// mukana parametri 472 eli paine aseman korkeudella, laitetaan tämä uudeksi ala-paineeksi
// luotaus-dataan
// ja laitetaan pinta-arvot muutenkin alimmalle tasolle pinta-datan mukaisiksi.
// HUOM! Oletus että löytyi ainakin yksi kerros, joka oli alle tämän pintakerroksen, koska
// en tee taulukkojen resize:a ainakaan nyt, eli taulukossa pitää olla tilaa tälle uudelle
// pintakerrokselle.
void NFmiSoundingDataOpt1::FixPressureDataSoundingWithGroundData(
    const boost::shared_ptr<NFmiFastQueryInfo> &theGroundDataInfo)
{
  if (theGroundDataInfo)
  {
    NFmiPoint wantedLatlon(itsLocation.GetLocation());
    theGroundDataInfo->Param(kFmiTemperature);
    float groundT = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiDewPoint);
    float groundTd = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiWindSpeedMS);
    float groundWS = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiWindDirection);
    float groundWD = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiWindUMS);
    float groundU = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiWindVMS);
    float groundV = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiWindVectorMS);
    float groundWv = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiPressureAtStationLevel);
    float groundStationPressure = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiHumidity);
    float groundRH = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);
    theGroundDataInfo->Param(kFmiTotalCloudCover);
    float groundN = theGroundDataInfo->InterpolatedValue(wantedLatlon, itsTime);

    if (groundStationPressure != kFloatMissing)
    {
      // oletus, kaikki vektorit on alustettu saman kokoisiksi kuin paine vektori
      if (theGroundDataInfo && itsPressureData.size() > 0)
      {
        // HUOM! luotausdatat ovat aina 'nousevassa' järjestyksessä eli maanpinta on taulukoissa
        // ensimmäisenä
        for (int i = 0; i < static_cast<int>(itsPressureData.size()); i++)
        {
          float currentPressure = itsPressureData[i];
          if (currentPressure != kFloatMissing && groundStationPressure >= currentPressure)
          {  // Eli nyt luotausdata taulukosta löytynyt paine on pienempi kuin paineasemakorkeudella
             // (eli ollaan juuri menty
            // groundStationPressure-lukeman yli siis pinnasta avaruutta kohden, ei isompaan paine
            // lukemaan), eli
            // uusi pinta on laitettava ennen tätä kerrosta (jos kyseessä on alin pinta taulukosta)
            if (i > 0)
            {
              // mennään siis edelliseen tasoon ja laitetaan se uudeksi pinta kerrokseksi
              i--;
              itsZeroHeightIndex = i;
              itsZeroHeight = 2;

              itsGeomHeightData[i] = 2;  // Oletus: pinta data on 2 metrin korkeudella
              itsPressureData[i] = groundStationPressure;
              itsTemperatureData[i] = groundT;
              itsDewPointData[i] = groundTd;
              itsWindSpeedData[i] = groundWS;
              itsWindDirectionData[i] = groundWD;
              itsWindVectorData[i] = groundWv;
              itsWindComponentUData[i] = groundU;
              itsWindComponentVData[i] = groundV;
              itsHumidityData[i] = groundRH;
              itsTotalCloudinessData[i] = groundN;

              // pitää ottaa vektoreista alkuosa pois, kun tuota itsZeroHeightIndex -dataosaa
              // ei näemmä käytetäkään missään
              if (itsZeroHeightIndex > 0)
              {  // huonoa koodia pitäisi olla vektorin vektori, että ei tarvitse aina muistaa
                 // kuinka monta vektoria on olemassa
                ::CutStartOfVector(itsGeomHeightData, itsZeroHeightIndex);
                ::CutStartOfVector(itsPressureData, itsZeroHeightIndex);
                ::CutStartOfVector(itsTemperatureData, itsZeroHeightIndex);
                ::CutStartOfVector(itsDewPointData, itsZeroHeightIndex);
                ::CutStartOfVector(itsWindSpeedData, itsZeroHeightIndex);
                ::CutStartOfVector(itsWindDirectionData, itsZeroHeightIndex);
                ::CutStartOfVector(itsWindVectorData, itsZeroHeightIndex);
                ::CutStartOfVector(itsWindComponentUData, itsZeroHeightIndex);
                ::CutStartOfVector(itsWindComponentVData, itsZeroHeightIndex);
                ::CutStartOfVector(itsHumidityData, itsZeroHeightIndex);
                ::CutStartOfVector(itsTotalCloudinessData, itsZeroHeightIndex);
                itsZeroHeightIndex = 0;
              }

              break;
            }
            else if (i == 0)
            {
              // tämä voi käydä esim. hybridi datan kanssa, eli hybridi luotaukseen lisätään alkuun
              // pinta arvot
              itsZeroHeightIndex = 0;
              itsZeroHeight = 2;

              itsGeomHeightData.push_front(2);  // Oletus: pinta data on 2 metrin korkeudella
              itsPressureData.push_front(groundStationPressure);
              itsTemperatureData.push_front(groundT);
              itsDewPointData.push_front(groundTd);
              itsWindSpeedData.push_front(groundWS);
              itsWindDirectionData.push_front(groundWD);
              itsWindVectorData.push_front(groundWv);
              itsWindComponentUData.push_front(groundU);
              itsWindComponentVData.push_front(groundV);
              itsHumidityData.push_front(groundRH);
              itsTotalCloudinessData.push_front(groundN);

              break;
            }
          }
        }
      }
    }
  }
}

// laskee jo laskettujen T ja Td avulla RH
void NFmiSoundingDataOpt1::CalculateHumidityData(void)
{
  size_t tVectorSize = itsTemperatureData.size();
  if (tVectorSize > 0 && itsDewPointData.size() == tVectorSize)
  {
    itsHumidityData.resize(tVectorSize, kFloatMissing);
    for (size_t i = 0; i < tVectorSize; i++)
    {
      if (itsTemperatureData[i] != kFloatMissing && itsDewPointData[i] != kFloatMissing)
        itsHumidityData[i] = static_cast<float>(
            NFmiSoundingFunctions::CalcRH(itsTemperatureData[i], itsDewPointData[i]));
    }
  }
}

// tätä kutsutaan FillParamData-metodeista
// sillä katsotaan mistä korkeudesta luotaus oikeasti alkaa
// Tehdään aloitus korkeus seuraavasti:
// Se pinta, miltä kaikki löytyvät 1. kerran sekä paine, että korkeus arvot
// ja lisäksi joko lämpötila tai tuulennopeus.
// Tai jos sellaista ei löydy asetetaan arvoksi 0.
// Poikkeus: jos löytyy paine ja korkeus tiedot alempaa ja heti sen jälkeen tarpeeksi
// lähellä (<  4 Hpa) on leveli, jossa on muita tietoja, mutta ei korkeutta, hyväksytään
// aiemman levelin korkeus.
void NFmiSoundingDataOpt1::InitZeroHeight(void)
{
  float closeLevelHeight = kFloatMissing;
  float closeLevelPressure = kFloatMissing;
  int closeLevelHeightIndex = -1;
  itsZeroHeight = 0;
  itsZeroHeightIndex = -1;
  // oletus, kaikki vektorit on alustettu saman kokoisiksi kuin paine vektori
  if (itsPressureData.size() > 0)
  {
    for (int i = 0; i < static_cast<int>(itsPressureData.size()); i++)
    {
      float P = itsPressureData[i];
      float z = itsGeomHeightData[i];
      float T = itsTemperatureData[i];
      float WS = itsWindSpeedData[i];
      if (P != kFloatMissing)
      {
        if (z != kFloatMissing &&
            (T != kFloatMissing ||  // lämpötilaa ei ehkä tarvitse olla etsittäessä ensimmäistä
                                    // validia kerrosta
             WS !=
                 kFloatMissing)  // tuulta ei tarvitse olla etsittäessä ensimmäistä validia kerrosta
            )
        {
          itsZeroHeight = z;
          itsZeroHeightIndex = i;
          break;  // lopetetaan kun 1. löytyi
        }
        else if (z != kFloatMissing)
        {  // jos löytyi vain paine ja korkeustiedot, otetaan ne varmuuden vuoksi talteen
          closeLevelHeight = z;
          closeLevelPressure = P;
          closeLevelHeightIndex = i;
        }
        else if (::fabs(closeLevelPressure - P) <= 4 &&
                 (T != kFloatMissing ||  // lämpötilaa ei ehkä tarvitse olla etsittäessä ensimmäistä
                                         // validia kerrosta
                  WS != kFloatMissing)  // tuulta ei tarvitse olla etsittäessä ensimmäistä validia
                                        // kerrosta
                 )
        {
          itsZeroHeight = closeLevelHeight;
          itsZeroHeightIndex = closeLevelHeightIndex;
          break;  // lopetetaan kun 1. löytyi
        }
      }
    }
  }
}

std::deque<float> &NFmiSoundingDataOpt1::GetParamData(FmiParameterName theId)
{
  static std::deque<float> dummy;
  switch (theId)
  {
    case kFmiTemperature:
      return itsTemperatureData;
    case kFmiDewPoint:
      return itsDewPointData;
    case kFmiHumidity:
      return itsHumidityData;
    case kFmiPressure:
      return itsPressureData;
    case kFmiGeopHeight:
    case kFmiGeomHeight:
    case kFmiFlAltitude:
      return itsGeomHeightData;
    case kFmiWindSpeedMS:
      return itsWindSpeedData;
    case kFmiWindDirection:
      return itsWindDirectionData;
    case kFmiWindUMS:
      return itsWindComponentUData;
    case kFmiWindVMS:
      return itsWindComponentVData;
    case kFmiWindVectorMS:
      return itsWindVectorData;
    case kFmiTotalCloudCover:
      return itsTotalCloudinessData;
    default:
      throw std::runtime_error(
          std::string(
              "NFmiSoundingDataOpt1::GetParamData - wrong paramId given (Error in Program?): ") +
          NFmiStringTools::Convert<int>(theId));
  }
}

void NFmiSoundingDataOpt1::ClearDatas(void)
{
  std::deque<float>().swap(itsTemperatureData);
  std::deque<float>().swap(itsDewPointData);
  std::deque<float>().swap(itsHumidityData);
  std::deque<float>().swap(itsPressureData);
  std::deque<float>().swap(itsGeomHeightData);
  std::deque<float>().swap(itsWindSpeedData);
  std::deque<float>().swap(itsWindDirectionData);
  std::deque<float>().swap(itsWindComponentUData);
  std::deque<float>().swap(itsWindComponentVData);
  std::deque<float>().swap(itsWindVectorData);
  std::deque<float>().swap(itsTotalCloudinessData);

  fPressureDataAvailable = false;
  fHeightDataAvailable = false;
  itsLFCIndexCache.Clear();
}

bool NFmiSoundingDataOpt1::ModifyT2DryAdiapaticBelowGivenP(double P, double T)
{
  if (P == kFloatMissing || T == kFloatMissing)
    return false;

  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &tV = GetParamData(kFmiTemperature);
  if (pV.size() > 0 && pV.size() == tV.size())
  {
    unsigned int ssize = static_cast<unsigned int>(pV.size());
    float wantedTPot = static_cast<float>(NFmiSoundingFunctions::T2tpot(T, P));
    float currentP = 1000;
    for (unsigned int i = 0; i < ssize; i++)
    {
      currentP = pV[i];
      if (currentP >= P)
      {  // muutoksia siis tehtiin niin kauan kuin oltiin alle annetun paineen
        float wantedT = static_cast<float>(NFmiSoundingFunctions::Tpot2t(wantedTPot, currentP));
        tV[i] = wantedT;
      }
      else
        break;
    }
    return true;
  }
  return false;
}

bool NFmiSoundingDataOpt1::ModifyTd2MixingRatioBelowGivenP(double P, double T, double Td)
{
  if (P == kFloatMissing || Td == kFloatMissing)
    return false;

  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &tV = GetParamData(kFmiTemperature);
  std::deque<float> &tdV = GetParamData(kFmiDewPoint);
  if (pV.size() > 0 && pV.size() == tV.size() && pV.size() == tdV.size())
  {
    unsigned int ssize = static_cast<unsigned int>(pV.size());

    float wantedMixRatio = static_cast<float>(NFmiSoundingFunctions::CalcMixingRatio(T, Td, P));
    for (unsigned int i = 0; i < ssize; i++)
    {
      float currentP = pV[i];
      if (currentP >= P)
      {  // muutoksia siis tehtiin niin kauan kuin oltiin alle annetun paineen
        float wantedTd = static_cast<float>(
            NFmiSoundingFunctions::CalcDewPoint(tV[i], wantedMixRatio, currentP));
        tdV[i] = wantedTd;
      }
      else
        break;
    }
    return true;
  }
  return false;
}

bool NFmiSoundingDataOpt1::ModifyTd2MoistAdiapaticBelowGivenP(double P, double Td)
{
  if (P == kFloatMissing || Td == kFloatMissing)
    return false;

  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &tdV = GetParamData(kFmiDewPoint);
  if (pV.size() > 0 && pV.size() == tdV.size())
  {
    unsigned int ssize = static_cast<unsigned int>(pV.size());

    float AOS = static_cast<float>(NFmiSoundingFunctions::OS(Td, P));
    float currentP = 1000;
    for (unsigned int i = 0; i < ssize; i++)
    {
      currentP = pV[i];
      if (currentP >= P)
      {  // muutoksia siis tehtiin niin kauan kuin oltiin alle annetun paineen
        float ATSA = static_cast<float>(NFmiSoundingFunctions::TSA(AOS, currentP));
        tdV[i] = ATSA;
      }
      else
        break;
    }
    return true;
  }
  return false;
}

static float FixCircularValues(float theValue, float modulorValue)
{
  if (theValue != kFloatMissing)
  {
    if (theValue < 0)
      return modulorValue - ::fmod(-theValue, modulorValue);
    else
      return ::fmod(theValue, modulorValue);
  }
  return theValue;
}

static float FixValuesWithLimits(float theValue, float minValue, float maxValue)
{
  if (theValue != kFloatMissing)
  {
    if (minValue != kFloatMissing)
      theValue = FmiMax(theValue, minValue);
    if (maxValue != kFloatMissing)
      theValue = FmiMin(theValue, maxValue);
  }
  return theValue;
}

bool NFmiSoundingDataOpt1::Add2ParamAtNearestP(float P,
                                               FmiParameterName parId,
                                               float addValue,
                                               float minValue,
                                               float maxValue,
                                               bool fCircularValue)
{
  if (P == kFloatMissing)
    return false;

  std::deque<float> &pV = GetParamData(kFmiPressure);
  std::deque<float> &paramV = GetParamData(parId);
  if (pV.size() > 0 && pV.size() == paramV.size())
  {
    float currentP = kFloatMissing;
    float currentParam = kFloatMissing;
    float closestPdiff = kFloatMissing;
    unsigned int closestIndex = 0;
    unsigned int ssize = static_cast<unsigned int>(pV.size());
    for (unsigned int i = 0; i < ssize; i++)
    {
      currentP = pV[i];
      currentParam = paramV[i];
      if (currentP != kFloatMissing && currentParam != kFloatMissing)
      {
        if (closestPdiff > ::fabs(P - currentP))
        {
          closestPdiff = ::fabs(P - currentP);
          closestIndex = i;
        }
      }
      if (currentP < P && closestPdiff != kFloatMissing)
        break;
    }

    if (closestPdiff != kFloatMissing)
    {
      float closestParamValue = paramV[closestIndex];
      closestParamValue += addValue;
      if (fCircularValue)
        closestParamValue = ::FixCircularValues(closestParamValue, maxValue);
      else
        closestParamValue = ::FixValuesWithLimits(closestParamValue, minValue, maxValue);

      paramV[closestIndex] = closestParamValue;
      return true;
    }
  }
  return false;
}

static float CalcU(float WS, float WD)
{
  if (WD ==
      999)  // jos tuulensuunta on vaihtelevaa (999), palautetaan 0 arvo (voisi olla myös puuttuvaa)
    return 0.f;
  float value = kFloatMissing;
  if (WS != kFloatMissing && WD != kFloatMissing)
  {
    value = WS * sin(((static_cast<int>(180 + WD) % 360) / 360.f) *
                     (2.f * static_cast<float>(kPii)));  // huom! tuulen suunta pitää ensin kääntää
                                                         // 180 astetta ja sitten muuttaa
                                                         // radiaaneiksi kulma/360 * 2*pii
  }
  return value;
}

static float CalcV(float WS, float WD)
{
  if (WD ==
      999)  // jos tuulensuunta on vaihtelevaa (999), palautetaan 0 arvo (voisi olla myös puuttuvaa)
    return 0;
  float value = kFloatMissing;
  if (WS != kFloatMissing && WD != kFloatMissing)
  {
    value = WS * cos(((static_cast<int>(180 + WD) % 360) / 360.f) *
                     (2.f * static_cast<float>(kPii)));  // huom! tuulen suunta pitää ensin kääntää
                                                         // 180 astetta ja sitten muuttaa
                                                         // radiaaneiksi kulma/360 * 2*pii
  }
  return value;
}

void NFmiSoundingDataOpt1::UpdateUandVParams(void)
{
  std::deque<float> &wsV = GetParamData(kFmiWindSpeedMS);
  std::deque<float> &wdV = GetParamData(kFmiWindDirection);
  std::deque<float> &uV = GetParamData(kFmiWindUMS);
  std::deque<float> &vV = GetParamData(kFmiWindVMS);
  if (wsV.size() > 0 && wsV.size() == wdV.size() && wsV.size() == uV.size() &&
      wsV.size() == vV.size())
  {
    unsigned int ssize = static_cast<unsigned int>(wsV.size());
    for (unsigned int i = 0; i < ssize; i++)
    {
      uV[i] = ::CalcU(wsV[i], wdV[i]);
      vV[i] = ::CalcV(wsV[i], wdV[i]);
    }
  }
}

// tarkistaa onko kyseisellä ajanhetkellä ja asemalla ei puuttuvaa luotaus-dataa
bool NFmiSoundingDataOpt1::HasRealSoundingData(
    boost::shared_ptr<NFmiFastQueryInfo> &theSoundingLevelInfo)
{
  if (theSoundingLevelInfo->Param(kFmiPressure) || theSoundingLevelInfo->Param(kFmiGeomHeight) ||
      theSoundingLevelInfo->Param(kFmiGeopHeight))
  {
    int cc = 0;
    for (theSoundingLevelInfo->ResetLevel(); theSoundingLevelInfo->NextLevel(); cc++)
    {
      if (theSoundingLevelInfo->FloatValue() != kFloatMissing)
        return true;  // jos miltään alku leveliltä löytyy yhtään korkeusdataa, on käyrä
                      // 'piirrettävissä'
      if (cc > 10)    // pitää löytyä dataa 10 ensimmäisen kerroksen aikana
        break;
    }
    cc = 0;  // käydään dataa läpi myös toisesta päästä, jos ei löytynyt
    for (theSoundingLevelInfo->LastLevel(); theSoundingLevelInfo->PreviousLevel(); cc++)
    {
      if (theSoundingLevelInfo->FloatValue() != kFloatMissing)
        return true;  // jos miltään alku leveliltä löytyy yhtään korkeusdataa, on käyrä
                      // 'piirrettävissä'
      if (cc > 10)    // pitää löytyä dataa 10 ensimmäisen kerroksen aikana
        break;
    }
  }
  return false;
}

// SHOW Showalter index
// SHOW	= T500 - Tparcel
// T500 = Temperature in Celsius at 500 mb
// Tparcel = Temperature in Celsius at 500 mb of a parcel lifted from 850 mb
double NFmiSoundingDataOpt1::CalcSHOWIndex(void)
{
  double indexValue = kFloatMissing;
  double T_850 = GetValueAtPressure(kFmiTemperature, 850);
  double Td_850 = GetValueAtPressure(kFmiDewPoint, 850);
  if (T_850 != kFloatMissing && Td_850 != kFloatMissing)
  {
    double Tparcel_from850to500 = CalcTOfLiftedAirParcel(T_850, Td_850, 850, 500);
    // 3. SHOW = T500 - Tparcel_from850to500
    double T500 = GetValueAtPressure(kFmiTemperature, 500);
    if (T500 != kFloatMissing && Tparcel_from850to500 != kFloatMissing)
      indexValue = T500 - Tparcel_from850to500;
  }
  return indexValue;
}

// LIFT Lifted index
// LIFT	= T500 - Tparcel
// T500 = temperature in Celsius of the environment at 500 mb
// Tparcel = 500 mb temperature in Celsius of a lifted parcel with the average pressure,
//			 temperature, and dewpoint of the layer 500 m above the surface.
double NFmiSoundingDataOpt1::CalcLIFTIndex(void)
{
  double indexValue = kFloatMissing;
  double P_500m_avg = kFloatMissing;
  double T_500m_avg = kFloatMissing;
  double Td_500m_avg = kFloatMissing;

  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  double h1 = 0 + ZeroHeight();
  double h2 = 500 + ZeroHeight();

  if (CalcLCLAvgValues(h1, h2, T_500m_avg, Td_500m_avg, P_500m_avg, false))
  {
    double Tparcel_from500mAvgTo500 =
        CalcTOfLiftedAirParcel(T_500m_avg, Td_500m_avg, P_500m_avg, 500);
    // 3. LIFT	= T500 - Tparcel_from500mAvgTo500
    double T500 = GetValueAtPressure(kFmiTemperature, 500);
    if (T500 != kFloatMissing && Tparcel_from500mAvgTo500 != kFloatMissing)
      indexValue = T500 - Tparcel_from500mAvgTo500;
  }
  return indexValue;
}

// KINX K index
// KINX = ( T850 - T500 ) + TD850 - ( T700 - TD700 )
//	T850 = Temperature in Celsius at 850 mb
//	T500 = Temperature in Celsius at 500 mb
//	TD850 = Dewpoint in Celsius at 850 mb
//	T700 = Temperature in Celsius at 700 mb
//	TD700 = Dewpoint in Celsius at 700 mb
double NFmiSoundingDataOpt1::CalcKINXIndex(void)
{
  double T850 = GetValueAtPressure(kFmiTemperature, 850);
  double T500 = GetValueAtPressure(kFmiTemperature, 500);
  double TD850 = GetValueAtPressure(kFmiDewPoint, 850);
  double T700 = GetValueAtPressure(kFmiTemperature, 700);
  double TD700 = GetValueAtPressure(kFmiDewPoint, 700);
  if (T850 != kFloatMissing && T500 != kFloatMissing && TD850 != kFloatMissing &&
      T700 != kFloatMissing && TD700 != kFloatMissing)
    return (T850 - T500) + TD850 - (T700 - TD700);
  return kFloatMissing;
}

// CTOT	Cross Totals index
//	CTOT	= TD850 - T500
//		TD850 	= Dewpoint in Celsius at 850 mb
//		T500 	= Temperature in Celsius at 500 mb
double NFmiSoundingDataOpt1::CalcCTOTIndex(void)
{
  double T500 = GetValueAtPressure(kFmiTemperature, 500);
  double TD850 = GetValueAtPressure(kFmiDewPoint, 850);
  if (T500 != kFloatMissing && TD850 != kFloatMissing)
    return (TD850 - T500);
  return kFloatMissing;
}

// VTOT	Vertical Totals index
//	VTOT	= T850 - T500
//		T850	= Temperature in Celsius at 850 mb
//		T500	= Temperature in Celsius at 500 mb
double NFmiSoundingDataOpt1::CalcVTOTIndex(void)
{
  double T500 = GetValueAtPressure(kFmiTemperature, 500);
  double T850 = GetValueAtPressure(kFmiTemperature, 850);
  if (T500 != kFloatMissing && T850 != kFloatMissing)
    return (T850 - T500);
  return kFloatMissing;
}

// TOTL	Total Totals index
//	TOTL	= ( T850 - T500 ) + ( TD850 - T500 )
//		T850 	= Temperature in Celsius at 850 mb
//		TD850	= Dewpoint in Celsius at 850 mb
//		T500 	= Temperature in Celsius at 500 mb
double NFmiSoundingDataOpt1::CalcTOTLIndex(void)
{
  double T850 = GetValueAtPressure(kFmiTemperature, 850);
  double T500 = GetValueAtPressure(kFmiTemperature, 500);
  double TD850 = GetValueAtPressure(kFmiDewPoint, 850);
  if (T850 != kFloatMissing && T500 != kFloatMissing && TD850 != kFloatMissing)
    return (T850 - T500) + (TD850 - T500);
  return kFloatMissing;
}

// HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
bool NFmiSoundingDataOpt1::GetValuesNeededInLCLCalculations(FmiLCLCalcType theLCLCalcType,
                                                            double &T,
                                                            double &Td,
                                                            double &P)
{
  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  double h1 = 0 + ZeroHeight();
  double h2 = 500 + ZeroHeight();

  bool status = false;
  if (theLCLCalcType == kLCLCalc500m)
    status = CalcLCLAvgValues(h1, h2, T, Td, P, false);
  else if (theLCLCalcType == kLCLCalc500m2)
    status = CalcLCLAvgValues(h1, h2, T, Td, P, true);
  else if (theLCLCalcType == kLCLCalcMostUnstable)
  {
    double maxThetaE = 0;
    status = FindHighestThetaE(T, Td, P, maxThetaE, 600);  // rajoitetaan max thetan etsintä 600
                                                           // mb:en asti (oli aiemmin 500, mutta
                                                           // niin korkealta voi löytyä suuria
                                                           // arvoja, jotka sotkevat todellisen
                                                           // MU-Capen etsinnän)
  }

  if (status == false)  // jos muu ei auta, laske pinta suureiden avulla
  {
    P = 1100;
    if (!GetValuesStartingLookingFromPressureLevel(
            T, Td, P))  // sitten lähimmät pinta arvot, jotka joskus yli 500 m
      return false;
  }
  return true;
}

// LCL-levelin painepinnan lasku käyttäen luotauksen haluttuja arvoja
double NFmiSoundingDataOpt1::CalcLCLPressureLevel(FmiLCLCalcType theLCLCalcType)
{
  // 1. calc T,Td,P values from 500 m layer avg or surface values
  double T = kFloatMissing, Td = kFloatMissing, P = kFloatMissing;
  if (!GetValuesNeededInLCLCalculations(theLCLCalcType, T, Td, P))
    return kFloatMissing;

  return NFmiSoundingFunctions::CalcLCLPressureFast(T, Td, P);
}

// Claculates LCL (Lifted Condensation Level)
// halutulla tavalla
double NFmiSoundingDataOpt1::CalcLCLIndex(FmiLCLCalcType theLCLCalcType)
{
  return CalcLCLPressureLevel(theLCLCalcType);
}
// palauttaa LCL:n korkeuden metreissä
double NFmiSoundingDataOpt1::CalcLCLHeightIndex(FmiLCLCalcType theLCLCalcType)
{
  return GetValueAtPressure(kFmiGeomHeight,
                            static_cast<float>(CalcLCLPressureLevel(theLCLCalcType)));
}

void NFmiSoundingDataOpt1::FillLFCIndexCache(FmiLCLCalcType theLCLCalcType,
                                             double theLfcIndexValue,
                                             double theELValue)
{
  switch (theLCLCalcType)
  {
    case kLCLCalcSurface:
    {
      itsLFCIndexCache.fSurfaceValueInitialized = true;
      itsLFCIndexCache.itsSurfaceValue = theLfcIndexValue;
      itsLFCIndexCache.itsSurfaceELValue = theELValue;
      break;
    }
    case kLCLCalc500m:
    {
      itsLFCIndexCache.f500mValueInitialized = true;
      itsLFCIndexCache.its500mValue = theLfcIndexValue;
      itsLFCIndexCache.its500mELValue = theELValue;
      break;
    }
    case kLCLCalc500m2:
    {
      itsLFCIndexCache.f500m2ValueInitialized = true;
      itsLFCIndexCache.its500m2Value = theLfcIndexValue;
      itsLFCIndexCache.its500m2ELValue = theELValue;
      break;
    }
    case kLCLCalcMostUnstable:
    {
      itsLFCIndexCache.fMostUnstableValueInitialized = true;
      itsLFCIndexCache.itsMostUnstableValue = theLfcIndexValue;
      itsLFCIndexCache.itsMostUnstableELValue = theELValue;
      break;
    }
    case kLCLCalcNone:
      break;
  }
}

bool NFmiSoundingDataOpt1::CheckLFCIndexCache(FmiLCLCalcType theLCLCalcTypeIn,
                                              double &theLfcIndexValueOut,
                                              double &theELValueOut)
{
  switch (theLCLCalcTypeIn)
  {
    case kLCLCalcSurface:
    {
      if (itsLFCIndexCache.fSurfaceValueInitialized)
      {
        theLfcIndexValueOut = itsLFCIndexCache.itsSurfaceValue;
        theELValueOut = itsLFCIndexCache.itsSurfaceELValue;
        return true;
      }
      break;
    }
    case kLCLCalc500m:
    {
      if (itsLFCIndexCache.f500mValueInitialized)
      {
        theLfcIndexValueOut = itsLFCIndexCache.its500mValue;
        theELValueOut = itsLFCIndexCache.its500mELValue;
        return true;
      }
      break;
    }
    case kLCLCalc500m2:
    {
      if (itsLFCIndexCache.f500m2ValueInitialized)
      {
        theLfcIndexValueOut = itsLFCIndexCache.its500m2Value;
        theELValueOut = itsLFCIndexCache.its500m2ELValue;
        return true;
      }
      break;
    }
    case kLCLCalcMostUnstable:
    {
      if (itsLFCIndexCache.fMostUnstableValueInitialized)
      {
        theLfcIndexValueOut = itsLFCIndexCache.itsMostUnstableValue;
        theELValueOut = itsLFCIndexCache.itsMostUnstableELValue;
        return true;
      }
      break;
    }
    case kLCLCalcNone:
      break;
  }
  return false;
}

using NFmiSoundingFunctions::MyPoint;

static double CalcCrossingPressureFromPoints(
    double P1, double P2, double T1, double T2, double theCurrentParcelT, double theLastParcelT)
{
  MyPoint p1(P1, T1);
  MyPoint p2(P2, T2);
  MyPoint p3(P1, theCurrentParcelT);
  MyPoint p4(P2, theLastParcelT);

  MyPoint intersectionPoint = NFmiSoundingFunctions::CalcTwoLineIntersectionPoint(p1, p2, p3, p4);
  return intersectionPoint.x;
}

class CapeAreaLfcData
{
 public:
  CapeAreaLfcData(void)
      : itsCapeAreaSize(0),
        itsLFC(kFloatMissing),
        itsEL(kFloatMissing),
        itsSizePoints(),
        fLFConWarmerSide(false)
  {
  }

  void Reset(void) { *this = CapeAreaLfcData(); }
  void AddCapeAreaPoint(
      const MyPoint &thePoint)  // x on positiivinen T-diff ja y on kyseisen levelin paine
  {
    itsSizePoints.push_back(thePoint);
  }

  void CalcData(void)
  {  // Tätä kutsutaan kun luokalle on lisätty kaikki cape-alueen pisteet
    if (itsSizePoints.size())
    {
      CalcCapeAreaSize();
      if (itsCapeAreaSize > 0)
      {
        itsLFC = itsSizePoints[0].y;
        itsEL = itsSizePoints[itsSizePoints.size() - 1].y;
      }
    }
  }

  double CapeAreaSize(void) const { return itsCapeAreaSize; }
  double LFC(void) const { return itsLFC; }
  double EL(void) const { return itsEL; }
  bool LFConWarmerSide(void) const { return fLFConWarmerSide; }
  void LFConWarmerSide(bool newValue) { fLFConWarmerSide = newValue; }
 private:
  void CalcCapeAreaSize(void)
  {
    itsCapeAreaSize = 0;
    if (itsSizePoints.size() >= 2)
    {  // pitää olla vähintään 2 pistettä, että pinta-ala voidaan laskea
      if (itsSizePoints[0].IsValid())
      {
        for (size_t i = 1; i < itsSizePoints.size(); i++)
        {
          if (itsSizePoints[i].IsValid())
            itsCapeAreaSize += CalcTrapezoidArea(itsSizePoints[i - 1], itsSizePoints[i]);
          else
          {  // joku piste sisälsi kFloatMissing arvon, koko data tulkitaan kelvottomaksi
            itsCapeAreaSize = 0;
            break;
          }
        }
      }
    }
  }

  double CalcTrapezoidArea(const MyPoint &P1, const MyPoint &P2)
  {
    // puolisuunnikkaan pinta-alan lasku (kolmio on erikoistapaus, jolloin toisen pisteen x-arvo on
    // 0):
    // A = (a + b) * h / 2 , missä
    // a = P1.x
    // b = P2.x
    // h = abs(P1.y - P2.y)
    double A = (P1.x + P2.x) * ::fabs(P1.y - P2.y) / 2.;
    return A;
  }

  double itsCapeAreaSize;  // viitteellinen CAPE alueen koko
  double itsLFC;
  double itsEL;
  std::vector<MyPoint> itsSizePoints;  // tähän talletetaan cape-alueen positiivinen lämpötilaerotus
                                       // ja paine-taso. Näiden pisteiden avulla voidaan laskea
                                       // suuntaa antava cape-alueen koko
  bool fLFConWarmerSide;  // onko LCL luotauksen lämpimämmällä puolella, jos on, tulee LFC:ksi LCL
                          // ja tämä alue kelpaa LFC+EL arvoihin
};

typedef std::vector<CapeAreaLfcData> LFCSearchDataVec;

static void InsertCapeAreaDataToVec(CapeAreaLfcData &theCapeAreaData,
                                    LFCSearchDataVec &theCapeAreaDataVec)
{
  theCapeAreaData.CalcData();
  if (theCapeAreaData.CapeAreaSize() > 0)  // lisätään listaan vain jos area oli suurempi kuin 0
    theCapeAreaDataVec.push_back(theCapeAreaData);
  // Lopuksi currentti capeAreaData pitää nollata
  theCapeAreaData.Reset();
}

static void GetLFCandELValues(LFCSearchDataVec &theLFCDataVecIn,
                              double LCLin,
                              double &LFCout,
                              double &ELout)
{
  // alustetaan arvot puuttuviksi
  LFCout = kFloatMissing;
  ELout = kFloatMissing;
  // katsotaan löytyikö CAPE-alueita
  if (theLFCDataVecIn.size())
  {
    // käydään loopissa läpi CAPE-alueita ja katsotaan löytyykö cape-aluetta, jossa LFC on LCL:n ylä
    // puolella, ja otetaan 1. sellainen käyttöön
    // TAI jos LCL-piste on ollut lämpimämmällä puolella, laitetaan LFC:ksi LCL ja EL:ksi poistumis
    // piste
    for (size_t i = 0; i < theLFCDataVecIn.size(); i++)
    {
      if (LCLin >= theLFCDataVecIn[i].LFC() || theLFCDataVecIn[i].LFConWarmerSide())
      {
        LFCout = theLFCDataVecIn[i].LFC();
        ELout = theLFCDataVecIn[i].EL();
        break;
      }
    }
  }
  if (LFCout != kFloatMissing && LFCout > LCLin)  // jos löytyi LFC, mutta sen arvo on alempana kuin
                                                  // LCL, laitetaan  LFC:n arvoksi LCL
    LFCout = LCLin;
}

static const double gUsedEpsilon =
    std::numeric_limits<double>::epsilon() * 500;  // tämä pitää olla luokkaa 10 pot -14

// LFC ja EL lasketaan seuraavasti:
// 1. Laske LCL.
// 2. Nosta ilmapakettia LCL-pisteen kautta
// 3. Tutki missä ilmapaketti menee luotauksen oikealla puolelle ja vastaavasti tuleesieltä pois.
// 4. Laita tälläiset CAPE-alueet talteen vektoriin myöhempiä tarkasteluja varten.
// 5. Laske lopuksi CAPE alueen entry- ja exit-pisteet.
// 6. Käy lopuksi läpi CAPE-alueet alhaalta ylös
// 7. Etsi ensimmäinen sellainen missä LFC on LCL:n yläpuolella ja ota siitä LFC ja EL arvot.
// 8. Jos ei löydy sellaista CAPE-aluetta, saavat LFC ja EL puuttuvan arvon.
double NFmiSoundingDataOpt1::CalcLFCIndex(FmiLCLCalcType theLCLCalcType, double &EL)
{
  double lfcIndexValue = kFloatMissing;
  // 1. Katso löytyykö valmiiksi lasketut arvot cachesta.
  if (CheckLFCIndexCache(theLCLCalcType, lfcIndexValue, EL))
    return lfcIndexValue;

  // 2. Hae halutun laskentatavan (sur/500mix/mostUnstable) mukaiset T, Td ja P arvot.
  double T = kFloatMissing, Td = kFloatMissing, P = kFloatMissing;
  if (!GetValuesNeededInLCLCalculations(theLCLCalcType, T, Td, P))
  {  // jos arvoja ei saada, täytä LFC cache puuttuvilla arvoilla.
    FillLFCIndexCache(theLCLCalcType, kFloatMissing, kFloatMissing);
    return kFloatMissing;
  }
  // 3. Laske LCL korkaus.
  double LCL = NFmiSoundingFunctions::CalcLCLPressureFast(T, Td, P);
  std::deque<float> &pValues = GetParamData(kFmiPressure);
  std::deque<float> &tValues = GetParamData(kFmiTemperature);
  double P_current = kFloatMissing;
  double P_previous = kFloatMissing;
  double T_current = kFloatMissing;
  double T_previous = kFloatMissing;
  double LFC = kFloatMissing;
  double TLifted_current = kFloatMissing;  // nostetun ilmapaketin lämpötila
  double TLifted_previous = kFloatMissing;
  double Diff_current = kFloatMissing;  // nostetun paketin ja ympäröivän ilman lämpötila erotus
  double Diff_previous = kFloatMissing;
  bool lclPointProcessed = false;
  bool processingLCLpoint = false;
  bool firstLevel = true;
  CapeAreaLfcData capeAreaData;      // tähän kerätään currentin cape-alueen data
  LFCSearchDataVec capeAreaDataVec;  // tähän kerätään kaikkien cape-alueiden datat sortattuna niin
                                     // että suurin on ensimmäisenä

  for (size_t i = 0; i < pValues.size(); i++)
  {
    // 4. Käy läpi kaikki ei puuttuvat P ja T kerrokset. Lisäksi aloitus korkeuden pitää olla sama
    // (tai korkeammalla oleva pinta)
    // kuin saatiin P:n arvoksi aiemmin.
    P_current = pValues[i];
    T_current = tValues[i];
    if (P_current != kFloatMissing && T_current != kFloatMissing && P_current <= P)
    {
      // 5. LCL-pisteeseen pitää saada yksi laskentapiste, muuten laskut menevät tietyissä
      // tilanteissa vähän pieleen
      if (lclPointProcessed == false && P_current < LCL && P_previous > LCL)
      {
        lclPointProcessed = true;
        P_current = LCL;
        T_current = GetValueAtPressure(kFmiTemperature, static_cast<float>(LCL));
        i--;  // pitää laittaa indekse pykälää alemmaksi, että LCL-pisteen jälkeiset laskut menevät
        // oikein
        processingLCLpoint = true;
      }

      // 6. Laske nostetun ilmapaketin lämpötila
      TLifted_current = CalcTOfLiftedAirParcel(T, Td, P, P_current);
      // 7. Laske nostetun ilmapaketin ja ympäröivän ilman lämpötila erotus
      Diff_current = TLifted_current - T_current;
      if (NFmiQueryDataUtil::IsEqualEnough(Diff_current, 0., gUsedEpsilon))
        Diff_current = 0;  // linux vs windows - float vs double laskenta ero sovittelu yritys
      if (processingLCLpoint && Diff_current > 0)
        capeAreaData.LFConWarmerSide(true);

      if (firstLevel && Diff_current > 0)
      {  // Jos on heti 'pinta' CAPE alue, sille pitää tehdä erillinen hanskaus
        capeAreaData.AddCapeAreaPoint(MyPoint(Diff_current, P_current));
      }
      else if (Diff_current > 0 && Diff_previous < 0)
      {  // Nyt mentiin CAPE-alueeseen, pitää lisätä kaksi pistettä eli entry-piste ja currentti
         // piste
        double entryPValue = ::CalcCrossingPressureFromPoints(
            P_current, P_previous, T_current, T_previous, TLifted_current, TLifted_previous);
        capeAreaData.AddCapeAreaPoint(MyPoint(0, entryPValue));
        capeAreaData.AddCapeAreaPoint(MyPoint(Diff_current, P_current));
      }
      else if (Diff_current < 0 && Diff_previous > 0)
      {  // Nyt poistutaan CAPE-alueesta, pitää lisätä poistumis piste
        double leavingPValue = ::CalcCrossingPressureFromPoints(
            P_current, P_previous, T_current, T_previous, TLifted_current, TLifted_previous);
        capeAreaData.AddCapeAreaPoint(MyPoint(0, leavingPValue));
        // Sitten pitää laskea LFC-datan arvot ja lisätä data mappiin jos CAPE:n alue oli suurempi
        // kuin 0
        ::InsertCapeAreaDataToVec(capeAreaData, capeAreaDataVec);
      }
      else if (Diff_current > 0)
      {  // Lisätään tämä leveli currenttiin CAPE alueeseen
        capeAreaData.AddCapeAreaPoint(MyPoint(Diff_current, P_current));
      }

      // Loopin loppuksi pitää päivittää eri previous arvot
      P_previous = P_current;
      T_previous = T_current;
      TLifted_previous = TLifted_current;
      Diff_previous = Diff_current;
      firstLevel = false;
      processingLCLpoint = false;
    }
  }

  // Jos luotaus loppui vaikka kesken, katsotaan jos saatiin kuitenkin CAPE aluetta kasaan
  ::InsertCapeAreaDataToVec(capeAreaData, capeAreaDataVec);

  ::GetLFCandELValues(capeAreaDataVec, LCL, LFC, EL);

  FillLFCIndexCache(theLCLCalcType, LFC, EL);
  return LFC;
}

// palauttaa LFC:n ja  EL:n korkeuden metreissä
double NFmiSoundingDataOpt1::CalcLFCHeightIndex(FmiLCLCalcType theLCLCalcType, double &ELheigth)
{
  double tmpValue = CalcLFCIndex(theLCLCalcType, ELheigth);
  ELheigth = GetValueAtPressure(kFmiGeomHeight, static_cast<float>(ELheigth));
  return GetValueAtPressure(kFmiGeomHeight, static_cast<float>(tmpValue));
}

// Calculates CAPE (500 m mix)
// theHeightLimit jos halutaan, voidaan cape lasku rajoittaa alle jonkin korkeus arvon (esim. 3000
// m)
double NFmiSoundingDataOpt1::CalcCAPE500Index(FmiLCLCalcType theLCLCalcType, double theHeightLimit)
{
  // 1. calc T,Td,P values from 500 m layer avg or surface values
  double T = kFloatMissing, Td = kFloatMissing, P = kFloatMissing;
  if (!GetValuesNeededInLCLCalculations(theLCLCalcType, T, Td, P))
    return kFloatMissing;

  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  if (theHeightLimit != kFloatMissing)
    theHeightLimit += ZeroHeight();

  std::deque<float> &pValues = GetParamData(kFmiPressure);
  std::deque<float> &tValues = GetParamData(kFmiTemperature);
  size_t ssize = pValues.size();
  double CAPE = 0;
  double g = 9.81;  // acceleration by gravity
  double Tparcel = 0;
  double Tenvi = 0;
  double currentZ = 0;
  double lastZ = kFloatMissing;
  double deltaZ = kFloatMissing;
  double TK = 273.15;
  for (size_t i = 0; i < ssize; i++)
  {
    if (pValues[i] != kFloatMissing &&
        pValues[i] < P)  // aloitetaan LFC etsintä vasta 'aloitus' korkeuden jälkeen
    {
      if (tValues[i] != kFloatMissing)  // kaikilla painepinnoilla ei ole lämpötilaa
      {
        double TofLiftedParcer = CalcTOfLiftedAirParcel(T, Td, P, pValues[i]);
        currentZ = GetValueAtPressure(kFmiGeomHeight, pValues[i]);  // interpoloidaan jos tarvis

        // integrate here if T parcel is greater than T environment
        if (TofLiftedParcer > tValues[i] && currentZ != kFloatMissing && lastZ != kFloatMissing &&
            TofLiftedParcer != kFloatMissing)
        {
          Tparcel = TofLiftedParcer + TK;
          Tenvi = tValues[i] + TK;
          deltaZ = currentZ - lastZ;
          CAPE += g * deltaZ * ((Tparcel - Tenvi) / Tenvi);
        }
        lastZ = currentZ;

        if (theHeightLimit != kFloatMissing && currentZ > theHeightLimit)
          break;  // lopetetaan laskut jos ollaan menty korkeus rajan yli, kun ensin on laskettu
                  // tämä siivu vielä mukaan
      }
    }
  }
  return CAPE;
}

// Calculates CAPE in layer between two temperatures given
double NFmiSoundingDataOpt1::CalcCAPE_TT_Index(FmiLCLCalcType theLCLCalcType,
                                               double Thigh,
                                               double Tlow)
{
  // 1. calc T,Td,P values from 500 m layer avg or surface values
  double T = kFloatMissing, Td = kFloatMissing, P = kFloatMissing;
  if (!GetValuesNeededInLCLCalculations(theLCLCalcType, T, Td, P))
    return kFloatMissing;

  std::deque<float> &pValues = GetParamData(kFmiPressure);
  std::deque<float> &tValues = GetParamData(kFmiTemperature);
  size_t ssize = pValues.size();
  double CAPE = 0;
  double g = 9.81;  // acceleration by gravity
  double Tparcel = 0;
  double Tenvi = 0;
  double currentZ = 0;
  double lastZ = kFloatMissing;
  double deltaZ = kFloatMissing;
  double TK = 273.15;
  for (size_t i = 0; i < ssize; i++)
  {
    if (pValues[i] != kFloatMissing &&
        pValues[i] < P)  // aloitetaan LFC etsintä vasta 'aloitus' korkeuden jälkeen
    {
      if (tValues[i] != kFloatMissing)  // kaikilla painepinnoilla ei ole lämpötilaa
      {
        currentZ = GetValueAtPressure(kFmiGeomHeight, pValues[i]);  // interpoloidaan jos tarvis
        if (Tlow < tValues[i] && Thigh > tValues[i])
        {
          double TofLiftedParcer = CalcTOfLiftedAirParcel(T, Td, P, pValues[i]);

          // integrate here if T parcel is greater than T environment
          if (TofLiftedParcer > tValues[i] && currentZ != kFloatMissing && lastZ != kFloatMissing)
          {
            Tparcel = TofLiftedParcer + TK;
            Tenvi = tValues[i] + TK;
            deltaZ = currentZ - lastZ;
            CAPE += g * deltaZ * ((Tparcel - Tenvi) / Tenvi);
          }
        }
        lastZ = currentZ;
      }
    }
  }
  return CAPE;
}

// Calculates CIN
// first layer of negative (TP - TE (= T-parcel - T-envi)) unless its the last also
double NFmiSoundingDataOpt1::CalcCINIndex(FmiLCLCalcType theLCLCalcType)
{
  // 1. calc T,Td,P values from 500 m layer avg or surface values
  double T = kFloatMissing, Td = kFloatMissing, P = kFloatMissing;
  if (!GetValuesNeededInLCLCalculations(theLCLCalcType, T, Td, P))
    return kFloatMissing;

  std::deque<float> &pValues = GetParamData(kFmiPressure);
  std::deque<float> &tValues = GetParamData(kFmiTemperature);
  size_t ssize = pValues.size();
  double CIN = 0;
  double g = 9.81;  // acceleration by gravity
  double Tparcel = 0;
  double Tenvi = 0;
  double currentZ = 0;
  double lastZ = kFloatMissing;
  double deltaZ = kFloatMissing;
  double TK = 273.15;  // celsius/kelvin change
  bool firstCinLayerFound = false;
  bool capeLayerFoundAfterCin = false;
  for (size_t i = 0; i < ssize; i++)
  {
    if (pValues[i] != kFloatMissing &&
        pValues[i] < P)  // aloitetaan LFC etsintä vasta 'aloitus' korkeuden jälkeen
    {
      if (tValues[i] != kFloatMissing)  // kaikilla painepinnoilla ei ole lämpötilaa
      {
        double TofLiftedParcer = CalcTOfLiftedAirParcel(T, Td, P, pValues[i]);
        currentZ = GetValueAtPressure(kFmiGeomHeight, pValues[i]);  // interpoloidaan jos tarvis

        // integrate here if T parcel is less than T environment
        if (TofLiftedParcer <= tValues[i] && currentZ != kFloatMissing && lastZ != kFloatMissing)
        {
          Tparcel = TofLiftedParcer + TK;
          Tenvi = tValues[i] + TK;
          deltaZ = currentZ - lastZ;
          CIN += g * deltaZ * ((Tparcel - Tenvi) / Tenvi);
          firstCinLayerFound = true;
        }
        else if (firstCinLayerFound)
        {
          capeLayerFoundAfterCin = true;
          break;  // jos 1. CIN layer on löydetty ja osutaan CAPE layeriin, lopetetaan
        }
        lastZ = currentZ;
      }
    }
  }
  if (!(firstCinLayerFound && capeLayerFoundAfterCin))
    CIN = 0;
  return CIN;
}

// startH and endH are in kilometers
// HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
double NFmiSoundingDataOpt1::CalcWindBulkShearComponent(double startH,
                                                        double endH,
                                                        FmiParameterName theParId)
{
  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  startH += ZeroHeight() / 1000.;  // zero height pitää muuttaa tässä metreistä kilometreiksi!
  endH += ZeroHeight() / 1000.;    // zero height pitää muuttaa tässä metreistä kilometreiksi!

  float startValue = GetValueAtHeight(theParId, static_cast<float>(startH * 1000));
  float endValue = GetValueAtHeight(theParId, static_cast<float>(endH * 1000));
  float shearComponent = endValue - startValue;
  if (endValue == kFloatMissing || startValue == kFloatMissing)
    shearComponent = kFloatMissing;
  return shearComponent;
}

// startH and endH are in kilometers
// HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
double NFmiSoundingDataOpt1::CalcThetaEDiffIndex(double startH, double endH)
{
  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  startH += ZeroHeight() / 1000.;  // zero height pitää muuttaa tässä metreistä kilometreiksi!
  endH += ZeroHeight() / 1000.;    // zero height pitää muuttaa tässä metreistä kilometreiksi!

  float startT = GetValueAtHeight(kFmiTemperature, static_cast<float>(startH * 1000));
  float endT = GetValueAtHeight(kFmiTemperature, static_cast<float>(endH * 1000));
  float startTd = GetValueAtHeight(kFmiDewPoint, static_cast<float>(startH * 1000));
  float endTd = GetValueAtHeight(kFmiDewPoint, static_cast<float>(endH * 1000));
  float startP = GetValueAtHeight(kFmiPressure, static_cast<float>(startH * 1000));
  float endP = GetValueAtHeight(kFmiPressure, static_cast<float>(endH * 1000));
  if (startT == kFloatMissing || startTd == kFloatMissing || startP == kFloatMissing)
    return kFloatMissing;
  double startThetaE = NFmiSoundingFunctions::CalcThetaE(startT, startTd, startP);
  if (endT == kFloatMissing || endTd == kFloatMissing || endP == kFloatMissing)
    return kFloatMissing;
  double endThetaE = NFmiSoundingFunctions::CalcThetaE(endT, endTd, endP);
  if (startThetaE == kFloatMissing || endThetaE == kFloatMissing)
    return kFloatMissing;
  double diffValue = startThetaE - endThetaE;
  return diffValue;
}

// Calculates Bulk Shear between two given layers
// startH and endH are in kilometers
double NFmiSoundingDataOpt1::CalcBulkShearIndex(double startH, double endH)
{
  // HUOM! asema korkeus otetaan huomioon CalcWindBulkShearComponent-funktiossa.
  double uTot = CalcWindBulkShearComponent(startH, endH, kFmiWindUMS);
  double vTot = CalcWindBulkShearComponent(startH, endH, kFmiWindVMS);
  double BS = ::sqrt(uTot * uTot + vTot * vTot);
  if (uTot == kFloatMissing || vTot == kFloatMissing)
    BS = kFloatMissing;
  return BS;
}

/* // **********  SRH calculation help from Pieter Groenemeijer ******************

Some tips here on how tyo calculate storm-relative helciity

How to calculate storm-relative helicity

Integrate the following from p = p_surface to p = p_top (or in case of height coordinates from h_surface to h_top):

storm_rel_helicity -= ((u_ID-u[p])*(v[p]-v[p+1]))-((v_ID - v[p])*(u[p]-u[p+1]));

Here, u_ID and v_ID are the forecast storm motion vectors calculated with the so-called ID-method. These can be calculated as follows:

where

/average wind
u0_6 = average 0_6 kilometer u-wind component
v0_6 = average 0_6 kilometer v-wind component
(you should use a pressure-weighted average in case you work with height coordinates)

/shear
shr_0_6_u = u_6km - u_surface;
shr_0_6_v = v_6km - v_surface;

/ shear unit vector
shr_0_6_u_n = shr_0_6_u / ((shr_0_6_u^2 + shr_0_6_v^2)**0.5);
shr_0_6_v_n = shr_0_6_v / ((shr_0_6_u^2 + shr_0_6_v^2)** 0.5);

/id-vector components
u_ID = u0_6 + shr_0_6_v_n * 7.5;
v_ID = v0_6 - shr_0_6_u_n * 7.5;

(7.5 are meters per second... watch out when you work with knots instead)

*/  // **********
                                                                                    // SRH
                                                                                    // calculation
                                                                                    // help from
                                                                                    // Pieter
                                                                                    // Groenemeijer
// ******************

// I use same variable names as with the Pieters help evem though calculations are not
// restricted to 0-6 km layer but they can be from any layer

// shear
// startH and endH are in kilometers
double NFmiSoundingDataOpt1::CalcBulkShearIndex(double startH,
                                                double endH,
                                                FmiParameterName theParId)
{
  // HUOM! asema korkeus otetaan huomioon CalcWindBulkShearComponent-funktiossa.
  double shr_0_6_component = CalcWindBulkShearComponent(startH, endH, theParId);
  return shr_0_6_component;
}

void NFmiSoundingDataOpt1::Calc_U_V_helpers(double &shr_0_6_u_n,
                                            double &shr_0_6_v_n,
                                            double &u0_6,
                                            double &v0_6)
{
  // HUOM! asema korkeus otetaan huomioon CalcWindBulkShearComponent-funktiossa.
  double shr_0_6_u = CalcWindBulkShearComponent(0, 6, kFmiWindUMS);
  double shr_0_6_v = CalcWindBulkShearComponent(0, 6, kFmiWindVMS);
  shr_0_6_u_n = NFmiSoundingFunctions::Calc_shear_unit_v_vector(shr_0_6_u, shr_0_6_v);
  shr_0_6_v_n = NFmiSoundingFunctions::Calc_shear_unit_v_vector(shr_0_6_v, shr_0_6_u);

  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  double h1 = 0 + ZeroHeight();     // 0 m + aseman korkeus
  double h2 = 6000 + ZeroHeight();  // 6000 m + aseman korkeus
  CalcAvgWindComponentValues(h1, h2, u0_6, v0_6);
}

// lasketaan u ja v ID:t 0-6km layerissä
// tämä on hodografissa 'left'
void NFmiSoundingDataOpt1::Calc_U_and_V_IDs_left(double &u_ID, double &v_ID)
{
  double shr_0_6_u_n = 0, shr_0_6_v_n = 0, u0_6 = 0, v0_6 = 0;
  Calc_U_V_helpers(shr_0_6_u_n, shr_0_6_v_n, u0_6, v0_6);

  u_ID = NFmiSoundingFunctions::CalcU_ID_left(u0_6, shr_0_6_v_n);
  v_ID = NFmiSoundingFunctions::CalcV_ID_left(v0_6, shr_0_6_u_n);
}

// lasketaan u ja v ID:t 0-6km layerissä
// tämä on hodografissa 'right'
void NFmiSoundingDataOpt1::Calc_U_and_V_IDs_right(double &u_ID, double &v_ID)
{
  double shr_0_6_u_n = 0, shr_0_6_v_n = 0, u0_6 = 0, v0_6 = 0;
  Calc_U_V_helpers(shr_0_6_u_n, shr_0_6_v_n, u0_6, v0_6);

  u_ID = NFmiSoundingFunctions::CalcU_ID_right(u0_6, shr_0_6_v_n);
  v_ID = NFmiSoundingFunctions::CalcV_ID_right(v0_6, shr_0_6_u_n);
}

// lasketaan u ja v mean 0-6km layerissä
void NFmiSoundingDataOpt1::Calc_U_and_V_mean_0_6km(double &u0_6, double &v0_6)
{
  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  double h1 = 0 + ZeroHeight();     // 0 m + aseman korkeus
  double h2 = 6000 + ZeroHeight();  // 6000 m + aseman korkeus
  CalcAvgWindComponentValues(h1, h2, u0_6, v0_6);
}

NFmiString NFmiSoundingDataOpt1::Get_U_V_ID_IndexText(const NFmiString &theText,
                                                      FmiDirection theStormDirection)
{
  NFmiString str(theText);
  str += "=";
  double u_ID = kFloatMissing;
  double v_ID = kFloatMissing;
  if (theStormDirection == kRight)
    Calc_U_and_V_IDs_right(u_ID, v_ID);
  else if (theStormDirection == kLeft)
    Calc_U_and_V_IDs_left(u_ID, v_ID);
  else
    Calc_U_and_V_mean_0_6km(u_ID, v_ID);

  if (u_ID == kFloatMissing || v_ID == kFloatMissing)
    str += " -/-";
  else
  {
    double WS = ::sqrt(u_ID * u_ID + v_ID * v_ID);
    NFmiWindDirection theDirection(u_ID, v_ID);
    double WD = theDirection.Value();

    if (WD != kFloatMissing)
      str += NFmiValueString::GetStringWithMaxDecimalsSmartWay(WD, 0);
    else
      str += "-";
    str += "/";
    str += NFmiValueString::GetStringWithMaxDecimalsSmartWay(WS, 1);
  }
  return str;
}

// Calculates Storm-Relative Helicity (SRH) between two given layers
// startH and endH are in kilometers
// käytetään muuttujan niminä samoja mitä on Pieterin helpissä, vaikka kyseessä ei olekaan laskut
// layerille 0-6km vaan mille välille tahansa
// HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
double NFmiSoundingDataOpt1::CalcSRHIndex(double startH, double endH)
{
  std::deque<float> &pV = GetParamData(kFmiPressure);
  if (pV.size() > 0)
  {
    // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
    startH += ZeroHeight() / 1000.;  // zero height pitää muuttaa tässä metreistä kilometreiksi!
    endH += ZeroHeight() / 1000.;  // zero height pitää muuttaa tässä metreistä kilometreiksi!

    size_t ssize = pV.size();
    std::deque<float> &uV = GetParamData(kFmiWindUMS);
    std::deque<float> &vV = GetParamData(kFmiWindVMS);
    std::deque<float> &tV = GetParamData(kFmiTemperature);
    std::deque<float> &tdV = GetParamData(kFmiDewPoint);

    double u_ID = kFloatMissing;
    double v_ID = kFloatMissing;
    Calc_U_and_V_IDs_right(u_ID, v_ID);
    double SRH = 0;
    double lastU = kFloatMissing;
    double lastV = kFloatMissing;
    bool foundAnyData = false;
    for (size_t i = 0; i < ssize; i++)
    {
      double p = pV[i];
      double u = uV[i];
      double v = vV[i];
      double t = tV[i];
      double td = tdV[i];
      if (p != kFloatMissing && u != kFloatMissing && v != kFloatMissing && t != kFloatMissing &&
          td != kFloatMissing)
      {
        double z = GetValueAtPressure(kFmiGeomHeight, static_cast<float>(p));
        if (z != kFloatMissing && z >= startH * 1000 && z <= endH * 1000)
        {
          if (lastU != kFloatMissing && lastV != kFloatMissing)
          {
            SRH -= NFmiSoundingFunctions::CalcSRH(u_ID, v_ID, lastU, u, lastV, v);
            foundAnyData = true;
          }
        }
        lastU = u;
        lastV = v;
      }
    }
    return foundAnyData ? SRH : kFloatMissing;
  }
  return kFloatMissing;
}

// theH in meters
double NFmiSoundingDataOpt1::CalcWSatHeightIndex(double theH)
{
  // HUOM! Pitää ottaa huomioon aseman korkeus kun tehdään laskuja!!!!
  theH += ZeroHeight();

  return GetValueAtHeight(kFmiWindSpeedMS, static_cast<float>(theH));
}

std::string NFmiSoundingDataOpt1::MakeCacheString(double T, double Td, double fromP, double toP)
{
  return (boost::format("%f,%f,%f,%f") % T % Td % fromP % toP).str();
}

// Laske ilmapaketin lämpötila nostamalla ilmapakettia
// Nosta kuiva-adiapaattisesti LCL-korkeuteen ja siitä eteenpäin kostea-adiapaattisesti
double NFmiSoundingDataOpt1::CalcTOfLiftedAirParcel(double T, double Td, double fromP, double toP)
{
  std::string cacheKeyStr = MakeCacheString(T, Td, fromP, toP);
  LiftedAirParcelCacheType::iterator it = itsLiftedAirParcelCache.find(cacheKeyStr);
  if (it != itsLiftedAirParcelCache.end())
    return it->second;  // palautetaan arvo cachesta

  double Tparcel = kFloatMissing;
  // 1. laske LCL kerroksen paine alkaen fromP:stä
  double lclPressure = NFmiSoundingFunctions::CalcLCLPressureFast(T, Td, fromP);
  if (lclPressure != kFloatMissing)
  {
    // Laske aloitus korkeuden lämpötilan potentiaali lämpötila.
    double TpotStart = NFmiSoundingFunctions::T2tpot(T, fromP);
    if (TpotStart != kFloatMissing)
    {
      if (lclPressure <= toP)  // jos lcl oli yli toP:n mb (eli pienempi kuin toP)
      {                        // nyt riittää pelkkä kuiva-adiapaattinen nosto fromP -> toP
        Tparcel = NFmiSoundingFunctions::Tpot2t(TpotStart, toP);
      }
      else
      {
        // Laske ilmapaketin lämpö lcl-korkeudella  kuiva-adiapaattisesti nostettuna
        double Tparcel_LCL = NFmiSoundingFunctions::Tpot2t(TpotStart, lclPressure);
        // laske kyseiselle korkeudelle vastaava kostea-adiapaatti arvo
        double TmoistLCL = NFmiSoundingFunctions::CalcMoistT(Tparcel_LCL, lclPressure);
        // kyseinen kostea-adiapaatti pitää konvertoida vielä (ADL-kielestä kopioitua koodia, ks.
        // OS- ja
        // TSA-funtioita) jotenkin 1000 mb:hen.
        if (TmoistLCL != kFloatMissing)
        {
          double AOS = NFmiSoundingFunctions::OS(TmoistLCL, 1000.);
          // Sitten lasketaan lämpötila viimein 500 mb:hen
          Tparcel = NFmiSoundingFunctions::TSA(AOS, toP);
        }
      }
    }
  }
  itsLiftedAirParcelCache[cacheKeyStr] = Tparcel;
  return Tparcel;
}

// Haetaan minimi lämpötilan arvo ja sen korkeuden paine.
bool NFmiSoundingDataOpt1::GetTrValues(double &theTMinValue, double &theTMinPressure)
{
  theTMinValue = kFloatMissing;
  theTMinPressure = kFloatMissing;
  std::deque<float> &Pvalues = GetParamData(kFmiPressure);
  if (Pvalues.size())
  {
    std::deque<float> &Tvalues = GetParamData(kFmiTemperature);
    for (size_t i = 0; i < Pvalues.size(); i++)
    {
      double P = Pvalues[i];
      double T = Tvalues[i];
      if (P != kFloatMissing && T != kFloatMissing)
      {
        if (theTMinValue == kFloatMissing)
        {
          theTMinValue = T;
          theTMinPressure = P;
        }
        else if (T < theTMinValue)
        {
          theTMinValue = T;
          theTMinPressure = P;
        }
      }
    }
  }
  return (theTMinValue != kFloatMissing) && (theTMinPressure != kFloatMissing);
}

// Haetaan maksimi tuulen nopeus, sen korkeuden korkeuden paine.
bool NFmiSoundingDataOpt1::GetMwValues(double &theMaxWsValue, double &theMaxWsPressure)
{
  theMaxWsValue = kFloatMissing;
  theMaxWsPressure = kFloatMissing;
  std::deque<float> &Pvalues = GetParamData(kFmiPressure);
  if (Pvalues.size())
  {
    std::deque<float> &WSvalues = GetParamData(kFmiWindSpeedMS);
    for (size_t i = 0; i < Pvalues.size(); i++)
    {
      double P = Pvalues[i];
      double WS = WSvalues[i];
      if (P != kFloatMissing && WS != kFloatMissing)
      {
        if (theMaxWsValue == kFloatMissing)
        {
          theMaxWsValue = WS;
          theMaxWsPressure = P;
        }
        else if (WS > theMaxWsValue)
        {
          theMaxWsValue = WS;
          theMaxWsPressure = P;
        }
      }
    }
  }
  return (theMaxWsValue != kFloatMissing) && (theMaxWsPressure != kFloatMissing);
}
