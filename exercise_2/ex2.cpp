#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
	cv::Mat img = cv::imread("img.jpg");
	if(img.empty()) {
		std::cerr << "reading of the file not possible" << std::endl;
		return -1;
	}

	cv::resize(img, img, cv::Size(448, 336));

	// gaussian pyramid
	std::vector<cv::Mat> gaussianPyramid;
	gaussianPyramid.push_back(img);

	cv::Mat current = img.clone();
	for(int i = 0; i < 4; i++) { // 4 levels
		// apply gaussian blur
		cv::GaussianBlur(current, current, cv::Size(5, 5), 0, 0);
		// size down current
		cv::pyrDown(current, current);
		// add current as a new level
		gaussianPyramid.push_back(current);
	}

	// print gaussian pyramid
	for(int i = 0; i < gaussianPyramid.size(); i++) {
		cv::imshow("Gaussian Level " + std::to_string(i), gaussianPyramid[i]);
	}

	/*Answer: As we move down the Gaussian Pyramid, the image becomes smaller and blurrier.
	        Fine details (called high-frequencies) disappear, while the overall structure (called low frequencies)
	        like color gradients and large objects remain visible.
	*/
	// laplacian pyramid
	std::vector<cv::Mat> laplacianPyramid;

	for(int i = 0; i < gaussianPyramid.size() - 1; i++) {
		cv::Mat up;
		// size up the next image to the size of the current to be able to subtract it later
		cv::pyrUp(gaussianPyramid[i + 1], up, gaussianPyramid[i].size());
		// subtract the next image from the current
		cv::Mat res = gaussianPyramid[i] - up;
		// add level to pyramid
		laplacianPyramid.push_back(res);
	}

	// print laplacian pyramid
	for(int i = 0; i < laplacianPyramid.size(); i++) {
		cv::imshow("Laplacian Level " + std::to_string(i), laplacianPyramid[i]);

		/*Answer: Each level of the Laplacian Pyramid highlights edges and fine details that were lost between two consecutive Gaussian levels.
		        In other words, the Laplacian shows the high-frequencies of the image.
		*/

		cv::waitKey(0);
		return 0;
	}
