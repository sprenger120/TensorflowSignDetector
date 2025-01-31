#include "TrainingData.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ftw.h>
#include <fnmatch.h>
#include <opencv2/opencv.hpp>
#include <SignClassifier.h>
#include "SignIdentifier.h"

using std::cout;
using std::getline;
using std::stringstream;

#define TRAINING_DATA_DATABASE_FILE_PATH "training_data/gt_train.txt"
#define TRAINING_DATA_TRAINING_PICTURES_PATH "training_data/png/"

namespace TrainingData {
TrainingData::TrainingData() {
  // gatherTrainingDataFiles();
  _trainingData = loadTrainingData();
  _perSignOccurance = countSignOccurances();
  _areaWithSigns = determineAreaWithSignsAndMaxSignSize();

  cout << "========== Satistics\n";
  cout << "Loaded " << _trainingData.size() << " training data elements\n";
  cout << "Cropped area with signs to " << _areaWithSigns << "\n";

  cout << "=== Sign occurances in training data\n";


  //print out 10 most occuring signs
  SignOccuranceArray signOccCopy = _perSignOccurance;
  for (char i = 0; i < 20; ++i) {
    size_t biggestID = 0;

    for (size_t indx = 1; indx < signOccCopy.size(); ++indx) {
      if (signOccCopy[biggestID].cnt < signOccCopy[indx].cnt) {
        biggestID = indx;
      }
    }

    cout << "#" << biggestID << " spotted " << signOccCopy[biggestID].cnt << " times\n";
    signOccCopy[biggestID].cnt = 0;
  }

}

TrainingData::~TrainingData() {
}

vector<TrainingData::trainingDataInfo> TrainingData::loadTrainingData() const {
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

SignOccuranceArray TrainingData::countSignOccurances() const {
  SignOccuranceArray occurances;
  for (const trainingDataInfo &tr : _trainingData) {
    for (const SignPlace &sign : tr.signs) {
      //if first occurance of sign, increase occurance size
      if (sign.getSignId() + 1 >= occurances.size()) {
        occurances.resize((size_t) sign.getSignId() + 1);
      }
      occurances[sign.getSignId()].cnt++;
    }
  }
  return occurances;
}

cv::Rect TrainingData::determineAreaWithSignsAndMaxSignSize() const {
  //todo
  cv::Point upperLeft(999999, 999999); //todo this may has to be increased if training pictures get larger
  cv::Point lowerRight(0, 0);

  int biggestSignW = 0;
  int biggestSignH = 0;
  int smallestSignW = 999999;
  int smallestSignH = 999999;

  for (const trainingDataInfo &tr : _trainingData) {
    for (const SignPlace &sign : tr.signs) {
      upperLeft.y = std::min(upperLeft.y, sign.getSignPlace().tl().y);
      upperLeft.x = std::min(upperLeft.x, sign.getSignPlace().tl().x);
      lowerRight.y = std::max(lowerRight.y, sign.getSignPlace().br().y);
      lowerRight.x = std::max(lowerRight.x, sign.getSignPlace().br().x);

      biggestSignH = std::max(biggestSignH, sign.getSignPlace().height);
      biggestSignW = std::max(biggestSignW, sign.getSignPlace().width);
      smallestSignH = std::min(biggestSignH, sign.getSignPlace().height);
      smallestSignW = std::min(biggestSignW, sign.getSignPlace().width);
    }
  }

  cout<<"Smallest Sign W:"<<smallestSignW<<" H:"<<smallestSignH
      <<" | Biggest Sign W:"<<biggestSignW<<" H: "<<biggestSignH<<"\n";

  return cv::Rect(upperLeft, lowerRight);
}


const cv::Rect &TrainingData::getAreaWithSigns() const {
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

//#define GENERATE_TRAINING_DATA
void TrainingData::evaluateSignDetector(bool quick) const
{
  int processedTrainingDataEntries = 0;
  const int maxTrainingDataEntriesToProcess = 50;
  SignIdentifier detector;
  SignClassifier classy;

  int signsInTrainingExamplesTotal = 0;
  int signsIdentifiedCorrectlyTotal = 0;
  int signsIdentifiedWhereNoneAreaTotal = 0;

  //int nonBackgroundSigns = 0; //how many
 // int classifiedCorrectlyFromBackground = 0;  //distinction from background successful

  int signsWithNonOtherIds = 0; //total occurances of signs with ids seen below
  int classifiedCorrectlyNonOther = 0; //sign of ids 3,11,12,13,38 corretly classified

  int classifiedSigns = 0;
  int falsePositives = 0;


  //count of how many signs were detected across all ids
  SignOccuranceArray detectedSignTypes(_perSignOccurance.size());
  //count of how many signs where in the training data across all ids
  SignOccuranceArray trainingSignTypes(_perSignOccurance.size());

  int positiveGeneratorNumber = 0;

  for (const trainingDataInfo& trainingEntry : _trainingData) {
    //###### Load image
    cv::Mat trainingPicture = cv::imread(TRAINING_DATA_TRAINING_PICTURES_PATH+trainingEntry.filename);

    if (trainingPicture.data==nullptr) {
      cout << "Unable to load picture for training data entry '" << trainingEntry.filename << "'\n";
      continue;
    }

    //###### worked training entries statistic
    ++processedTrainingDataEntries;
    if (processedTrainingDataEntries>maxTrainingDataEntriesToProcess) {
      break;
    }



    //##### identify signs
    vector<SignPlace> detectedSigns = detector.detect(trainingPicture, _areaWithSigns);


    //#### visualize marked signs from training data
    if (!quick) {
      //draw training signs outlines, pink
      for (const SignPlace& trainingSign : trainingEntry.signs) {
        trainingSign.drawOutline(trainingPicture, cv::Scalar(255, 0, 255), true, true);
      }
    }



    //#### check identification performance #################################################################
    vector<bool> identificatorHasSpottedSign(trainingEntry.signs.size());
    signsInTrainingExamplesTotal += identificatorHasSpottedSign.size();

    //fill per sign identification statistic total
    for (size_t i = 0; i<trainingEntry.signs.size(); ++i) {
      trainingSignTypes[trainingEntry.signs[i].getSignId()].cnt++;
    }

    int signIdentifiedWhereNoneAre = 0;
    int signsIdentifiedCorrectly = 0;
    //cross check detected signs with training data
    //todo filter out multiple detection of same trainingSign
    for (SignPlace& _identifiedSign :  detectedSigns) {
      SignPlace& identifiedSign = _identifiedSign;
      bool matched = false;

      // ######### classify
      cv::Mat identifiedSignROI(trainingPicture, identifiedSign.getSignPlace());
      SignID sig = -1;
      classy.classify(identifiedSignROI, sig);
      SignPlace identifiedAndClassifiedSign(identifiedSign.getSignPlace(), sig);
      identifiedSign = identifiedAndClassifiedSign;
      classifiedSigns++;


      // draw classified sign
      if (sig != -1 && !quick) {
        SignPlace newSig(identifiedSign.getSignPlace(), sig);
        newSig.drawOutline(trainingPicture, cv::Scalar(0, 255, 0), true);
      }

      // analyze identification performance
      for (size_t currentSign = 0; currentSign<trainingEntry.signs.size(); ++currentSign) {
        if (trainingEntry.signs[currentSign].isOverlappingEnough(identifiedSign)) {
#ifdef GENERATE_TRAINING_DATA
          std::stringstream ss;
          ss<<"train_data/" << trainingEntry.signs[i].getSignId()<<"/"
             <<positiveGeneratorNumber<<".jpg";

          cv::imwrite(ss.str(), identifiedSignROI);
          positiveGeneratorNumber++;
#endif

          //mark sign in current training example as successfully spotted
          if (!identificatorHasSpottedSign[currentSign]) {
            //spotted the first sign,  do some statistics

            //per sign detection statistics
            detectedSignTypes[trainingEntry.signs[currentSign].getSignId()].cnt++;

            signsIdentifiedCorrectly++;
            identificatorHasSpottedSign[currentSign] = true;
          }

          matched = true;

          //non other sign
          if (trainingEntry.signs[currentSign].getSignId() == 3 ||
              trainingEntry.signs[currentSign].getSignId() == 11 ||
              trainingEntry.signs[currentSign].getSignId() == 12 ||
              trainingEntry.signs[currentSign].getSignId() == 13 ||
              trainingEntry.signs[currentSign].getSignId() == 38)
          {
            signsWithNonOtherIds++;
            if (trainingEntry.signs[currentSign].getSignId() == identifiedSign.getSignId()) {
              classifiedCorrectlyNonOther++;
            }
          }

          break;
        }
      }


      if (matched) {
        //correctly spotted sign will be drawn in green
        if (!quick) {
          //detecSign.drawOutline(trainingPicture, cv::Scalar(0, 255, 0));
        }
      } else {
#ifdef GENERATE_TRAINING_DATA
        positiveGeneratorNumber++;
        std::stringstream ss;
        if (processedTrainingDataEntries < 50) {
          ss << "train_data/negatives/" << positiveGeneratorNumber << ".jpg";
          cv::imwrite(ss.str(), identifiedSignROI);
        }
#endif
        if (identifiedSign.getSignId() != -1) {
          falsePositives++;
        }

        signIdentifiedWhereNoneAre++;
        //not correctly spotted, red
        if (!quick) {
          //detecSign.drawOutline(trainingPicture, cv::Scalar(0, 0, 255));
        }
      }
    }

    signsIdentifiedWhereNoneAreaTotal += signIdentifiedWhereNoneAre;
    signsIdentifiedCorrectlyTotal += signsIdentifiedCorrectly;

    cout << "Signs in example " << trainingEntry.signs.size()
         << "  Identifier got " << signsIdentifiedCorrectly
         << " signs  | Detected " << signIdentifiedWhereNoneAre <<
         "  where none are \n";

    if (!quick) {
      //draw area with signs outline
      //cv::rectangle(trainingPicture, _areaWithSigns, cv::Scalar(255, 0, 0), 2);
      //show with imgview
      cv::imshow("SignDetecc", trainingPicture);
      // waits two seconds, kills programm if esc was pressed
      while (true) {
        int k = cv::waitKey(20);
        if (k==27) {
          cv::destroyAllWindows();
          return;
        }
        if (k==32) {
          break;
        }
      }
    }

  }

  cout<<"=== Finished===\n";
  cout<<"Processed "<<processedTrainingDataEntries<<" training examples\n";

  cout << "=== Identificator performance \n";
  cout << "Identifier detect rate: " << ((signsIdentifiedCorrectlyTotal*100)/(signsInTrainingExamplesTotal)) << "% \n"
       << "Signs Detected where none are total: " << signsIdentifiedWhereNoneAreaTotal<<"\n";

  cout << "=== Classification performance \n";
  cout<< "False Positives (from all classified signs) "
      <<(float(falsePositives) / float(classifiedSigns))*100.0f<<"% total:"<<falsePositives<<"\n"

      << "Signs with ids: 3,11,12,13,38 classified correctly (seen only from identified signs) (true positives):"
      <<((classifiedCorrectlyNonOther*100) / signsWithNonOtherIds)<<"%\n";


  cout << "=== Per Sign detection performance \n";

  /*
   *
   *  //count of how many signs were detected across all ids
  SignOccuranceArray detectedSignTypes(_perSignOccurance.size());
  //count of how many signs where in the training data across all ids
  SignOccuranceArray trainingSignTypes(_perSignOccurance.size());
   */
  for (size_t indx = 0; indx<detectedSignTypes.size(); ++indx) {
    detectedSignTypes[indx].signId = indx;
    if (trainingSignTypes[indx].cnt > 0) {
      detectedSignTypes[indx].temp = float(detectedSignTypes[indx].cnt)/float(trainingSignTypes[indx].cnt);
    }else {
      detectedSignTypes[indx].temp = -100.0f;
    }
  }

  std::sort(detectedSignTypes.begin(), detectedSignTypes.end(), [](const SignCount& a, const SignCount& b) {
    return a.temp>b.temp;
  });

  for (size_t indx = 0; indx<detectedSignTypes.size(); ++indx) {
    if (detectedSignTypes[indx].temp  < 0) {
      break;
    }
    cout << "#" << detectedSignTypes[indx].signId
         << " rate:  " << (detectedSignTypes[indx].temp * 100.0f) << " % "
         <<"("<<SignIDToName(detectedSignTypes[indx].signId)<<")\n";
  }

}


const char* TrainingData::SignIDToName(size_t signId) const
{
  switch (signId) {
  case 0:
    return "Höchstgeschw. 20";
  case 1:
    return "Höchstgeschw. 30";
  case 2:
    return "Höchstgeschw. 50";
  case 3:
    return "Höchstgeschw. 60";
  case 4:
    return "Höchstgeschw. 70";
  case 5:
    return "Höchstgeschw. 80";
  case 6:
    return "Ende Höchstgeschw. 80";
  case 7:
    return "Höchstgeschw. 100";
  case 8:
    return "Höchstgeschw. 120";
  case 9:
    return "Überholverbot";
  case 10:
    return "Überholverbot >3,5t";
  case 11:
    return "Vorfahrt";
  case 12:
    return "Vorfahrtsstraße";
  case 13:
    return "Vorfahrt gewähren";
  case 14:
    return "Stop";
  case 15:
    return "Verbot für alle Fahrzeuge";
  case 16:
    return "Verbot für alle LKW";
  case 17:
    return "Verbot der Einfahrt, Gegenverkehr";
  case 18:
    return "Achtung";
  case 19:
    return "Kurve Links";
  case 20:
    return "Kurve Rechts";
  case 21:
    return "Doppelkurve";
  case 22:
    return "Unebene Fahrbahn";
  case 23:
    return "Schleudergefahr N&S";
  case 24:
    return "Rechts verengte Fahrbahn";
  case 25:
    return "Bauarbeiten";
  case 26:
    return "Ampel";
  case 27:
    return "Fußgänger";
  case 28:
    return "Kinderz";
  case 29:
    return "Fahrradverkehr";
  case 30:
    return "Schnee- od. Eisglätte";
  case 31:
    return "Wildwechsel";
  case 32:
    return "Ende der Geschwindigkeitsbegrenzung";
  case 33:
    return "Fahrtrichtung Rechts";
  case 34:
    return "Fahrtrichtung Links";
  case 35:
    return "Fahrtrichtung Geradeaus";
  case 36:
    return "Fahrtrichtung Geraudeaus+Rechts";
  case 37:
    return "Fahrtrichtung Geraudeaus+Links";
  case 38:
    return "Fahrtrichtung Rechts vorbei";
  case 39:
    return "Fahrtrichtung Links vorbei";
  case 40:
    return "Kreisverkehr";
  case 41:
    return "Ende überholverbot";
  case 42:
    return "Ende überholverbot >3.5t";
  default:
    return "unknown sign";
  }
}

}