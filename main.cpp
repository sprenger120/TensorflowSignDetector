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
#include <numeric>

using namespace std;
using namespace cv;

vector<string> picturesFiles;

//https://stackoverflow.com/questions/983376/recursive-folder-scanning-in-c
static const char *filters[] = {
    "*.png"
};

static int fileCallback(const char *fpath, const struct stat *sb, int typeflag) {
  /* if it's a file */
  if (typeflag == FTW_F) {
    int i;
    /* for each filter, */
    for (i = 0; i < sizeof(filters) / sizeof(filters[0]); i++) {
      /* if the filename matches the filter, */
      if (fnmatch(filters[i], fpath, FNM_CASEFOLD) == 0) {
        /* do something */
        picturesFiles.push_back(string(fpath));
        //printf("found image: %s\n", fpath);
        break;
      }
    }
  }

  /* tell ftw to continue */
  return 0;
}

struct SignPlace {
  /*
   upperLeft
   \/
   ******
   *    *
   *    *
   ******
   *    ^
   *    lowerRight
   */
  cv::Point upperLeft;
  cv::Point lowerRight;
  int signId;
};

struct trainingDataInfo {
  //header: img; x_start; y_start; x_end; y_end; id
  string filename;

  //borders around signs
  vector<SignPlace> signs;
};

//where training data directoy structure is stored seen from working directory
string PATH("training_data");

unsigned int sign_count[43];

int main(int argc, char *argv[]) {
  for (unsigned int &i : sign_count) {
    i = 0;
  }

  std::cout << argv[0] << std::endl;
  vector<trainingDataInfo> trainingData;
  // commando prompt that takes filename,
  //read sign coordinates from file and display rectangle with imgview


  //grab all pictures files
  ftw((PATH + "/png").c_str(), fileCallback, 16); //file tree walk (linux)

  cout << "Found " << picturesFiles.size() << " actual pictures\n";

  //read csv file
  std::ifstream file(PATH + "/gt_train.txt");

  if (!file) {
    cout << "Unable to open gt_train file\n";
    return -1;
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
    trainingDataInfo *ptrAffectedFile = nullptr;
    for (trainingDataInfo &trData : trainingData) {
      if (trData.filename == lineBuffer) {
        ptrAffectedFile = &trData;
        break;
      }
    }
    if (ptrAffectedFile == nullptr) {
      //create new entry, noone found
      trainingDataInfo trData;
      trData.filename = lineBuffer;
      trainingData.push_back(trData);
      //trainingData vector might create a copy so we get the ptr out of it
      ptrAffectedFile = &trainingData[trainingData.size() - 1];
    }

    SignPlace rect;

    //read x_start
    std::getline(ss_currLine, lineBuffer, ';');
    rect.upperLeft.x = stoi(lineBuffer);

    //read y_start
    std::getline(ss_currLine, lineBuffer, ';');
    rect.upperLeft.y = stoi(lineBuffer);

    //read x_end
    std::getline(ss_currLine, lineBuffer, ';');
    rect.lowerRight.x = stoi(lineBuffer);

    //read y_end
    std::getline(ss_currLine, lineBuffer, ';');
    rect.lowerRight.y = stoi(lineBuffer);

    //read id
    std::getline(ss_currLine, lineBuffer, ';');
    rect.signId = stoi(lineBuffer);

    ptrAffectedFile->signs.push_back(rect);
  }

  cout << "Loaded training data for " << trainingData.size() << " pictures\n";

  std::vector<int> sign_heights;
  std::vector<int> sign_widths;

  //first demo;  scrolling through files and rendering a rectangle around pictures
  for (trainingDataInfo &trData : trainingData) {
    cv::Mat matPicture = cv::imread(PATH + "/png/" + trData.filename);
    if (matPicture.data == nullptr) {
      //cout << "Unable to load picture for training data entry '" << trData.filename << "'\n";
      continue;
    }

    //crop image and save to png
    cv::imshow("SignDetecc_original", matPicture);
    for (SignPlace &rec : trData.signs) {
      int sign_width = rec.lowerRight.x - rec.upperLeft.x;
      int sign_height = rec.lowerRight.y - rec.upperLeft.y;
      sign_heights.push_back(sign_height);
      sign_widths.push_back(sign_width);
      std::cout << trData.filename << "(" << matPicture.cols << "x" << matPicture.rows << ")"
                << ": Found Sign with width=" << sign_width << " and height="
                << sign_height << std::endl;
      cv::Rect region(rec.upperLeft.x, rec.upperLeft.y,
                      sign_width, sign_height);
      cv::Mat cropped = matPicture(region);
      std::stringstream ss;
      ss << "training_data/croppedSigns/sign_id=" << rec.signId << "_no=" << sign_count[rec.signId]++ << ".png";
      imwrite(ss.str(), cropped);
    }
  }

  std::nth_element(sign_heights.begin(), sign_heights.begin() + sign_heights.size() / 2, sign_heights.end());
  std::nth_element(sign_widths.begin(), sign_widths.begin() + sign_widths.size() / 2, sign_widths.end());
  int sign_width_median = sign_widths[sign_widths.size() / 2];
  int sign_height_median = sign_heights[sign_heights.size() / 2];
  double sign_width_mean = std::accumulate(sign_widths.begin(), sign_widths.end(), 0) / (double) sign_widths.size();
  double sign_height_mean = std::accumulate(sign_heights.begin(), sign_heights.end(), 0) / (double) sign_heights.size();
  int sign_width_max = *std::max_element(sign_widths.begin(), sign_widths.end());
  int sign_height_max = *std::max_element(sign_heights.begin(), sign_heights.end());
  int sign_width_min = *std::min_element(sign_widths.begin(), sign_widths.end());
  int sign_height_min = *std::min_element(sign_heights.begin(), sign_heights.end());
  std::cout << "A total of " << sign_heights.size() << " signs were processed." << std::endl;
  std::cout << "The mean values of (width x height) are (" << sign_width_mean << " x " << sign_height_mean << ")"
            << std::endl;
  std::cout << "The median values of (width x height) are (" << sign_width_median << " x " << sign_height_median << ")"
            << std::endl;
  std::cout << "The maximum values of (width x height) are (" << sign_width_max << " x " << sign_height_max << ")"
            << std::endl;
  std::cout << "The minimum values of (width x height) are (" << sign_width_min << " x " << sign_height_min << ")"
            << std::endl;
  cv::destroyAllWindows();
  return 0;
}
