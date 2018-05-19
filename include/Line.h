#pragma once

#include <memory>
#include <opencv2/core/mat.hpp>

/**
 * A line represents a string of points in car-coordinates.
 *
 * A line instance shares its data. So a implicit copy will
 * point to the same line and modify both.
 *
 * To copy a line use Line::clone()
 */

class Line {
  friend class RoadFilter;
public:
  typedef cv::Point2f Point;
  typedef cv::Point2f Direction;
  struct JoinInfo {
    bool joinable;
    double cost;
    cv::Scalar distance;

    bool parallel;
    double intersectCoeff;
    double otherIntersectCoeff;
  };

  enum Type {
    Unkown,
    Continuous,
    Dashed,
  };

private:
  struct State {
    double partContinuous;
    double partDashed;

    cv::Mat_<Point> points;
    uint32_t number;

    bool valid; // Next fields can be invalidated due to operations

    double length;    // Length of line
    double curvature; // Number of points / line Length
    // double avgAngle;  // Average of all line segment angles

    Direction directionFirst;
    Direction directionLast;
    Direction directionOverall;

    float baselineIntersection;
    float baselineDistance;
  };

public:
  /**
   * Construct an empty line.
   *
   * @param params Params to use for operations.
   */
  Line();

  /**
   * Add a point to the end of this line.
   *
   * @param point Point to add.
   */
  void add(Point point);

  /**
   * Append points of another line.
   *
   * @param line Other line.
   */
  void add(Line line);

  /**
   * Append a number of raw points.
   *
   * @param points Raw points to append.
   */
  void add(cv::Mat points);

  /**
   * Add a point to the front of the line.
   *
   * @param point Point to add.
   */
  void addFront(Point point);

  /**
   * Add a point to the back of the line.
   *
   * @param point Point to add.
   */
  void addBack(Point point);

  /**
   * Simplifies the current line.
   *
   * This tries to reduce the number of points in this
   * line, while retaining the overall shape.
   *
   * It also tries to remove some defects that make later
   * processing harder.
   */
  void simplify();

  /**
   * Attemps to split this line, if split-parameters are
   * exceeded.
   *
   * This returns an empty line, if no split is needed.
   *
   * If a non-empty line is returned, the returned line
   * should be split as well.
   *
   * @returns a new line
   */
  Line trySplit();

  /**
   * Calculates the info needed to join this line with
   * the specified line.
   *
   * It also determines if joining is possible (according to the params)
   *
   * @param other Other line to join with
   * @returns JoinInfo for the possible join.
   */
  const JoinInfo calculateJoinInfo(const Line &other);

  /**
   * Joins this line with another line.
   *
   * The JoinInfo must be joinable.
   *
   * @param other Line to join with.
   * @param info JoinInfo calculated with calculateJoinInfo
   */
  void join(const Line &other, const JoinInfo &info);

  /**
   * Number of points in this line.
   *
   * @returns Number of points.
   */
  size_t size() const;

  /**
   * Checks if line is empty (size() < 2)
   *
   * @returns True if size() < 2
   */
  bool isEmpty() const;

  /**
   * Get the length of this line in meters.
   *
   * @returns Line length in m
   */
  double getLength() const;

  /**
   * Gets a measure for the curvature of this line.
   *
   * @returns curvature
   */
  double getCurvature() const;

  /**
   * Projected direction at beginning of line.
   *
   * @returns a direction
   */
  Direction getDirectionFirst() const;

  /**
   * Projected direction at end of line.
   *
   * @returns a direction
   */
  Direction getDirectionLast() const;

  /**
   * Overall direction of the line.
   *
   * @returns a direction
   */
  Direction getDirectionOverall() const;

  bool isValid() const;

  //there is a bug that causes the point matrix to get corrupted
  //check if this happens and
  //todo fix that!!!!
  bool isValid2() const;

  bool distancesToLine(const Line &other, double &min, double &max) const;

  float getBaselineIntersection() const;
  float getBaselienDistance() const;
  void invalidateState();

  Type getType() const;
  void setType(Type type);

  Point &first();
  const Point &first() const;
  Point &last();
  const Point &last() const;
  const cv::Mat_<Point> getPoints() const;

  Line shiftedLine(double amount) const;

  void forEachPoint(std::function<void(Point)>) const;
  void forEachPoint(std::function<void(Point, uint32_t)>) const;
  void forEachSegment(std::function<void(Point, Point)> func) const;
  void forEachAngle(std::function<void(Point, Point, Point)> func) const;

  bool operator==(const Line &b) const;
  Line &operator=(const Line &b);

  uint32_t getNumber() const;
  static void resetLineNumber();

private:
  mutable std::shared_ptr<State> state;
  void calculateState() const;

  static uint32_t nextLineNumber;

  bool _distancesToLine(const Line &other, double &min, double &max) const;
};
