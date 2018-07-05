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
 *      * (4x4 blurring hilft gegen farbrauschen - hat detektionsrate global um ~5% gesenkt)
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

/**
 * HSV  - H Spectrum
 * red  H = 0
 * yellow H = 30
 * pure green  H=60
 * cyan H = 90
 * blue H = 120
 */

//max size values, integers unless stated otherwise
#define SIZE_CATEGORY_TINY_W_H 12
#define SIZE_CATEGORY_SMALL_W_H 30
#define SIZE_CATEGORY_NORMAL_W_H 128
#define SIZE_CATEOGRY_LONG_H_FACTOR 2.0 //double, how much more height there should be seen from w
#define SIZE_CATEGORY_WIDE_W_FACTOR 2.0 //double analogous to SIZE_CATEOGRY_LONG_H_FACTOR

#define SIZE_CATEGORY_SMALL_ENLARGEMENT_FACTOR 0.5 //double,  see explaination after big ################ break

//#define SIGN_IDENT_DEBUG
#define PRESERVE_ORIGINAL

const vector<SignPlace> SignIdentifier::detect(cv::Mat& inputImage, const cv::Rect& areaWithSigns)
{
  //crop of area with no sign, leaving a region of interest
  cv::Mat inputROI;

#ifdef PRESERVE_ORIGINAL
  //when in debug mode we want to preserve the original image
  //so that TrainingData can still use it to draw stuff on it without it being totally
  //destroyed by all the steps in this function
  cv::Mat inputROIunedited(inputImage, areaWithSigns);
  inputROIunedited.copyTo(inputROI);
#else
  inputROI = cv::Mat(inputImage, areaWithSigns);
#endif

  //all coordinates from inputROI are no longer aligned with inputImage
  //we have to add some offset to the coordinates to get back to the original ones
  //the offset is exacly the upperLeft starting point of our roi - how convenient
  cv::Point roiOffset(areaWithSigns.x, areaWithSigns.y);


#ifdef SIGN_IDENT_DEBUG
  cv::imshow("0OriginalROI", inputROI);
  cv::waitKey(10);
#endif

  //Convert to hsv
  //cv::blur(inputROI,inputROI, cv::Size(3,3)); -> see notes
  cvtColor( inputROI, inputROI, CV_BGR2HSV );

#ifdef SIGN_IDENT_DEBUG
  cv::Mat h,s, s_unfilteredByH;
  inputROI.copyTo(h);
  inputROI.copyTo(s);
  inputROI.copyTo(s_unfilteredByH);
#endif

  //reduce inputROI to just the S channel and apply some filtering from the H channel
  //to block out all leaves
  for (int y = 0;y<inputROI.rows; ++y) {
    for(int x =0;x<inputROI.cols;++x) {
      cv::Vec3b pixel = inputROI.at<cv::Vec3b>(y,x); //0=H, 1=S, 2=V

#ifdef SIGN_IDENT_DEBUG
      //debug pictures for each H and S channel
      h.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[0],pixel[0],pixel[0]);
      s_unfilteredByH.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[1],pixel[1],pixel[1]);
#endif

      //filter by hue
      if (pixel[0] > 32 && pixel[0] < 105) {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,0);
#ifdef SIGN_IDENT_DEBUG
        s.at<cv::Vec3b>(y,x) = cv::Vec3d(0,0,0);
#endif
      } else {
        inputROI.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[1],pixel[1],pixel[1]);
#ifdef SIGN_IDENT_DEBUG
          s.at<cv::Vec3b>(y,x) = cv::Vec3d(pixel[1],pixel[1],pixel[1]);
#endif
      }
     }
  }

#ifdef SIGN_IDENT_DEBUG
  cv::imshow("HChannel", h);
  cv::imshow("1SChannelFiltered", s);
  cv::imshow("SChannelUnfiltered", s_unfilteredByH);
#endif


  //threshold our image so that only very saturated colors are coming through (all signs)
  //50 vor s threshold seems to remove all the street
  cv::threshold(inputROI, inputROI, 50, 255,CV_THRESH_BINARY);

