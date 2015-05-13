// Rubix.cpp : Defines the entry point for the console application.

#include <iostream>
#include <string>
#include <windows.h>
#include "stdafx.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace std;

Mat src, src_gray;
int thresh = 65;
int canny = 75;
int sizeContours = 100;
int groupTolerance = 25;
int difSlope = 125;

void print_cube(vector<vector<int>> cube)
{
	for(int i = 0; i < 3; i++)
	{
		printf("Face %d\n",i);
		for(int j = 0; j < 9; j++)
		{
			printf("%d ",cube[i][j]);
		}
		printf("\n\n");
	}
	printf("\n\n");
}

bool comparator(Point2f a, Point2f b)
{
	return a.x<b.x;
}

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

	/// Approximate contours to polygons + get bounding rects
	vector<vector<Point>> contours_poly(contours.size());
	vector<Rect> boundRect(contours.size());
	vector<Point2f> center(contours.size());
	vector<Moments> contArea(contours.size());
	vector<float> radius(contours.size());
	vector<vector<int>> cube(3, vector<int>(9));
	vector<vector<float>> slopes(contours.size(), vector<float>(4));

	/// Get contour information
	for(size_t i = 0; i < contours.size(); i++)
	{ 
		contArea[i] = moments(contours[i], false);
		approxPolyDP(Mat(contours[i]), contours_poly[i], 10, true);
		boundRect[i] = boundingRect( Mat(contours_poly[i]) );
		minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
	}

	/// Set default NULL values for cube
	for(size_t i = 0; i < cube.size(); i++)
	{
		for(size_t j = 0; j < cube[0].size(); j++)
		{
			cube[i][j] = -1;
		}
	}

	/// Draw hough lines
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
	}

	/// Drawing for contour lines
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	for(size_t i = 0; i < contours.size(); i++)
	{
		if(contArea[i].m00>sizeContours)
		{
			Scalar color = mean(src(boundingRect(contours[i])));
			ostringstream oss;
			oss<< i;
			string tmpLabel = oss.str();
			putText(drawing, tmpLabel, center[i], FONT_HERSHEY_PLAIN, 2, Scalar(255,255,255));
			drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );

			vector<Point2f> top, bot;
			for(size_t j=0; j < contours_poly[i].size(); j++)
			{
				if(contours_poly[i][j].y < center[i].y)
				{
					top.push_back(contours_poly[i][j]);
				} else {
					bot.push_back(contours_poly[i][j]);
				}
			}
			sort(top.begin(),top.end(),comparator);
			sort(bot.begin(),bot.end(),comparator);

			Point2f tl = top[0];
			Point2f tr = top[top.size()-1];
			Point2f bl = bot[0];
			Point2f br = bot[top.size()-1];
			
			if(tr.x-tl.x) {
				slopes[i][0] = (tr.y-tl.y)/(tr.x-tl.x);
			}
			if(br.x-bl.x) {
				slopes[i][1] = (br.y-bl.y)/(br.x-bl.x);
			}
			if(tr.x-br.x) {
				slopes[i][2] = (tr.y-br.y)/(tr.x-br.x);
			}
			if(tl.x-bl.x) {
				slopes[i][3] = (tl.y-bl.y)/(tl.x-bl.x);
			}

			bool isAdded = false;
			for(size_t j=0; j<3; j++)
			{
				if (!isAdded && cube[j][0] != -1 && cube[j][0] != i)
				{
					float slopeDifference = 0;
					for(size_t k=0; k<4; k++)
					{
						float mySlope = slopes[i][k];
						float compareSlope = slopes[cube[j][0]][k];
						slopeDifference += (mySlope - compareSlope);
					}

					float myWidth = boundRect[i].width;
					float myHeight = boundRect[i].height;
					float compareWidth = boundRect[cube[j][0]].width;
					float compareHeight = boundRect[cube[j][0]].height;
					float widthDifference = abs(myWidth - compareWidth);
					float heightDifference = abs(myHeight - compareHeight);
					if(widthDifference < groupTolerance && heightDifference < groupTolerance && slopeDifference < difSlope)
					{
						for(size_t k=0; k<cube[j].size(); k++)
						{
							if(!isAdded && cube[j][k] == -1)
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
				for(size_t j=0; j<3; j++)
				{
					if(!isAdded && cube[j][0] == -1)
					{
						cube[j][0] = i;
						isAdded = true;
						break;
					}
				}
			}
			if(!isAdded)
			{
				for(size_t j=0; j<cube[2].size(); j++)
				{
					if(!isAdded && cube[2][j] == -1)
					{
						cube[2][j] = i;
						isAdded = true;
						break;
					}
				}
			}
		}

	}

	/// Output cube to log
	print_cube(cube);

	/// Show in a window
	namedWindow( "Contours", CV_WINDOW_AUTOSIZE );
	imshow( "Contours", drawing );

	namedWindow( "Edges", CV_WINDOW_AUTOSIZE );
	imshow( "Edges", cdst );
}

int main( int argc, char** argv )
{
	/// Load source image and convert it to gray
	src = imread("images/rubix1.png");

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
	createTrackbar( " Slope:", "Source", &difSlope, 255, thresh_callback );
	thresh_callback( 0, 0 );

	waitKey(0);
	return(0);
}