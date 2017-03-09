// ======================================================================
/*!
 * \file
 * \brief Interface of namespace NFmiSvgTools
 */
// ======================================================================

#ifndef NFMISVGTOOLS_H
#define NFMISVGTOOLS_H

class NFmiArea;
class NFmiPoint;
class NFmiSvgPath;

namespace NFmiSvgTools
{
bool IsInside(const NFmiSvgPath& thePath, const NFmiPoint& thePoint);
double Distance(const NFmiSvgPath& thePath, const NFmiPoint& thePoint);
double GeoDistance(const NFmiSvgPath& thePath, const NFmiPoint& thePoint);
void BoundingBox(
    const NFmiSvgPath& thePath, double& theXmin, double& theYmin, double& theXmax, double& theYmax);
void LatLonToWorldXY(NFmiSvgPath& thePath, const NFmiArea& theArea);

}  // namespace NFmiSvgTools

#endif  // NFMISVGTOOLS_H

// ======================================================================
