// ======================================================================
/*!
 * \file NFmiMultiPolygonAreaMask.cpp
 * \brief Implementation of class NFmiMultiPolygonAreaMask
 */
// ======================================================================
/*!
 * \class NFmiMultiPolygonAreaMask
 *
 *  Luokka hoitaa maskauksen, jossa on annettu useita polygoneja/polkuja
 *  maskialueiksi. Jolloin jokaisella erillisellä alueella on oma maski arvonsa.
 *
 *  Apuna käytetään listaa NFmiSvgPath-otuksia, jotka kukin hoitaa yhden maskialueen.
 *
 *  Osaa lukea ja kirjoittaa itsensä (poiketen muista areamaski-luokista), koska
 *  data on hankalassa muodossa ja halutaan tallettaa että käyttö olisi
 *  mahdollisimman yksinkertaista.
 *
 *  HUOM! Ei ole optimoitu käyttämään valmiiksi laskettuja binääri kenttiä (esim.
 *  NFmiBitmask-luokkaa käyttäen). Polygoni lasketa voi olla raskasta ja optimointia
 *  pitää varmaan joskus tehdä.
 *
 *  HUOM! Write:a ei ole toteutettu, koska sitä ei ole toteutettu
 *  NFmiSvgPath luokassakaan.
 */
// ======================================================================

#include "NFmiMultiPolygonAreaMask.h"
#include "NFmiBitmapAreaMask.h"
#include "NFmiSvgPath.h"
#include "NFmiSvgTools.h"
#include "NFmiPoint.h"
#include "NFmiGrid.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiMultiPolygonAreaMask::~NFmiMultiPolygonAreaMask(void) { Clear(); }
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiMultiPolygonAreaMask::NFmiMultiPolygonAreaMask(void)
    : NFmiAreaMaskImpl(), itsAreaPolygons(), itsAreaPolygonValues(), fInitialized(false)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theOperation Undocumented
 * \param theMaskType Undocumented
 * \param theDataType Undocumented
 * \param thePostBinaryOperator Undocumented
 */
// ----------------------------------------------------------------------

NFmiMultiPolygonAreaMask::NFmiMultiPolygonAreaMask(const NFmiCalculationCondition& theOperation,
                                                   Type theMaskType,
                                                   NFmiInfoData::Type theDataType,
                                                   BinaryOperator thePostBinaryOperator)
    : NFmiAreaMaskImpl(theOperation, theMaskType, theDataType, thePostBinaryOperator),
      itsAreaPolygons(),
      itsAreaPolygonValues(),
      fInitialized(false)
{
}

// ----------------------------------------------------------------------
/*!
 *  Katsotaan ensin minkä SVGPath-olion sisällä annettu piste mahdollisesti
 *  sijaitsee ja palautetaan kyseisen polun (polygonin) arvo.
 *
 * \param theLatLon Undocumented
 */
// ----------------------------------------------------------------------

double NFmiMultiPolygonAreaMask::CalcValueFromLocation(const NFmiPoint& theLatLon) const
{
  int size = itsAreaPolygons.size();
  for (int i = 0; i < size; i++)
    if (NFmiSvgTools::IsInside(*itsAreaPolygons[i], theLatLon)) return itsAreaPolygonValues[i];
  return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 *  Ei ole vielä toteutettu, palauttaa tyhjää!
 *
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiString NFmiMultiPolygonAreaMask::MakeSubMaskString(void) const
{
  NFmiString str;
  return str;
}

// ----------------------------------------------------------------------
/*!
 *  Siivoaa tietorakenteet tyhjiksi. Deletoi polku-olioiden pointterit!
 */
// ----------------------------------------------------------------------

void NFmiMultiPolygonAreaMask::Clear(void)
{
  int size = itsAreaPolygons.size();
  for (int i = 0; i < size; i++)
    delete itsAreaPolygons[i];

  std::vector<NFmiSvgPath*>().swap(
      itsAreaPolygons);  // tyhjennetään vectorit empty-object-swap -tempulla.
  std::vector<double>().swap(itsAreaPolygonValues);
}

// ----------------------------------------------------------------------
/*!
 * Write the object to the given output stream.
 *
 * \param file The output stream to write to
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

std::ostream& NFmiMultiPolygonAreaMask::Write(std::ostream& file) const
{
  int size = itsAreaPolygons.size();
  for (int i = 0; i < size; i++)
  {
    file << itsAreaPolygonValues[i] << std::endl;
    file << *itsAreaPolygons[i] << std::endl;
  }
  return file;
}

// ----------------------------------------------------------------------
/*!
 *  Luetaan polygonit tiedostosta (NFmiSvgPath-oliot ja niiden alue arvot.)
 *  Formaatti:				/par
 * \code
 *  area1arvo:
 *  area1Polku
 *  area2arvo:
 *  area2Polku
 *  jne...
 * \endcode
 *
 *  area1 esim. "M10 10 L 10 20 20 20 20 10 Z" (simppeli laatikko, pitää olla heittomerkkeineen!)
 *  HUOM! Ei talleteta lukumäärää alkuun, vaan jatketaan kunnes eof-tulee.
 */
// ----------------------------------------------------------------------

std::istream& NFmiMultiPolygonAreaMask::Read(std::istream& file)
{
  NFmiSvgPath* tmp = 0;
  double value;
  Clear();
  do
  {
    file >> value;
    if (file.fail()) break;
    tmp = new NFmiSvgPath;
    file >> *tmp;
    if (file.fail())
    {
      delete tmp;
      break;
    }
    itsAreaPolygons.push_back(tmp);
    itsAreaPolygonValues.push_back(value);
  } while (!file.fail());
  return file;
}

// ----------------------------------------------------------------------
/*!
 * \brief
 * Tällä funktiolla luodaan valmis 'cache'-maski haluttuun projektioon
 * ja hilaan.
 *
 *  Luodaan tästä multipolygon maskista bitmapMaski. Tämän käyttö on sitten
 *  nopeampaa kuin se, että aina kysytään, onko joku piste tämän polygoni-
 *  satsin sisällä tietyllä ehdolla. Eli tällä voidaan luoda cache-maski
 *  tietynlaiselle gridille tietyillä ehdoilla.
 *
 * \param theGrid Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiBitmapAreaMask* NFmiMultiPolygonAreaMask::CreateBitmapAreaMask(const NFmiGrid& theGrid)
{
  NFmiBitmapAreaMask* mask = new NFmiBitmapAreaMask(theGrid, 0, 0);
  if (mask)
  {
    NFmiGrid grid(theGrid);  // tehdään hilasta kopio, että voidaan käydä hila läpi
    for (grid.Reset(); grid.Next();)
    {
      mask->Mask(grid.Index(), this->IsMasked(grid.LatLon()));
    }
  }
  return mask;
}

// ======================================================================