#ifdef SIGN_IDENT_DEBUG
  //cv::imshow("2Threshold", inputROI);
#endif


  //Denoise thresholded image with morphological operators
  //double application or bigger kernel sizes only reduce identification rate significantly
  int erosion_size = 1;
  int dilation_size = 1;
  cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT,
          cv::Size( 2*erosion_size + 1, 2*erosion_size+1 ),
          cv::Point( erosion_size, erosion_size ) );
  cv::Mat element2 = getStructuringElement( cv::MORPH_RECT,
      cv::Size( 2*dilation_size + 1, 2*dilation_size+1 ),
      cv::Point( dilation_size, dilation_size ) );
  erode( inputROI, inputROI, element );
  dilate( inputROI, inputROI, element2 );

#ifdef SIGN_IDENT_DEBUG
  cv::imshow("3Morph", inputROI);
#endif

//####################################################################################################

  // Now we are left with a picture that contains clusters of white pixels
  // we want to separate clusters and work on their bounding boxes
  // Convert inputROI from 24bpp to 8bpp because the next operations don't require more and we want to save
  // some cache time
  // white is seen as pixels of interest,  black is ignored
  cv::Mat inputROIGray, inputROIGrayNegative;
  cvtColor( inputROI, inputROIGray, CV_BGR2GRAY );
  inputROIGray.copyTo(inputROIGrayNegative);
  cv::bitwise_not(inputROIGrayNegative, inputROIGrayNegative);
  vector<cv::Rect> preFilteredRects;

  //rect size categories:
  //1. tiny - w&h: >12
  //2. small - w&h <12, >25
  //3. normal - w&h <15, >128
  //4. huge - w&h <128
  //5. long - small-normal on w but  h > 2*w
  //6. broad - tiny - huge but  w > 2*w

  //origins
  //1. tiny - noise and small foilage clusters, trivial reject
  //2. small signs, inner parts of signs, trivial accept, enlarge
  //3. hopefully signs but sadly also many foilage clusters, trivial accept
  //4. large foilage clusters, trivial reject
  //5. long - foilage clusters,  signs that

  //rect operations:
  //1. trivial reject - self explanatory
  //2. trivial accept - self explanatory
  //3. enlarge, accept - Most signs have a very saturated outer border and will be found
  //fine by theirselves (outer border is white, inner mostly black)
  //some signs have a white border and a colored inside which causes the resulting rect to only be
  //showing the inside. Without further action those rects won't be able to be classified. To fix this
  // we enlarge the rect by smallCategoryEnlargementFactor.
  //Because there are also legit signs that are in the small category we add both the enlarged and non enlarged
  //rects to the preFiltered List
  //4. bottom, top extract
  //Sometimes the sign post or will be in the pixel cluster aswell (most of the time sign will be on the bottom or top
  //of it) so here we will add rects of the bottom and top

  findPixelClustersFloodFillMethod(inputROIGray, preFilteredRects, true, false);


  //Because of the fact that the Hue filter won't be able to remove all leaves and trees
  //it sometimes happens that a sign will be completely enclosed within a big pixel cluster and only
  //its black middle will stick out
  //To still grab those signs we use the inverted image from the previous clusterFinder and only
  //add enlarged small clusters to avoid adding too many false positives
  findPixelClustersFloodFillMethod(inputROIGrayNegative, preFilteredRects, false, true);


  // Filter signs and add new rects in special cases (long size category)
  // we also have to ensure that all rects going into the outSigns list have their coordinates within
  // the inputImage or else other openCV operations with these rects will crash the program
  vector<SignPlace> outSigns;

  for (const cv::Rect& rec: preFilteredRects) {
    //tiny filtering already done by findPixelClustersFloodFillMethod

    //see if rect is in huge category -> reject
    if (rec.width > SIZE_CATEGORY_NORMAL_W_H && rec.height > SIZE_CATEGORY_NORMAL_W_H) {
      continue;
    }

    //see if rect is in broad category -> reject
    if (double(rec.width) / double(rec.height) > SIZE_CATEGORY_WIDE_W_FACTOR) {
      continue;
    }

    //see if maybe in long category but w is too big -> reject
    if (rec.width > SIZE_CATEGORY_NORMAL_W_H) {
      continue;
    } else if (double(rec.height) / double(rec.width) > SIZE_CATEOGRY_LONG_H_FACTOR) {
      cv::Size size(rec.width, rec.width);
      //upper
      // missing roi offset
      outSigns.emplace_back(
          clipRect(cv::Rect(rec.tl()+roiOffset, size), inputImage),-1);

      //lower
      outSigns.emplace_back(
          clipRect(cv::Rect(rec.br() - cv::Point(rec.width,rec.width)+roiOffset, size),inputImage), -1);

      continue; // no need to add the large object, the klassifier can't even detect anything on it
    }

    //apply roiOffset (see start of function for explanation)
    outSigns.emplace_back(clipCoordinate(rec.tl()+roiOffset, inputImage),
        clipCoordinate(rec.br()+roiOffset, inputImage), -1);
  }

