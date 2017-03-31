// ======================================================================
/*!
 * \file NFmiGdalArea.h
 * \brief Interface of class NFmiGdalArea
 */
// ======================================================================

#ifdef UNIX

#pragma once

#include "NFmiArea.h"
#include <boost/shared_ptr.hpp>

class OGRSpatialReference;
class OGRCoordinateTransformation;

class _FMI_DLL NFmiGdalArea : public NFmiArea
{
 public:
  virtual ~NFmiGdalArea();

  NFmiGdalArea();

  NFmiGdalArea(const NFmiGdalArea& theGdalArea);

  NFmiGdalArea(const std::string& theDatum,
               const std::string& theDescription,
               const NFmiPoint& theBottomLeftLatLon,
               const NFmiPoint& theTopRightLatLon,
               const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
               const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
               bool usePacificView = false);

  NFmiGdalArea(const std::string& theDatum,
               const OGRSpatialReference& theCRS,
               const NFmiPoint& theBottomLeftLatLon,
               const NFmiPoint& theTopRightLatLon,
               const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
               const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
               bool usePacificView = false);

  NFmiGdalArea(const std::string& theDatum,
               const OGRSpatialReference& theCRS,
               double theXmin,
               double theYmin,
               double theXmax,
               double theYmax,
               const NFmiPoint& theTopLeftXY = NFmiPoint(0.f, 0.f),
               const NFmiPoint& theBottomRightXY = NFmiPoint(1.f, 1.f),
               bool usePacificView = false);

  virtual NFmiArea* Clone() const;

  NFmiGdalArea& operator=(const NFmiGdalArea& theArea);

  bool operator==(const NFmiGdalArea& theArea) const;
  bool operator!=(const NFmiGdalArea& theArea) const;

  bool operator==(const NFmiArea& theArea) const;
  bool operator!=(const NFmiArea& theArea) const;

  virtual unsigned long ClassId() const;
  virtual const char* ClassName() const;
  const std::string AreaStr() const;
  const std::string& Datum() const;
  virtual const std::string WKT() const;

  virtual std::ostream& Write(std::ostream& file) const;
  virtual std::istream& Read(std::istream& file);

  virtual const NFmiPoint ToLatLon(const NFmiPoint& theXYPoint) const;
  virtual const NFmiPoint ToXY(const NFmiPoint& theLatLonPoint) const;
  virtual const NFmiPoint XYToWorldXY(const NFmiPoint& theXYPoint) const;
  virtual const NFmiPoint WorldXYToLatLon(const NFmiPoint& theXYPoint) const;
  virtual const NFmiPoint LatLonToWorldXY(const NFmiPoint& theLatLonPoint) const;
  virtual const NFmiRect WorldRect() const;
  virtual NFmiArea* NewArea(const NFmiPoint& theBottomLeftLatLon,
                            const NFmiPoint& theTopRightLatLon,
                            bool allowPacificFix = true) const;

  virtual double WorldXYHeight() const;
  virtual double WorldXYWidth() const;

 private:
  void init();

  std::string itsDatum;
  std::string itsDescription;
  std::string itsWKT;

  NFmiPoint itsBottomLeftLatLon;
  NFmiPoint itsTopRightLatLon;
  NFmiRect itsWorldRect;

  boost::shared_ptr<OGRSpatialReference> itsSpatialReference;
  boost::shared_ptr<OGRCoordinateTransformation> itsLatLonToWorldXYTransformation;
  boost::shared_ptr<OGRCoordinateTransformation> itsWorldXYToLatLonTransformation;

};  // class NFmiGdalArea

#endif  // NFMIGDALAREA_H

// ======================================================================
