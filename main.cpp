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
#include <thread>

#include "TrainingData.h"

using namespace std;
using namespace cv;


int main() {
  TrainingData::TrainingData trainingData;

  bool quick = true;
  int workerThreadCount = 8;
  TrainingData::EvaluationResult finalResult;

  if (quick) {
    vector<TrainingData::EvaluationResult> results;
    size_t workPerThread = trainingData.getTrainingExamplesCount() / workerThreadCount;
    size_t rest = trainingData.getTrainingExamplesCount() % workerThreadCount;

    for (;workerThreadCount>=0;workerThreadCount--) {

    }





  } else {
    trainingData.evaluateSignDetector(quick, finalResult);
  }




  cv::destroyAllWindows();
  return 0;
}
