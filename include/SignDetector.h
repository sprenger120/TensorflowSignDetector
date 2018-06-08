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

  const vector<SignPlace> detect(const cv::Mat input,  const cv::Rect& areaWithSigns) const;
};

#endif //SIGN_DETECC_SIGNDETECTOR_H
