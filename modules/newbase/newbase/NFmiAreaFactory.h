// ======================================================================
/*!
 * \file
 * \brief Interface of namespace NFmiAreaFactory
 *
 * HUOM!!! NFmiAreaFactory ei ole käytössä MSVC60:lla koska
 * NFmiStringTools-funktiot eivät ole käytössä.
 */
// ======================================================================

#ifndef NMIFAREAFACTORY_H
#define NMIFAREAFACTORY_H

#include "NFmiPoint.h"
#include <string>

#include <boost/shared_ptr.hpp>

class NFmiArea;

namespace NFmiAreaFactory
{
typedef boost::shared_ptr<NFmiArea> return_type;

bool DoPossiblePacificFix(NFmiPoint &bottomLeftLatlon,
                          NFmiPoint &topRightLatlon,
                          bool &pacificView);

return_type Create(const std::string &theProjection);

return_type CreateProj(const std::string &projString,
                       const NFmiPoint &bottomLeftLatLon,
                       const NFmiPoint &topRightLatLon,
                       const NFmiPoint &topLeftXY = NFmiPoint(0.0f, 0.0f),
                       const NFmiPoint &bottomRightXY = NFmiPoint(1.0f, 1.0f));

}  // namespace NFmiAreaFactory

#endif  // NMIFAREAFACTORY_H

// ======================================================================
