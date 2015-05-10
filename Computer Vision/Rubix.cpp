// Rubix.cpp : Defines the entry point for the console application.

#include <iostream>
#include <string>
//#include <stdio.h>
#include <windows.h>
#include "stdafx.h"
//#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace std;

Mat src, src_gray;
int thresh = 65;
int canny = 75;
int sizeContours = 100;
int groupTolerance = 50;
int difSlope = 5;

void thresh_callback(int, void*)
{
	Mat threshold_output, dst, cdst;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	vector<Vec2f> lines;

	/// Detect edges using Threshold
	threshold(src_gray, threshold_output, thresh, 255, THRESH_BINARY);

	/// Detect lines using Canny
	if(canny == 0) canny=1;
	Canny(src, dst, 50, 200, 3);
	cvtColor(dst, cdst, CV_GRAY2BGR);
	HoughLines(dst, lines, 1, CV_PI/180, canny, 0, 0);

	/// Find contours
	findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	//findContours(lines, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	/// Approximate contours to polygons + get bounding rects
	vector<vector<Point>> contours_poly(contours.size());
	vector<Rect> boundRect(contours.size());
	vector<Point2f> center(contours.size());
	vector<Moments> contArea(contours.size());
	vector<float> radius(contours.size());
	vector<vector<int>> cube(3, vector<int>(9));

	for(int i = 0; i<contours.size(); i++)
	{ 
		contArea[i] = moments(contours[i], false);
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
		boundRect[i] = boundingRect( Mat(contours_poly[i]) );
		minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
	}

	// Draw lines
	for(size_t i = 0; i < lines.size(); i++)
	{
		float rho = lines[i][0], theta = lines[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        pt1.x = cvRound(x0 + 1000*(-b));
        pt1.y = cvRound(y0 + 1000*(a));
        pt2.x = cvRound(x0 - 1000*(-b));
        pt2.y = cvRound(y0 - 1000*(a));
        line(cdst, pt1, pt2, Scalar(0,0,255), 3, CV_AA);
		//printf("Line[%d]: %d,%d,%d,%d\n", i, pt1.x, pt1.y, pt2.x, pt2.y);
	}

	/// Draw polygonal contour + bonding rects
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	for(int i = 0; i<contours.size(); i++)
	{
		if(contArea[i].m00>sizeContours)
		{
			Scalar color = mean(src(boundingRect(contours[i])));
			ostringstream oss;
			oss<< i;
			string tmpLabel = oss.str();
			putText(drawing, tmpLabel, center[i], FONT_HERSHEY_PLAIN, 2, Scalar(255,255,255));

			approxPolyDP(Mat(contours[i]), contours_poly[i], arcLength(Mat(contours[i]), true)*0.02, true);
			drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
			//rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );

			printf("Line[%d]:\n", i);
			for(int j = 0; j<contours_poly[0].size(); j++)
			{
				printf("%d ", contours_poly[i][j]);
			}
			printf("\n");

			bool isAdded = false;

			for(int j = 0; j < 3; j++)
			{
				if(cube[j][0] != NULL)
				{
					int width = boundRect[cube[j][0]].width;
					int height = boundRect[cube[j][0]].height;
					int dif = abs(boundRect[i].width - width) + abs(boundRect[i].height - height);
					if(dif < groupTolerance)
					{
						for(int k = 0; k < 9; k++)
						{
							if(cube[j][k] == NULL)
							{
								cube[j][k] = i;
								isAdded = true;
								break;
							}
						}
					}
				}
			}

			if(!isAdded)
			{
				for(int j = 0; j < 3; j++)
				{
					if(cube[j][0] == NULL)
					{
						cube[j][0] = i;
						isAdded = true;
						break;
					}
				}
			}
			if(!isAdded)
			{
				for(int j = 0; j < 3; j++)
				{
					for(int k = 0; k < 9; k++)
					{
						if(cube[j][k] == NULL)
						{
							cube[j][k] = i;
							isAdded = true;
							break;
						}
					}
					if(isAdded) break;
				}
			}
		}
	}

	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 9; j++)
		{
			printf("%d ",cube[i][j]);
		}
		printf("\n");
	}
	printf("\n\n");

	/// Show in a window
	namedWindow( "Contours", CV_WINDOW_AUTOSIZE );
	imshow( "Contours", drawing );

	namedWindow( "Edges", CV_WINDOW_AUTOSIZE );
	imshow( "Edges", cdst );
}

int main( int argc, char** argv )
{
	/// Load source image and convert it to gray
	src = imread("images/rubix.png");

	/// Convert image to gray and blur it
	cvtColor( src, src_gray, CV_BGR2GRAY );
	blur( src_gray, src_gray, Size(3,3) );

	/// Create Window
	char* source_window = "Source";
	namedWindow( source_window, CV_WINDOW_AUTOSIZE );
	imshow( source_window, src );

	createTrackbar( " Threshold:", "Source", &thresh, 255, thresh_callback );
	createTrackbar( " Canny:", "Source", &canny, 255, thresh_callback );
	createTrackbar( " Size:", "Source", &sizeContours, 10000, thresh_callback );
	createTrackbar( " Tolerance:", "Source", &groupTolerance, 100, thresh_callback );
	createTrackbar( " Slope:", "Source", &difSlope, 100, thresh_callback );
	thresh_callback( 0, 0 );

	waitKey(0);
	return(0);
}