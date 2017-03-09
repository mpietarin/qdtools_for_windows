// ======================================================================
/*!
 * \file NFmiBitMask.cpp
 * \brief Implementation of class NFmiBitMask
 */
// ======================================================================
/*!
 * \class NFmiBitMask
 *
 * This class contains bitfield for X x Y grid data. This is used with
 * NFmiMaskedDataPool to mask used data. You can set on and off
 * bits at wanted index and then ask with given index if mask is on or off.
 *
 * \note
 * Even if this is bit field for 2 dimensional data, it can be accessad
 * only with one index that means index = (Y * XNumber) + X.
 * At the end of file there is couple of class-functions that can be used to
 * initialize NFmiBitMask with given NFmiGridBase entity. E.g. you can initialize
 * bitmask so that with given grid all the values over e.g. 0 are set to 1 and
 * others are set to 0.
 *
 * \see NFmiMaskedDataPool and NFmiGridBase
 *
 * \todo Several methods take as parameters const references to bool or long
 *       types, use regular params instead.
 */
// ======================================================================

#include "NFmiBitMask.h"
#include "NFmiQueryInfo.h"
#include "NFmiDataModifier.h"

using namespace std;

long NFmiBitMask::itsNumberOfBitsInLong = sizeof(long) * 8;

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theSize Undocumented
 */
// ----------------------------------------------------------------------

NFmiBitMask::NFmiBitMask(long theSize)
    : itsSize(), itsNumberOfBytesInMask(), itsNumberOfLongsInMask(), itsMaskData()
{
  itsMaskData = 0;
  Init(theSize);
}

// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theBitMask The other object being copied
 */
// ----------------------------------------------------------------------

NFmiBitMask::NFmiBitMask(const NFmiBitMask& theBitMask)
    : itsSize(theBitMask.itsSize),
      itsNumberOfBytesInMask(theBitMask.itsNumberOfBytesInMask),
      itsNumberOfLongsInMask(theBitMask.itsNumberOfLongsInMask),
      itsMaskData(0)
{
  if (theBitMask.itsMaskData)
  {
    itsMaskData = new long[itsNumberOfLongsInMask];
    memcpy(itsMaskData, theBitMask.itsMaskData, itsNumberOfBytesInMask);
  }
}

//---------------------------------------------------------------------------
//	Bit manipulation methods for the long typed mask arrays
//---------------------------------------------------------------------------

// ----------------------------------------------------------------------
/*!
 * \param theSize Undocumented
 * \todo Input is a const reference to a long???
 */
// ----------------------------------------------------------------------

void NFmiBitMask::CalculateMaskProperties(const unsigned long& theSize)
{
  itsSize = theSize;  // The number of BITS to be stored in mask array

  itsNumberOfBitsInLong = sizeof(long) * 8;

  if (theSize % itsNumberOfBitsInLong == 0)
    itsNumberOfLongsInMask = theSize / itsNumberOfBitsInLong;
  else
    itsNumberOfLongsInMask = theSize / itsNumberOfBitsInLong + 1;

  itsNumberOfBytesInMask = itsNumberOfLongsInMask * sizeof(long);
}

// ----------------------------------------------------------------------
/*!
 * \param theSize Undocumented
 */
// ----------------------------------------------------------------------

void NFmiBitMask::Init(long theSize)
{
  // Initializes the mask array with 0.

  CalculateMaskProperties(theSize);  // theSize: the number of BITS to be stored in mask array

  if (itsMaskData) delete[] itsMaskData;

  itsMaskData = new long[itsNumberOfLongsInMask];

  Mask(false);  // Unset all of the bits of the current mask
}

// ----------------------------------------------------------------------
/*!
 * \param theBitMask Undocumented
 */
// ----------------------------------------------------------------------

