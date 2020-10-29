// ======================================================================
/*!
 * \file NFmiCalculatedAreaMask.h
 * \brief Interface of class NFmiCalculatedAreaMask and its derivatives
 *
 * \todo Separate individual classes into individual files
 */
// ======================================================================

#pragma once

#include "NFmiAreaMaskImpl.h"
#include "NFmiMetTime.h"

class NFmiFastQueryInfo;

//! Tarkoitus on tehdä tästä mm. auringonnousukulman ja lat/lon arvojen maskien esiluokka.

class NFmiCalculatedAreaMask : public NFmiAreaMaskImpl
{
 public:
  ~NFmiCalculatedAreaMask(void);
  NFmiCalculatedAreaMask(void);
  NFmiCalculatedAreaMask(const NFmiDataIdent& theParam,
                         const NFmiCalculationCondition& theOperation);
  NFmiCalculatedAreaMask(const NFmiCalculatedAreaMask& theMask);
  NFmiCalculatedAreaMask& operator=(const NFmiCalculatedAreaMask& theMask);
  NFmiAreaMask* Clone(void) const override;

  bool IsWantedParam(const NFmiDataIdent& theParam, const NFmiLevel* theLevel = 0) const override;
  const NFmiDataIdent* DataIdent(void) const override;
  const NFmiParam* Param(void) const override;

 protected:
  NFmiDataIdent* itsDataIdent;

};  // class NFmiCalculatedAreaMask

//! Undocumented
class NFmiLatLonAreaMask : public NFmiCalculatedAreaMask
{
 public:
  ~NFmiLatLonAreaMask(void);
  NFmiLatLonAreaMask(void);
  NFmiLatLonAreaMask(const NFmiDataIdent& theParam, const NFmiCalculationCondition& theOperation);
  NFmiLatLonAreaMask(const NFmiLatLonAreaMask& theMask);
  NFmiAreaMask* Clone(void) const override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;

 private:
};  // class NFmiLatLonAreaMask

//! Undocumented
class NFmiElevationAngleAreaMask : public NFmiLatLonAreaMask
{
 public:
  ~NFmiElevationAngleAreaMask(void);
  NFmiElevationAngleAreaMask(void);
  NFmiElevationAngleAreaMask(const NFmiElevationAngleAreaMask& theMask);
  NFmiElevationAngleAreaMask(const NFmiDataIdent& theParam,
                             const NFmiCalculationCondition& theOperation);
  bool Time(const NFmiMetTime& theTime) override;
  NFmiAreaMask* Clone(void) const override;
  double Value(const NFmiCalculationParams& theCalculationParams,
               bool fUseTimeInterpolationAlways) override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;
  NFmiMetTime itsTime;  // tähän aikaaan lasketaan nousukulma

};  // class NFmiElevationAngleAreaMask

//! Undocumented
class NFmiJulianDayAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiJulianDayAreaMask(void);
  NFmiJulianDayAreaMask(void);
  NFmiJulianDayAreaMask(const NFmiJulianDayAreaMask& theMask);
  NFmiJulianDayAreaMask(const NFmiDataIdent& theParam,
                        const NFmiCalculationCondition& theOperation);
  bool Time(const NFmiMetTime& theTime) override;
  NFmiAreaMask* Clone(void) const override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;
  double itsJulianDay;  // tähän lasketaan Time-metodissa haluttu julian day

};  // class NFmiJulianDayAreaMask

//! Undocumented
class NFmiLocalHourAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiLocalHourAreaMask(void);
  NFmiLocalHourAreaMask(void);
  NFmiLocalHourAreaMask(const NFmiLocalHourAreaMask& theMask);
  NFmiLocalHourAreaMask(const NFmiDataIdent& theParam,
                        const NFmiCalculationCondition& theOperation);
  NFmiAreaMask* Clone(void) const override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;

};  // class NFmiLocalHourAreaMask

//! Undocumented
class NFmiUtcHourAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiUtcHourAreaMask(void);
  NFmiUtcHourAreaMask(void);
  NFmiUtcHourAreaMask(const NFmiUtcHourAreaMask& theMask);
  NFmiUtcHourAreaMask(const NFmiDataIdent& theParam, const NFmiCalculationCondition& theOperation);
  NFmiAreaMask* Clone(void) const override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;

};  // class NFmiUtcHourAreaMask

