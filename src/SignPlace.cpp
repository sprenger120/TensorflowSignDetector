#include "SignPlace.h"
#include "../include/SignPlace.h"


//minimum overlap a given signPlace must have to be considered good enough
#define SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE 0.5f
//maximum area difference of a given signPlace  may have.
//0.5 would allow a area difference between  50 - 150%;
//0.2:  80-120%
#define SIGN_PLACE_MAX_AREA_DIFFERENCE 0.5f


SignPlace::SignPlace(const cv::Point& upperLeft, const cv::Point& lowerRight, const int signId)
    :signPosition(upperLeft, lowerRight), signId(signId)
{
  //coordinate sanity check
  if (upperLeft.y > lowerRight.y || upperLeft.x > lowerRight.x) {
    throw "illegal coordinates";
  }
}
SignPlace::SignPlace(const cv::Rect& r, const int signId) :
signPosition(r), signId(signId)
{
}

SignPlace& SignPlace::operator=(const SignPlace& other) {
  signPosition = other.signPosition;
  signId = other.signId;
}


const bool SignPlace::isOverlappingEnough(const SignPlace& givenSignPl) const
{
  //when given signPlace is so big that it completely encloses this signPlace or in reverse (signPlace encloses
  //given signPlace) we reject
  //Although overlap is required for the klassification step, excess overlap will cause this step to fail

  //small (<1) areaQuotient - given is larger, great (>1) quotient - given is smaller
  //see SIGN_PLACE_MAX_AREA_DIFFERENCE for more information
  const float areaQuotient = (float)  signPosition.area() / (float) givenSignPl.signPosition.area();
  if (areaQuotient < 1.0f-SIGN_PLACE_MAX_AREA_DIFFERENCE || areaQuotient > 1.0f+SIGN_PLACE_MAX_AREA_DIFFERENCE)
  {
    return false;
  }

  cv::Rect unionRect = signPosition & givenSignPl.signPosition;

  //given signplace must cover at least SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE % of this
  //signplaces area

  //union area can't exceed this signPlace's area
  float unionArea = unionRect.area();
  if (unionArea < 0.1) {
    return false;
  }
  float areaQuotient1 = unionArea / (float) signPosition.area();
  return areaQuotient1 > SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE &&
      areaQuotient1 < 1+SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE;
}


void SignPlace::drawOutline(cv::Mat pic,const cv::Scalar& color, bool drawSignID, bool drawInside) const
{
  cv::rectangle(pic,signPosition ,color,2);
  if (drawSignID) {
    if (drawInside) {
      cv::putText(pic, std::to_string(signId), signPosition.tl()+cv::Point(10, 10),
          cv::FONT_HERSHEY_PLAIN, 3, color, 3);
    } else {
      cv::putText(pic, std::to_string(signId), signPosition.br()+cv::Point(10, 10),
          cv::FONT_HERSHEY_PLAIN, 3, color, 3);
    }
  }
}



const cv::Rect& SignPlace::getSignPlace() const
{
  return signPosition;
}

const int SignPlace::getSignId() const
{
  return signId;
}