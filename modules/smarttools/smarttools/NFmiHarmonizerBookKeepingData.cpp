
#include "NFmiHarmonizerBookKeepingData.h"

NFmiHarmonizerBookKeepingData::NFmiHarmonizerBookKeepingData(void)
    : itsHarmonizerTimesSet(), fHarmonizeAllTimes(false), itsHarmonizerParams()
{
}

NFmiHarmonizerBookKeepingData::NFmiHarmonizerBookKeepingData(
    const std::set<NFmiMetTime> &theHarmonizerTimesSet,
    bool /* foobar */,
    const NFmiParamBag &theHarmonizerParams)
    : itsHarmonizerTimesSet(theHarmonizerTimesSet),
      fHarmonizeAllTimes(false),
      itsHarmonizerParams(theHarmonizerParams)
{
}

NFmiHarmonizerBookKeepingData::~NFmiHarmonizerBookKeepingData(void)
{
}

void NFmiHarmonizerBookKeepingData::InsertTime(const NFmiMetTime &theTime)
{
  itsHarmonizerTimesSet.insert(theTime);
}
