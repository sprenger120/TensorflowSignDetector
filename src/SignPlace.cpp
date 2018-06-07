#include <opencv2/core/types.hpp>
#include "SignPlace.h"
#include "../include/SignPlace.h"


#define SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE 0.5

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
    :upperLeft(upperLeft), lowerRight(lowerRight), signId(signId) { }