#ifdef SIGN_IDENT_DEBUG
  cv::waitKey(1000000);
#endif

  return outSigns;
}

cv::Rect SignIdentifier::clipRect(const cv::Rect& rect, const cv::Mat& clipMat) const {
  return cv::Rect(
      clipCoordinate(rect.tl(), clipMat),
      clipCoordinate(rect.br(), clipMat));
}

cv::Point SignIdentifier::clipCoordinate(const cv::Point& pnt, const cv::Mat& clipMat) const {
  return cv::Point(
      std::min(clipMat.cols-1, std::max(pnt.x, 0)),
      std::min(clipMat.rows-1, std::max(pnt.y, 0)));
}

void SignIdentifier::findPixelClustersFloodFillMethod(cv::Mat& mat, vector<cv::Rect>& preFilteredRects,
    bool addPreEnlargedAswell, bool onlyAddSmallCategory)
{
  for (int y = 0;y<mat.rows; ++y) {
    for (int x = 0; x < mat.cols; ++x) {
      uchar pixel = mat.at<uchar>(y, x);

      if (pixel > 0) { //ignore black pixels
#ifdef SIGN_IDENT_DEBUG
        //cv::imshow("4PixelClusterFind", mat);
        //cv::waitKey(100000);
#endif
        cv::Rect filledRect;
        cv::floodFill(mat, cv::Point(x,y), cv::Scalar(0), &filledRect);

        //don't add tiny rects
        if (filledRect.width < SIZE_CATEGORY_TINY_W_H || filledRect.height < SIZE_CATEGORY_TINY_W_H) {
          continue;
        }

        //small category enlarger
        if (filledRect.width > SIZE_CATEGORY_TINY_W_H &&
            filledRect.width < SIZE_CATEGORY_SMALL_W_H &&
            filledRect.height > SIZE_CATEGORY_TINY_W_H &&
            filledRect.height < SIZE_CATEGORY_SMALL_W_H)
        {
          cv::Point enlargementFactor(
              (int)(double(filledRect.width)*SIZE_CATEGORY_SMALL_ENLARGEMENT_FACTOR),
              (int)(double(filledRect.height)*SIZE_CATEGORY_SMALL_ENLARGEMENT_FACTOR));
          cv::Rect filledRect_(filledRect.tl() - enlargementFactor, filledRect.br()+enlargementFactor);
          preFilteredRects.push_back(filledRect_);

          if (addPreEnlargedAswell) {
            preFilteredRects.push_back(filledRect);
          }
        }else{
          if (!onlyAddSmallCategory) {
            preFilteredRects.push_back(filledRect);
          }
        }
      }
    }
  }
}


/*
vector<cv::Rect> SignIdentifier::findPixelClustersContoursMethod(const cv::Scalar& color, cv::Mat grayS, cv::Mat targetDraw) {
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
    cv::Rect contourOutline(upperLeft, lowerRight);
    output.emplace_back(upperLeft, lowerRight);

   if (contourOutline.width < 20 || contourOutline.height < 20) {
      continue;
    }
    cv::rectangle(targetDraw,upperLeft, lowerRight, color,1);

  }
  return output;
}
*/