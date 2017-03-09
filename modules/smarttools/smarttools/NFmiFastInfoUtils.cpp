#include "NFmiFastInfoUtils.h"
#include "NFmiSmartInfo.h"
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiProducerName.h>
#include <newbase/NFmiFileString.h>

namespace NFmiFastInfoUtils
{
bool IsInfoShipTypeData(NFmiFastQueryInfo &theInfo)
{
  if (theInfo.IsGrid() == false)
  {
    FmiProducerName prodId = static_cast<FmiProducerName>(theInfo.Producer()->GetIdent());
    if (prodId == kFmiSHIP || prodId == kFmiBUOY)
      return true;
  }
  return false;
}

// synop-dataa ei tarvitsee asettaa mihink‰‰n, mutta
// mm. luotaus plotin yhteydess‰ pit‰‰ etsi‰ oikea korkeus/level
void SetSoundingDataLevel(const NFmiLevel &theWantedSoundingPressureLevel, NFmiFastQueryInfo &info)
{
  if (theWantedSoundingPressureLevel.LevelType() == kFmiSoundingLevel)
  {  // sounding plotin yhteydess‰ pit‰‰ asettaa level haluttuun korkeuteen
    float levelValue = theWantedSoundingPressureLevel.LevelValue();
    FmiParameterName parName = static_cast<FmiParameterName>(info.Param().GetParamIdent());
    unsigned long parIndex = info.ParamIndex();
    bool subParaUsed = info.IsSubParamUsed();
    if (info.Param(kFmiPressure))
    {
      for (info.ResetLevel(); info.NextLevel();)
        if (info.FloatValue() == levelValue)
          break;
    }
    if (subParaUsed)
      info.Param(parName);  // pakko vet‰‰ t‰m‰ hitaalla tavalla jostain syyst‰
    else
      info.ParamIndex(parIndex);
  }
}

unsigned long GetMaskedCount(boost::shared_ptr<NFmiFastQueryInfo> &theInfo,
                             NFmiMetEditorTypes::Mask theMask,
                             bool fAllowRightClickSelection)
{
  if (theMask == NFmiMetEditorTypes::kFmiDisplayedMask && !fAllowRightClickSelection)
    return 0;  // jos hiiren oikealla ei ole sallittua valita pisteit‰ kartalta, t‰m‰ palauttaa 0:aa

  NFmiSmartInfo *info = dynamic_cast<NFmiSmartInfo *>(theInfo.get());
  if (info)
    return info->LocationMaskedCount(theMask);
  else
    return 0;
}

std::string GetTotalDataFilePath(const boost::shared_ptr<NFmiFastQueryInfo> &theInfo)
{
  NFmiFileString totalFilePath(theInfo->DataFilePattern());
  totalFilePath.FileName(theInfo->DataFileName());
  return std::string(totalFilePath);
}
}
