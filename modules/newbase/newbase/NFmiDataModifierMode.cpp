// ======================================================================
/*!
 * \file NFmiDataModifierMode.cpp
 * \brief Implementation of class NFmiDataModifierMode
 */
// ======================================================================
/*!
 * \class NFmiDataModifierMode
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiDataModifierMode.h"
#include <algorithm>

NFmiDataModifierMode::NFmiDataModifierMode(void) : NFmiDataModifier(), itsDataVector() {}
NFmiDataModifierMode::~NFmiDataModifierMode(void) {}
NFmiDataModifierMode::NFmiDataModifierMode(const NFmiDataModifierMode& theOther)
    : NFmiDataModifier(theOther), itsDataVector(theOther.itsDataVector)
{
}

NFmiDataModifier* NFmiDataModifierMode::Clone(void) const
{
  return new NFmiDataModifierMode(*this);
}

void NFmiDataModifierMode::Calculate(float theValue)
{
  if (theValue != kFloatMissing)
  {
    checkedVector<Data>::iterator pos =
        std::find(itsDataVector.begin(), itsDataVector.end(), Data(theValue));
    if (pos != itsDataVector.end())
      (*pos).Increase();
    else
      itsDataVector.push_back(Data(theValue));
  }
}

void NFmiDataModifierMode::Clear(void) { itsDataVector.clear(); }
float NFmiDataModifierMode::CalculationResult(void)
{
  checkedVector<Data>::iterator pos = std::max_element(itsDataVector.begin(), itsDataVector.end());
  if (pos != itsDataVector.end()) return (*pos).Value();
  return kFloatMissing;
}
