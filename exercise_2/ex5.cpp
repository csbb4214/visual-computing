#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <vector>

using namespace cv;

Mat applyRotation(const Mat& img, double angle);
Mat applyScale(const Mat& img, double scale);
Mat applyBrightness(const Mat& img, double alpha, int beta);

void testAKAZE(const Mat& img_1, const Mat& img_2, const std::string& trans_name);
void testBRISK(const Mat& img_1, const Mat& img_2, const std::string& trans_name);

int main() {
	// Read image
	Mat img_original = imread("img.jpg", IMREAD_GRAYSCALE);

	if(img_original.empty()) {
		std::cout << "Error: Could not read img.jpg" << std::endl;
		return -1;
	}

	resize(img_original, img_original, Size(448, 336));

	std::cout << "Image loaded: " << img_original.cols << "x" << img_original.rows << std::endl;
	std::cout << std::endl;

	// Apply transformations
	Mat img_rotated = applyRotation(img_original, 45);
	Mat img_scaled = applyScale(img_original, 0.7);
	Mat img_brightness = applyBrightness(img_original, 1.3, 20);

	// Test AKAZE
	std::cout << "=== AKAZE Detector ===" << std::endl;
	testAKAZE(img_original, img_rotated, "Rotation 45°");
	testAKAZE(img_original, img_scaled, "Scale 0.7x");
	testAKAZE(img_original, img_brightness, "Brightness");

	std::cout << std::endl;

	// Test BRISK
	std::cout << "=== BRISK Detector ===" << std::endl;
	testBRISK(img_original, img_rotated, "Rotation 45°");
	testBRISK(img_original, img_scaled, "Scale 0.7x");
	testBRISK(img_original, img_brightness, "Brightness");

	waitKey(0);

	return 0;
}

void testAKAZE(const Mat& img_1, const Mat& img_2, const std::string& trans_name) {
	// Create AKAZE detector
	Ptr<AKAZE> detector = AKAZE::create();

	// Detect keypoints
	std::vector<KeyPoint> keypoints_1, keypoints_2;
	detector->detect(img_1, keypoints_1);
	detector->detect(img_2, keypoints_2);

	// Compute descriptors
	Mat descriptors_1, descriptors_2;
	detector->compute(img_1, keypoints_1, descriptors_1);
	detector->compute(img_2, keypoints_2, descriptors_2);

	// Match descriptors (AKAZE uses binary descriptors)
	BFMatcher matcher(NORM_HAMMING);
	std::vector<DMatch> matches;
	matcher.match(descriptors_1, descriptors_2, matches);

	// Filter good matches
	double min_dist = 100;
	for(const auto& m : matches) {
		if(m.distance < min_dist) min_dist = m.distance;
	}

	std::vector<DMatch> good_matches;
	double threshold = std::max(2.5 * min_dist, 30.0);
	for(const auto& m : matches) {
		if(m.distance <= threshold) { good_matches.push_back(m); }
	}

	// Print results
	std::cout << trans_name << ":" << std::endl;
	std::cout << "  Keypoints: " << keypoints_1.size() << " / " << keypoints_2.size() << std::endl;
	std::cout << "  Total matches: " << matches.size() << std::endl;
	std::cout << "  Good matches: " << good_matches.size() << std::endl;

	// Draw and show matches
	Mat img_matches;
	drawMatches(img_1, keypoints_1, img_2, keypoints_2, good_matches, img_matches);

	std::string window_name = "AKAZE - " + trans_name;
	imshow(window_name, img_matches);
}

void testBRISK(const Mat& img_1, const Mat& img_2, const std::string& trans_name) {
	// Create BRISK detector
	Ptr<BRISK> detector = BRISK::create();

	// Detect keypoints
	std::vector<KeyPoint> keypoints_1, keypoints_2;
	detector->detect(img_1, keypoints_1);
	detector->detect(img_2, keypoints_2);

	// Compute descriptors
	Mat descriptors_1, descriptors_2;
	detector->compute(img_1, keypoints_1, descriptors_1);
	detector->compute(img_2, keypoints_2, descriptors_2);

	// Match descriptors (BRISK uses binary descriptors)
	BFMatcher matcher(NORM_HAMMING);
	std::vector<DMatch> matches;
	matcher.match(descriptors_1, descriptors_2, matches);

	// Filter good matches
	double min_dist = 100;
	for(const auto& m : matches) {
		if(m.distance < min_dist) min_dist = m.distance;
	}

	std::vector<DMatch> good_matches;
	double threshold = std::max(2.5 * min_dist, 30.0);
	for(const auto& m : matches) {
		if(m.distance <= threshold) { good_matches.push_back(m); }
	}

	// Print results
	std::cout << trans_name << ":" << std::endl;
	std::cout << "  Keypoints: " << keypoints_1.size() << " / " << keypoints_2.size() << std::endl;
	std::cout << "  Total matches: " << matches.size() << std::endl;
	std::cout << "  Good matches: " << good_matches.size() << std::endl;

	// Draw and show matches
	Mat img_matches;
	drawMatches(img_1, keypoints_1, img_2, keypoints_2, good_matches, img_matches);

	std::string window_name = "BRISK - " + trans_name;
	imshow(window_name, img_matches);
}

Mat applyRotation(const Mat& img, double angle) {
	Point2f center(img.cols / 2.0f, img.rows / 2.0f);
	Mat rot_mat = getRotationMatrix2D(center, angle, 1.0);

	double abs_cos = abs(rot_mat.at<double>(0, 0));
	double abs_sin = abs(rot_mat.at<double>(0, 1));
	int new_w = int(img.rows * abs_sin + img.cols * abs_cos);
	int new_h = int(img.rows * abs_cos + img.cols * abs_sin);

	rot_mat.at<double>(0, 2) += new_w / 2.0 - center.x;
	rot_mat.at<double>(1, 2) += new_h / 2.0 - center.y;

	Mat result;
	warpAffine(img, result, rot_mat, Size(new_w, new_h));
	return result;
}

Mat applyScale(const Mat& img, double scale) {
	Mat result;
	resize(img, result, Size(), scale, scale, INTER_LINEAR);
	return result;
}

Mat applyBrightness(const Mat& img, double alpha, int beta) {
	Mat result;
	img.convertTo(result, -1, alpha, beta);
	return result;
}