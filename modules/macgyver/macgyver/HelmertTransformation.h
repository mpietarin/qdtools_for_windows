/**
 *
 *  @ref 3-dimensional Helmert transformation for near coordinate systems
 *
 */

#pragma once

#include <string>
#include <boost/array.hpp>
#include "ReferenceEllipsoid.h"

namespace Fmi
{
/**
 *  @ref 3-dimensional Helmert transformation for near coordinate systems
 */
class HelmertTransformation
{
 public:
  enum FmiSphereConvScalingType
  {
    FMI_SPHERE_NO_SCALING,
    FMI_SPHERE_PRESERVE_EAST_WEST_SCALE,
    FMI_SPHERE_PRESERVE_SOUTH_NORTH_SCALE
  };

 public:
  /**
   *  @brief Default constructor: initializes as identity transformation
   */
  HelmertTransformation();

  boost::array<double, 3> operator()(const boost::array<double, 3>& x) const;

  /**
   *  @brief Creates Helmert transformation from FMI sphere to provided
   *         reference ellipsoid
   *
   *  It is assumed that sphere touches reference ellipsoid at provided
   *  geodetic coordinates
   *
   *  @param r the radius of sphere
   *  @param lat the latitude of touching point
   *  @param lon the longintude of touching point
   *  @param ref the reference ellipsoid to use
   *  @param scaling_type specifies scaling type
   */
  void set_fmi_sphere_to_reference_ellipsoid_conv(
      double r,
      double lat,
      double lon,
      const ReferenceEllipsoid& ref,
      enum FmiSphereConvScalingType scaling_type = FMI_SPHERE_NO_SCALING);

  /**
   *  @brief Creates Helmert transformation to FMI sphere from provided
   *         reference ellipsoid
   *
   *  It is assumed that sphere touches reference ellipsoid at provided
   *  geodetic coordinates
   *
   *  @param r the radius of sphere
   *  @param lat the latitude of touching point
   *  @param lon the longintude of touching point
   *  @param ref the reference ellipsoid to use
   *  @param scaling_type specifies scaling type
   */
  void set_reference_ellipsoid_to_fmi_sphere_conv(
      double r,
      double lat,
      double lon,
      const ReferenceEllipsoid& ref,
      enum FmiSphereConvScalingType scaling_type = FMI_SPHERE_NO_SCALING);

 public:
  double m;
  double ex;
  double ey;
  double ez;
  double tx;
  double ty;
  double tz;
};

/**
 *  @brief Generates string with +towgs84 parameter for conversion to specified FMI sphere
 *         to WGS84
 *
 *  It is assumed that sphere touches reference ellipsoid at provided
 *  geodetic coordinates
 *
 *  @param r the radius of sphere
 *  @param lat the latitude of touching point
 *  @param lon the longintude of touching point
 */
std::string get_fmi_sphere_towgs84_proj4_string(
    double r,
    double lat,
    double lon,
    enum Fmi::HelmertTransformation::FmiSphereConvScalingType scaling_type =
        Fmi::HelmertTransformation::FMI_SPHERE_NO_SCALING);
}
