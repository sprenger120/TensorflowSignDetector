#include "Line.h"

#include <opencv2/imgproc/imgproc.hpp>


#include "Utils.h"

Line::Line()
    : state(std::make_shared<State>()){
  this->state->valid = false;
  this->state->number = Line::nextLineNumber++;
}

void Line::add(Point point) {
  this->state->points.push_back(point);

  this->invalidateState();
}

void Line::add(Line line) { this->add(line.state->points); }

void Line::add(cv::Mat points) {
  this->state->points.push_back(points);

  this->invalidateState();
}

void Line::addFront(Point point) {
  cv::Mat_<Point> points;

  points.push_back(point);
  points.push_back(this->state->points);

  this->state->points = points;

  this->invalidateState();
}

void Line::addBack(Point point) {
  this->state->points.push_back(point);
  this->invalidateState();
}


void Line::simplify() {
  if (size() < 2) {
    // cant simplify when there are too little points
    return;
  }
  cv::Mat_<Point> temp;

  //todo remove magic number for lineApproxEps
  cv::approxPolyDP(this->state->points, temp, 1 /*params.lineApproxEps*/, false);

  this->state->points.release();

  this->state->points.push_back(temp(0));

  for (uint32_t i = 1; i < temp.rows; i++) {
    auto &last = this->state->points(this->state->points.rows - 1);
    auto cur = temp(i);
    //todo remove magic number
    if (cv::norm(last - cur) >= 20/*this->params.lineMinDistance*/) {
      this->state->points.push_back(cur);
    }
  }

  this->invalidateState();
}

Line Line::trySplit() {
  Line newLine;

  // Need at least 3 points to split
  if (this->size() < 3) {
    return newLine;
  }

  auto lastLastPoint = this->state->points(0);
  auto lastPoint = this->state->points(1);
  double rollingAngle = std::numeric_limits<double>::quiet_NaN();
  double decay = 0.5;

  for (int i = 2; i < this->state->points.rows; i++) {
    auto curPoint = this->state->points(i);

    bool split = false;
    double angle =
        Utils::calcAngle(curPoint - lastPoint, lastPoint - lastLastPoint);

    // Single angle is way to high, split.
    if (std::abs(angle) >= 30.0) {
      split = true;
    }

    if (std::isnan(rollingAngle)) {
      rollingAngle = angle;
    }

    // Does not fit previous angles
    if (std::abs(rollingAngle - angle) >= 15.0) {
      split = true;
    }

    // We will split this line at point i-1.
    // Both lines will contain point i-1 after this operation
    if (split) {
      newLine.add(
          this->state->points.rowRange(i - 1, this->state->points.rows));
      this->state->points = this->state->points.rowRange(0, i);
      break;
    }

    rollingAngle = decay * rollingAngle + (1 - decay) * rollingAngle;
    lastLastPoint = lastPoint;
    lastPoint = curPoint;
  }

  this->invalidateState();

  newLine.setType(this->getType());
  this->setType(this->getType());

  return newLine;
}

const Line::JoinInfo Line::calculateJoinInfo(const Line &other) {
  JoinInfo info = {0};

  info.cost = DBL_MAX;

  double angle =
      Utils::calcAngle(this->getDirectionLast(), other.getDirectionFirst());

  // ROS_INFO("Angle %d|%d: %f", this->getNumber(), other.getNumber(), angle);

  if (!Utils::checkRange(150.0, std::abs(angle), 210.0)) {
    // ROS_INFO("Reject #1 %d|%d: %f", this->getNumber(), other.getNumber(),
    //          angle);
    info.joinable = false;

    return info;
  }

  // Lines are almost parallel, need special joining for these
  if (Utils::checkRange(172.0, std::abs(angle), 188.0)) {
    info.parallel = true;

    // Rotate b direction 90 deg and calculate distance
    Direction rotatedOther(other.getDirectionFirst().y,
                           other.getDirectionFirst().x);

    // TODO: HOw can this happen?
    if (!Utils::findIntersections(
            this->last(), this->getDirectionLast(), other.first(), rotatedOther,
            info.intersectCoeff, info.otherIntersectCoeff)) {
      info.joinable = false;

      return info;
    }

    info.joinable = true;
    info.distance[0] = cv::norm(this->getDirectionLast()) * info.intersectCoeff;
    info.distance[1] = cv::norm(rotatedOther) * info.otherIntersectCoeff;
    // Joining against the direction is more costly
    info.cost = std::abs(info.distance[0]) + 2.0 * std::abs(info.distance[1]);

    return info;
  }

  Utils::findIntersections(this->last(), this->getDirectionLast(),
                           other.first(), other.getDirectionFirst(),
                           info.intersectCoeff, info.otherIntersectCoeff);

  double lower = -0.15;
  double upper = 0.50;

  // Don't joinf when to far way
  if (!Utils::checkRange(lower, info.intersectCoeff, upper) ||
      !Utils::checkRange(lower, info.otherIntersectCoeff, upper)) {
    // ROS_INFO("Reject #2 %d|%d: %f, %f", this->getNumber(), other.getNumber(),
    //          info.intersectCoeff, info.otherIntersectCoeff);
    info.joinable = false;

    return info;
  }

  // Joing is possible and within parameters
  info.joinable = true;
  info.parallel = false;
  info.distance[0] = std::abs(cv::norm(this->last() - other.first()));
  info.distance[1] = 0;
  info.cost = info.distance[0];

  return info;
}

