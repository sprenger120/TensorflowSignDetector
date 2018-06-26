#include "TrainingData.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ftw.h>
#include <fnmatch.h>
#include <opencv2/opencv.hpp>
#include "SignDetector.h"

using std::cout;
using std::getline;
using std::stringstream;

#define TRAINING_DATA_DATABASE_FILE_PATH "training_data/gt_train.txt"
#define TRAINING_DATA_TRAINING_PICTURES_PATH "training_data/png/"

namespace TrainingData {
TrainingData::TrainingData()
{
 // gatherTrainingDataFiles();
  _trainingData = loadTrainingData();
  _perSignOccurance = countSignOccurances();
  _areaWithSigns = determineAreaWithSigns();

  cout << "========== Satistics\n";
  cout << "Loaded " << _trainingData.size() << " training data elements\n";
  cout << "Cropped area with signs to " << _areaWithSigns << "\n";

  cout << "=== Sign occurances in training data\n";


  //print out 10 most occuring signs
  SignOccuranceArray signOccCopy = _perSignOccurance;
  for (char i = 0; i<20; ++i) {
    size_t biggestID = 0;

    for (size_t indx = 1; indx<signOccCopy.size(); ++indx) {
      if (signOccCopy[biggestID].cnt<signOccCopy[indx].cnt) {
        biggestID = indx;
      }
    }

    cout << "#" << biggestID << " spotted " << signOccCopy[biggestID].cnt << " times\n";
    signOccCopy[biggestID].cnt = 0;
  }

}

TrainingData::~TrainingData()
{
}
vector<TrainingData::trainingDataInfo> TrainingData::loadTrainingData() const
{
  vector<trainingDataInfo> trainingData;

  //read csv file
  std::ifstream file(TRAINING_DATA_DATABASE_FILE_PATH);

  if (!file) {
    throw "Unable to open gt_train file\n";
  }

  //read whole file into stringstream
  std::stringstream fileBuffer;
  string currentLine;
  fileBuffer << file.rdbuf();
  file.close();
  //discard first line -> header
  std::getline(fileBuffer, currentLine, '\n');

  //loops until no more lines to delimit
  string lineBuffer;
  while (std::getline(fileBuffer, currentLine, '\n')) {
    stringstream ss_currLine(currentLine);
    std::getline(ss_currLine, lineBuffer, ';');

    //search if a trainingDataInfo is already created
    //have to do this because there might me multiple entries for the same file
    //signalling that there are multiple signs in the picture
    //lineBuffer is affectedFile currently
    trainingDataInfo* ptrAffectedFile = nullptr;
    for (trainingDataInfo& trData : trainingData) {
      if (trData.filename==lineBuffer) {
        ptrAffectedFile = &trData;
        break;
      }
    }
    if (ptrAffectedFile==nullptr) {
      //create new entry, noone found
      trainingDataInfo trData;
      trData.filename = lineBuffer;
      trainingData.push_back(trData);
      //trainingData vector might create a copy so we get the ptr out of it
      ptrAffectedFile = &trainingData[trainingData.size()-1];
    }

    cv::Point upperLeft;
    cv::Point lowerRight;


    //read x_start
    std::getline(ss_currLine, lineBuffer, ';');
    upperLeft.x = stoi(lineBuffer);

    //read y_start
    std::getline(ss_currLine, lineBuffer, ';');
    upperLeft.y = stoi(lineBuffer);

    //read x_end
    std::getline(ss_currLine, lineBuffer, ';');
    lowerRight.x = stoi(lineBuffer);

    //read y_end
    std::getline(ss_currLine, lineBuffer, ';');
    lowerRight.y = stoi(lineBuffer);

    //read id
    std::getline(ss_currLine, lineBuffer, ';');
    int signId = stoi(lineBuffer);

    ptrAffectedFile->signs.emplace_back(upperLeft, lowerRight, signId);
  }

  //finished reading training data
  //filter out all training entries where we don't have the file for it
  /*for (size_t i = trainingData.size(); i<=0; i--) {
    string& trainingFilename = trainingData[i].filename;

    bool found = false;
    for (string& searchFilename : _trainingDataFileList) {
      if (searchFilename==trainingFilename) {
        found = true;
        break;
      }
    }
    if (!found) {
      cout << "Dropped training data for: " << trainingFilename << "\n";
      trainingData.erase(trainingData.begin()+i);
    }
  }*/
  return trainingData;
}
SignOccuranceArray TrainingData::countSignOccurances() const
{
  SignOccuranceArray occurances;
  for (const trainingDataInfo& tr : _trainingData) {
    for (const SignPlace& sign : tr.signs) {
      //if first occurance of sign, increase occurance size
      if (sign.getSignId()+1>=occurances.size()) {
        occurances.resize((size_t) sign.getSignId()+1);
      }
      occurances[sign.getSignId()].cnt++;
    }
  }
  return occurances;
}
cv::Rect TrainingData::determineAreaWithSigns() const
{
  cv::Point upperLeft(10000, 100000); //todo this may have to be increased if training pictures get larger
  cv::Point lowerRight(0, 0);

  for (const trainingDataInfo& tr : _trainingData) {
    for (const SignPlace& sign : tr.signs) {
      upperLeft.y = std::min(upperLeft.y, sign.getUpperLeft().y);
      upperLeft.x = std::min(upperLeft.x, sign.getUpperLeft().x);
      lowerRight.y = std::max(lowerRight.y, sign.getLowerRight().y);
      lowerRight.x = std::max(lowerRight.x, sign.getLowerRight().x);
    }
  }
  return cv::Rect(upperLeft, lowerRight);
}
const cv::Rect& TrainingData::getAreaWithSigns() const
{
  return _areaWithSigns;
}
/*
static int __fileCallback(const char* fpath, const struct stat* sb, int typeflag)
{
  //https://stackoverflow.com/questions/983376/recursive-folder-scanning-in-c
  static const char* filters[] = {
      "*.png"
  };

  if (typeflag==FTW_F) {
    int i;

    for (i = 0; i<sizeof(filters)/sizeof(filters[0]); i++) {
      /
      if (fnmatch(filters[i], fpath, FNM_CASEFOLD)==0) {

        _trainingDataFileList.emplace_back(fpath);
        //printf("found image: %s\n", fpath);
        break;
      }
    }
  }


  return 0;
}

void TrainingData::gatherTrainingDataFiles() const
{
  ftw(TRAINING_DATA_TRAINING_PICTURES_PATH, __fileCallback, 16);
}
*/

void TrainingData::evaluateSignDetector(bool quick) const
{
  int processedTrainingDataEntries = 0;
  int signCountTotal = 0;
  int correctlySpottedSigns = 0;
  int signDetectedWhereNoneIs = 0;
  SignOccuranceArray correctlyKlassifiedSigns(_perSignOccurance.size());

  SignDetector detector;

  for(const trainingDataInfo& trainingEntry : _trainingData) {
    cv::Mat trainingPicture = cv::imread(TRAINING_DATA_TRAINING_PICTURES_PATH +trainingEntry.filename);
    if (trainingPicture.data==nullptr) {
      cout << "Unable to load picture for training data entry '" << trainingEntry.filename << "'\n";
      continue;
    }

    ++processedTrainingDataEntries;
    signCountTotal += trainingEntry.signs.size();

    const vector<SignPlace> detectedSigns = detector.detect(trainingPicture, _areaWithSigns);

    //cross check detected signs with training data
    //todo filter out multiple detection of same trainingSign
    for (const SignPlace& detecSign :  detectedSigns) {
      bool matched = false;

      for (const SignPlace& trainingSign : trainingEntry.signs) {
        if (trainingSign.isOverlappingEnough(detecSign)) {
          matched = true;
          break;
        }
      }

      if (matched) {
        correctlySpottedSigns++;
        //correctly spotted sign will be drawn in green
        if (!quick) {
          detecSign.drawOutline(trainingPicture, cv::Scalar(0, 255, 0));
        }
      } else {
        signDetectedWhereNoneIs++;
        //not correctly spotted, red
        if (!quick) {
          detecSign.drawOutline(trainingPicture, cv::Scalar(0, 0, 255));
        }
      }
    }



    if (!quick) {
      //draw area with signs outline
      cv::rectangle(trainingPicture, _areaWithSigns, cv::Scalar(255,0,0), 2);

      //draw training signs outlines, pink
      for (const SignPlace& trainingSign : trainingEntry.signs) {
       trainingSign.drawOutline(trainingPicture, cv::Scalar(255,0,255));
      }

      //show with imgview
      cv::imshow("SignDetecc", trainingPicture);
      // waits two seconds, kills programm if esc was pressed
      for(int i=0;i<20;++i){
        int k = cv::waitKey(100);
        if(k==27){
          cv::destroyAllWindows();
          return;
        }
      }

    }
  }
}



}