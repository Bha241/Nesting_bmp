#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

using namespace cv;

#include <iostream>
#include <opencv2/opencv.hpp>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

using namespace cv;

struct BMPInfo {
    Mat image;
    Point position;
    Rect boundingBox;
};

struct tempSol {
    Mat image;
    bool validity;
};

double calculateRequiredArea(const std::vector<std::string>& filenames, size_t startIndex, int batchSize) {
    double totalArea = 0;
    size_t count = 0;
    for (size_t i = startIndex; i < startIndex + batchSize; ++i) {
        const auto& filename = filenames[i];
        Mat image = imread(filename, IMREAD_GRAYSCALE);
        if (!image.empty()) {
            totalArea += image.cols * image.rows;
            count++;
            if (count == batchSize) {
                break;
            }
        }
    }
    return totalArea;
}

double calculateUtilization(const Mat& canvas) {
    int givenArea = canvas.cols * canvas.rows;
    int areaUtilized = countNonZero(canvas);
    double utilization = (areaUtilized / static_cast<double>(givenArea)) * 100;
    return utilization;
}

bool overlapCheck(const Mat& image, const Mat& canvas, Rect roi) {

    Mat canvas1 = canvas.clone();
    image.copyTo(canvas1(roi), image);
    // Threshold the images to create binary images
    Mat binaryImage1, binaryImage2;
    threshold(canvas1, binaryImage1, 0, 255, THRESH_BINARY);
    threshold(canvas, binaryImage2, 0, 255, THRESH_BINARY);

    // Perform bitwise AND operation to check overlap
    Mat overlap;
    bitwise_and(binaryImage1, binaryImage2, overlap);

    // Check if there is any overlap
    bool hasOverlap = (countNonZero(overlap) > 0);
    return hasOverlap;

}
bool checkOverlap(const Rect& box1, const Rect& box2) {
    return (box1 & box2).area() > 0;
}

struct tempSol validSolution(const Mat& canvas, const Mat& image, const std::vector<BMPInfo>& bmpList, Point position) {
    struct tempSol holder;
    BMPInfo bmpInfo;
    bmpInfo.image = image.clone();
    bmpInfo.position = position;

    Mat binaryImage1, binaryImage2;
    threshold(canvas, binaryImage1, 0, 255, THRESH_BINARY); // Invert the binary image
    threshold(bmpInfo.image, binaryImage2, 0, 255, THRESH_BINARY_INV); // Invert the binary image

    cv::Mat placeholder(canvas.rows, canvas.cols, CV_8UC1, cv::Scalar(0));
    cv::Mat binaryCanvas(canvas.rows, canvas.cols, CV_8UC1, cv::Scalar(0));

    Mat leftROI(binaryCanvas, Rect(0, 0, binaryImage1.cols, binaryImage1.rows));
    binaryImage1.copyTo(leftROI);

    // Copy img2 to the right portion of combinedImage, shifted by shift
    Mat rightROI(placeholder, Rect(position.x, position.y, binaryImage2.cols, binaryImage2.rows));
    binaryImage2.copyTo(rightROI);

