// ======================================================================
/*!
 * \file NFmiRelativeTimeIntegrationIterator.h
 * \brief Interface of class NFmiRelativeTimeIntegrationIterator
 */
// ======================================================================

#ifndef NFMIRELATIVETIMEINTEGRATIONITERATOR_H
#define NFMIRELATIVETIMEINTEGRATIONITERATOR_H

#include "NFmiDataIterator.h"

//! Undocumented

class _FMI_DLL NFmiRelativeTimeIntegrationIterator : public NFmiDataIterator
{
 public:
  virtual ~NFmiRelativeTimeIntegrationIterator();
  NFmiRelativeTimeIntegrationIterator(
      NFmiQueryInfo* theData,
      int theStepnumber,       // integroitavien aika-askelten lukumäärä
      int theStartShift = 0);  // Integroinnin loppuaika aikaskeleina

  virtual void DoForEach(NFmiDataModifier* theDataModifier);

 protected:
  int itsStartShift;
  int itsStepnumber;

};  // class NFmiRelativeTimeIntegrationIterator

/*!
 * Jouduin tekemään uuden relatiivisen aikaiteraattorin, koska en ymmärrä
 * Persan logiikkaa, enkä saa vanhaa iteraattoria toimimaan kuten haluan.
 * Idea on sama kuin vahasaa iteraattorissa, mutta DoForEach toimii erilailla.
 */

class _FMI_DLL NFmiRelativeTimeIntegrationIterator2 : public NFmiDataIterator
{
 public:
  virtual ~NFmiRelativeTimeIntegrationIterator2();
  NFmiRelativeTimeIntegrationIterator2(NFmiQueryInfo* theData,
                                       int theStepnumber,
                                       int theStartShift = 0);

  virtual void DoForEach(NFmiDataModifier* theDataModifier);

 protected:
  int itsStartShift;  //! Integroinnin alkuaika aikaskeleina
  int itsStepnumber;  //! integroitavien aika-askelten lukumäärä

};  // class NFmiRelativeTimeIntegrationIterator

#endif  // NFMIRELATIVETIMEINTEGRATIONITERATOR_H

// ======================================================================
