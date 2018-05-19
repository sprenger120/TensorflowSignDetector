#include "Utils.h"

#include <opencv2/imgproc.hpp>

namespace Utils {


double calcAngle(Line::Direction a, Line::Direction b) {
  // left direction is neg angle
  // rigth direction is pos angle
  // ROS_INFO("vecAngle %f|%f \\/ %f|%f", src_vec.y, src_vec.x, trgt_vec.y,
  //          trgt_vec.x);
  double angle = (std::atan2(a.y, a.x) - std::atan2(b.y, b.x)) * 180.0 / CV_PI;
  return (-1.0 * angle);
}

double calcAngle180(Line::Direction a, Line::Direction b) {
  double angle = calcAngle(a, b);

  while (angle > 180.0) {
    angle -= 360.0;
  }

  while (angle < -180.0) {
    angle += 360.0;
  }

  return angle;
}

double calcFitness(double value, double good, double neutral,
                   double deadpointHigh, double deadpointLow) {

  if (value < deadpointLow || value > deadpointHigh) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  double x =
      (-2.0) / (neutral - good) * value - neutral * (-2.0) / (neutral - good);

  return std::tanh(x);
}

Line::Point shiftedPoint(Line::Point basePoint, Line::Point nextPoint,
                         double amount) {
  Line::Direction direction = nextPoint - basePoint;
  Line::Direction normal(direction.y, -direction.x);

  normal /= cv::norm(normal);

  return basePoint + amount * normal;
}

Line::Point shiftedPoint(Line::Point basePoint, Line::Point prevPoint,
                         Line::Point nextPoint, double amount) {
  return shiftedPoint(basePoint, nextPoint, amount);
}

bool findIntersections(Line::Point a, Line::Direction aDir, Line::Point b,
                       Line::Direction bDir, double &resultA, double &resultB) {
  double cross = aDir.x * bDir.y - aDir.y * bDir.x;

  if (std::abs(cross) < 0.00001) {
    return false;
  }

  cv::Point2d x1 = b - a;
  resultA = (x1.x * bDir.y - x1.y * bDir.x) / cross;
  cv::Point2d x2 = a - b;
  resultB = (x1.x * aDir.y - x1.y * aDir.x) / cross;

  return true;
}
}