    // Convert the combined image to binary (black and white) using thresholding
    bitwise_or(binaryCanvas, placeholder, binaryCanvas);
    // Find contours in the combined binary image
    std::vector<std::vector<Point>> combinedContours;
    findContours(binaryCanvas, combinedContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<Point>> contours1, contours2;
    findContours(binaryImage1, contours1, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    findContours(binaryImage2, contours2, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    double totalInitialArea = 0.0;
    double totalCombinedArea = 0.0;

    for (const auto& contour : contours1) {
        totalInitialArea += contourArea(contour);
    }
    for (const auto& contour : contours2) {
        totalInitialArea += contourArea(contour);
    }
    // Calculate total contour area of the combined image

    for (const auto& contour : combinedContours) {
        totalCombinedArea += contourArea(contour);
    }

    if (totalInitialArea - totalCombinedArea > 0)
    {
        holder.validity = false;
        return holder;
    }
    else
    {
        holder.validity = true;
        holder.image = binaryCanvas.clone();
        return holder;
    }
}

int placeFPs(const std::string& inputDir, int filesPlaced, int batchSize, Mat& canvas, int desiredUtilization, std::vector<BMPInfo>& bmpList) {
    int areaUtilization = 0;
    std::vector<int> orientations = { 0, 45, 90, 135, 180, 225, 270, 315 };
    BMPInfo bmpInfo;
    batchSize += filesPlaced;

    for (int fileIndex = filesPlaced; fileIndex < batchSize; fileIndex++) {
        int orientationIndex = rand() % orientations.size();
        int orientation = orientations[orientationIndex];
        std::ostringstream fileIndexStr;
        fileIndexStr << std::setw(3) << std::setfill('0') << fileIndex;
        std::ostringstream orientationStr;
        orientationStr << std::setw(3) << std::setfill('0') << orientation;
        std::string filename = inputDir + "/" + fileIndexStr.str() + "_" + orientationStr.str() + ".bmp";
        Mat image = imread(filename, IMREAD_GRAYSCALE);

        if (image.empty()) {
            continue;
        }

        int x = rand() % (canvas.cols - image.cols);
        int y = rand() % (canvas.rows - image.rows);
        Rect roi(Point(x, y), image.size());

        //struct tempSol solution = validSolution(canvas, image, bmpList, Point(x, y));
        if (!overlapCheck(image, canvas, roi)) {

            bmpInfo.image = image.clone();
            bmpInfo.position = Point(x, y);
            bmpInfo.boundingBox = Rect(Point(x, y), image.size());
            bmpList.push_back(bmpInfo);
            Mat binaryImage;
            threshold(image, binaryImage, 0, 255, THRESH_BINARY_INV);
            binaryImage.copyTo(canvas(roi), binaryImage);

            areaUtilization = calculateUtilization(canvas);
            return areaUtilization;
        }

    }
    areaUtilization = calculateUtilization(canvas);
    filesPlaced = batchSize;
    bmpList.clear();

    return areaUtilization;
}

int main() {
    std::string inputDir = "C:/cpp_code/Output_final";
    std::string outputDir = "C:/cpp_code/Final_output";
    int numOrient = 8;
    srand(time(nullptr));
    std::vector<cv::String> filenames;
    cv::glob(inputDir + "/*.bmp", filenames, false);
    std::vector<BMPInfo> bmpList;
    int paperHeight = 400;
    int paperWidth = 400;
    Scalar backgroundColor = Scalar(0);
    Mat canvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
    Mat finalCanvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
    int filesPlaced = 0;
    int initialPositionX = 0;
    int batchSize = (filenames.size() - filesPlaced) / 20;
    int givenArea = canvas.rows * canvas.cols;
    int remainingArea;
    while (filesPlaced < filenames.size() / numOrient) {

        bool canPlaceNBmps = false;
        while (!canPlaceNBmps) {
            double requiredArea = calculateRequiredArea(filenames, filesPlaced, batchSize);
            double areaUtilized = calculateUtilization(canvas);
            remainingArea = givenArea - areaUtilized;
            if (givenArea > requiredArea) {
                canPlaceNBmps = true;
                continue;
            }
            batchSize--;
        }
        std::cout << "Batch Size :" << batchSize << std::endl;
        int desiredUtilization = 20;
        int areaUtilization = 0;
        while (areaUtilization < desiredUtilization) {
            areaUtilization = placeFPs(inputDir, filesPlaced, batchSize, canvas, desiredUtilization, bmpList);

        }


        std::cout << "Area Utilization :" << areaUtilization << "%" << std::endl;

        cv::Mat tempCanvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
        finalCanvas.copyTo(tempCanvas(Rect(0, 0, finalCanvas.cols, finalCanvas.rows)));
        imshow("trial", canvas);
        waitKey(10);
        canvas.copyTo(tempCanvas(Rect(initialPositionX, 0, canvas.cols, canvas.rows)));
        finalCanvas = tempCanvas.clone();
        filesPlaced += batchSize;
        paperWidth += 400;
        std::string outputFile = outputDir + "/New canvas.jpg";
        imwrite(outputFile, finalCanvas);
        initialPositionX += 400;
        canvas.setTo(Scalar(0));
        bmpList.clear();
    }
    return 0;
}

