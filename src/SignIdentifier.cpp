#include "SignIdentifier.h"

SignIdentifier::SignIdentifier() { }
SignIdentifier::~SignIdentifier()
{
}

/**
 * #1 problem:  identifikation der schilder auf bild
 *  -> straßenschilder grundsätzlich sehr gesättigt
 *      * erster versuch im RGB raum (raussuchen des dominierenden kanals) zu anfällig für
 *      schlechte beleuchtung,  keine guten ergebnisse
 *      * im HSV raum ist sättigung beleuchtungsinvariant (sofern die kamera die farbe überhaupt registriert,
 *      extreme überbelichtung lässt das schild weiß werden -> keine sättigung und schwarze flecken im S kanal)
 *  -> schilder nun beleucuntungsinvariant erkennbar
 *  -> blätter zeichnen sich als großes rauschen im S kanal ab
 *      * schilder sind nur sehr schwer auszumachen
 *      * idee: alle grünen sachen komplett schwärzen da es keine grünen straßenschilder gibt
 *          H von ... bis ... grün (360° range,  opencv geht von 0-180, alles halbieren)
 *          * muss sehr aufgedreht werden damit auch viel grün verschwindet  als nebeneffekt werden jetzt auch gelbe
 *          schilder nicht mehr erkannt...
 *      * scheint teile der bäume zu entfernen aber farbrauschen der kameras macht es schwer
 *          alles zu entfernen
 *      * 4x4 blurring hilft gegen farbrauschen
 *
 *  -> schilder sind jetzt entweder gut separiert oder stehen als schwarzer fleck in einer großen weißen
 *      fläche von grünzeug
 *
 *  -> schilderextraktion durch pain bucket fill operation (wie bei ms paint)
 *      dadurch werden zusammenhängende (ohne teile die durch andersfarbige pixel vollständig abgetrennt sind
 *      zusammengefasst),  dann kann eine bounding box darum gezogen werden (opencv contour findung)
 *
 *      * es müssen von vornerein alle viel zu kleinen bounding boxes ignoriert werden (rauschen, einzelne blätter)
 *
 *      * es müssen von allen zu langen bildern nur der kopf oder nur der boden betrachtet werden  (manchmal ist der pfeiler des schildes
 *      mit drauf),
 *
 *      * es müssen auf alle viel zu großen bereiche ignoriert werden (größe büsche)
 *
 *      * es muss das invertierte bild betrachtet werden da oft schilder die hinter großen büschen stehen als schwarzer flech
 *      in der großen weißen fläche erscheinen  -> invertiert ist es dann ein weißer fleck
 *      dort muss aber die gefundene region vergrößert werden weil meistens mit der methode nur die innenfläche der schilder
 *      gefunden wird
 *
 *
 *
 */

const vector<SignPlace> SignIdentifier::detect(const cv::Mat inputImage, const cv::Rect& areaWithSigns)
{
  //cropp of area with no sign, leaving a region of interest
  cv::Mat inputROIunedited(inputImage, areaWithSigns);
  cv::Mat inputROI;
  inputROIunedited.copyTo(inputROI);

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


 // red  H = 0
  // yellow H = 30
  // pure green  H=60
  // cyan H = 90
  // blue H = 120
  //
  //

  /*cv::imshow("original", inputROI);
  cv::waitKey(10);
*/
   /*
  cv::Mat g;
  inputROI.copyTo(g);

  for (int y = 0;y<inputROI.rows; ++y) {
    for (int x = 0; x<inputROI.cols; ++x) {
      //char histAdjPixel = equalized.at<char>(y,x);
      cv::Vec3b pixel = inputROI.at<cv::Vec3b>(y, x);
      g.at<cv::Vec3b>(y,x) = cv::Vec3d((float(y)/float(inputROI.rows))*255.0f,255 - (float(y)/float(inputROI.rows))*255.0f,0);
    }
  }

  cv::imshow("g", g);
  cv::waitKey(10);

  cvtColor( g, g, CV_BGR2HSV );

  for (int y = 0;y<inputROI.rows; ++y) {
    for (int x = 0; x<inputROI.cols; ++x) {
      //char histAdjPixel = equalized.at<char>(y,x);
      cv::Vec3b pixel = g.at<cv::Vec3b>(y, x);
      g.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[0],pixel[0],pixel[0]);
    }
  }

  cv::imshow("g_hsv", g);
  cv::waitKey(10);
*/


  cv::blur(inputROI,inputROI, cv::Size(4,4));

  //cv::equalizeHist(equalized,equalized);
  cvtColor( inputROI, inputROI, CV_BGR2HSV );

  cv::Mat h,s,v, s_uncropped;

  inputROI.copyTo(h);
  inputROI.copyTo(s);
  inputROI.copyTo(v);
  inputROI.copyTo(s_uncropped);


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


      h.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[0],pixel[0],pixel[0]);

      //blacken saturation image by hue

      if (pixel[0] > 30 && pixel[0] < 105) {
          s.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,0);
      } else {
          s.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[1],pixel[1],pixel[1]);
      }

      s_uncropped.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[1],pixel[1],pixel[1]);
      v.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[2],pixel[2],pixel[2]);

      //std::cout<<<<"\n";
     /* if (redRatio > 0.45) {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,255);
      } /*else if (blueRatio > 0.45) {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(255,0,0);
      } *else {

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
      }*


      // =  * 2;
*/
     }
  }

