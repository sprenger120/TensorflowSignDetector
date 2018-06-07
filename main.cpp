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

// https://docs.opencv.org/2.4/doc/tutorials/imgproc/histograms/histogram_calculation/histogram_calculation.html
void showHist(Mat src, string name) {
    Mat dst;

    /// Separate the image in 3 places ( B, G and R )
    vector<Mat> bgr_planes;
    split(src, bgr_planes);

    /// Establish the number of bins
    int histSize = 256;

    /// Set the ranges ( for B,G,R) )
    float range[] = {0, 256};
    const float *histRange = {range};

    bool uniform = true;
    bool accumulate = false;

    Mat b_hist, g_hist, r_hist;

    /// Compute the histograms:
    calcHist(&bgr_planes[0], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate);
    calcHist(&bgr_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate);
    calcHist(&bgr_planes[2], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate);

    // Draw the histograms for B, G and R
    int hist_w = 512;
    int hist_h = 400;
    int bin_w = cvRound((double) hist_w / histSize);

    Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

    /// Normalize the result to [ 0, histImage.rows ]
    normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
    normalize(g_hist, g_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
    normalize(r_hist, r_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

    /// Draw for each channel
    for (int i = 1; i < histSize; i++) {
        line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(b_hist.at<float>(i - 1))),
             Point(bin_w * (i), hist_h - cvRound(b_hist.at<float>(i))),
             Scalar(255, 0, 0), 2, 8, 0);
        line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(g_hist.at<float>(i - 1))),
             Point(bin_w * (i), hist_h - cvRound(g_hist.at<float>(i))),
             Scalar(0, 255, 0), 2, 8, 0);
        line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(r_hist.at<float>(i - 1))),
             Point(bin_w * (i), hist_h - cvRound(r_hist.at<float>(i))),
             Scalar(0, 0, 255), 2, 8, 0);
    }

    /// Display
    namedWindow(name, CV_WINDOW_AUTOSIZE);
    imshow(name, histImage);
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

    std::cout << argv[0] << std::endl;
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


    //first demo;  scrolling through files and rendering a rectangle around pictures
    for (trainingDataInfo &trData : trainingData) {
        cv::Mat matPicture = cv::imread(PATH + "/png/" + trData.filename);
        std::cout << "Loading picture: " << trData.filename << std::endl;
        if (matPicture.data == nullptr) {
            cout << "Unable to load picture for training data entry '" << trData.filename << "'\n";
            continue;
        }

        //crop image and save to png
        cv::namedWindow("cropped", CV_WINDOW_AUTOSIZE);
        cv::imshow("SignDetecc_original", matPicture);
        for (SignPlace &rec : trData.signs) {
            cv::Rect region(rec.upperLeft.x, rec.upperLeft.y,
                            rec.lowerRight.x - rec.upperLeft.x, rec.lowerRight.y - rec.upperLeft.y);
            showHist(matPicture, "hist_full_image");
            cv::Mat cropped = matPicture(region);
            imshow("cropped", cropped);
            showHist(cropped, "hist_cropped");
            waitKey(0);
            //std::stringstream ss;
            //ss << "sign_id=" << rec.signId << "_no=" << sign_count[rec.signId]++ << ".png";
            //imwrite(ss.str(), cropped);
        }
    }


    cv::destroyAllWindows();
    return 0;
}
