#pragma once

#include "boost/shared_ptr.hpp"
#include <newbase/NFmiAreaMask.h>

class NFmiAreaMaskInfo;

// Simple-condition koostuu 2-3 osiosta, jotka laitetaan n�ihin Part-olioihin.
// Part koostuu joko yhdest� maskista TAI kahdesta maskista ja niihin liittyv�st�
// laskuoperaatioista. Esim. pelkk� yksi maski (T_ec) tai kaksi maskia ja lasku kuten (T_ec - Td_ec)
class NFmiSimpleConditionPartInfo
{
  boost::shared_ptr<NFmiAreaMaskInfo> itsMask1;
  NFmiAreaMask::CalculationOperator itsCalculationOperator = NFmiAreaMask::NotOperation;
  boost::shared_ptr<NFmiAreaMaskInfo> itsMask2;

 public:
  NFmiSimpleConditionPartInfo(boost::shared_ptr<NFmiAreaMaskInfo> &mask1,
                              NFmiAreaMask::CalculationOperator calculationOperator,
                              boost::shared_ptr<NFmiAreaMaskInfo> &mask2)
      : itsMask1(mask1), itsCalculationOperator(calculationOperator), itsMask2(mask2)
  {
  }

  boost::shared_ptr<NFmiAreaMaskInfo> Mask1() const { return itsMask1; }
  NFmiAreaMask::CalculationOperator CalculationOperator() const { return itsCalculationOperator; }
  boost::shared_ptr<NFmiAreaMaskInfo> Mask2() const { return itsMask2; }
};

// Single-condition koostuu 2-3 osiosta (Part), joissa voi olla 1-2 maskia ja mahdollinen
// laskuoperaatio. Normaali tapauksessa on siis kaksi osiota ehdossa esim. T_ec > T_hir TAI laskujen
// kera T_ec - Td_ec > T_hir - Td_hir TAI range tapaus, miss� on kolme osiota kuten -5 < T_ec < 0
// (l�mp�tila -5:n ja 0 v�lill�) tai Td_ec - 2 < T_ec - 1 < T_hir + 1
class NFmiSingleConditionInfo
{
 public:
  NFmiSingleConditionInfo(void);
  ~NFmiSingleConditionInfo(void);
  NFmiSingleConditionInfo(const boost::shared_ptr<NFmiSimpleConditionPartInfo> &part1,
                          FmiMaskOperation conditionOperand1,
                          const boost::shared_ptr<NFmiSimpleConditionPartInfo> &part2,
                          FmiMaskOperation conditionOperand2,
                          const boost::shared_ptr<NFmiSimpleConditionPartInfo> &part3);

  boost::shared_ptr<NFmiSimpleConditionPartInfo> Part1() { return itsPart1; }
  FmiMaskOperation ConditionOperand1() const { return itsConditionOperand1; }
  boost::shared_ptr<NFmiSimpleConditionPartInfo> Part2() { return itsPart2; }
  FmiMaskOperation ConditionOperand2() const { return itsConditionOperand2; }
  boost::shared_ptr<NFmiSimpleConditionPartInfo> Part3() { return itsPart3; }

 protected:
  // part1 and part2 are always present, because they form basic simple condition:
  // part1 condition part2 e.g. T_ec > 0 where T_ec would be NFmiInfoAreaMask object and 0 would be
  // NFmiCalculationConstantValue object.
  boost::shared_ptr<NFmiSimpleConditionPartInfo> itsPart1;
  // Condition between part1 and part2, always present
  FmiMaskOperation itsConditionOperand1;
  boost::shared_ptr<NFmiSimpleConditionPartInfo> itsPart2;
  // Condition between part2 and part3, present only if there is part3 and we are dealing with range
  // case. Range conditions could be inside (like -5 < T_ec < 0 meaning between values -5 and 0) or
  // outside (-5 > T_ec > 0 meaning under -5 and over 0)
  FmiMaskOperation itsConditionOperand2;
  // mask3 is only present if there is range case.
  boost::shared_ptr<NFmiSimpleConditionPartInfo> itsPart3;
};

// Simple-condition koostuu yhdest� tai kahdesta single-conditionista. ne yhdistet��n
// tarvittaessa loogisella operaattorilla (and/or/xor).
// Esim. Ec l�mp�tila on positiivinen ja ollaan maalla: "T_ec > 0 and topo > 0"
class NFmiSimpleConditionInfo
{
 public:
  NFmiSimpleConditionInfo(void);
  ~NFmiSimpleConditionInfo(void);
  NFmiSimpleConditionInfo(const boost::shared_ptr<NFmiSingleConditionInfo> &condition1,
                          NFmiAreaMask::BinaryOperator conditionOperator,
                          const boost::shared_ptr<NFmiSingleConditionInfo> &condition2);

  boost::shared_ptr<NFmiSingleConditionInfo> Condition1() { return itsCondition1; }
  NFmiAreaMask::BinaryOperator ConditionOperator() const { return itsConditionOperator; }
  boost::shared_ptr<NFmiSingleConditionInfo> Condition2() { return itsCondition2; }

 protected:
  // part1 and part2 are always present, because they form basic simple condition:
  // part1 condition part2 e.g. T_ec > 0 where T_ec would be NFmiInfoAreaMask object and 0 would be
  // NFmiCalculationConstantValue object.
  boost::shared_ptr<NFmiSingleConditionInfo> itsCondition1;
  // Condition between part1 and part2, always present
  NFmiAreaMask::BinaryOperator itsConditionOperator;
  boost::shared_ptr<NFmiSingleConditionInfo> itsCondition2;
};
