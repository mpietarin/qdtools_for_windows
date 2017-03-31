// ======================================================================
/*!
 * \file NFmiParam.h
 * \brief Interface of class NFmiParam
 */
// ======================================================================

#pragma once

#include "NFmiIndividual.h"

//! Undocumented

class _FMI_DLL NFmiParam : public NFmiIndividual
{
 public:
  NFmiParam(void);
  NFmiParam(const NFmiParam &theParam);

  explicit NFmiParam(unsigned long theIdent,
                     const NFmiString &theName = "Koiranpentu",
                     double theMinValue = kFloatMissing,
                     double theMaxValue = kFloatMissing,
                     float theScale = kFloatMissing,
                     float theBase = kFloatMissing,
                     const NFmiString itsPrecision = "%.1f",
                     FmiInterpolationMethod theInterpolationMethod = kNearestPoint);

  NFmiParam &operator=(const NFmiParam &theParam);
  bool operator==(const NFmiParam &theParam) const;
  bool operator<(const NFmiParam &theParam) const;

  virtual NFmiParam *Clone(void) const;

  double MaxValue(void) const;
  double MinValue(void) const;
  void MaxValue(double newValue);
  void MinValue(double newValue);

  float Scale(void) const;
  float Base(void) const;
  const NFmiString &Precision(void) const;
  void Scale(float newValue);
  void Base(float newValue);
  void Precision(const NFmiString &newValue);
  bool IsScaleBase(void);

  FmiInterpolationMethod InterpolationMethod(void) const;
  void InterpolationMethod(FmiInterpolationMethod theInterpolationMethod);

  virtual std::ostream &Write(std::ostream &file) const;
  virtual std::istream &Read(std::istream &file);
  virtual const char *ClassName(void) const;

 protected:
 private:
  double itsMinValue;
  double itsMaxValue;
  float itsScale;
  float itsBase;
  FmiInterpolationMethod itsInterpolationMethod;
  NFmiString itsPrecision;

};  // class NFmiParam

// ----------------------------------------------------------------------
/*!
 * Void constructor
 */
// ----------------------------------------------------------------------

inline NFmiParam::NFmiParam(void)
    : NFmiIndividual(),
      itsMinValue(kFloatMissing),
      itsMaxValue(kFloatMissing),
      itsScale(kFloatMissing),
      itsBase(kFloatMissing),
      itsInterpolationMethod(kNoneInterpolation),
      itsPrecision("%.1f")
{
}

// ----------------------------------------------------------------------
/*!
 * Equality comparison
 *
 * \param theParam The object to compare with
 * \return True if the objects are equal
 */
// ----------------------------------------------------------------------

inline bool NFmiParam::operator==(const NFmiParam &theParam) const
{
  return GetIdent() == theParam.GetIdent();
}

inline bool NFmiParam::operator<(const NFmiParam &theParam) const
{
  return GetIdent() < theParam.GetIdent();
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline double NFmiParam::MaxValue(void) const { return itsMaxValue; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline double NFmiParam::MinValue(void) const { return itsMinValue; }
// ----------------------------------------------------------------------
/*!
 * \param newValue Undocumented
 */
// ----------------------------------------------------------------------

inline void NFmiParam::MaxValue(double newValue) { itsMaxValue = newValue; }
// ----------------------------------------------------------------------
/*!
 * \param newValue Undocumented
 */
// ----------------------------------------------------------------------

inline void NFmiParam::MinValue(double newValue) { itsMinValue = newValue; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline const NFmiString &NFmiParam::Precision(void) const { return itsPrecision; }
// ----------------------------------------------------------------------
/*!
 * \param newValue Undocumented
 */
// ----------------------------------------------------------------------

inline void NFmiParam::Scale(float newValue) { itsScale = newValue; }
// ----------------------------------------------------------------------
/*!
 * \param newValue Undocumented
 */
// ----------------------------------------------------------------------

inline void NFmiParam::Base(float newValue) { itsBase = newValue; }
// ----------------------------------------------------------------------
/*!
 * \param newValue Undocumented
 */
// ----------------------------------------------------------------------

inline void NFmiParam::Precision(const NFmiString &newValue) { itsPrecision = newValue; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline bool NFmiParam::IsScaleBase(void)
{
  return !(itsScale == kFloatMissing || itsBase == kFloatMissing);
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline FmiInterpolationMethod NFmiParam::InterpolationMethod(void) const
{
  return itsInterpolationMethod;
}

// ----------------------------------------------------------------------
/*!
 * \param theInterpolationMethod Undocumented
 */
// ----------------------------------------------------------------------

inline void NFmiParam::InterpolationMethod(FmiInterpolationMethod theInterpolationMethod)
{
  itsInterpolationMethod = theInterpolationMethod;
}

// ----------------------------------------------------------------------
/*!
 * Create a new copy of the object
 *
 * \return The new copy of the object
 * \todo Should return an auto_ptr
 */
// ----------------------------------------------------------------------

inline NFmiParam *NFmiParam::Clone(void) const { return new NFmiParam(*this); }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

inline const char *NFmiParam::ClassName(void) const { return "NFmiParam"; }
// ----------------------------------------------------------------------
/*!
 * Output operator for class NFmiParam
 *
 * \param file The output stream to write to
 * \param ob The object to write
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

inline std::ostream &operator<<(std::ostream &file, const NFmiParam &ob) { return ob.Write(file); }
// ----------------------------------------------------------------------
/*!
 * Input operator for class NFmiParam
 *
 * \param file The input stream to read from
 * \param ob The object into which to read the new contents
 * \return The input stream read from
 */
// ----------------------------------------------------------------------

inline std::istream &operator>>(std::istream &file, NFmiParam &ob) { return ob.Read(file); }
