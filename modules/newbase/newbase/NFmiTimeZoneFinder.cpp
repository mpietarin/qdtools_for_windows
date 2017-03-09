// ======================================================================
/*!
 * \file
 * \brief Implementation of class NFmiTimeZoneFinder
 */
// ======================================================================
/*!
 * \class NFmiTimeZoneFinder
 *
 * This class is intended for finding out the timezone at the given
 * geographic coordinates.
 *
 * Sample usage:
 *
 * \code
 * #include "NFmiTimeZoneFinder.h"
 * ...
 * NFmiTimeZoneFinder finder;
 * finder.ReadFile("timezones.txt");
 * float tz = finder.Find(NFmiPoint(25,60));
 * if(tz != kFloatMissing)
 * { ... }
 * \endcode
 *
 *
 */
// ======================================================================

#include "NFmiTimeZoneFinder.h"
#include "NFmiFileSystem.h"
#include "NFmiPoint.h"
#include "NFmiPreProcessor.h"
#include <list>
#include <memory>
#include <set>
#include <stdexcept>
#include <iostream>
#include <sstream>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief A simple edge
 *
 * This needs to be a class since it is placed into a set.
 * That is, we need comparison operators.
 */
// ----------------------------------------------------------------------

class NFmiTimeZoneEdge
{
 public:
  NFmiTimeZoneEdge(const NFmiPoint& thePoint1, const NFmiPoint& thePoint2)
      : itsPoint1(thePoint1), itsPoint2(thePoint2)
  {
    if (itsPoint1.Y() > itsPoint2.Y()) swap(itsPoint1, itsPoint2);
  }

  NFmiTimeZoneEdge(double theX1, double theY1, double theX2, double theY2)
      : itsPoint1(NFmiPoint(theX1, theY1)), itsPoint2(NFmiPoint(theX2, theY2))
  {
    if (itsPoint1.Y() > itsPoint2.Y()) swap(itsPoint1, itsPoint2);
  }

  double X1(void) const { return itsPoint1.X(); }
  double Y1(void) const { return itsPoint1.Y(); }
  double X2(void) const { return itsPoint2.X(); }
  double Y2(void) const { return itsPoint2.Y(); }
  bool operator==(const NFmiTimeZoneEdge& theOther) const;
  bool operator<(const NFmiTimeZoneEdge& theOther) const;

 private:
  NFmiPoint itsPoint1;
  NFmiPoint itsPoint2;

};  // class NFmiTimeZoneEdge

// ----------------------------------------------------------------------
/*!
 * \brief Equality comparison
 */
// ----------------------------------------------------------------------

bool NFmiTimeZoneEdge::operator==(const NFmiTimeZoneEdge& theOther) const
{
  return (X1() == theOther.X1() && Y1() == theOther.Y1() && X2() == theOther.X2() &&
          Y2() == theOther.Y2());
}

// ----------------------------------------------------------------------
/*!
 * \brief Less-than comparison
 */
// ----------------------------------------------------------------------

bool NFmiTimeZoneEdge::operator<(const NFmiTimeZoneEdge& theOther) const
{
  if (Y1() != theOther.Y1()) return (Y1() < theOther.Y1());
  if (Y2() != theOther.Y2()) return (Y2() < theOther.Y2());
  if (X1() != theOther.X1()) return (X1() < theOther.X1());
  return (X2() < theOther.X2());
}

// ----------------------------------------------------------------------
/*!
 * \brief A simple ring
 */
// ----------------------------------------------------------------------

class NFmiTimeZoneRing
{
 public:
  NFmiTimeZoneRing(void) : itsSignedArea(0.0), itsStarted(false) {}
  void Add(const NFmiPoint& thePoint);
  bool Inside(const NFmiPoint& thePoint) const;
  bool Clockwise(void) const;

 private:
  static double MaxLength;

  void CheckClosed(void) const;
  void Add(const NFmiTimeZoneEdge& theEdge);

  typedef set<NFmiTimeZoneEdge> storage_type;
  storage_type itsData;
  double itsSignedArea;
  NFmiPoint itsFirstPoint;
  NFmiPoint itsLastPoint;
  bool itsStarted;
  double itsMinX;
  double itsMinY;
  double itsMaxX;
  double itsMaxY;

};  // class NFmiTimeZoneRing

