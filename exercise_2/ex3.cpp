#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
	cv::Mat img = cv::imread("img.jpg");
	if(img.empty()) {
		std::cerr << "Reading of the file not possible" << std::endl;
		return -1;
	}

	cv::resize(img, img, cv::Size(448, 336));

	// 4 orientations (0°, 45°, 90°, 135°)
	std::vector<double> orientations = {0, CV_PI / 4, CV_PI / 2, 3 * CV_PI / 4};

	// Different parameter sets: {sigma, wavelength, aspect_ratio}
	std::vector<std::tuple<double, double, double>> params = {{2.0, 5.0, 0.5}, {4.0, 10.0, 0.5}, {6.0, 20.0, 0.8}};

	for(int p = 0; p < params.size(); p++) {
		double sigma = std::get<0>(params[p]);
		double wavelength = std::get<1>(params[p]);
		double aspect = std::get<2>(params[p]);

		std::vector<cv::Mat> responses;

		for(double theta : orientations) {
			// Create Gabor kernel
			cv::Mat kernel = cv::getGaborKernel(cv::Size(31, 31), sigma, theta, wavelength, aspect, 0, CV_32F);

			// Apply filter
			cv::Mat filtered;
			cv::filter2D(img, filtered, CV_32F, kernel);
			responses.push_back(filtered);
		}

		// Combine using per-pixel maximum
		cv::Mat combined = responses[0].clone();
		for(int i = 1; i < responses.size(); i++) {
			cv::max(combined, responses[i], combined);
		}

		// Normalize for display
		cv::normalize(combined, combined, 0, 255, cv::NORM_MINMAX);
		combined.convertTo(combined, CV_8U);

		// Show the combined result for this parameter set
		cv::imshow("Combined Gabor Parameter set " + std::to_string(p), combined);
	}

	/*
	    The three main parameters of the Gabor filter affect the output in different ways:

	    Sigma – Controls the width of the Gaussian envelope.
	       - A larger sigma -> Coarser features; I.e. edges appear smoother and more blurred.
	       - A smaller sigma -> Fine details; I.e. edges are sharper and less blurred.

	    Wavelength – Determines the frequency of the sinusoidal component.
	       - A larger wavelength -> Broader patterns. I.e. only larger textures are highlighted.
	       - A smaller wavelength -> Fine patterns. I.e. small, closely spaced textures are emphasized.

	    Aspect Ratio – Defines the ellipticity of the Gaussian envelope.
	       - γ < 1 -> Enhanced sensitivity to a specific orientation. I.e. the features are more edgy as there are less curved
	   lines.
	       - γ = 1 -> Equally sensitive to all orientations. I.e. the lines are less edgy as there are curved lines.
	*/

	cv::waitKey(0);
	return 0;
}
