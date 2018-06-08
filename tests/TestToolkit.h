#ifndef SIGN_DETECC_TESTTOOLKIT_H
#define SIGN_DETECC_TESTTOOLKIT_H
#include <math.h>
#include <cmath>
#define EPSILON 0.0001


//Negative tests should normally fail
//Positive tests should normally pass

template<typename CheckType>
class TestToolkit {
public:

  static void assertEqual(CheckType a, CheckType b)
  {
    if (a != b) {
      throw "assertion fail";
    }
  }

  static void assertNotEqual(CheckType a, CheckType b)
  {
    if (a == b) {
      throw "assertion fail";
    }
  }

  static void assertTrue(bool a)
  {
    if (!a) {
      throw "assertion fail";
    }
  }

  static void assertFalse(bool a)
  {
    if (a) {
      throw "assertion fail";
    }
  }


  static void assertEqualFloat(float a, float b) {
    if (std::abs(a-b) > EPSILON) {
      throw "assertion fail";
    }
  }
};
#endif //SIGN_DETECC_TESTTOOLKIT_H