// class NFmiMinuteAreaMask
class NFmiMinuteAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiMinuteAreaMask(void);
  NFmiMinuteAreaMask(void);
  NFmiMinuteAreaMask(const NFmiMinuteAreaMask& theMask);
  NFmiMinuteAreaMask(const NFmiDataIdent& theParam, const NFmiCalculationCondition& theOperation);
  NFmiAreaMask* Clone(void) const override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;
};

//! Undocumented
class NFmiForecastHourAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiForecastHourAreaMask(void);
  NFmiForecastHourAreaMask(const boost::shared_ptr<NFmiFastQueryInfo>& theInfo);
  NFmiForecastHourAreaMask(const NFmiForecastHourAreaMask& theMask);
  NFmiForecastHourAreaMask& operator=(const NFmiForecastHourAreaMask& theMask);
  NFmiForecastHourAreaMask(const boost::shared_ptr<NFmiFastQueryInfo>& theInfo,
                           const NFmiDataIdent& theParam,
                           const NFmiCalculationCondition& theOperation);
  NFmiAreaMask* Clone(void) const override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;

 private:
  boost::shared_ptr<NFmiFastQueryInfo> itsInfo;  // kysyy vain editoitavan datan ajan

};  // class NFmiForecastHourAreaMask

//! Luokka palauttaa (CalcValueFromLocation) datan timestepin tunneissa.
class NFmiTimeStepAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiTimeStepAreaMask(void);
  NFmiTimeStepAreaMask(const boost::shared_ptr<NFmiFastQueryInfo>& theInfo);
  NFmiTimeStepAreaMask(const NFmiTimeStepAreaMask& theMask);
  NFmiTimeStepAreaMask& operator=(const NFmiTimeStepAreaMask& theMask);
  NFmiTimeStepAreaMask(const boost::shared_ptr<NFmiFastQueryInfo>& theInfo,
                       const NFmiDataIdent& theParam,
                       const NFmiCalculationCondition& theOperation);
  NFmiAreaMask* Clone(void) const override;
  double Value(const NFmiCalculationParams& theCalculationParams,
               bool fUseTimeInterpolationAlways) override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;  // tee tämä lapsiin!!!
  const NFmiString MakeSubMaskString(void) const override;

 private:
  boost::shared_ptr<NFmiFastQueryInfo>
      itsInfo;  // kysyy vain editoitavan datan currentin ajan aika-askeleen tunneissa

};  // class NFmiForecastHourAreaMask

//! Luokka palauttaa (CalcValueFromLocation) datan hilan x tai y suuntaisen koon metreissä.
class NFmiGridSizeAreaMask : public NFmiElevationAngleAreaMask
{
 public:
  ~NFmiGridSizeAreaMask(void);
  NFmiGridSizeAreaMask(const boost::shared_ptr<NFmiFastQueryInfo>& theInfo, bool calcXValue);
  NFmiGridSizeAreaMask(const boost::shared_ptr<NFmiFastQueryInfo>& theInfo,
                       const NFmiDataIdent& theParam,
                       const NFmiCalculationCondition& theOperation,
                       bool calcXValue);
  NFmiGridSizeAreaMask(const NFmiGridSizeAreaMask& theMask);
  NFmiAreaMask* Clone(void) const override;
  double Value(const NFmiCalculationParams& theCalculationParams,
               bool fUseTimeInterpolationAlways) override;

 protected:
  double CalcValueFromLocation(const NFmiPoint& theLatLon) const override;
  const NFmiString MakeSubMaskString(void) const override;

 private:
  boost::shared_ptr<NFmiFastQueryInfo>
      itsInfo;       // kysyy vain editoitavan datan currentin ajan aika-askeleen tunneissa
  bool fCalcXValue;  // siis jos true, laskee etäisyyden x-suunnassa, muuten y-suunnassa

  NFmiGridSizeAreaMask& operator=(const NFmiGridSizeAreaMask& theMask);
};  // class NFmiGridSizeAreaMask

// ======================================================================
