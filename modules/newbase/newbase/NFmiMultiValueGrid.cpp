// ======================================================================
/*!
 * \file NFmiMultiValueGrid.cpp
 * \brief Implementation of class NFmiMultiValueGrid
 */
// ======================================================================
/*!
 * \class NFmiMultiValueGrid
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiMultiValueGrid.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 *
 * \todo Don't bother setting
 */
// ----------------------------------------------------------------------

NFmiMultiValuedGrid::~NFmiMultiValuedGrid(void)
{
  if (itsParamDescriptor) delete itsParamDescriptor;
  itsParamDescriptor = 0;
  if (itsTimeDescriptor) delete itsTimeDescriptor;
  itsTimeDescriptor = 0;
  if (itsVPlaceDescriptor) delete itsVPlaceDescriptor;
  itsVPlaceDescriptor = 0;
  if (itsMultiData) delete itsMultiData;
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theGrid Undocumented
 * \param theParamDescriptor Undocumented
 * \param theTimeDescriptor Undocumented
 * \param theVPlaceDescriptor Undocumented
 */
// ----------------------------------------------------------------------

NFmiMultiValuedGrid::NFmiMultiValuedGrid(const NFmiGrid &theGrid,
                                         const NFmiParamDescriptor &theParamDescriptor,
                                         const NFmiTimeDescriptor &theTimeDescriptor,
                                         const NFmiVPlaceDescriptor &theVPlaceDescriptor)
    : NFmiGrid(theGrid), itsMultiData(0), itsCurrentDataStatus(0)
{
  itsParamDescriptor = new NFmiParamDescriptor(theParamDescriptor);
  itsTimeDescriptor = new NFmiTimeDescriptor(theTimeDescriptor);
  itsHPlaceDescriptor = 0;
  itsVPlaceDescriptor = new NFmiVPlaceDescriptor(theVPlaceDescriptor);
  itsRefRawData = 0;
  Init();
}

// ----------------------------------------------------------------------
/*!
 * Undocumented
 */
// ----------------------------------------------------------------------

void NFmiMultiValuedGrid::Init(void)
{
  itsMultiData = new NFmiDataPool();
  if (!itsMultiData->Init(NFmiGridBase::Size() * NFmiQueryInfo::Size()))
  {
    std::cout << "Out of memory. Execution failed.";
  }
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 * \todo Remove unnecessary last return statement
 */
// ----------------------------------------------------------------------

bool NFmiMultiValuedGrid::FirstValue(void)
{
  Save();
  NFmiQueryInfo::First();
  return Edit();
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiMultiValuedGrid::Save(void)
{
  float *sourceAddress = 0;

  if (!itsMultiData) return false;

  // Mika: unsigned int
  // if(NFmiQueryInfo::Index() < 0 || NFmiQueryInfo::Index() >= NFmiQueryInfo::Size())

  if (NFmiQueryInfo::Index() >= NFmiQueryInfo::Size()) return false;
  if (!itsData->FloatValueAddress(0, &sourceAddress)) return false;

  itsMultiData->Index(itsCurrentDataStatus);
  return itsMultiData->MemCopy(NFmiGridBase::Size(), sourceAddress);
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiMultiValuedGrid::Edit(void)
{
  float *sourceAddress = 0;

  if (!itsMultiData) return false;
  // Mika: Index() is unsigned int
  // if(NFmiQueryInfo::Index() < 0 || NFmiQueryInfo::Index() >= NFmiQueryInfo::Size())

  if (NFmiQueryInfo::Index() >= NFmiQueryInfo::Size()) return false;
  if (!itsMultiData->FloatValueAddress(NFmiQueryInfo::Index() * NFmiGridBase::Size(),
                                       &sourceAddress))
    return false;

  itsData->First();
  return itsData->MemCopy(NFmiGridBase::Size(), sourceAddress);
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

void NFmiMultiValuedGrid::UpDate(void)
{
  Save();
  itsCurrentDataStatus = NFmiQueryInfo::Index() * NFmiGridBase::Size();
  Edit();
}

// ----------------------------------------------------------------------
/*!
 * \param dx Undocumented
 * \param dy Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

float NFmiMultiValuedGrid::FloatValue(long dx, long dy)
{
  UpDate();
  return static_cast<float>(NFmiGrid::FloatValue(dx, dy));
}

// ----------------------------------------------------------------------
/*!
 * \param data Undcoumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiMultiValuedGrid::FloatValue(float data)
{
  UpDate();
  return NFmiGrid::FloatValue(data);
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiMultiValuedGrid::InterpolateToLatLonPoint(double newLon, double newLat, double &theValue)
{
  UpDate();
  return NFmiGrid::InterpolateToLatLonPoint(newLon, newLat, theValue);
}

// ======================================================================
