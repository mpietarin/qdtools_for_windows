// ======================================================================
/*!
 * \file NFmiSatelQueryData.h
 * \brief Interface of class NFmiSatelQueryData
 */
// ======================================================================
/*!
 * \class NFmiSatelQueryData
 *
 * Undocumented
 *
 */
// ======================================================================

#ifndef NFMISATELQUERYDATA_H
#define NFMISATELQUERYDATA_H

#include "NFmiQueryData.h"
#include "NFmiSatel.h"

//! Undocumented
class _FMI_DLL NFmiSatelQueryData : public NFmiQueryData
{
 public:
  ~NFmiSatelQueryData(void);
  NFmiSatelQueryData(void);
  NFmiSatelQueryData(const NFmiSatelQueryData &theData);
  NFmiSatelQueryData(NFmiQueryInfo &theInfo);

  virtual NFmiSatel *Value(void);
  bool Value(NFmiSatel *theData);

  virtual unsigned long ClassId(void) const;

  using NFmiQueryData::Clone;
  virtual NFmiQueryData *Clone(void);

  virtual std::ostream &Write(std::ostream &file) const;
  virtual std::istream &Read(std::istream &file);

 private:
};  // class NFmiSatelQueryData

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

inline NFmiSatelQueryData::~NFmiSatelQueryData(void)
{
  Reset();
  while (Next())
  {
    delete Value();
  }
}

// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

inline NFmiSatelQueryData::NFmiSatelQueryData(void) : NFmiQueryData(kVoid) {}
// ----------------------------------------------------------------------
/*!
 * Copy constructor
 *
 * \param theData The object being copied
 */
// ----------------------------------------------------------------------

inline NFmiSatelQueryData::NFmiSatelQueryData(const NFmiSatelQueryData &theData)
    : NFmiQueryData(theData)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theInfo Undocumented
 */
// ----------------------------------------------------------------------

inline NFmiSatelQueryData::NFmiSatelQueryData(NFmiQueryInfo &theInfo)
    : NFmiQueryData(theInfo, kVoid)
{
}

// ----------------------------------------------------------------------
/*!
 * \
 */
// ----------------------------------------------------------------------

inline NFmiSatel *NFmiSatelQueryData::Value(void) { return static_cast<NFmiSatel *>(VoidValue()); }
// ----------------------------------------------------------------------
/*!
 * \param theData Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline bool NFmiSatelQueryData::Value(NFmiSatel *theData)
{
  return VoidValue(static_cast<void *>(theData));
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline unsigned long NFmiSatelQueryData::ClassId(void) const { return kNFmiSatelQueryData; }
// ----------------------------------------------------------------------
/*!
 * Write the object to the given output stream
 *
 * \param file The output stream to write to
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

inline std::ostream &NFmiSatelQueryData::Write(std::ostream &file) const
{
  NFmiQueryData::Write(file);
  Reset();
  while (Next())
    file << *Value();
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

inline std::istream &NFmiSatelQueryData::Read(std::istream &file)
{
  NFmiQueryData::Read(file);
  Reset();
  while (Next())
  {
    NFmiSatel *strBufr = new NFmiSatel();
    file >> *strBufr;
    Value(strBufr);
  }
  return file;
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 * \todo Should return auto_ptr
 */
// ----------------------------------------------------------------------

inline NFmiQueryData *NFmiSatelQueryData::Clone(void)
{
  NFmiSatelQueryData *data = new NFmiSatelQueryData(*this);
  Reset();
  data->Reset();
  while (Next())
  {
    data->Next();
    data->Value(new NFmiSatel(*Value()));
  }

  return static_cast<NFmiQueryData *>(data);
}

#endif  // NFMISATELQUERYDATA_H

// ======================================================================