void Line::join(const Line &other, const JoinInfo &info) {
  // Don't pass an unjoinable info
  assert(info.joinable);
  // Don't join with ourself
  assert(this->state != other.state);
  // Don't join with an empty line
  assert(other.getPoints().rows > 0);

  // Account for the type
  this->state->partContinuous += other.state->partContinuous;
  this->state->partDashed += other.state->partDashed;

  if (info.parallel) {
    // Remove this->last() and other.first() replace with midPoint
    Point mid = this->last() + 0.5 * (other.first() - this->last());

    this->last() = mid;
    this->add(other.getPoints().rowRange(1, other.getPoints().rows));
  } else if (true) { // TODO: This seems more reasonable for most cases
    this->add(this->last() + 0.5 * (other.first() - this->last()));
    this->add(other);
  } else if (info.intersectCoeff <= 0.0 && info.otherIntersectCoeff <= 0.0) {
    // Remove a.last() and b.first() insert actual intersection point and then
    // merge
    this->last() =
        this->last() + this->getDirectionLast() * info.intersectCoeff;
    this->add(other.getPoints().rowRange(1, other.getPoints().rows));
  } else if (info.intersectCoeff <= 0.0 && info.otherIntersectCoeff > 0.0) {
    // Replace a.last() with intersection point
    this->last() =
        this->last() + this->getDirectionLast() * info.intersectCoeff;
    this->add(other);
  } else if (info.intersectCoeff > 0.0 && info.otherIntersectCoeff <= 0.0) {
    // Replace b.first() with intersection point
    this->add(this->last() + this->getDirectionLast() * info.intersectCoeff);
    this->add(other.getPoints().rowRange(1, other.getPoints().rows));
  } else if (info.intersectCoeff > 0.0 && info.otherIntersectCoeff > 0.0) {
    // Add intersection point
    this->add(this->last() + this->getDirectionLast() * info.intersectCoeff);
    this->add(other);
  } else {
    //ROS_INFO("unreached");
  }

  this->simplify();
  this->invalidateState();
}

size_t Line::size() const { return this->state->points.rows; }

bool Line::isEmpty() const { return this->size() < 2; }

double Line::getLength() const {
  this->calculateState();

  return this->state->length;
}

double Line::getCurvature() const {
  this->calculateState();

  return this->state->curvature;
}

Line::Direction Line::getDirectionFirst() const {
  this->calculateState();

  return this->state->directionFirst;
}

Line::Direction Line::getDirectionLast() const {
  this->calculateState();

  return this->state->directionLast;
}

Line::Direction Line::getDirectionOverall() const {
  this->calculateState();

  return this->state->directionOverall;
}

bool Line::distancesToLine(const Line &other, double &min, double &max) const {
  bool found = false;
  double _min;
  double _max;

  // We need to do that in both cases to get all possible combinations.

  if (this->_distancesToLine(other, _min, _max)) {
    found = true;

    min = _min;
    max = _max;
  }

  if (other._distancesToLine(*this, _min, _max)) {
    found = true;

    min = std::min(min, _min);
    max = std::max(max, _max);
  }

  return found;
}

bool Line::_distancesToLine(const Line &other, double &min, double &max) const {
  bool found = false;

  min = 1000000;
  max = 0;

  for (size_t i = 0, j2 = 0; i < this->size() - 1 && j2 < other.size() - 1;
       i++) {
    Point a = this->state->points(i);
    Direction dirA = this->state->points(i + 1) - a;

    for (size_t j = j2; j < other.size() - 1; j++) {
      Point b = other.state->points(j);
      Direction dirB = other.state->points(j + 1) - b;
      double resultA, resultB;

      // Flip dirB
      Direction temp = dirB;
      dirB.x = -temp.y;
      dirB.y = temp.x;

      dirB /= cv::norm(dirB);

      if (Utils::findIntersections(a, dirA, b, dirB, resultA, resultB)) {
        // Found intersection

        // It's within this segment
        if (Utils::checkRange(0, resultA, 1.0)) {
          double dist = cv::norm(resultB * dirB);

          if (dist < min) {
            min = dist;
          }
          if (dist > max) {
            max = dist;
          }

          found = true;
          j2 = j;
          // break;
        }
      }
    }
  }

  return found;
}

float Line::getBaselineIntersection() const {
  this->calculateState();

  return this->state->baselineIntersection;
}

float Line::getBaselienDistance() const {
  this->calculateState();

  return this->state->baselineDistance;
}

void Line::invalidateState() { this->state->valid = false; }

