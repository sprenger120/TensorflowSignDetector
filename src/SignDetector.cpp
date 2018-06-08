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



  /*
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

    *for(Rect& rec : trData.signs) {
      //color scalaer is bgr format
      cv::rectangle(matPicture,rec.upperLeft, rec.lowerRight, cv::Scalar(0,255,0),2);
      cv::putText(matPicture, to_string(rec.signId), rec.lowerRight + cv::Point(10,10),
          cv::FONT_HERSHEY_PLAIN, 3, cv::Scalar(0,255,0), 3);
    }*
  }
*/


  cv::Mat equalized;
  cvtColor( inputROI, equalized, CV_BGR2GRAY );
  cv::equalizeHist(equalized,equalized);
  cv::imshow("original", inputROI);
  cv::waitKey(10);


  for (int y = 0;y<inputROI.rows; ++y) {
    for(int x =0;x<inputROI.cols;++x) {
      //char histAdjPixel = equalized.at<char>(y,x);
      cv::Vec3b pixel = inputROI.at<cv::Vec3b>(y,x);



      float lightSum = pixel[2] + pixel[1] + pixel[0];

      float chRed = pixel[2];
      float chGreen = pixel[1];
      float chBlue = pixel[0];

     // float histAdjFactor = float(histAdjPixel) / 255;

      float redRatio = (chRed / lightSum);
      float greenRatio = chGreen / lightSum;
      float blueRatio = chBlue / lightSum;



      //std::cout<<<<"\n";

      if (redRatio > 0.45) {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,255);
      } /*else if (blueRatio > 0.45) {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(255,0,0);
      } */else {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,0);
      }


      //inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(blueRatio*255,greenRatio*255,redRatio*255);


      /*int red = int(float(pixel[2]) - (float(pixel[0])*1 + float(pixel[1])*1.5 )) *2;
      int blue = int(float(pixel[0]) - (float(pixel[2])*1 + float(pixel[1])*1.5 )) *2;

      inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(red,red,red);
*/
      /*if (diff > 75 && pixel[0] == largestValue) {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(255,0,0);
      } else {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,0);
      }*/


      // =  * 2;

     }
  }

  /**
   * Dilate / Erode
   */

  int erosion_elem = 0;
  int erosion_size = 1;
  int dilation_elem = 1;
  int dilation_size = 1;
  int const max_elem = 2;
  int const max_kernel_size = 21;



  cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT,
          cv::Size( 2*erosion_size + 1, 2*erosion_size+1 ),
          cv::Point( erosion_size, erosion_size ) );

  /// Apply the erosion operation
  erode( inputROI, inputROI, element );


  cv::Mat element2 = getStructuringElement( cv::MORPH_RECT,
      cv::Size( 2*dilation_size + 1, 2*dilation_size+1 ),
      cv::Point( dilation_size, dilation_size ) );
  /// Apply the dilation operation
  dilate( inputROI, inputROI, element2 );


  /**
   * Canny Edge
   */

  int edgeThresh = 1;
  int lowThreshold = 20;
  int const max_lowThreshold = 100;
  int ratio = 3;
  int kernel_size = 3;


  cv::Mat cannysMat;
  //cv::blur(inputROI,inputROI, cv::Size(4,4));
  cv::Canny( inputROI, cannysMat, lowThreshold, lowThreshold*ratio, kernel_size );

  cv::Mat bw;
  cvtColor( inputROI, bw, CV_BGR2GRAY );
  morphThinning(bw);
  cv::threshold(bw, bw, 50,255,cv::THRESH_BINARY);

  cv::imshow("signdetector", bw);
  cv::waitKey(100000);

  return vector<SignPlace>();
}


void SignDetector::morphThinning(cv::Mat &img) const {
  cv::Mat skel(img.size(), CV_8UC1, cv::Scalar(0));
  cv::Mat temp;
  cv::Mat eroded;
  cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

  do {
    cv::erode(img, eroded, element);
    cv::dilate(eroded, temp, element);
    cv::subtract(img, temp, temp);
    cv::bitwise_or(skel, temp, skel);
    eroded.copyTo(img);

  } while (cv::countNonZero(img) > 0);

  img = skel;
}
