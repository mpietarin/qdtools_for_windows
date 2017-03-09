// ======================================================================
/*!
 * \file NFmiParamDataModifier.cpp
 * \brief Implementation of class NFmiParamDataModifier
 */
// ======================================================================
/*!
 * \class NFmiParamDataModifier
 *
 * Undocumented
 *
 */
// ======================================================================

#include "NFmiParamDataModifier.h"
#include "NFmiQueryInfo.h"
#include "NFmiDataModifierList.h"
#include <stdexcept>

// ----------------------------------------------------------------------
/*!
 * Destructor
 */
// ----------------------------------------------------------------------

NFmiParamDataModifier::~NFmiParamDataModifier(void)
{
  delete itsParam;
  delete itsLevel;
  delete itsSubList;
}

// ----------------------------------------------------------------------
/*!
 * Constructor
 *
 * \param theParam Undocumented
 * \param theLevel Undocumented
 * \param theJoinOperator Undocumented
 */
// ----------------------------------------------------------------------

NFmiParamDataModifier::NFmiParamDataModifier(NFmiDataIdent* theParam,
                                             NFmiLevel* theLevel,
                                             FmiJoinOperator theJoinOperator)
    : NFmiDataModifier(theJoinOperator),
      itsParam(theParam ? new NFmiDataIdent(*theParam) : 0),
      itsLevel(theLevel ? new NFmiLevel(*theLevel) : 0),
      itsSubList(0)
{
  itsSubList = new NFmiDataModifierList;
}

// ----------------------------------------------------------------------
/*!
 * \param theParam Undocumented
 * \param theLevel Undocumented
 * \return Undocumented
 */
// ----------------------------------------------------------------------

bool NFmiParamDataModifier::Match(const NFmiDataIdent& theParam, const NFmiLevel* theLevel)
{
  // HUOM!!! Tarkistus tehdään ainakin aluksi parId tasolla ja levelID tasolla!!!!!!
  if (*itsParam->GetParam() == *theParam.GetParam())
  {
    if (itsLevel && theLevel && (*itsLevel == *theLevel))
      return true;
    else if (!(itsLevel && theLevel))  // jos molemmat ovat 0-pointtereita myös true
      return true;
  }
  return false;
}

// ----------------------------------------------------------------------
/*!
 * \param theModifier Undocumented
 * \return Undocumented
 *
 * \todo Not implemented yet!
 */
// ----------------------------------------------------------------------

bool NFmiParamDataModifier::AddSubModifier(NFmiDataModifier* /* theModifier */)
{
  throw std::runtime_error("NFmiParamDataModifier::AddSubModifier not implemented yet!");
}

// ----------------------------------------------------------------------
/*!
 * \return Undocumented
 */
// ----------------------------------------------------------------------

NFmiDataModifierList* NFmiParamDataModifier::SubModifiers(void) { return itsSubList; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented, unused
 * \return Undocumented, always false
 */
// ----------------------------------------------------------------------

bool NFmiParamDataModifier::BoolOperation(float /* theValue */) { return false; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented, unused
 * \return Undocumented, always theValue itself
 */
// ----------------------------------------------------------------------

float NFmiParamDataModifier::FloatOperation(float theValue) { return theValue; }
// ----------------------------------------------------------------------
/*!
 * \return Undocumented, always kFloatMissing
 */
// ----------------------------------------------------------------------

float NFmiParamDataModifier::CalculationResult(void) { return kFloatMissing; }
// ----------------------------------------------------------------------
/*!
 * \param theValue Undocumented, unused
 */
// ----------------------------------------------------------------------

void NFmiParamDataModifier::Calculate(float /* theValue */) {}
// ----------------------------------------------------------------------
/*!
 * Undocumented
 */
// ----------------------------------------------------------------------

void NFmiParamDataModifier::Clear(void) {}
// ----------------------------------------------------------------------
/*!
 * \param file The output stream to write to
 * \return The output stream written to
 */
// ----------------------------------------------------------------------

std::ostream& NFmiParamDataModifier::WriteOperand(std::ostream& file) const
{
  file << "<operand type='dataparam'>";
  file << "<id>" << itsParam->GetParam()->GetIdent() << "</id>";
  file << "</operand>";
  return file;
}

// ======================================================================
