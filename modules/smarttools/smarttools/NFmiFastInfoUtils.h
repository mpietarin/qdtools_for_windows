#pragma once

#include "NFmiMetEditorTypes.h"
#include <boost/shared_ptr.hpp>

class NFmiFastQueryInfo;
class NFmiLevel;

namespace NFmiFastInfoUtils
{
bool IsInfoShipTypeData(NFmiFastQueryInfo &theInfo);
void SetSoundingDataLevel(const NFmiLevel &theWantedSoundingPressureLevel, NFmiFastQueryInfo &info);
unsigned long GetMaskedCount(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             NFmiMetEditorTypes::Mask theMask,
                             bool fAllowRightClickSelection);
std::string GetTotalDataFilePath(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo);
}
