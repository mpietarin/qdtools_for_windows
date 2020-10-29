#pragma once
//**********************************************************
// C++ Class Name : NFmiSmartToolCalculationSection
// ---------------------------------------------------------
//  Author         : pietarin
//  Creation Date  : Thur - Jun 20, 2002
//
// Tämä luokka hoitaa calculationsectionin yksi laskurivi kerrallaan.
// esim.
// T = T + 1
// P = P + 2
//**********************************************************

#include <boost/shared_ptr.hpp>
#include <newbase/NFmiAreaMask.h>
#include <newbase/NFmiDataMatrix.h>

class NFmiPoint;
class NFmiMetTime;
class NFmiSmartToolCalculation;
class NFmiFastQueryInfo;
class NFmiMacroParamValue;

class NFmiSmartToolCalculationSection
{
 public:
  void Calculate(const NFmiCalculationParams &theCalculationParams,
                 NFmiMacroParamValue &theMacroParamValue);
  void Calculate_ver2(const NFmiCalculationParams &theCalculationParams);
  void SetTime(const NFmiMetTime &theTime);
  boost::shared_ptr<NFmiFastQueryInfo> FirstVariableInfo(void);

  NFmiSmartToolCalculationSection(void);
  NFmiSmartToolCalculationSection(const NFmiSmartToolCalculationSection &theOther);
  ~NFmiSmartToolCalculationSection(void);

  void AddCalculations(const boost::shared_ptr<NFmiSmartToolCalculation> &value);
  std::vector<boost::shared_ptr<NFmiSmartToolCalculation> > &GetCalculations(void)
  {
    return itsCalculations;
  }

 private:
  std::vector<boost::shared_ptr<NFmiSmartToolCalculation> > itsCalculations;  // omistaa+tuhoaa
};