/*  cv::imshow("H", h);
  cv::waitKey(10);
  cv::imshow("s", s);
  cv::waitKey(10);
  cv::imshow("s_uncropped", s_uncropped);
*/

 /*
*/
  //50 vor s threshold seems to remove all the street
  cv::threshold(s, s, 50, 255,CV_THRESH_BINARY);
/*
  cv::imshow("s_thresh", s);
  cv::waitKey(10);
  */

  /**
   * Dilate / Erode
   */

  int erosion_elem = 0;
  int erosion_size = 1;
  int dilation_elem = 0;
  int dilation_size = 1;
  int const max_elem = 2;
  int const max_kernel_size = 21;



  cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT,
          cv::Size( 2*erosion_size + 1, 2*erosion_size+1 ),
          cv::Point( erosion_size, erosion_size ) );

  /// Apply the erosion operation
  erode( s, s, element );


  cv::Mat element2 = getStructuringElement( cv::MORPH_RECT,
      cv::Size( 2*dilation_size + 1, 2*dilation_size+1 ),
      cv::Point( dilation_size, dilation_size ) );
  /// Apply the dilation operation
  dilate( s, s, element2 );


  /**
   * Canny Edge
   */
/*
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
*/

  vector<cv::Rect> output;

  cv::Mat grayS;
  cvtColor( s, grayS, CV_BGR2GRAY );


  auto positiveout = findAndDrawContours(cv::Scalar(0,255,0), grayS, s);
  output.insert(output.end(), positiveout.begin(), positiveout.end());


  cv::bitwise_not(grayS, grayS);


  erode( grayS, grayS, element );
  dilate( grayS, grayS, element2 );


  cv::imshow("inverted", grayS);

  auto negativeout = findAndDrawContours(cv::Scalar(0,0,255), grayS, s);
  output.insert(output.end(), negativeout.begin(), negativeout.end());

  vector<SignPlace> outSigns;

  for (const cv::Rect& rec: output) {
    if (rec.width < 20 || rec.height < 20) {
      continue;
    }
  /*
    if (rec.width > 100 || rec.height > 100) {
      continue;
    }*/



    cv::Point ul(rec.x+roiOffset.x,rec.y+roiOffset.y);
    cv::Point lr(rec.x+roiOffset.x+rec.width,rec.y+roiOffset.y+rec.height);

    outSigns.emplace_back(ul, lr, -1);
  }


  cv::imshow("s_thresh_morph", s);
  cv::waitKey(100000);



  return outSigns;
}


vector<cv::Rect> SignIdentifier::findAndDrawContours(const cv::Scalar& color, cv::Mat grayS, cv::Mat targetDraw) {
  vector< vector<cv::Point> > contours;
  cv::findContours(grayS, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

  vector<cv::Rect> output;

  for (const vector<cv::Point>& contour : contours) {
    cv::Point upperLeft(10000, 100000); //todo this may has to be increased if training pictures get larger
    cv::Point lowerRight(0, 0);

    for (const cv::Point& pnt : contour) {
      upperLeft.y = std::min(upperLeft.y, pnt.y);
      upperLeft.x = std::min(upperLeft.x, pnt.x);
      lowerRight.y = std::max(lowerRight.y, pnt.y);
      lowerRight.x = std::max(lowerRight.x, pnt.x);
    }
  //  cv::Rect contourOutline(upperLeft, lowerRight);
    output.emplace_back(upperLeft, lowerRight);

  /*  if (contourOutline.width < 20 || contourOutline.height < 20) {
      continue;
    }
    cv::rectangle(targetDraw,upperLeft, lowerRight, color,1);
*/
  }
  return output;
}



void SignIdentifier::morphThinning(cv::Mat &img) const {
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