// Any edge longer than this in y-direction will be split in half
// for extra speed while traversing the set of edges. Experimentally
// using a shorter distance such as 1 will not gain much benefit,
// whereas using a large value such as 90 will make the Inside
// method 3 times slower.

double NFmiTimeZoneRing::MaxLength = 5;

// ----------------------------------------------------------------------
/*!
 * \brief Add a point to the polygon
 */
// ----------------------------------------------------------------------

void NFmiTimeZoneRing::Add(const NFmiPoint& thePoint)
{
  if (!itsStarted || thePoint.X() < itsMinX) itsMinX = thePoint.X();
  if (!itsStarted || thePoint.X() > itsMaxX) itsMaxX = thePoint.X();
  if (!itsStarted || thePoint.Y() < itsMinY) itsMinY = thePoint.Y();
  if (!itsStarted || thePoint.Y() > itsMaxY) itsMaxY = thePoint.Y();

  if (itsStarted)
  {
    itsSignedArea += itsLastPoint.X() * thePoint.Y() - thePoint.X() * itsLastPoint.Y();
    //	  cout << itsLastPoint.X() << ' ' << itsLastPoint.Y()
    //		   << "\t--> "
    //		   << thePoint.X() << ' ' << thePoint.Y()
    //		   << "\t" << itsSignedArea << endl;
    Add(NFmiTimeZoneEdge(itsLastPoint, thePoint));
  }
  else
    itsFirstPoint = thePoint;

  itsLastPoint = thePoint;

  itsStarted = true;
}

// ----------------------------------------------------------------------
/*!
 * Add an edge to the polygon
 */
// ----------------------------------------------------------------------

void NFmiTimeZoneRing::Add(const NFmiTimeZoneEdge& theEdge)
{
  if (theEdge.Y2() - theEdge.Y1() >= MaxLength)
  {
    double midx = 0.5 * (theEdge.X1() + theEdge.X2());
    double midy = 0.5 * (theEdge.Y1() + theEdge.Y2());
    Add(NFmiTimeZoneEdge(theEdge.X1(), theEdge.Y1(), midx, midy));
    Add(NFmiTimeZoneEdge(midx, midy, theEdge.X2(), theEdge.Y2()));
  }
  else
    itsData.insert(theEdge);
}

// ----------------------------------------------------------------------
/*!
 * \brief Make sure the ring is closed
 */
// ----------------------------------------------------------------------

