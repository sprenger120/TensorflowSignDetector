#include <opencv2/core/types.hpp>
#include "SignPlace.h"
#include "../include/SignPlace.h"


//minimum overlap a given signPlace must have to be considered good enough
#define SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE 0.5f
//maximum area difference of a given signPlace  may have.
//0.5 would allow a area difference between  50 - 150%;
//0.2:  80-120%
#define SIGN_PLACE_MAX_AREA_DIFFERENCE 0.5f

const cv::Point_<int>& SignPlace::getUpperLeft() const
{
  return upperLeft;
}
const cv::Point_<int>& SignPlace::getLowerRight() const
{
  return lowerRight;
}
const int SignPlace::getSignId() const
{
  return signId;
}
SignPlace::SignPlace(const cv::Point& upperLeft, const cv::Point& lowerRight, const int signId)
    :upperLeft(upperLeft), lowerRight(lowerRight), signId(signId)
{
  //coordinate sanity check
  if (upperLeft.y > lowerRight.y || upperLeft.x > lowerRight.x) {
    throw "illegal coordinates";
  }

  //precalculate ara
  area = getArea();
}


const float SignPlace::getArea() const
{
 return (lowerRight.x - upperLeft.x) * (lowerRight.y - upperLeft.y);
}

const bool SignPlace::isOverlappingEnough(const SignPlace& givenSignPl) const
{
  //given signplace must cover at least SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE % of this
  //signplaces area

  // ---> x
  // |
  // |
  // \/ y

  //reject - union is empty
  if (upperLeft.y > givenSignPl.lowerRight.y || //given is above
      lowerRight.y < givenSignPl.upperLeft.y || //given is below
      upperLeft.x > givenSignPl.lowerRight.x || //given is left
      lowerRight.x < givenSignPl.upperLeft.x //given is right
      )
  {
    return false;
  }

  //reject - targetSign area too big or too small
  //small (<1) areaQuotient - given is larger, great (>1) quotient - given is smaller
  //see SIGN_PLACE_MAX_AREA_DIFFERENCE for more information
  const float areaQuotient = area / givenSignPl.area;
  if (areaQuotient < 1.0f-SIGN_PLACE_MAX_AREA_DIFFERENCE || areaQuotient > 1.0f+SIGN_PLACE_MAX_AREA_DIFFERENCE)
  {
    return false;
  }

  cv::Point unionUpperLeft;
  cv::Point unionLowerRight;

  //calculate union of both signplaces

  //top line
  unionUpperLeft.y = std::max(upperLeft.y, givenSignPl.upperLeft.y);

  //lower Line
  unionLowerRight.y = std::min(lowerRight.y, givenSignPl.lowerRight.y);

  //left line
  unionUpperLeft.x = std::min(upperLeft.x, givenSignPl.upperLeft.x);

  //right line
  unionLowerRight.x = std::min(lowerRight.x, givenSignPl.lowerRight.x);

  SignPlace unionArea(unionUpperLeft, unionLowerRight, 0);
  //union area can't exceed this signPlace's area
  return unionArea.area / area > SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE;
}