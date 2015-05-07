// Computer Vision.cpp : Defines the entry point for the console application.

#include <stdio.h>
#include "stdafx.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

void createWindow(string windowTitle, Mat image)
{
	//imwrite("images/edited/" + windowTitle + ".jpg", image);
	namedWindow(windowTitle);
	imshow(windowTitle,image);
}

void convolution_image(Mat &input, int kernel_size, string windowTitle)
{
	Mat convolution,kernel;
	kernel = Mat::ones( kernel_size, kernel_size, CV_32F )/ (float)(kernel_size*kernel_size);
	filter2D(input, convolution, -1 , kernel, Point( -1, -1 ), 0, BORDER_DEFAULT );
	createWindow(windowTitle,convolution);
}

Mat erosion_dilation(int elem, int size)
{
	int type;
	if( elem == 0 ){ type = MORPH_RECT; }
	else if( elem == 1 ){ type = MORPH_CROSS; }
	else if( elem == 2) { type = MORPH_ELLIPSE; }
	return getStructuringElement(type, Size(2*size + 1, 2*size+1), Point(size, size));
}

void erosion_image(Mat &input, int erosion_elem, int erosion_size, string windowTitle)
{
	Mat erosion;
	Mat element = erosion_dilation(erosion_elem, erosion_size);
	erode(input, erosion, element);
	createWindow(windowTitle,erosion);
}

void dilation_image(Mat &input, int dilation_elem, int dilation_size, string windowTitle)
{
	Mat dilation;
	Mat element = erosion_dilation(dilation_elem, dilation_size);
	dilate(input, dilation, element);
	createWindow(windowTitle,dilation);
}

void draw(Mat hist, int histSize, Mat histImage, int bin_w, int hist_h, int rgb)
{
	Scalar a;
	switch(rgb)
	{
	case 1: a=Scalar(0,0,255);break;
	case 2: a=Scalar(0,255,0);break;
	case 3: a=Scalar(255,0,0);break;
	}
	int flag=1,first;
	for( int i = 0; i <histSize; i++ )
	{  
		if (flag==1)
		{
			if (cvRound(hist.at<float>(i)!=0))
			{
				flag=2;
				first=i;
			}
			else
			{
				i++;
			}
		}
		else
		{
			if (cvRound(hist.at<float>(i)!=0))
			{
				line( histImage, Point( bin_w*(first), hist_h - cvRound(hist.at<float>(first)) ) ,
					Point( bin_w*(i), hist_h - cvRound(hist.at<float>(i)) ),
					a, 2, 8, 0  );
				first=i;
			}
			else
			{
				i++;
			}
		}

	}
}

Mat histogram_image(vector<Mat> rgb)
{
	vector<Mat> rgb_planes;
	rgb_planes=rgb;
	int histSize = 255;
	float range[] = { 0, 255 } ;
	const float* histRange = { range };
	bool uniform = true; bool accumulate = false;
	Mat r_hist, g_hist, b_hist;

	calcHist( &rgb_planes[0], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &rgb_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &rgb_planes[2], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate );

	int bin_w = cvRound( (double) 400/histSize );

	Mat histImage(400, 400, CV_8UC3, Scalar(0,0,0));

	normalize(r_hist, r_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	normalize(g_hist, g_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

	draw(r_hist,255,histImage,bin_w,400,1);
	draw(g_hist,255,histImage,bin_w,400,2);
	draw(b_hist,255,histImage,bin_w,400,3);
	return histImage;
}

void contrast_image(Mat &input)
{
	Mat contrast, beforeHist, afterHist;
	vector<Mat> rgb_planes;
	split(input, rgb_planes);
	beforeHist = histogram_image(rgb_planes);

	equalizeHist(rgb_planes[0],rgb_planes[0]);
	equalizeHist(rgb_planes[1],rgb_planes[1]);
	equalizeHist(rgb_planes[2],rgb_planes[2]);
	afterHist = histogram_image(rgb_planes);
	merge(rgb_planes,contrast);

	createWindow("Histogram Before",beforeHist);
	createWindow("Histogram After",afterHist);
	createWindow("Contrast (Histogram)",contrast);
}

void quantization_image(Mat &input, int colors, string windowTitle)
{
	Mat quantization;
	if(input.data != quantization.data){
		quantization.create(input.size(), input.type());
	}
	uchar buffer[256];
	for(int i = 0; i != 256; ++i){
		buffer[i] = i / colors * colors + colors / 2;
	}
	Mat table(1, 256, CV_8U, buffer, sizeof(buffer));
	LUT(input, table, quantization);
	createWindow(windowTitle,quantization);
}

void contrast_image(Mat &input, int alpha, string windowTitle)
{
	Mat contrast = Mat::zeros( input.size(), input.type() );
	for(int y = 0; y < input.rows; y++) {
		for(int x = 0; x < input.cols; x++) {
			for(int c = 0; c < 3; c++) {
				contrast.at<Vec3b>(y,x)[c] = saturate_cast<uchar>(alpha * (input.at<Vec3b>(y,x)[c]));
			}
		}
	}
	createWindow(windowTitle,contrast);
}

void removal_image(Mat &input, string windowTitle)
{
	Mat smooth, smooth2;
	medianBlur(input, smooth, 7);
	createWindow(windowTitle,smooth);
}

int comp_main( int argc, char** argv )
{
	Mat image = imread("images/boldt.jpg",1);
	Mat grayscale = imread("images/boldt.jpg",0);

	/*Mat image = imread("images/tu.jpg",1);
	Mat grayscale = imread("images/tu.jpg",0);*/

	createWindow("Original RGB",image);
	createWindow("Original w/ Grayscale",grayscale);

	convolution_image(image,5,"Convolution");
	erosion_image(image,0,2,"Erosion");
	dilation_image(image,0,2,"Dilation");
	contrast_image(image);
	quantization_image(image,64,"RGB Quantization");
	quantization_image(grayscale,64,"Grayscale Quantization");
	contrast_image(image,2, "High Contrast");
	removal_image(image,"Noise Removal");

	cv::waitKey();

	return 0;
}