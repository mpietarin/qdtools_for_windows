// ======================================================================
/*!
 * \file NFmiPKJArea.h
 * \brief Interface of class NFmiPKJArea
 */
// ======================================================================

#ifndef NFMIPKJAREA_H
#define NFMIPKJAREA_H

#include "NFmiKKJArea.h"

//! Undocumented
class _FMI_DLL NFmiPKJArea : public NFmiKKJArea
{
 public:
  virtual ~NFmiPKJArea(void);
  NFmiPKJArea(void);
  NFmiPKJArea(const NFmiPKJArea& thePKJArea);

  NFmiPKJArea(const NFmiPoint& theBottomLeftLatLon,
              const NFmiPoint& theTopRightLatLon,
              const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
              const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
              bool usePacificView = false);

  NFmiPKJArea(const NFmiPoint& theBottomLeftLatLon,
              const double theWidthInMeters,
              const double theHeightInMeters,
              const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
              const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f));

  NFmiPKJArea(const NFmiPoint& theBottomLeftWorldXY,  // defaults to meters
              const NFmiPoint& theTopRightWorldXY,    // defaults to meters
              const bool& IsWorldXYInMeters,          // dummy argument
              const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
              const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f));

  void Init(bool fKeepWorldRect = false);
  virtual NFmiArea* NewArea(const NFmiPoint& theBottomLeftLatLon,
                            const NFmiPoint& theTopRightLatLon,
                            bool allowPacificFix = true) const;

  virtual NFmiArea* Clone(void) const;

  virtual unsigned long ClassId(void) const;
  virtual const char* ClassName(void) const;

  NFmiPKJArea& operator=(const NFmiPKJArea& theArea);

  virtual bool operator==(const NFmiPKJArea& theArea) const;
  virtual bool operator!=(const NFmiPKJArea& theArea) const;

  using NFmiKKJArea::operator==;
  using NFmiKKJArea::operator!=;
  virtual bool operator==(const NFmiArea& theArea) const;
  virtual bool operator!=(const NFmiArea& theArea) const;

  virtual std::ostream& Write(std::ostream& file) const;
  virtual std::istream& Read(std::istream& file);

  virtual const std::string WKT() const;

 protected:
  virtual int ZoneNumberByLongitude(const double lon) const;
  virtual int ZoneNumberByEasting(const double easting) const;

 private:
  NFmiPoint CornerWorldXY(const NFmiPoint& latlonPoint) const;

};  // class NFmiPKJArea

//! Undocumented, should be removed
typedef NFmiPKJArea* PNFmiPKJArea;

// ----------------------------------------------------------------------
/*!
 * The destructor does nothing special.
 */
// ----------------------------------------------------------------------

inline NFmiPKJArea::~NFmiPKJArea(void) {}
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline unsigned long NFmiPKJArea::ClassId(void) const { return kNFmiPKJArea; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline const char* NFmiPKJArea::ClassName(void) const { return "NFmiPKJArea"; }
#endif  // NFMIPKJAREA_H

// ======================================================================
