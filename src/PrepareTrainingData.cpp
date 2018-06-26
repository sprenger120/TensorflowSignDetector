#include <iostream>
#include <opencv/cv.hpp>
#include <opencv/highgui.h>
#include <fstream>
#include <ctime>
#include <string>
#include <stdio.h>
#include <ftw.h>
#include <fnmatch.h>
#include <algorithm>
#include <string>
#include <numeric>
#include <random>

#include "TrainingData.h"

void PrintWidthHeightStatistics(vector<int> &sign_heights, vector<int> &sign_widths) {
  nth_element(sign_heights.begin(), sign_heights.begin() + sign_heights.size() / 2, sign_heights.end());
  nth_element(sign_widths.begin(), sign_widths.begin() + sign_widths.size() / 2, sign_widths.end());
  int sign_width_median = sign_widths[sign_widths.size() / 2];
  int sign_height_median = sign_heights[sign_heights.size() / 2];
  double sign_width_mean = accumulate(sign_widths.begin(), sign_widths.end(), 0) / (double) sign_widths.size();
  double sign_height_mean = accumulate(sign_heights.begin(), sign_heights.end(), 0) / (double) sign_heights.size();
  int sign_width_max = *max_element(sign_widths.begin(), sign_widths.end());
  int sign_height_max = *max_element(sign_heights.begin(), sign_heights.end());
  int sign_width_min = *min_element(sign_widths.begin(), sign_widths.end());
  int sign_height_min = *min_element(sign_heights.begin(), sign_heights.end());
  std::cout << "A total of " << sign_heights.size() << " signs were processed." << std::endl;
  std::cout << "The mean values of (width x height) are (" << sign_width_mean << " x " << sign_height_mean << ")"
            << std::endl;
  std::cout << "The median values of (width x height) are (" << sign_width_median << " x " << sign_height_median << ")"
            << std::endl;
  std::cout << "The maximum values of (width x height) are (" << sign_width_max << " x " << sign_height_max << ")"
            << std::endl;
  std::cout << "The minimum values of (width x height) are (" << sign_width_min << " x " << sign_height_min << ")"
            << std::endl;
}

void CropWithStatistics(TrainingData::TrainingData td) {
  unsigned int sign_count[43];
  std::vector<int> sign_heights;
  std::vector<int> sign_widths;
  for (auto trData : td.GetTrainingData()) {
    cv::Mat matPicture = cv::imread("training_data/png/" + trData.filename);
    if (matPicture.data == nullptr) {
      //cout << "Unable to load picture for training data entry '" << trData.filename << "'\n";
      continue;
    }
    for (SignPlace &rec : trData.signs) {
      int sign_width = rec.getLowerRight().x - rec.getUpperLeft().x;
      int sign_height = rec.getLowerRight().y - rec.getUpperLeft().y;
      sign_heights.push_back(sign_height);
      sign_widths.push_back(sign_width);
      std::cout << trData.filename << "(" << matPicture.cols << "x" << matPicture.rows << ")"
                << ": Found Sign with width=" << sign_width << " and height="
                << sign_height << std::endl;
      cv::Rect region(rec.getUpperLeft().x, rec.getUpperLeft().y,
                      sign_width, sign_height);
      cv::Mat cropped = matPicture(region);
      std::stringstream ss;
      ss << "training_data/croppedSigns/sign_id=" << rec.getSignId() << "_no=" << sign_count[rec.getSignId()]++
         << ".png";
      imwrite(ss.str(), cropped);
    }
  }
  PrintWidthHeightStatistics(sign_heights, sign_widths);
}

void GenerateBackgroundSamples(TrainingData::TrainingData td,
                               unsigned int samples_per_picture,
                               unsigned int sample_height,
                               unsigned int sample_width) {
  for (auto trData : td.GetTrainingData()) {
    cv::Mat matPicture = cv::imread("training_data/png/" + trData.filename);
    if (matPicture.data == nullptr) {
      //cout << "Unable to load picture for training data entry '" << trData.filename << "'\n";
      continue;
    }
    std::random_device d;
    std::uniform_int_distribution<int> left_rd(0, td.getAreaWithSigns().width - sample_width - 1);
    std::uniform_int_distribution<int> top_rd(0, td.getAreaWithSigns().height - sample_height - 1);
    matPicture = matPicture(td.getAreaWithSigns());
    std::vector<cv::Rect> samples;
    for (auto i = 0; i < samples_per_picture; ++i) {
      cv::Rect sample_rect;
      bool intersects_sign = true;
      while (intersects_sign) {
        intersects_sign = false;
        auto top = (unsigned int) std::round(top_rd(d));
        auto left = (unsigned int) std::round(left_rd(d));
        sample_rect = cv::Rect(left, top, sample_width, sample_height);
        for (auto &sign : trData.signs) {
          if (sample_rect.contains(sign.getLowerRight()) || sample_rect.contains(sign.getUpperLeft())) {
            intersects_sign = true;
            continue;
          }
        }
      }
      cv::Mat cropped = matPicture(sample_rect);
      std::stringstream ss;
      ss << "training_data/background/bg_" << trData.filename << "_no=" << i
         << ".png";
      imwrite(ss.str(), cropped);

    }
  }
}

int main() {
  TrainingData::TrainingData trainingData;
  //CropWithStatistics(trainingData);
  GenerateBackgroundSamples(trainingData, 5, 44, 43);
  cv::destroyAllWindows();
  return 0;
}
