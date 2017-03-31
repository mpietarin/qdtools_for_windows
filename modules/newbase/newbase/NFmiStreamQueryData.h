// ======================================================================
/*!
 * \file NFmiStreamQueryData.h
 * \brief Interface of class NFmiStreamQueryData
 */
// ======================================================================

#pragma once

#include "NFmiFastQueryInfo.h"
#include "NFmiQueryData.h"

//! Undocumented
class _FMI_DLL NFmiStreamQueryData
{
 public:
  ~NFmiStreamQueryData(void);
  NFmiStreamQueryData(void);
  NFmiStreamQueryData(NFmiQueryData* theQueryData, bool isOwnerData = false);

  // Marko Muutin rajapinnan niin, että theQueryData pointterita tuli pointterin pointteri.
  // Tällöin voin muuttaa koodin niin, että luettu data annetaan pointterin pointteriin ja omistus
  // siirtyy ulos NFmiStreamQueryData-oliolta.

  bool ReadData(const NFmiString& theFileName, NFmiQueryData** theQueryData = 0);
  bool ReadLatestData(const NFmiString& theFileName, NFmiQueryData** theQueryData = 0);
  bool SafeReadLatestData(const NFmiString& theFileName, NFmiQueryData** theQueryData = 0);
  bool WriteData(const NFmiString& theFileName,
                 NFmiQueryData* theQueryData = 0,
                 long theLibVersion = 6) const;

  bool ReadIn(NFmiQueryData* theQueryData = 0);
  bool ReadIn(NFmiQueryInfo* theQueryInfo);
  bool WriteCout(NFmiQueryData* theQueryData = 0) const;

  bool IsData(void);

  NFmiQueryData* QueryData(bool theOwnerData = false);
  NFmiFastQueryInfo* QueryInfoIter(void);

 private:
  NFmiStreamQueryData(const NFmiStreamQueryData& theData);
  NFmiStreamQueryData& operator=(const NFmiStreamQueryData& theData);

  NFmiQueryData* itsQueryData;
  NFmiFastQueryInfo* itsQueryDataIter;
  bool itsOwnerData;  // Jos false, omistaa itsQueryData:n ja tuhoaa sen destruktorissa, jos true,
                      // ei omista eikä tuhoa.

};  // class NFmiStreamQueryData

// ======================================================================
