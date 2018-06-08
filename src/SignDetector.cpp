#include "SignDetector.h"

SignDetector::SignDetector() { }
SignDetector::~SignDetector()
{
}


const vector<SignPlace> SignDetector::detect(const cv::Mat inputImage, const cv::Rect& areaWithSigns) const
{
  //cropp of area with no sign, leaving a region of interest
  cv::Mat inputROI(inputImage, areaWithSigns);

  //all coordinates from inputROI are no longer aligned with the original image
  //we have to add some offset to the coordinates to get back to the original ones
  //the offset is exacly the upperLeft starting point of our roi - how convenient
  cv::Point roiOffset(areaWithSigns.x, areaWithSigns.y);




  return vector<SignPlace>();
}

