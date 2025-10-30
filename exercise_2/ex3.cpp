#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
	cv::Mat img = cv::imread("img.jpg");
	if(img.empty()) {
		std::cerr << "reading of the file not possible" << std::endl;
		return -1;
	}

	cv::resize(img, img, cv::Size(448, 336));

	// Define 4 orientations (0, 45, 90, 135 degrees)
	std::vector<double> orientations = {0, CV_PI / 4, CV_PI / 2, 3 * CV_PI / 4};

	// Vector for storing individual Gabor filter responses
	std::vector<cv::Mat> responses;

	for(double theta : orientations) {
		// Create Gabor kernel
		cv::Mat kernel = cv::getGaborKernel(cv::Size(31, 31), 4.0, theta, 10.0, 0.5, 0, CV_32F);

		// Apply filter
		cv::Mat filtered;
		cv::filter2D(img, filtered, CV_32F, kernel);

		responses.push_back(filtered);
	}

	// Combine using per-pixel maximum across all orientations
	cv::Mat combined = responses[0].clone();
	for(int i = 1; i < responses.size(); i++) {
		cv::max(combined, responses[i], combined);
	}

	// Normalize and convert to displayable format
	cv::normalize(combined, combined, 0, 255, cv::NORM_MINMAX);
	combined.convertTo(combined, CV_8U);

	// Display individual responses
	for(int i = 0; i < responses.size(); i++) {
		cv::Mat display;
		cv::normalize(responses[i], display, 0, 255, cv::NORM_MINMAX);
		display.convertTo(display, CV_8U);
		cv::imshow("Gabor Orientation " + std::to_string(i), display);
	}

	// Display final combined result
	cv::imshow("Combined Gabor Response", combined);
	cv::waitKey(0);

	return 0;
}
