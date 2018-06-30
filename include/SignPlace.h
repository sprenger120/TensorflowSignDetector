#ifndef SIGN_DETECC_SIGNPLACE_H
#define SIGN_DETECC_SIGNPLACE_H
#include <opencv2/opencv.hpp>

/**
 * Contains the region where a sign is placed in a training picture
 * and some helper functionality that aids in analyzing SignDetectors performance.
 */
class SignPlaceTest;

class SignPlace {
public:
  SignPlace(const cv::Point& upperLeft, const cv::Point& lowerRight, const int signId);
  const cv::Point& getUpperLeft() const;
  const cv::Point& getLowerRight() const;
  const int getSignId() const;
  SignPlace& operator=(const SignPlace&);

  /**
   * Evaluates if the given SignPlace is overlapping enough
   * (Set by SIGN_PLACE_MIN_OVERLAPPING_PERCENTAGE) with this SignPlace.
   * Primarily used in detected sign evaluation.
   * Doesn't compare signIds
   * @return true if there is enough overlap
   */
  const bool isOverlappingEnough(const SignPlace&) const;

  /**
   * Draws outline of rect defined by this class
   * @param pic
   * @param color
   */
  void drawOutline(cv::Mat pic, const cv::Scalar& color) const;
private:
  const float getArea() const;

  /*
   *
   upperLeft, left line
   \/
   ****** <--- top line
   *    *
   *    *
   ****** <--- lower line
   *    ^
   *    lowerRight, right line
   */
  cv::Point upperLeft;
  cv::Point lowerRight;
  int signId;
  float area;

  friend SignPlaceTest;
};


#endif //SIGN_DETECC_SIGNPLACE_H
