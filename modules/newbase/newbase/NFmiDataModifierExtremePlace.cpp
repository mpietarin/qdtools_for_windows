// ======================================================================
/*!
 * \file NFmiDataModifierExtremePlace.cpp
 * \brief Implementation of class NFmiDataModifierExtreme
 */
// ======================================================================
/*!
 * \class NFmiDataModifierExtremePlace
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierExtremePlace.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierExtremePlace::~NFmiDataModifierExtremePlace(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierExtremePlace::NFmiDataModifierExtremePlace(void)
    : itsExtremeValue(), itsExtremeLocation()
{
}

// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierExtremePlace::SetLocation(NFmiQueryInfo* theQI)
{  // virtual const NFmiLocation * Location(void) const;

  if (theQI) itsExtremeLocation = *(theQI->Location());
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiLocation NFmiDataModifierExtremePlace::GetLocation(void) { return itsExtremeLocation; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierExtremePlace::Calculate(float theValue, NFmiQueryInfo* theQI)
{
  if (IsAllowedValue(theValue))
  {
    //	 if(value != kFloatMissing && value != kRadarPrecipitationMissing)  //puuttuvat
    // tutkasateet==65535??
    if (IsNewExtreme(theValue))
    {
      itsExtremeValue = theValue;
      // TÄSSÄ SAA
      SetLocation(theQI);
    }
  }
}

// ======================================================================
