#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
	cv::Mat img = cv::imread("img.jpg");
	if(img.empty()) {
		std::cerr << "reading of the file not possible" << std::endl;
		return -1;
	}

	cv::Mat resized;
	cv::resize(img, resized, cv::Size(600, 400));

	// points in the original
	std::vector<cv::Point2f> src = {{100, 100}, {500, 100}, {100, 400}, {500, 400}};

	// points in the perspective
	std::vector<cv::Point2f> perspective = {{200, 120}, {400, 120}, {80, 400}, {520, 400}};

	// Answer: The transformation shows the original picture in the form of a trapezoid, which by definition has two non-parallel sides. This wouldn´t be
	// possible with an affine transformation, since affine transformations always preserve parallel lines.

	cv::Mat M = cv::getPerspectiveTransform(src, perspective);
	cv::Mat dst;
	cv::warpPerspective(resized, dst, M, resized.size());

	cv::imshow("Original", resized);
	cv::imshow("Perspective Transform", dst);
	cv::waitKey(0);

	return 0;
}
