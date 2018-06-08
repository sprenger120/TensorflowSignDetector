#ifndef SIGN_DETECC_TRAININGDATA_H
#define SIGN_DETECC_TRAININGDATA_H
#include <string>
#include <vector>
#include "SignPlace.h"
#include "Types.h"

using std::string;
using std::vector;


/**
 * Contains all training date information (such as what signs are where in which training example)
 * Does analysis on some training data aspects
 */

namespace TrainingData {
struct SignCount {
  //this could be a simple int typedef but for some sleeker
  //code we have to make sure that a new instance of this is always 0 at the
  //start
  int cnt = 0;
};
typedef vector<SignCount> SignOccuranceArray;
//static int __fileCallback(const char* fpath, const struct stat* sb, int typeflag);
//static vector<string> _trainingDataFileList;


class TrainingData {
public:
  TrainingData();
  virtual ~TrainingData();

  /**
   * Evaluates SignDetectors performance across all training data sets and
   * outputs a detection statistic.
   * SignDetector will be constructed once on start
   *
   * @param quick if set to false there will be a frame by frame slideshow showing the
   * sign detectors performance, if false all training data will be checked quickly
   */
  void evaluateSignDetector(const bool quick) const;

  /**
   * Returns the area that will contain signs across all loaded training data
   * @return
   */
  const cv::Rect& getAreaWithSigns() const;
private:
  struct trainingDataInfo {
    //header: img; x_start; y_start; x_end; y_end; id
    string filename;

    //borders around signs
    vector<SignPlace> signs;
  };

  /**
   * Searches for all *.png files in the TRAINING_DATA_TRAINING_PICTURES_PATH
   * directory. Later used to sort out training data without picture
   * @return
   */
 // void gatherTrainingDataFiles() const;

  /**
   * Parses the TRAINING_DATA_DATABASE_FILE_PATH
   * @return filled out trainingDataInfo instances
   */
  vector<trainingDataInfo> loadTrainingData() const;
  /**
   * Counts how many times a sign is in the _trainingData database
   * @return
   */
  SignOccuranceArray countSignOccurances() const;

  /**
   * Assuming all training pictures have the same dimensions, this will search for
   * the area in which no sign is situated.
   * @return
   */
  cv::Rect determineAreaWithSigns() const;

  vector<trainingDataInfo> _trainingData;
  SignOccuranceArray _perSignOccurance;  //index is signID
  cv::Rect _areaWithSigns;
};
}
#endif //SIGN_DETECC_TRAININGDATA_H
