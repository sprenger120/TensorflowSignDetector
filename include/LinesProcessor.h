#pragma once

#include <list>
#include <opencv2/core/mat.hpp>

class DebugManager;
class Params;
class Line;

class LinesProcessor {
public:
  LinesProcessor();

  std::list<Line> process(cv::Mat input);


  std::list<Line> makeLines(cv::Mat &img);
  std::list<Line> splitLines(std::list<Line> in_lines);
  std::list<Line> joinLines(std::list<Line> lines);
};
