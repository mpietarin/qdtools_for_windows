// ======================================================================
/*!
 * \file NFmiInfoModifier.h
 * \brief Interface of class NFmiInfoModifier
 */
// ======================================================================

#ifndef NFMINFOMODIFIER_H
#define NFMINFOMODIFIER_H

#include "NFmiParamDataModifier.h"

class NFmiQueryInfo;

//! Undocumented
class _FMI_DLL NFmiInfoModifier : public NFmiParamDataModifier
{
 public:
  virtual ~NFmiInfoModifier();
  NFmiInfoModifier(NFmiDataIdent* theParam,
                   NFmiLevel* theLevel,
                   NFmiQueryInfo* theData = 0,
                   FmiJoinOperator theJoinOperator = kFmiAdd);
  NFmiQueryInfo* Info(void);
  virtual double FloatValue(void);
  virtual double PeekValue(long dx, long dy, long dt);

 protected:
  NFmiQueryInfo* itsData;

 private:
  NFmiInfoModifier(const NFmiInfoModifier& theMod);
  NFmiInfoModifier& operator=(const NFmiInfoModifier& theMod);

};  // class NFmiInfoModifier

#endif  // NFMINFOMODIFIER_H

// ======================================================================
