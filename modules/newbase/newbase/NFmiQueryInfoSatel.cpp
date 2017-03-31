// ======================================================================
/*!
 * \file NFmiQueryInfoSatel.cpp
 * \brief Implementation of class NFmiQueryInfoSatel
 */
// ======================================================================
/*!
 * \class NFmiQueryInfoSatel
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiQueryInfoSatel.h"
#include "NFmiQueryData.h"
#include "NFmiSaveBaseFactory.h"

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiQueryInfoSatel::~NFmiQueryInfoSatel() { Destroy(); }
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

NFmiQueryInfoSatel::NFmiQueryInfoSatel() : NFmiQueryInfo(), itsSatelName(nullptr) {}
// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theParamDescriptor Undocumented
 * \param theTimeDescriptor Undocumented
 * \param theHPlaceDescriptor Undocumented
 * \param theVPlaceDescriptor Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryInfoSatel::NFmiQueryInfoSatel(const NFmiParamDescriptor &theParamDescriptor,
                                       const NFmiTimeDescriptor &theTimeDescriptor,
                                       const NFmiHPlaceDescriptor &theHPlaceDescriptor,
                                       const NFmiVPlaceDescriptor &theVPlaceDescriptor)
    : NFmiQueryInfo(
          theParamDescriptor, theTimeDescriptor, theHPlaceDescriptor, theVPlaceDescriptor),
      itsSatelName(new NFmiString("Alustus tehty"))
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theInfo Undocumented
 * \param theParamDescriptor Undocumented
 * \param theTimeDescriptor Undocumented
 * \param theHPlaceDescriptor Undocumented
 * \param theVPlaceDescriptor Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryInfoSatel::NFmiQueryInfoSatel(NFmiQueryData *theInfo,
                                       NFmiParamDescriptor *theParamDescriptor,
                                       NFmiTimeDescriptor *theTimeDescriptor,
                                       NFmiHPlaceDescriptor *theHPlaceDescriptor,
                                       NFmiVPlaceDescriptor *theVPlaceDescriptor)
    : NFmiQueryInfo(
          theInfo, theParamDescriptor, theTimeDescriptor, theHPlaceDescriptor, theVPlaceDescriptor),
      itsSatelName(nullptr)
{
}

// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theInfo The other object being copied
 */
// ----------------------------------------------------------------------

NFmiQueryInfoSatel::NFmiQueryInfoSatel(const NFmiQueryInfoSatel &theInfo)
    : NFmiQueryInfo(theInfo), itsSatelName(new NFmiString(*theInfo.itsSatelName))
{
}

// ----------------------------------------------------------------------
/*!
 * Undocumented
 */
// ----------------------------------------------------------------------

void NFmiQueryInfoSatel::Destroy()
{
  delete itsSatelName;
  itsSatelName = nullptr;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiQueryInfo *NFmiQueryInfoSatel::Clone() const { return new NFmiQueryInfoSatel(*this); }
// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param theInfo The other object being assigned
 * \return The object assigned to
 * \bug The method is not implemented correctly
 */
// ----------------------------------------------------------------------

NFmiQueryInfo &NFmiQueryInfoSatel::operator=(const NFmiQueryInfo & /* theInfo */) { return *this; }
// ----------------------------------------------------------------------
/*!
 * Write the object to the given output stream
 *
 * \param file The output stream to write to
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

std::ostream &NFmiQueryInfoSatel::Write(std::ostream &file) const
{
  NFmiQueryInfo::Write(file);

  file << static_cast<char *>(*itsSatelName) << std::endl;

  return file;
}

// ----------------------------------------------------------------------
/*!
 * Read new object contents from the given input stream
 *
 * \param file The input stream to read from
 * \return The input stream read from
 */
// ----------------------------------------------------------------------

std::istream &NFmiQueryInfoSatel::Read(std::istream &file)
{
  Destroy();

  NFmiQueryInfo::Read(file);

  char theSatelName[256];
  file >> theSatelName;

  itsSatelName = new NFmiString(theSatelName);

  return file;
}

// ======================================================================
