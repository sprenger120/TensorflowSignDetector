#ifndef SIGN_DETECC_SIGNDETECTOR_H
#define SIGN_DETECC_SIGNDETECTOR_H
#include "SignPlace.h"
#include <vector>
#include <cv.h>

using std::vector;

/**
 * Contains the detection and klassification code
 */

class SignDetector {
public:
  SignDetector();
  virtual ~SignDetector();

  const vector<SignPlace> detect(const cv::Mat input,  const cv::Rect& areaWithSigns);
private:
  void morphThinning(cv::Mat &img) const;

  void findAndDrawContours(const cv::Scalar& color, cv::Mat grayS, cv::Mat targetDraw);
};

#endif //SIGN_DETECC_SIGNDETECTOR_H
