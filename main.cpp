#include <iostream>
#include <opencv2/opencv.hpp>
//#include <opencv2/videoio.hpp>
#include <fstream>
#include <ctime>
#include <string>
#include <stdio.h>
#include <ftw.h>
#include <fnmatch.h>
#include <algorithm>
#include <string>

#include "TrainingData.h"

using namespace std;
using namespace cv;

int main() {
  TrainingData::TrainingData trainingData;

  trainingData.evaluateSignDetector(true);

  cv::destroyAllWindows();
  return 0;
}
