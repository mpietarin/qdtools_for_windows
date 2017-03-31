// ======================================================================
/*!
 * \file NFmiRect.cpp
 * \brief Implementation of class NFmiRect
 */
// ======================================================================
/*!
 * \class NFmiRect
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiRect.h"
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <fstream>

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param firstCorner Undocumented
 * \param oppositeCorner Undocumented
 */
// ----------------------------------------------------------------------

NFmiRect::NFmiRect(const NFmiPoint &firstCorner, const NFmiPoint &oppositeCorner)
    : itsPlace(std::min(firstCorner.X(), oppositeCorner.X()),
               std::min(firstCorner.Y(), oppositeCorner.Y())),
      itsSize(NFmiPoint(std::max(firstCorner.X(), oppositeCorner.X()),
                        std::max(firstCorner.Y(), oppositeCorner.Y())) -
              itsPlace)
{
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param left Undocumented
 * \param top Undocumented
 * \param right Undocumented
 * \param bottom Undocumented
 */
// ----------------------------------------------------------------------

NFmiRect::NFmiRect(double left, double top, double right, double bottom)
    : itsPlace(std::min(left, right), std::min(top, bottom)),
      itsSize(NFmiPoint(std::max(left, right), std::max(top, bottom)) - itsPlace)
{
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRect::Inflate(double theValue)
{
  itsSize += NFmiPoint(2 * theValue, 2 * theValue);
  itsPlace -= NFmiPoint(theValue, theValue);
}

// ----------------------------------------------------------------------
/*!
 * \param theXValue Undocumented
 * \param theYValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRect::Inflate(double theXValue, double theYValue)
{
  itsSize += NFmiPoint(2 * theXValue, 2 * theYValue);
  itsPlace -= NFmiPoint(theXValue, theYValue);
}

// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRect::Inflate(const NFmiPoint &theValue)
{
  NFmiPoint theSizingValue(theValue);
  theSizingValue += theValue;
  itsSize += theSizingValue;
  itsPlace -= theValue;
}

// ----------------------------------------------------------------------
/*!
 * \param thePoint Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

const NFmiPoint NFmiRect::NearestCorner(const NFmiPoint &thePoint) const
{
  double dLeftTop = TopLeft().Distance(thePoint);
  double dRightTop = TopRight().Distance(thePoint);
  double dLeftBottom = BottomLeft().Distance(thePoint);
  double dRightBottom = BottomRight().Distance(thePoint);

  if (dLeftTop <= dRightTop)
  {
    if (dLeftTop <= dLeftBottom)
      return TopLeft();
    else
      return BottomLeft();
  }
  else
  {
    if (dRightTop <= dRightBottom)
      return TopRight();
    else
      return BottomRight();
  }
}

// ----------------------------------------------------------------------
/*!
 * \param theRatioXperY Undocumented
 * \param fKeepX Undocumented
 * \param theDirection Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

// Säätää rectin aspect ration halutuksi. fKeepX ollessa tosi ei muuteta
// x:n arvoja. Korjaukset tehdään halutun nurkan ('theDirection') suhteen
// eli valittu nurkka "pysyy paikallaan". Jos muutos on mahdoton (esim.
// aspectratio=0), palauttaa funktio false:n muuten true:n.

bool NFmiRect::AdjustAspectRatio(double theRatioXperY, bool fKeepX, FmiDirection theDirection)
{
  if (theRatioXperY == 0.) return false;

  NFmiRect originalRect(itsPlace, itsPlace + itsSize);  // 9.9.99/EL

  if (fKeepX)
    Size(NFmiPoint(Width(), Width() / theRatioXperY));
  else
    Size(NFmiPoint(theRatioXperY * Height(), Height()));

  double newWidth = Width();
  double newHeight = Height();

  // Origona on nurkka 'kTopLeft' missä (X,Y) = TopLeft()
  // X kasvaa "vasemmalta oikealle"
  // Y kasvaa "ylhäältä alas"

  switch (theDirection)
  {
    case kTopLeft:
      return true;  // Nurkka on origossa - Place() pysyy paikallaan

    case kTopRight:
      Place(NFmiPoint(originalRect.TopRight().X() - newWidth, TopRight().Y()));
      return true;

    case kBottomLeft:
      Place(NFmiPoint(originalRect.BottomLeft().X(), originalRect.BottomLeft().Y() - newHeight));
      return true;

    case kBottomRight:
      Place(NFmiPoint(originalRect.BottomRight().X() - newWidth,
                      originalRect.BottomRight().Y() - newHeight));
      return true;

    case kCenter:
      Center(originalRect.Center());
      return true;

    default:
      return false;
  }
}

// ----------------------------------------------------------------------
/*!
 * Projisoi annetun maailma koordinaatin siten että jos piste on rectin
 * sisällä, palautettu rect saa arvon 0,0 - 1,1 sen mukaan miten se rectin
 * sisään osui. Origo on top-left ja 1,1 on bottom-right kulma. Jos se on
 * ruudun ulkona, tulee mukaan mahdolliset negatiiviset ja 1:stä suuremmat arvot.
 *
 * \param thePoint The relative world point that is projected to rect dimensions.
 * \return The given point in rect's world coordinates.
 */
// ----------------------------------------------------------------------
const NFmiPoint NFmiRect::Project(const NFmiPoint &thePlace) const
{
  NFmiPoint newPoint = thePlace - TopLeft();
  return NFmiPoint(newPoint.X() / Width(), newPoint.Y() / Height());
}

// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param thePoint The other object being assigned
 * \return The assigned object
 */
// ----------------------------------------------------------------------

NFmiRect &NFmiRect::operator+=(const NFmiPoint &thePoint)
{
  itsPlace += thePoint;
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Substraction operator
 *
 * \param thePoint The coordinates being substracted
 * \return The resulting rectangle
 */
// ----------------------------------------------------------------------

NFmiRect &NFmiRect::operator-=(const NFmiPoint &thePoint)
{
  itsPlace -= thePoint;
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Addition operator
 *
 * \param theRect The rectangle being added
 * \return The resulting rectangle
 */
// ----------------------------------------------------------------------

NFmiRect &NFmiRect::operator+=(const NFmiRect &theRect)
{
  itsPlace = NFmiPoint(Left() < theRect.Left() ? Left() : theRect.Left(),
                       Top() < theRect.Top() ? Top() : theRect.Top());

  itsSize = NFmiPoint(Right() > theRect.Right() ? Right() : theRect.Right(),
                      Bottom() > theRect.Bottom() ? Bottom() : theRect.Bottom()) -
            itsPlace;
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Substraction operator
 *
 * \param theRect The rectangle being substracted
 * \result The resulting rectangle
 */
// ----------------------------------------------------------------------

NFmiRect &NFmiRect::operator-=(const NFmiRect &theRect)
{
  itsPlace = NFmiPoint(Left() > theRect.Left() ? Left() : theRect.Left(),
                       Top() > theRect.Top() ? Top() : theRect.Top());

  itsSize = NFmiPoint(Right() < theRect.Right() ? Right() : theRect.Right(),
                      Bottom() < theRect.Bottom() ? Bottom() : theRect.Bottom()) -
            itsPlace;
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * \param newCenter Undocumented
 */
// ----------------------------------------------------------------------

void NFmiRect::Center(const NFmiPoint &newCenter)
{
  Place(NFmiPoint(newCenter.X() - (Width() / 2.), newCenter.Y() - (Height() / 2.)));
}

// ----------------------------------------------------------------------
/*!
 * Palauttaa kahta rectiä ympäröivän rectin.
 * \param theRect Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

const NFmiRect NFmiRect::SmallestEnclosing(const NFmiRect &theRect) const
{
  double left = Left() < theRect.Left() ? Left() : theRect.Left();
  double top = Top() < theRect.Top() ? Top() : theRect.Top();
  double right = Right() > theRect.Right() ? Right() : theRect.Right();
  double bottom = Bottom() > theRect.Bottom() ? Bottom() : theRect.Bottom();
  return NFmiRect(left, top, right, bottom);
}

// ----------------------------------------------------------------------
/*!
 * Palauttaa pienimmän rect:in, mikä pitää sisällään this-rect:in ja
 * parametrina annetun rect:in eli leikkaus.
 * \param theRect Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

const NFmiRect NFmiRect::Intersection(const NFmiRect &theRect) const
{
  double left = Left() > theRect.Left() ? Left() : theRect.Left();
  double top = Top() > theRect.Top() ? Top() : theRect.Top();
  double right = Right() < theRect.Right() ? Right() : theRect.Right();
  double bottom = Bottom() < theRect.Bottom() ? Bottom() : theRect.Bottom();
  return NFmiRect(left, top, right, bottom);
}

// ----------------------------------------------------------------------
/*!
 * Write the rectangle to the given output stream
 *
 * \param file The output stream to write to
 * \result The output stream written to
 */
// ----------------------------------------------------------------------

std::ostream &NFmiRect::Write(std::ostream &file) const
{
  file << itsPlace;
  file << itsSize;
  return file;
}

// ----------------------------------------------------------------------
/*!
 * Read new rectangle definition from the given input stream
 *
 * \param file The input stream to read from
 * \return The input stream read from
 */
// ----------------------------------------------------------------------

std::istream &NFmiRect::Read(std::istream &file)
{
  file >> itsPlace;
  file >> itsSize;
  return file;
}

// ----------------------------------------------------------------------
/*!
 * Addition of a point and a rectangle
 *
 * \param leftPoint Undocumented
 * \param rightRect Undocumented
 * \return Undocumemted
 */
// ----------------------------------------------------------------------

NFmiRect operator+(const NFmiPoint &leftPoint, const NFmiRect &rightRect)
{
  return NFmiRect(NFmiPoint(leftPoint.X() + rightRect.Left(), leftPoint.Y() + rightRect.Top()),
                  NFmiPoint(leftPoint.X() + rightRect.Right(), leftPoint.Y() + rightRect.Bottom()));
}

// ----------------------------------------------------------------------
/*!
 * Substraction of a point and a rectangle
 *
 * \param leftPoint Undocumented
 * \param rightRect Undocumented
 * \return Undocumemted
 */
// ----------------------------------------------------------------------

NFmiRect operator-(const NFmiPoint &leftPoint, const NFmiRect &rightRect)
{
  return NFmiRect(NFmiPoint(leftPoint.X() - rightRect.Left(), leftPoint.Y() - rightRect.Top()),
                  NFmiPoint(leftPoint.X() - rightRect.Right(), leftPoint.Y() - rightRect.Bottom()));
}

// ----------------------------------------------------------------------
/*!
 * Addition of a point and a rectangle
 *
 * \param leftRect Undocumented
 * \param rightPoint Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

NFmiRect operator+(const NFmiRect &leftRect, const NFmiPoint &rightPoint)
{
  return NFmiRect(NFmiPoint(rightPoint.X() + leftRect.Left(), rightPoint.Y() + leftRect.Top()),
                  NFmiPoint(rightPoint.X() + leftRect.Right(), rightPoint.Y() + leftRect.Bottom()));
}

// ----------------------------------------------------------------------
/*!
 * Substraction of a rectangle and a point
 *
 * \param leftRect Undocumented
 * \param rightPoint Undocumented
 * \result Undocumented
 */
// ----------------------------------------------------------------------

NFmiRect operator-(const NFmiRect &leftRect, const NFmiPoint &rightPoint)
{
  return NFmiRect(NFmiPoint(rightPoint.X() - leftRect.Left(), rightPoint.Y() - leftRect.Top()),
                  NFmiPoint(rightPoint.X() - leftRect.Right(), rightPoint.Y() - leftRect.Bottom()));
}

bool NFmiRect::Intersect(const NFmiRect &theRect) const
{
  if (this->Left() < theRect.Right() && this->Right() > theRect.Left() &&
      this->Top() < theRect.Bottom() && this->Bottom() > theRect.Top())
    return true;
  else
    return false;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return hash value for the rectangle
 */
// ----------------------------------------------------------------------

std::size_t NFmiRect::HashValue() const
{
  std::size_t hash = itsPlace.HashValue();
  boost::hash_combine(hash, itsSize.HashValue());
  return hash;
}

// ======================================================================
