// ======================================================================
/*!
 * \file NFmiDataModifierExtreme.cpp
 * \brief Implementation of class NFmiDataModifierExtreme
 */
// ======================================================================
/*!
 * \class NFmiDataModifierExtreme
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierExtreme.h"
#include "NFmiQueryInfo.h"

// ----------------------------------------------------------------------
/*!
 * Destructor does nothing special
 */
// ----------------------------------------------------------------------

NFmiDataModifierExtreme::~NFmiDataModifierExtreme(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiDataModifierExtreme::NFmiDataModifierExtreme(void) : itsExtremeValue(), itsExtremeTime() {}
NFmiDataModifierExtreme::NFmiDataModifierExtreme(const NFmiDataModifierExtreme &theOther)
    : NFmiDataModifier(theOther),
      itsExtremeValue(theOther.itsExtremeValue),
      itsExtremeTime(theOther.itsExtremeTime)
{
}

// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierExtreme::SetTime(NFmiQueryInfo *theQI)
{
  if (theQI) itsExtremeTime = theQI->Time();
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiTime NFmiDataModifierExtreme::GetTime(void) { return itsExtremeTime; }
// ----------------------------------------------------------------------
/*!
 * \param theQI Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierExtreme::Calculate(NFmiQueryInfo *theQI)
{
  float value = theQI->FloatValue();
  Calculate(value);
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierExtreme::Calculate(float theValue)
{
  if (IsAllowedValue(theValue))
  {
    //	 if(value != kFloatMissing && value != kRadarPrecipitationMissing)  //puuttuvat
    // tutkasateet==65535??
    if (IsNewExtreme(theValue))
    {
      itsExtremeValue = theValue;
      // EI ENÄÄ SAA AIKAA TALTEEN, EI TAIDA OLLA MISSÄÄN KÄYTÖSSÄ
      //	SetTime(theQI);
    }
  }
}
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiDataModifierExtreme::Calculate(float theValue, NFmiQueryInfo *theQI)
{
  if (IsAllowedValue(theValue))
  {
    //	 if(value != kFloatMissing && value != kRadarPrecipitationMissing)  //puuttuvat
    // tutkasateet==65535??
    if (IsNewExtreme(theValue))
    {
      itsExtremeValue = theValue;
      // TÄSSÄ SAA
      SetTime(theQI);
    }
  }
}

// ======================================================================
