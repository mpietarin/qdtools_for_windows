// ======================================================================
/*!
 * \file
 * \brief Interface of class NFmiIndexMaskSource
 */
// ======================================================================

#ifndef NFMIINDEXMASKSOURCE_H
#define NFMIINDEXMASKSOURCE_H

#include "NFmiIndexMask.h"
#include "NFmiMetTime.h"
#include <map>

class NFmiIndexMaskSource
{
 public:
  ~NFmiIndexMaskSource();
  NFmiIndexMaskSource();
  NFmiIndexMaskSource(const NFmiIndexMaskSource& theSource);
  NFmiIndexMaskSource& operator=(const NFmiIndexMaskSource& theSource);

  void Insert(const NFmiMetTime& theTime, const NFmiIndexMask& theMask);
  const NFmiIndexMask& Find(const NFmiMetTime& theTime) const;
  void Clear(void);

 private:
  typedef std::map<NFmiMetTime, NFmiIndexMask> storage_type;
  storage_type itsData;

};  // class NFmiIndexMaskSource

#endif  // NFMIINDESMASKSOURCE_H

// ======================================================================
