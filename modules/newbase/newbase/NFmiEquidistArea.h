// ======================================================================
/*!
 * \file NFmiEquidistArea.h
 * \brief Interface of class NFmiEquidistArea
 */
// ======================================================================

#pragma once

#include "NFmiAngle.h"
#include "NFmiAzimuthalArea.h"

//! Undocumented

class _FMI_DLL NFmiEquidistArea : public NFmiAzimuthalArea
{
 public:
  virtual ~NFmiEquidistArea(void);
  NFmiEquidistArea(void);
  NFmiEquidistArea(const NFmiEquidistArea& theEquidistArea);

  NFmiEquidistArea(double theRadialRangeInMeters,
                   const NFmiPoint& theCenterLatLon,
                   const NFmiPoint& theTopLeftXY,
                   const NFmiPoint& theBottomRightXY);

  NFmiEquidistArea(const NFmiPoint& theBottomLeftLatLon,
                   const NFmiPoint& theTopRightLatLon,
                   const double theOrientation = 0.,
                   const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
                   const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
                   const double theCentralLatitude = 90.,  // 90 deg. = North Pole
                   bool usePacificView = false);

  NFmiEquidistArea(const NFmiPoint& theBottomLeftLatLon,
                   const double theWidthInMeters,
                   const double theHeightInMeters,
                   const double theOrientation = 0.,
                   const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
                   const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
                   const double theCentralLatitude = 90.);  // 90 deg. = North Pole

  NFmiEquidistArea(const double theRadialRange,
                   const double theOrientation = 0.,
                   const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
                   const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
                   const double theCentralLatitude = 90.);  // 90 deg. = North Pole

  virtual void Init(bool fKeepWorldRect = false);
  virtual NFmiArea* NewArea(const NFmiPoint& theBottomLeftLatLon,
                            const NFmiPoint& theTopRightLatLon,
                            bool allowPacificFix = true) const;

  NFmiEquidistArea& operator=(const NFmiEquidistArea& theArea);
  bool operator==(const NFmiEquidistArea& theArea) const;
  bool operator!=(const NFmiEquidistArea& theArea) const;

  using NFmiAzimuthalArea::operator==;
  using NFmiAzimuthalArea::operator!=;

  bool operator==(const NFmiArea& theArea) const;
  bool operator!=(const NFmiArea& theArea) const;

  virtual NFmiArea* Clone(void) const;
  virtual unsigned long ClassId(void) const;
  virtual const char* ClassName(void) const;
  const std::string AreaStr(void) const;
  virtual const std::string WKT() const;

 protected:
  virtual double K(const double delta) const;
  virtual double CalcDelta(const double xyDistance) const;
  virtual double DistanceFromPerspectivePointToCenterOfEarth(void) const;

 private:
};  // class NFmiEquidistArea

//! Undocumented, should be removed
typedef NFmiEquidistArea* PNFmiEquidistArea;

// ----------------------------------------------------------------------
/*!
 * The destructor does nothing special
 */
// ----------------------------------------------------------------------

inline NFmiEquidistArea::~NFmiEquidistArea(void) {}
// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

inline NFmiEquidistArea::NFmiEquidistArea(void) : NFmiAzimuthalArea() {}
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline unsigned long NFmiEquidistArea::ClassId(void) const { return kNFmiEquiDistArea; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline const char* NFmiEquidistArea::ClassName(void) const { return "NFmiEquiDistArea"; }

// ======================================================================
