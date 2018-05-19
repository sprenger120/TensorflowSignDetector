#include "LinesProcessor.h"

#include "Line.h"
#include "Utils.h"

LinesProcessor::LinesProcessor()
{

}

std::list<Line> LinesProcessor::process(cv::Mat input) {
  Line::resetLineNumber();

  auto lines = this->makeLines(input);

  // Initial simplify pass, reduces point count per line greatly
  for (auto &line : lines) {
    line.simplify();
  }

  // Reject all lines deemd to small
  lines.remove_if([&](Line &l) { return l.getLength() < 0.1; });

  for (auto &line : lines) {
    if (line.getLength() < 0.35) {
      line.setType(Line::Dashed);
    } else {
      line.setType(Line::Continuous);
    }
  }

 // auto &debugAllLines = this->debugManager.getDebugImage(
  //    "all_lines", kDebugImageStyleTopview, kDebugImageLinesCategory);

  for (auto &line : lines) {
    if (line.getType() == Line::Dashed) {
   //   debugAllLines.draw(line, cv::Scalar(255, 0, 0), 4);
    } else {
     // debugAllLines.draw(line, cv::Scalar(0, 0, 255), 4);
    }
  }

  int before = lines.size();

  lines = this->splitLines(lines);

  int mid = lines.size();

  for (auto &line : lines) {
    line.simplify();
  }

  // Reject all lines deemd to small
  lines.remove_if([&](Line &l) { return l.getLength() < 0.1; });

 // auto &debugSplitLines = this->debugManager.getDebugImage(
  //    "split_lines", kDebugImageStyleTopview, kDebugImageLinesCategory);

  for (auto &line : lines) {
    if (line.getType() == Line::Dashed) {
    //  debugSplitLines.draw(line, cv::Scalar(255, 0, 0), 4);
    } else {
      //debugSplitLines.draw(line, cv::Scalar(0, 0, 255), 4);
    }
  }

  int after = lines.size();
  // ROS_INFO("SPlit %d / %d / %d", before, mid, after);

  lines = this->joinLines(lines);

  //auto &debugJoinedLines = this->debugManager.getDebugImage(
   //   "join_lines", kDebugImageStyleTopview,
    //  kDebugImageGeneralCategory | kDebugImageLinesCategory);

  for (auto &line : lines) {
    if (line.getType() == Line::Dashed) {
     // debugJoinedLines.draw(line, cv::Scalar(255, 0, 0), 4);
    } else {
      //debugJoinedLines.draw(line, cv::Scalar(0, 0, 255), 4);
    }
  }

  return lines;
}

std::list<Line> LinesProcessor::makeLines(cv::Mat &img) {
  std::list<Line> lines;

  cv::Size size = img.size();
  cv::Size imgSize = img.size();
  cv::Point imagePoint(0, 0);

  // Can be treated as continues array
  if (img.isContinuous()) {
    size.width *= size.height;
    size.height = 1;
  }

  for (int i = 0; i < size.height; i++) {
    const uchar *ptr = img.ptr<uchar>(i);

    for (int j = 0; j < size.width; j++, ptr++) {
      if (*ptr != 0) {
        Line::Point point(imagePoint);
        Line *nearestLine = NULL;
        bool nearestFront = false;
        float nearestDistance = FLT_MAX;

        for (auto &line : lines) {
          float dist = cv::norm(line.last() - point);

          if (dist < nearestDistance) {
            nearestLine = &line;
            nearestDistance = dist;
            nearestFront = false;
          } else {
            dist = cv::norm(point - line.first());

            if (dist < nearestDistance) {
              nearestLine = &line;
              nearestDistance = dist;
              nearestFront = true;
            }
          }
        }

        //todo remove magic number for lineAppendDistance
        if (nearestDistance >= 5) {
          lines.push_back(Line());
          nearestLine = &lines.back();
        }

        if (nearestFront) {
          nearestLine->addFront(point);
        } else {
          nearestLine->add(point);
        }
      }

      // Book keeping for current row/column
      imagePoint.x++;
      if (imagePoint.x >= imgSize.width) {
        imagePoint.x = 0;
        imagePoint.y++;
      }
    }
  }

  return lines;
}

std::list<Line> LinesProcessor::splitLines(std::list<Line> in_lines) {
  std::list<Line> lines;
  std::list<Line> candidates(in_lines);

  while (!candidates.empty()) {
    Line line = candidates.front();

    candidates.pop_front();
    lines.push_back(line);

    Line possNew = line.trySplit();

    if (possNew.size() > 0) {
      candidates.push_back(possNew);
    }
  }

  return lines;
}

std::list<Line> LinesProcessor::joinLines(std::list<Line> lines) {
  typedef std::tuple<Line, Line, Line::JoinInfo> Candidate;

  // TODO: this seems to be somewhat exessive.
  bool didJoin = false;
  uint32_t joinPasses = 0;
  do {
    didJoin = false;
    joinPasses++;

    std::list<Candidate> candidates;

    for (auto &line : lines) {
      Line::JoinInfo bestInfo;
      Line bestLine;

      bestInfo.joinable = false;
      bestInfo.cost = DBL_MAX;

      for (auto &other : lines) {
        if (line == other) {
          continue;
        }

        auto info = line.calculateJoinInfo(other);

        if (info.cost < bestInfo.cost) {
          bestInfo = info;
          bestLine = other;
        }
      }

      if (bestInfo.joinable &&
          Utils::checkRange(-0.05, bestInfo.distance[0], 0.50) &&
          Utils::checkRange(-0.15, bestInfo.distance[1], 0.15)) {
        candidates.push_back(Candidate(line, bestLine, bestInfo));
      }
    }

    candidates.sort([](Candidate &a, Candidate &b) {
      return std::get<2>(a).cost < std::get<2>(b).cost;
    });

    while (!candidates.empty()) {
      auto candidate = candidates.front();

      candidates.pop_front();

      auto line = std::get<0>(candidate);
      auto other = std::get<1>(candidate);
      auto info = std::get<2>(candidate);

      line.join(other, info);
      lines.remove_if([&](Line &l) { return l == other; });
      candidates.remove_if([&](Candidate &c) {
        return std::get<0>(c) == other || std::get<1>(c) == other;
      });

      didJoin = true;
    }
  } while (didJoin);

  // ROS_INFO("Did %u join passes.", joinPasses);

  return lines;
}
