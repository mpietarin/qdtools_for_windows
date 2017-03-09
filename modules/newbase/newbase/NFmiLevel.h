// ======================================================================
/*!
 * \file NFmiLevel.h
 * \brief Interface of class NFmiLevel
 */
// ======================================================================

#ifndef NFMILEVEL_H
#define NFMILEVEL_H

#include "NFmiLevelType.h"
#include "NFmiIndividual.h"

//! Undocumented

class _FMI_DLL NFmiLevel : public NFmiIndividual
{
 public:
  ~NFmiLevel(void);
  NFmiLevel(void);
  NFmiLevel(const NFmiLevel& theLevel);
  NFmiLevel(unsigned long theIdent, const NFmiString& theName, float theLevelValue);
  NFmiLevel(FmiLevelType theLevelType, float theLevelValue);

  bool operator==(const NFmiLevel& theLevel) const;
  bool operator<(const NFmiLevel& theLevel) const;
  NFmiLevel& operator=(const NFmiLevel& theLevel);

  unsigned long LevelTypeId(void) const;
  float LevelValue(void) const;
  void LevelValue(float theLevelValue) { itsLevelValue = theLevelValue; }
  FmiLevelType LevelType(void) const;

  virtual std::ostream& Write(std::ostream& file) const;
  virtual std::istream& Read(std::istream& file);

  virtual const char* ClassName(void) const;

  bool IsMissing() const;

 private:
  float itsLevelValue;

};  // class NFmiLevel

#endif  // NFMILEVEL_H

// ======================================================================
