// ======================================================================
/*!
 * \file NFmiBitmapAreaMask.cpp
 * \brief Implementation of class NFmiBitmapAreaMask
 */
// ======================================================================

#include "NFmiBitmapAreaMask.h"
#include "NFmiFastQueryInfo.h"
#include "NFmiArea.h"
#include "NFmiGrid.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiBitmapAreaMask::~NFmiBitmapAreaMask(void) { Destroy(); }
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiBitmapAreaMask::NFmiBitmapAreaMask(void)
    : NFmiAreaMaskImpl(),
      itsBitmask(),
      itsArea(0),
      itsGridXSize(0),
      itsGridYSize(0),
      itsDataIdent(0),
      itsLevel(0)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theXSize Undocumented
 * \param theYSize Undocumented
 * \param theArea Undocumented
 * \param theDataIdent Undocumented
 * \param theLevel Undocumented
 * \param thePostBinaryOperator Undocumented
 */
// ----------------------------------------------------------------------

NFmiBitmapAreaMask::NFmiBitmapAreaMask(int theXSize,
                                       int theYSize,
                                       const NFmiArea* theArea,
                                       const NFmiDataIdent* theDataIdent,
                                       const NFmiLevel* theLevel,
                                       BinaryOperator thePostBinaryOperator)
    : NFmiAreaMaskImpl(
          NFmiCalculationCondition(), kBinary, NFmiInfoData::kStationary, thePostBinaryOperator),
      itsBitmask(theXSize * theYSize, false),
      itsArea(theArea ? theArea->Clone() : 0),
      itsGridXSize(theXSize),
      itsGridYSize(theYSize),
      itsDataIdent(theDataIdent ? new NFmiDataIdent(*theDataIdent) : 0),
      itsLevel(theLevel ? new NFmiLevel(*theLevel) : 0)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theGrid Undocumented
 * \param theDataIdent Undocumented
 * \param theLevel Undocumented
 * \param thePostBinaryOperator Undocumented
 */
// ----------------------------------------------------------------------

NFmiBitmapAreaMask::NFmiBitmapAreaMask(const NFmiGrid& theGrid,
                                       const NFmiDataIdent* theDataIdent,
                                       const NFmiLevel* theLevel,
                                       BinaryOperator thePostBinaryOperator)
    : NFmiAreaMaskImpl(
          NFmiCalculationCondition(), kBinary, NFmiInfoData::kStationary, thePostBinaryOperator),
      itsBitmask(theGrid.XNumber() * theGrid.YNumber(), false),
      itsArea(theGrid.Area()->Clone()),
      itsGridXSize(theGrid.XNumber()),
      itsGridYSize(theGrid.YNumber()),
      itsDataIdent(theDataIdent ? new NFmiDataIdent(*theDataIdent) : 0),
      itsLevel(theLevel ? new NFmiLevel(*theLevel) : 0)
{
}

// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theMask The other object being copied
 *
 */
// ----------------------------------------------------------------------

NFmiBitmapAreaMask::NFmiBitmapAreaMask(const NFmiBitmapAreaMask& theMask)
    : NFmiAreaMaskImpl(theMask),
      itsBitmask(theMask.itsBitmask),
      itsArea(theMask.itsArea ? theMask.itsArea->Clone() : 0),
      itsGridXSize(theMask.itsGridXSize),
      itsGridYSize(theMask.itsGridYSize),
      itsDataIdent(theMask.itsDataIdent ? new NFmiDataIdent(*theMask.itsDataIdent) : 0),
      itsLevel(theMask.itsLevel ? new NFmiLevel(*theMask.itsLevel) : 0)
{
}

// ----------------------------------------------------------------------
/*!
 * Clone the object
 *
 * \return A new copy of the object
 * \todo Should return an boost::shared_ptr
 */
// ----------------------------------------------------------------------

NFmiAreaMask* NFmiBitmapAreaMask::Clone(void) const { return new NFmiBitmapAreaMask(*this); }
// ----------------------------------------------------------------------
/*!
 * Internal data destructuction utility
 */
// ----------------------------------------------------------------------

void NFmiBitmapAreaMask::Destroy(void)
{
  delete itsArea;
  itsArea = 0;
  delete itsDataIdent;
  itsDataIdent = 0;
  delete itsLevel;
  itsLevel = 0;
}

// ----------------------------------------------------------------------
/*!
 * Initialize the binary mask based on the given info and condition,
 * making the mask the size of the grid in the given info. Note that
 * the info is not stored for later use.
 *
 * \param theInfo The info containing the grid
 * \param theOperation The condition
 */
// ----------------------------------------------------------------------

#ifdef UNIX
// RHEL6 compiler bug forces to omit the reference
void NFmiBitmapAreaMask::Init(boost::shared_ptr<NFmiFastQueryInfo> theInfo,
                              const NFmiCalculationCondition& theOperation)
