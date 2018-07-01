#ifndef SIGN_DETECC_SIGNDETECTOR_H
#define SIGN_DETECC_SIGNDETECTOR_H
#include "SignPlace.h"
#include <vector>
#include <cv.h>

using std::vector;

/**
 * Contains the detection code
 */

class SignIdentifier {
public:
  SignIdentifier();
  virtual ~SignIdentifier();

  const vector<SignPlace> detect(cv::Mat& input,  const cv::Rect& areaWithSigns);
private:
  /**
   * Seeks out pixel clusters and adds their rects to preFilteredRects
   * Applies rules and methods stated in detect after ###################### break
   * Input matrix will be altered!
   * Won't add rects of the tiny size category
   * @param mat
   * @param preFilteredRects
   * @param addPreEnlargedAswell true If the unenlarged rects should be added to preFilteredRects aswell
   * @param onlyAddSmallCategory true if only signs from the small category should be added
   */
  void findPixelClustersFloodFillMethod(cv::Mat& mat, vector<cv::Rect>& preFilteredRects,
      bool addPreEnlargedAswell, bool onlyAddSmallCategory);

  cv::Point clipCoordinate(const cv::Point& pnt, const cv::Mat& clipMat) const;
  cv::Rect clipRect(const cv::Rect& pnt, const cv::Mat& clipMat) const;
  //vector<cv::Rect>  findPixelClustersContoursMethod(const cv::Scalar& color, cv::Mat grayS, cv::Mat targetDraw);
};

#endif //SIGN_DETECC_SIGNDETECTOR_H
