#include <iostream>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <ctime>
#include <string>
#include <stdio.h>
#include <ftw.h>
#include <fnmatch.h>
#include <algorithm>
#include <string>
#include "Line.h"
#include "LinesProcessor.h"

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

struct trainingDataInfo{
  //header: img; x_start; y_start; x_end; y_end; id
  string filename;

  //borders around signs
  vector<SignPlace> signs;
};


//where training data directoy structure is stored seen from working directory
string PATH("training_data");

int main() {
  vector<trainingDataInfo> trainingData;
    // commando prompt that takes filename,
    //read sign coordinates from file and display rectangle with imgview


  //grab all pictures files
  ftw((PATH + "/png").c_str(), fileCallback, 16);

  cout<<"Found "<<picturesFiles.size()<<" actual pictures\n";

  //read csv file
  std::ifstream file( PATH + "/gt_train.txt" );

  if ( !file )
  {
    cout<<"Unable to open gt_train file\n";
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
    trainingDataInfo* ptrAffectedFile = nullptr;
    for(trainingDataInfo& trData : trainingData) {
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
      ptrAffectedFile = &trainingData[trainingData.size()-1];
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

  cout<<"Loaded training data for "<<trainingData.size()<<" pictures\n";


  int edgeThresh = 1;
  int lowThreshold = 30;
  int const max_lowThreshold = 100;
  int ratio = 3;
  int kernel_size = 3;
  LinesProcessor linesProcessor;

  //first demo;  scrolling through files and rendering a rectangle around pictures
  for(trainingDataInfo& trData : trainingData) {
    cv::Mat matPicture = cv::imread(PATH + "/png/" + trData.filename);
    if (matPicture.data == nullptr) {
      cout<<"Unable to load picture for training data entry '"<<trData.filename<<"'\n";
      continue;
    }

    cv::imshow("SignDetecc_original", matPicture);
    blur( matPicture, matPicture, cv::Size(4,4) );

    cvtColor( matPicture, matPicture, CV_BGR2GRAY );
    equalizeHist( matPicture, matPicture );
    //matPicture.convertTo(matPicture, -1, 2, 0);

    //matPicture.convertTo(matPicture, CV_8UC3, 1, 50);

    /// Reduce noise with a kernel 3x3


    /// Canny detector
    cv::Mat cannysMat;
    Canny( matPicture, cannysMat, lowThreshold, lowThreshold*ratio, kernel_size );

    matPicture = cannysMat;


    auto list = linesProcessor.makeLines(matPicture);


    int erosion_elem = 0;
    int erosion_size = 1;
    int dilation_elem = 1;
    int dilation_size = 1;
    int const max_elem = 2;
    int const max_kernel_size = 21;

/*
    Mat element = getStructuringElement( MORPH_RECT,
                                         Size( 2*erosion_size + 1, 2*erosion_size+1 ),
                                         Point( erosion_size, erosion_size ) );

    /// Apply the erosion operation
    erode( matPicture, matPicture, element );


    Mat element2 = getStructuringElement( MORPH_RECT,
                                         Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                         Point( dilation_size, dilation_size ) );
    /// Apply the dilation operation
    //dilate( matPicture, matPicture, element2 );
*/

    Mat houghsPic(matPicture.rows, matPicture.cols, CV_8UC1);
    //cvtColor(matPicture, houghsPic, CV_GRAY2BGR);
    vector<Vec4i> lines;
    HoughLinesP(matPicture, lines, 1, CV_PI/180, 50, 50, 10 );
    for( size_t i = 0; i < lines.size(); i++ )
    {
      Vec4i l = lines[i];
      line( houghsPic, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255), 1, CV_AA);
    }


    //matPicture = houghsPic;

    /*for(Rect& rec : trData.signs) {
      //color scalaer is bgr format
      cv::rectangle(matPicture,rec.upperLeft, rec.lowerRight, cv::Scalar(0,255,0),2);
      cv::putText(matPicture, to_string(rec.signId), rec.lowerRight + cv::Point(10,10),
          cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0,255,0), 3);
    }*/




    cv::imshow("SignDetecc", matPicture);
    // waits two seconds, kills programm if esc was pressed
    for(int i=0;i<20;++i){
      int k = cv::waitKey(100);
      if(k==27){
        cv::destroyAllWindows();
        return 0;
      }
    }
  }




  cv::destroyAllWindows();
  return 0;
}