void NFmiBitMask::Init(NFmiBitMask* theBitMask)
{
  // Initializes the mask array with the input bit mask

  CalculateMaskProperties(theBitMask->Size());

  if (itsMaskData) delete[] itsMaskData;

  itsMaskData = new long[itsNumberOfLongsInMask];

  // Initialize the mask with input bit mask
  memcpy(itsMaskData, theBitMask->itsMaskData, itsNumberOfBytesInMask);
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \param theBit Undocumented
 * \return Undocumented
 * \todo Input parameters are const references to elementary objects????
 */
// ----------------------------------------------------------------------

bool NFmiBitMask::Mask(const long& theIndex, const bool& theBit)
{
  // Sets/unsets the bit value in the long located in mask array at index 'theIndex'.
  // This proceeds in two steps:
  //
  //	 1) In the mask array, find the long item position by index 'theIndex'
  //	 2) Set/unset the bit value in the current long by index 'longIndex'

  if (theIndex >= itsSize) return false;

  long arrayIndex = theIndex / itsNumberOfBitsInLong;   // The long position in mask array
  long longIndex = theIndex % itsNumberOfBitsInLong;    // The bit position in long
  LongBit(longIndex, theBit, itsMaskData[arrayIndex]);  // Set/unset the bit value in current long

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \param theBit Undocumented
 * \return Undocumented
 * \todo Input is a const reference to a bool????
 */
// ----------------------------------------------------------------------

bool NFmiBitMask::Mask(const bool& theBit)
{
  // Sets/unsets ALL of the bits of the mask array value to 'theBit'.
  // This is done by filling the whole mask array with masked longs.

  for (long i = 0; i < itsNumberOfLongsInMask; i++)
    LongBit(theBit, itsMaskData[i]);

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \param theIndex Undocumented
 * \return Undocumented
 * \todo Const reference to a long as input????
 */
// ----------------------------------------------------------------------

bool NFmiBitMask::IsMasked(const long& theIndex) const
{
  // Tells whether the data value associated with index 'theIndex' is masked or not
  // by returning the bit value from the long located in mask array at index 'theIndex'.
  // This proceeds in two steps:
  //
  //	 1) In the mask array, find the long item position by index 'theIndex'
  //	 2) Extract the bit value from the current long by index 'longIndex'

  if (theIndex >= itsSize) return false;

  long arrayIndex = theIndex / itsNumberOfBitsInLong;  // The long position in mask array
  long longIndex = theIndex % itsNumberOfBitsInLong;   // The bit position in long
  return IsLongBit(longIndex,
                   itsMaskData[arrayIndex]);  // Return the bit value in current long as Boolean
}

//---------------------------------------------------------------------------
//	Bit manipulation methods for the long type
//---------------------------------------------------------------------------

// ----------------------------------------------------------------------
/*!
 * \param theBitIndex Undocumented
 * \param theBit Undocumented
 * \param theMaskedValue Undocumented
 * \todo Remove use of trivial const references
 */
// ----------------------------------------------------------------------

void NFmiBitMask::LongBit(const long& theBitIndex, const bool& theBit, long& theMaskedValue)
{
  // Set/unsets the bit 'theBitIndex' in long 'theMaskedValue' into Boolean 'theBit'

  long maskedLong = 1L << theBitIndex;

  if (theBit)
    theMaskedValue = theMaskedValue | maskedLong;  // Set bit
  else
    theMaskedValue = theMaskedValue & ~maskedLong;  // Unset bit
}

// ----------------------------------------------------------------------
/*!
 * \param theBit Undocumented
 * \param theMaskedValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiBitMask::LongBit(const bool& theBit, long& theMaskedValue)
{
  // Set ALL bits of 'theMaskedValue' into Boolean 'theBit'
  if (theBit)
    theMaskedValue = ~0L;  // Turn all bits in long into 1
  else
    theMaskedValue = 0L;  // Turn all bits in long into 0
}

// ----------------------------------------------------------------------
/*!
 * \param theBitIndex Undocumented
 * \param theMaskedValue Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiBitMask::IsLongBit(const long& theBitIndex, const long& theMaskedValue) const
{
  // Tells whether the bit 'theBitIndex' in long 'theMaskedValue' is set or unset.
  long maskedLong = 1L << theBitIndex;

  return (theMaskedValue & maskedLong) == maskedLong;
}

//   3. Operators

// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param theBitMask The other object being copied
 * \return The object assigned to
 * \bug Must protect from self assignment
 */
// ----------------------------------------------------------------------

NFmiBitMask& NFmiBitMask::operator=(const NFmiBitMask& theBitMask)
{
  itsSize = theBitMask.Size();
  itsNumberOfBitsInLong = theBitMask.itsNumberOfBitsInLong;
  itsNumberOfBytesInMask = theBitMask.itsNumberOfBytesInMask;
  itsNumberOfLongsInMask = theBitMask.itsNumberOfLongsInMask;

  if (itsMaskData) delete[] itsMaskData;

  itsMaskData = new long[itsNumberOfLongsInMask];

  memcpy(itsMaskData, theBitMask.itsMaskData, itsNumberOfBytesInMask);

  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Equality comparison
 *
 * \param theBitMask The other object being compared to
 * \return True, if the objects are equal
 */
// ----------------------------------------------------------------------

bool NFmiBitMask::operator==(const NFmiBitMask& theBitMask) const
{
  if (itsSize == theBitMask.Size())
  {
    if (memcmp(itsMaskData, theBitMask.itsMaskData, itsNumberOfBytesInMask) == 0) return true;
  }

  return false;
}

// ----------------------------------------------------------------------
/*!
 * Inequality comparison
 *
 * \param theBitMask The other object being compared to
 * \return True, if the objects are not equal
 */
// ----------------------------------------------------------------------

bool NFmiBitMask::operator!=(const NFmiBitMask& theBitMask) const { return !(*this == theBitMask); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

long NFmiBitMask::MaskedCount(void) const
{
  long returnValue = 0;
  for (long i = 0; i < itsSize; i++)
    if (IsMasked(i)) returnValue++;
  return returnValue;
}

// ----------------------------------------------------------------------
/*!
 * \param theData Undocumented
 * \param theMaskMethod Undocumented
 */
// ----------------------------------------------------------------------

void NFmiBitMask::Init(NFmiQueryInfo* theData, NFmiDataModifier* theMaskMethod)
{
  Init(theData->HPlaceDescriptor().Size());
  long i = 0;
  theData->ResetLocation();
  while (theData->NextLocation())
  {
    Mask(i++, theMaskMethod->BoolOperation(theData->FloatValue()));
  }
}

// Kaikki maskin bitit käännetään vastakkaisiksi.
void NFmiBitMask::InverseMask(void)
{
  for (int i = 0; i < itsNumberOfLongsInMask; i++)
    itsMaskData[i] = ~itsMaskData[i];  // bitwise not kääntää bitit vastakkain
}

// ======================================================================
