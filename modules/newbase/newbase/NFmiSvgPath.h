// ======================================================================
/*!
 * \file
 * \brief Interface of class NFmiSvgPath
 */
// ======================================================================

#pragma once

#include "NFmiDataMatrix.h"
#include "NFmiDef.h"
#include <iosfwd>

class NFmiPoint;

class _FMI_DLL NFmiSvgPath
{
 public:
  NFmiSvgPath();

  //! Undocumented
  enum ElementType
  {
    kElementNotValid = 0,  //!< virhetilanteisiin
    kElementMoveto,        //!< "M" moveto absoluuttinen
    kElementLineto,        //!< "L" lineto absoluuttinen
    kElementClosePath      //!< "Z" close path
  };

  struct Element
  {
    Element(ElementType theType, double theX = 0, double theY = 0)
        : itsType(theType), itsX(theX), itsY(theY)
    {
    }
    ElementType itsType;
    double itsX;
    double itsY;

   private:
    Element();
  };

  typedef Element value_type;
  typedef const value_type& const_reference;
  typedef value_type& reference;
  typedef checkedVector<value_type> storage_type;
  typedef storage_type::size_type size_type;
  typedef storage_type::difference_type difference_type;
  typedef storage_type::const_iterator const_iterator;
  typedef storage_type::iterator iterator;

  size_type size(void) const;
  bool empty(void) const;
  void clear(void);

  void push_back(const Element& theElement);

  //@{ \name Kirjoitus- ja luku-operaatiot
  std::ostream& Write(std::ostream& file) const;
  std::istream& Read(std::istream& file);
  //@}

  const_iterator begin(void) const;
  const_iterator end(void) const;
  iterator begin(void);
  iterator end(void);

  const_reference front(void) const;
  const_reference back(void) const;

  bool IsInside(const NFmiPoint& thePoint) const;
  storage_type& GetData(void) { return itsData; }

 private:
  storage_type itsData;

  // Hidden bounding box information for faster isInside implementation
  mutable bool itsBBoxValid;
  mutable double itsMinX;
  mutable double itsMinY;
  mutable double itsMaxX;
  mutable double itsMaxY;

};  // class NFmiSvgPath

// ----------------------------------------------------------------------
/*!
 * \brief begin metodi
 */
// ----------------------------------------------------------------------

inline NFmiSvgPath::const_iterator NFmiSvgPath::begin(void) const { return itsData.begin(); }
// ----------------------------------------------------------------------
/*!
 * \brief begin metodi
 */
// ----------------------------------------------------------------------

inline NFmiSvgPath::iterator NFmiSvgPath::begin(void)
{
  itsBBoxValid = false;
  return itsData.begin();
}

// ----------------------------------------------------------------------
/*!
 * \brief end metodi
 */
// ----------------------------------------------------------------------

inline NFmiSvgPath::const_iterator NFmiSvgPath::end(void) const { return itsData.end(); }
// ----------------------------------------------------------------------
/*!
 * \brief end metodi
 */
// ----------------------------------------------------------------------

inline NFmiSvgPath::iterator NFmiSvgPath::end(void)
{
  itsBBoxValid = false;
  return itsData.end();
}

// ----------------------------------------------------------------------
/*!
 * \brief Palauttaa ensimmäisen elementin
 */
// ----------------------------------------------------------------------

inline NFmiSvgPath::const_reference NFmiSvgPath::front(void) const { return itsData.front(); }
// ----------------------------------------------------------------------
/*!
 * \brief Palauttaa viimeisen elementin
 */
// ----------------------------------------------------------------------

inline NFmiSvgPath::const_reference NFmiSvgPath::back(void) const { return itsData.back(); }
// ----------------------------------------------------------------------
/*!
 * Global operator>> overload for NFmiSvgPath class
 *
 * \param os The output stream to write to
 * \param item The object to write out
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream& os, const NFmiSvgPath& item)
{
  return item.Write(os);
}

// ----------------------------------------------------------------------
/*!
 * Global operator<< overload for NFmiSvgPath class
 *
 * \param is The input stream to read from
 * \param item The object into which to read the new contents
 * \return The input stream read from
 */
// ----------------------------------------------------------------------

inline std::istream& operator>>(std::istream& is, NFmiSvgPath& item) { return item.Read(is); }

// ======================================================================
