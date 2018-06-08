#include "SignPlaceTest.h"
#include "TestToolkit.h"
#include "SignPlace.h"
#include <cv.h>
#include <iostream>

SignPlaceTest::SignPlaceTest()
{
  std::cout<<"Testing SignPlace";
  testConstructor();
  testIsOverlappingEnough();
}

void SignPlaceTest::testConstructor()
{
  //test sanity test
  try {
    //negative, upperL lower than lowerR
    SignPlace s(cv::Point(10,101), cv::Point(100,100),0);
    throw "test failed";
  }catch (...){
  }

  try {
    //negative, upperL more right than lowerR
    SignPlace s(cv::Point(101,10), cv::Point(100,100),0);
    throw "test failed";
  }catch (...){
  }

  //positive
  SignPlace s1(cv::Point(10,10), cv::Point(100,100),0);


  //test area, positive
  SignPlace s2(cv::Point(10,10), cv::Point(20,20),0);
  TestToolkit<int>::assertEqualFloat(s2.area, 100.0f);
}


void SignPlaceTest::testIsOverlappingEnough()
{
  //positive, good overlap
  SignPlace s(cv::Point(10,10), cv::Point(100,100),0);
  SignPlace s2(cv::Point(20,20), cv::Point(100,100),0);
  TestToolkit<int>::assertTrue(s.isOverlappingEnough(s2));

  //negative, poor overlap
  SignPlace s3(cv::Point(99,99), cv::Point(200,200),0);
  TestToolkit<int>::assertFalse(s.isOverlappingEnough(s3));

  //negative, 100% overlap, size difference too big
  SignPlace s4(cv::Point(0,0), cv::Point(100,100),0);
  SignPlace s5(cv::Point(0,0), cv::Point(300,300),0);
  TestToolkit<int>::assertFalse(s4.isOverlappingEnough(s5));

  //negative, 100% overlap, size difference too small
  TestToolkit<int>::assertFalse(s5.isOverlappingEnough(s4));

  //negative, no overlap, to the right
  SignPlace s6(cv::Point(100,100), cv::Point(200,200),0);
  SignPlace s7(cv::Point(201,100), cv::Point(300,100),0);
  TestToolkit<int>::assertFalse(s6.isOverlappingEnough(s7));

  //negative, no overlap, to the left
  TestToolkit<int>::assertFalse(s7.isOverlappingEnough(s6));

  //negative, no overlap, to the top
  SignPlace s9(cv::Point(100,0), cv::Point(200,99),0);
  TestToolkit<int>::assertFalse(s6.isOverlappingEnough(s9));

  //negative, no overlap, to the bottom
  TestToolkit<int>::assertFalse(s9.isOverlappingEnough(s6));
}