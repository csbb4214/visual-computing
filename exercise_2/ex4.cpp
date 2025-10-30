#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    vector<string> images = {"image1.jpg", "image2.jpg"};

    for (auto &path : images) {
        Mat img = imread(path, IMREAD_GRAYSCALE);
        if (img.empty()) {
            cerr << "Could not load " << path << endl;
            continue;
        }

        // --- 1. Apply Gaussian Blur to reduce noise ---
        Mat blurred;
        GaussianBlur(img, blurred, Size(5,5), 1.0);

        // --- 2. Compute Otsu threshold ---
        double otsu_thresh_val = threshold(blurred, Mat(), 0, 255, THRESH_BINARY | THRESH_OTSU);
        double highThresh = otsu_thresh_val;
        double lowThresh = otsu_thresh_val * 0.5; // standard ratio

        // --- 3. Apply Canny using Otsu thresholds ---
        Mat edgesCanny;
        Canny(blurred, edgesCanny, lowThresh, highThresh);

        // --- 4. Apply other detectors for comparison ---
        Mat sobelX, sobelY, sobel, laplacian;
        Sobel(blurred, sobelX, CV_16S, 1, 0);
        Sobel(blurred, sobelY, CV_16S, 0, 1);
        convertScaleAbs(sobelX + sobelY, sobel);

        Laplacian(blurred, laplacian, CV_16S);
        convertScaleAbs(laplacian, laplacian);

        // --- 5. Show results ---
        imshow("Original - " + path, img);
        imshow("Canny (Otsu) - " + path, edgesCanny);
        imshow("Sobel - " + path, sobel);
        imshow("Laplacian - " + path, laplacian);

        waitKey(0);
    }

    return 0;
}
