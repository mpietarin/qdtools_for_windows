// ======================================================================
/*!
 * \file
 * \brief Interface of namespace NFmiAreaTools
 */
// ======================================================================

#ifndef NFMIAREATOOLS_H
#define NFMIAREATOOLS_H

class NFmiArea;

namespace NFmiAreaTools
{
void LatLonBoundingBox(const NFmiArea& theArea,
                       double& theMinLon,
                       double& theMinLat,
                       double& theMaxLon,
                       double& theMaxLat);
}

#endif  // NFMIAREATOOLS_H

// ======================================================================
