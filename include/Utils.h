#pragma once

#include "Line.h"


namespace Utils {

/**
 * Calculate the angle between two directions.
 *
 * Calculates the angle in degrees between the two directions.
 *
 * @param a Direction for angle
 * @param b Other direction for angle
 * @returns Angle between a and b.
 */
double calcAngle(Line::Direction a, Line::Direction b);

/**
 * Calculates the angle between two direction and normalizes it between
 * [-180,180].
 *
 * @param a Direction for angle
 * @param b Other direction for angle
 * @returns Angle between a and b.
 */
double calcAngle180(Line::Direction a, Line::Direction b);

/**
 * Calculate a fitness values between [0, 1].
 *
 * if value < deadpointLow -> NaN
 * if value > deadpointHigh -> NaN
 * if value < good -> value approaches 1.0
 * if value ~ neutral -> value approaches 0.0
 * if value > neutral -> value approaches -1.0
 *
 * Currently the middle interpolation is implemented as a sigmoid function.
 *
 * @param value to calculate fitness for.
 * @param good Value considered good.
 * @param neutral Value considered neutral.
 * @param deadpointHigh Value above that is dead (returns Nan)
 * @param deadpointLow Value below that is dead (returns NaN)
 * @returns [0, 1] or NaN
 */
double calcFitness(double value, double good, double neutral,
                   double deadpointHigh = DBL_MAX,
                   double deadpointLow = -DBL_MAX);

/**
 * Check that value is in specified range (inclusive).
 *
 * @param lower Lower bound of range
 * @param value Value to check
 * @param upper Upper bound of range
 * @returns True if lower <= value <= upper
 */
static bool checkRange(double lower, double value, double upper) {
  return lower <= value && value <= upper;
}

/**
 * Shifts the given point.
 *
 * @param basePoint point to shift
 * @param nextPoint next point to use as norm vec
 * @param amount amount to shift line
 * @returns shifted points
 */
Line::Point shiftedPoint(Line::Point basePoint, Line::Point nextPoint,
                         double amount);

/**
 * Shifts the given point.
 *
 * @param basePoint point to shift
 * @param prevPoint prev point to use as norm vec
 * @param nextPoint next point to use as norm vec
 * @param amount amount to shift line
 * @returns shifted points
 */
Line::Point shiftedPoint(Line::Point basePoint, Line::Point prevPoint,
                         Line::Point nextPoint, double amount);

/**
 * Find intersection between to lines.
 *
 * @param a Base Point of Line a
 * @param aDir Direction of Line a
 * @param b Base Point of Line b
 * @param bDir Direction of Line b
 * @param resultA aDir factor for intersect point
 * @param resultB bDir factor for intersect point
 * @returns True if calculation is possible (lines non parallel)
 */
bool findIntersections(Line::Point a, Line::Direction aDir, Line::Point b,
                       Line::Direction bDir, double &resultA, double &resultB);
};