#else
void NFmiBitmapAreaMask::Init(boost::shared_ptr<NFmiFastQueryInfo>& theInfo,
                              const NFmiCalculationCondition& theOperation)
#endif
{
  Destroy();
  itsBitmask.resize(theInfo->HPlaceDescriptor().Size(), false);
  itsArea = theInfo->Area() ? theInfo->Area()->Clone() : 0;
  itsDataIdent = new NFmiDataIdent(theInfo->Param());
  itsLevel = theInfo->Level() ? new NFmiLevel(*theInfo->Level()) : 0;
  if (theInfo->IsGrid())
  {
    itsGridXSize = theInfo->Grid()->XNumber();
    itsGridYSize = theInfo->Grid()->YNumber();
  }
  float value = kFloatMissing;
  for (theInfo->ResetLocation(); theInfo->NextLocation();)
  {
    value = theInfo->FloatValue();
    itsBitmask[theInfo->LocationIndex()] = theOperation.IsMasked(value);
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theLatLon Undocumented
 * \return Undocumented
 *
 * \note If the mask is not enabled, true is returned for all points
 */
// ----------------------------------------------------------------------

bool NFmiBitmapAreaMask::IsMasked(const NFmiPoint& theLatLon) const
{
  if (!fEnabled)
    return true;
  else
    return IsMasked(LatLon2Index(theLatLon));
}

// ----------------------------------------------------------------------
/*!
 * A special case for optimized use. This cannot be use for all classes,
 * but is suitable for example for NFmiInfoAreaMask and NFmiBinaryAreaMask
 * classes.
 *
 * \param theIndex Undocumented
 * \return Undocumented
 *
 * \note If the mask is not enabled, true is returned for all points
 */
// ----------------------------------------------------------------------

bool NFmiBitmapAreaMask::IsMasked(int theIndex) const
{
  if (!fEnabled)
    return true;
  else if (CheckIndex(theIndex))
    return itsBitmask[theIndex];

  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theLatLon Undocumented
 * \return Undocumented
 *
 * \note If the mask is not enabled, true is returned for all points
 */
// ----------------------------------------------------------------------.

// Maskin luonteesta johtuen tämä ei käytä laskuissaan normaaleja suurempi tai pienempi ehtoja,
// vaan jos maski on päällä, arvo on yksi ja muuten nolla.

double NFmiBitmapAreaMask::MaskValue(const NFmiPoint& theLatLon) const
{
  if (!fEnabled)
    return 1;
  else
    return IsMasked(LatLon2Index(theLatLon)) ? 1 : 0;
}

// ----------------------------------------------------------------------
/*!
 * \param theLatLon Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

// Maskin luonteesta johtuen tämä ei käytä laskuissaan normaaleja suurempi tai pienempi ehtoja,
// vaan jos maski on päällä, arvo on yksi ja muuten nolla.
// Huom!! tässä ei tarvitse tarkistaa onko enabloitu, koska se tehdään ylempänä.

double NFmiBitmapAreaMask::CalcValueFromLocation(const NFmiPoint& theLatLon) const
{
  return IsMasked(LatLon2Index(theLatLon)) ? 1 : 0;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 * \todo Not implemented yet
 */
// ----------------------------------------------------------------------

const NFmiString NFmiBitmapAreaMask::MakeSubMaskString(void) const
{
  return NFmiString("NFmiBitmapAreaMask::MakeSubMaskString ei ole toteutettu");
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \param newStatus Undocumented
 */
// ----------------------------------------------------------------------

void NFmiBitmapAreaMask::Mask(int theIndex, bool newStatus)
{
  if (CheckIndex(theIndex)) itsBitmask[theIndex] = newStatus;
}

// ----------------------------------------------------------------------
/*!
 * \param theLatLon Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

int NFmiBitmapAreaMask::LatLon2Index(const NFmiPoint& theLatLon) const
{
  if (itsArea)
  {
    NFmiPoint xyPoint(itsArea->ToXY(theLatLon));  // tämä on 'suhteellisessa' 0,0 - 1,1 maailmassa
    int x = static_cast<int>(round((itsGridXSize - 1) * itsArea->Width() * xyPoint.X()));
    int y = static_cast<int>(round((itsGridYSize - 1) * itsArea->Height() * xyPoint.Y()));
    // pitää vielä kääntää
    y = static_cast<int>(round((itsGridYSize - 1) * itsArea->Height() - y));
    int finalIndex = y * itsGridXSize + x;
    return finalIndex;
  }
  return -1;
}

// HUOM! toimii vain NFmiBinaryMask:in kanssa. Asettaa koko maskin kaikki arvot halutuksi.
void NFmiBitmapAreaMask::SetAll(bool theNewState)
{
  checkedVector<bool>(itsBitmask.size(), theNewState).swap(itsBitmask);
}

// ======================================================================