void NFmiTimeZoneRing::CheckClosed(void) const
{
  if (itsFirstPoint != itsLastPoint) throw runtime_error("NFmiTimeZoneRing is not closed!");
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if the ring is clockwise
 */
// ----------------------------------------------------------------------

bool NFmiTimeZoneRing::Clockwise(void) const
{
  if (!itsStarted) return false;
  CheckClosed();
  return (itsSignedArea < 0);
}

// ----------------------------------------------------------------------
/*!
 * \brief Insidedness test
 */
// ----------------------------------------------------------------------

bool NFmiTimeZoneRing::Inside(const NFmiPoint& thePoint) const
{
  if (thePoint.X() == kFloatMissing || thePoint.Y() == kFloatMissing) return false;

  if (!itsStarted) return false;

  if (thePoint.X() < itsMinX || thePoint.X() > itsMaxX || thePoint.Y() < itsMinY ||
      thePoint.Y() > itsMaxY)
    return false;

  CheckClosed();

  const double min_lon = -180.0;
  const double min_y = thePoint.Y() - MaxLength;
  NFmiTimeZoneEdge dummy(min_lon, min_y, min_lon, min_y);
  storage_type::const_iterator it = itsData.lower_bound(dummy);

  double x = thePoint.X();
  double y = thePoint.Y();

  int counter = 0;
  for (; it != itsData.end(); ++it)
  {
    if (y <= it->Y1()) break;
    if (y <= it->Y2())
      if (x <= max(it->X1(), it->X2()))
        if (it->Y1() != it->Y2())
        {
          if (it->X1() == it->X2())
            counter++;
          else if (x <= (y - it->Y1()) * (it->X2() - it->X1()) / (it->Y2() - it->Y1()) + it->X1())
            counter++;
        }
  }
  return (counter % 2 != 0);
}

// ----------------------------------------------------------------------
/*!
 * \brief Time zone polygon
 *
 * A timezone polygon consists of one or more non-intersecting
 * simple closed rings. The winding direction of each individual
 * ring determines whether the ring is actual area or a hole in it.
 *
 * We follow the ESRI shape file convention, which says each
 * counterclockwise ring is a hole.
 */
// ----------------------------------------------------------------------

class NFmiTimeZonePolygon
{
 public:
  NFmiTimeZonePolygon(float theTZ) : itsTimeZone(theTZ) {}
  void Add(const NFmiTimeZoneRing& theRing);
  bool Inside(const NFmiPoint& thePoint) const;
  float TimeZone(void) const { return itsTimeZone; }
 private:
  NFmiTimeZonePolygon(void);
  float itsTimeZone;
  typedef list<NFmiTimeZoneRing> storage_type;
  storage_type itsRings;

};  // class NFmiTimeZonePolygon

// ----------------------------------------------------------------------
/*!
 * \brief Add a ring to the time zone polygon
 *
 * It is essential that holes are pushed to front, otherwise
 * Inside method will not work.
 *
 * \param theRing The ring
 */
// ----------------------------------------------------------------------

void NFmiTimeZonePolygon::Add(const NFmiTimeZoneRing& theRing)
{
  if (theRing.Clockwise())
    itsRings.push_back(theRing);
  else
    itsRings.push_front(theRing);
}

// ----------------------------------------------------------------------
/*!
 * \brief Test whether the given point is inside the time zone polygon
 *
 * It is essential that Add pushes holes to front, otherwise this
 * will not work. Holes must be tested before the actual coverage.
 *
 * \param thePoint The point
 * \return True if the point is inside the polygon
 */
// ----------------------------------------------------------------------

bool NFmiTimeZonePolygon::Inside(const NFmiPoint& thePoint) const
{
  if (itsRings.empty()) return false;

  int i = 0;
  for (storage_type::const_iterator it = itsRings.begin(); it != itsRings.end(); ++it)
  {
    if (it->Inside(thePoint)) return it->Clockwise();
    i++;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple for NFmiTimeZoneFinder
 */
// ----------------------------------------------------------------------

class NFmiTimeZoneFinderPimple
{
 public:
  NFmiTimeZoneFinderPimple() : itsData() {}
  float Find(const NFmiPoint& thePoint) const;

  typedef list<NFmiTimeZonePolygon> storage_type;
  storage_type itsData;

};  // class NFmiTimeZoneFinderPimple

// ----------------------------------------------------------------------
/*!
 * \brief Find the timezone of the given point
 *
 * \param theLatLon The point
 * \return The time zone in hours, or kFloatMissing
 */
// ----------------------------------------------------------------------

float NFmiTimeZoneFinderPimple::Find(const NFmiPoint& theLatLon) const
{
  if (theLatLon.X() == kFloatMissing || theLatLon.Y() == kFloatMissing) return kFloatMissing;

  int i = 0;
  for (storage_type::const_iterator it = itsData.begin(); it != itsData.end(); ++it)
  {
    if (it->Inside(theLatLon)) return it->TimeZone();
    i++;
  }
  return kFloatMissing;
}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

NFmiTimeZoneFinder::~NFmiTimeZoneFinder(void) {}
// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

NFmiTimeZoneFinder::NFmiTimeZoneFinder(void) { itsPimple.reset(new NFmiTimeZoneFinderPimple()); }
// ----------------------------------------------------------------------
/*!
 * \brief Copy constructor
 *
 * \param theFinder The finder to copy
 */
// ----------------------------------------------------------------------

NFmiTimeZoneFinder::NFmiTimeZoneFinder(const NFmiTimeZoneFinder& theFinder)
{
  itsPimple.reset(new NFmiTimeZoneFinderPimple(*theFinder.itsPimple));
}

// ----------------------------------------------------------------------
/*!
 * \brief Assignment operator
 *
 * \param theFinder The finder to copy
 * \return The finder assigned to
 */
// ----------------------------------------------------------------------

NFmiTimeZoneFinder& NFmiTimeZoneFinder::operator=(const NFmiTimeZoneFinder& theFinder)
{
  if (this != &theFinder)
  {
    itsPimple.reset(new NFmiTimeZoneFinderPimple(*theFinder.itsPimple));
  }
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * \brief Find the timezone of the given point
 *
 * \param theLatLon The point
 * \return The time zone in hours, or kFloatMissing
 */
// ----------------------------------------------------------------------

float NFmiTimeZoneFinder::Find(const NFmiPoint& theLatLon) const
{
  return itsPimple->Find(theLatLon);
}

// ----------------------------------------------------------------------
/*!
 * \brief Test whether the finder is empty
 *
 * \return True if the finder is empty
 */
// ----------------------------------------------------------------------

bool NFmiTimeZoneFinder::Empty(void) const { return itsPimple->itsData.empty(); }
// ----------------------------------------------------------------------
/*!
 * \brief Clear the finder
 */
// ----------------------------------------------------------------------

void NFmiTimeZoneFinder::Clear(void) { itsPimple->itsData.clear(); }
// ----------------------------------------------------------------------
/*!
 * \brief Read the given timezone polygon file
 *
 * Any previous contents are cleared first.
 *
 * The basic premize is that the command
 * \code
 * timezone <float>
 * \endcode
 * starts a new NFmiTimeZonePolygon, and each
 * \code
 * ring
 * \endcode
 * command beging a new ring in it. Naturally each timezone
 * command (and eof) also terminate the previous ring/timezone.
 *
 * \param theFileName The file to read
 * \return False on failure
 */
// ----------------------------------------------------------------------

bool NFmiTimeZoneFinder::ReadFile(const string& theFileName)
{
  Clear();

  if (!NFmiFileSystem::FileExists(theFileName)) return false;

  const bool strip_pound = true;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include", "", "");
  if (!processor.ReadAndStripFile(theFileName)) return false;

  string text = processor.GetString();

  istringstream input(text);

  boost::shared_ptr<NFmiTimeZonePolygon> poly;
  boost::shared_ptr<NFmiTimeZoneRing> ring;

  string command;
  while (input >> command)
  {
    if (command == "timezone")
    {
      if (ring.get() != 0) poly->Add(*ring);
      if (poly.get() != 0) itsPimple->itsData.push_back(*poly);

      float tz;
      input >> tz;
      poly.reset(new NFmiTimeZonePolygon(tz));
      ring.reset(new NFmiTimeZoneRing());
    }
    else if (command == "xy")
    {
      double x, y;
      input >> x >> y;

      if (ring.get() == 0)
      {
        itsPimple->itsData.clear();
        return false;
      }

      ring->Add(NFmiPoint(x, y));
    }
    else if (command == "ring")
    {
      if (poly.get() == 0 || ring.get() == 0)
      {
        itsPimple->itsData.clear();
        return false;
      }
      poly->Add(*ring);
      ring.reset(new NFmiTimeZoneRing());
    }
    else
    {
      itsPimple->itsData.clear();
      return false;
    }
  }

  if (ring.get()) poly->Add(*ring);
  if (poly.get()) itsPimple->itsData.push_back(*poly);

  return true;
}

#ifndef NDEBUG

// ----------------------------------------------------------------------
/*!
 * \brief Verify correct operation
 */
// ----------------------------------------------------------------------

#include <iostream>
void NFmiTimeZoneFinder::Check(const NFmiPoint& theLatLon) const
{
  int count = 0;
  int poly = 0;
  float tz = 0;

  int p = 0;
  for (NFmiTimeZoneFinderPimple::storage_type::const_iterator it = itsPimple->itsData.begin();
       it != itsPimple->itsData.end();
       ++it)
  {
    bool match = it->Inside(theLatLon);
    if (match)
    {
      if (count == 1)
      {
        cout << theLatLon.X() << ' ' << theLatLon.Y() << " matches many polygons:" << endl;
        cout << "match " << count << '\t' << poly << '\t' << tz << endl;
      }
      count++;
      poly = p;
      tz = it->TimeZone();
      if (count > 1) cout << "match " << count << '\t' << p << '\t' << tz << endl;
    }
    p++;
  }
}

#endif  // NDEBUG

// ======================================================================
