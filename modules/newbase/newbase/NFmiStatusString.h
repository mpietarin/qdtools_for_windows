// ======================================================================
/*!
 * \file NFmiStatusString.h
 * \brief Interface of class NFmiStatusString
 */
// ======================================================================

#ifndef NFMISTATUSSTRING_H
#define NFMISTATUSSTRING_H

#include "NFmiString.h"

//! Undocumented
class _FMI_DLL NFmiStatusString : public NFmiString
{
 public:
  ~NFmiStatusString(void);
  NFmiStatusString(void);
  NFmiStatusString(const NFmiStatusString& theStatusString);
  NFmiStatusString(const NFmiString& theString, long theStatus);

  NFmiStatusString& operator=(const NFmiStatusString& theStausString);

  long Status(void);
  const NFmiString& String(void);
  virtual unsigned long ClassId(void) const { return kNFmiStatusString; };
  virtual NFmiString* Clone() const;

  virtual std::ostream& Write(std::ostream& file) const;
  virtual std::istream& Read(std::istream& file);

 private:
  long itsStatus;

};  // class NFmiStatusString

#endif  // NFMISTATUSSTRING_H

// ======================================================================
