#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
	std::vector<std::string> images = {"img.jpg", "img2.jpg"};

	for(auto& path : images) {
		cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
		if(img.empty()) {
			std::cerr << "reading of the file not possible" << std::endl;
			return -1;
		}

		cv::resize(img, img, cv::Size(448, 336));

		// apply Gaussian Blur to reduce noise
		cv::Mat blurred;
		cv::GaussianBlur(img, blurred, cv::Size(5, 5), 1.0);

		// compute Otsu threshold
		cv::Mat thresh;
		double otsu_thresh_val = cv::threshold(blurred, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
		double highThresh = otsu_thresh_val;
		double lowThresh = otsu_thresh_val * 0.5;

		// apply Canny using Otsu thresholds
		cv::Mat edgesCanny;
		cv::Canny(blurred, edgesCanny, lowThresh, highThresh);

		// apply other detectors for comparison
		cv::Mat sobelX, sobelY, sobel, laplacian;
		cv::Sobel(blurred, sobelX, CV_16S, 1, 0);
		cv::Sobel(blurred, sobelY, CV_16S, 0, 1);
		cv::convertScaleAbs(sobelX + sobelY, sobel);

		cv::Laplacian(blurred, laplacian, CV_16S);
		cv::convertScaleAbs(laplacian, laplacian);

		// print results
		cv::imshow("Original - " + path, img);
		cv::imshow("Canny (Otsu) - " + path, edgesCanny);
		cv::imshow("Sobel - " + path, sobel);
		cv::imshow("Laplacian - " + path, laplacian);
	}

	cv::waitKey(0);
	return 0;
}