Line::Type Line::getType() const {
  if (this->state->partContinuous == 0 && this->state->partDashed == 0) {
    return Type::Unkown;
  }

  if (this->state->partContinuous > this->state->partDashed) {
    return Type::Continuous;
  } else {
    return Type::Dashed;
  }
}

void Line::setType(Line::Type type) {
  switch (type) {
  case Type::Unkown:
    this->state->partContinuous = 0.0;
    this->state->partDashed = 0.0;
    break;
  case Type::Continuous:
    this->state->partContinuous = this->getLength();
    this->state->partDashed = 0.0;
    break;
  case Type::Dashed:
    this->state->partContinuous = 0.0;
    this->state->partDashed = this->getLength();
    break;
  }
}

Line::Point &Line::first() { return this->state->points(0); }

const Line::Point &Line::first() const { return this->state->points(0); }

Line::Point &Line::last() {
  return this->state->points(this->state->points.rows - 1);
}

const Line::Point &Line::last() const {
  return this->state->points(this->state->points.rows - 1);
}

const cv::Mat_<Line::Point> Line::getPoints() const {
  return this->state->points;
}

Line Line::shiftedLine(double amount) const {
  Line line;

  if (this->isEmpty()) {
    return line;
  }

  // Do first point manually
  line.add(Utils::shiftedPoint(this->state->points(0), this->state->points(1),
                               amount));

  this->forEachAngle([&](Point prev, Point cur, Point next) {
    line.add(Utils::shiftedPoint(cur, prev, next, amount));
  });

  // Do last point manually
  line.add(Utils::shiftedPoint(
      this->state->points(this->state->points.rows - 1),
      this->state->points(this->state->points.rows - 2), -amount));

  return line;
}

void Line::forEachPoint(std::function<void(Point)> func) const {
  this->forEachPoint([&func](Point p, uint32_t idx) { func(p); });
}

void Line::forEachPoint(std::function<void(Point, uint32_t)> func) const {
  for (int i = 0; i < this->state->points.rows; i++) {
    func(this->state->points(i), i);
  }
}

void Line::forEachSegment(std::function<void(Point, Point)> func) const {
  if (this->state->points.rows < 2) {
    return;
  }

  auto lastPoint = this->state->points(0);

  for (int i = 1; i < this->state->points.rows; i++) {
    auto curPoint = this->state->points(i);

    func(lastPoint, curPoint);
    lastPoint = curPoint;
  }
}

void Line::forEachAngle(std::function<void(Point, Point, Point)> func) const {
  if (this->state->points.rows < 3) {
    return;
  }

  auto lastLastPoint = this->state->points(0);
  auto lastPoint = this->state->points(1);

  for (int i = 2; i < this->state->points.rows; i++) {
    auto curPoint = this->state->points(i);

    func(lastLastPoint, lastPoint, curPoint);
    lastLastPoint = lastPoint;
    lastPoint = curPoint;
  }
}

bool Line::operator==(const Line &b) const { return this->state == b.state; }

Line &Line::operator=(const Line &b) {
  this->state = b.state;
  return *this;
}

void Line::calculateState() const {
  // Stats are still valid, skip
  if (this->state->valid) {
    return;
  }

  double length = 0.0;

  this->forEachSegment(
      [&length](Point a, Point b) { length += std::abs(cv::norm(a - b)); });

  this->state->length = length;
  this->state->curvature = this->state->points.rows / length;

  if (this->state->points.rows < 2) {
    this->state->directionFirst = Point(0, 0);
    this->state->directionLast = Point(0, 0);
    this->state->baselineIntersection = std::numeric_limits<float>::quiet_NaN();
    this->state->baselineDistance = std::numeric_limits<float>::quiet_NaN();
  } else {
    Point a;
    Point b;

    a = this->state->points(0);
    b = this->state->points(1);

    this->state->directionFirst = (a - b) / cv::norm(a - b);

    a = this->state->points(this->state->points.rows - 1);
    b = this->state->points(this->state->points.rows - 2);

    this->state->directionLast = (a - b) / cv::norm(a - b);

    a = this->state->points(0);
    b = this->state->points(this->state->points.rows - 1);

    this->state->directionOverall = (a - b) / cv::norm(a - b);

    double coeff1;
    double coeff2;

    if (Utils::findIntersections(Point(0, 0), Point(0, 1), this->last(),
                                 this->state->directionLast, coeff1, coeff2)) {
      this->state->baselineIntersection = coeff1;
      this->state->baselineDistance = coeff2;
    } else {
      this->state->baselineIntersection =
          std::numeric_limits<float>::quiet_NaN();
      this->state->baselineDistance = std::numeric_limits<float>::quiet_NaN();
    }
  }

  this->state->valid = true;
}

uint32_t Line::getNumber() const { return this->state->number; }

void Line::resetLineNumber() { nextLineNumber = 0; }

bool Line::isValid() const {return this->state->valid;}

uint32_t Line::nextLineNumber = 0;

bool Line::isValid2() const {
  return  state->points.size().width > 0 &&
      state->points.size().height > 0;
}