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

double calculateBoundingBox(const Mat& grayCanvas) {
    int top = 0, bottom = grayCanvas.rows - 1, left = 0, right = grayCanvas.cols - 1;

    for (; top < grayCanvas.rows; ++top) {
        if ((countNonZero(grayCanvas.row(top))) != 0)
            break;
    }

    for (; bottom >= 0; --bottom) {
        if ((countNonZero(grayCanvas.row(bottom))) != 0)
            break;
    }

    for (; left < grayCanvas.cols; ++left) {
        if ((countNonZero(grayCanvas.col(left))) != 0)
            break;
    }

    for (; right >= 0; --right) {
        if ((countNonZero(grayCanvas.col(right))) != 0)
            break;
    }

    int width = right - left + 1;
    int height = bottom - top + 1;
    double Area = width * height;
    return Area;
}

double calculateRequiredArea(const std::string& inputDir, size_t startIndex, int batchSize) {
    double totalArea = 0;
    std::vector<int> orientations = { 0, 45, 90, 135, 180, 225, 270, 315 };
    for (int fileIndex = startIndex; fileIndex < startIndex+batchSize; fileIndex++) {
        int orientationIndex = rand() % orientations.size();
        int orientation = orientations[orientationIndex];
        std::ostringstream fileIndexStr;
        fileIndexStr << std::setw(3) << std::setfill('0') << fileIndex;
        std::ostringstream orientationStr;
        orientationStr << std::setw(3) << std::setfill('0') << orientation;
        std::string filename = inputDir + "/" + fileIndexStr.str() + "_" + orientationStr.str() + ".bmp";
        Mat image = imread(filename, IMREAD_GRAYSCALE);
        if (!image.empty()) {
            totalArea += countNonZero(image);
        }
    }
    return totalArea;
}

double calculateUtilization(const Mat& canvas) {
    double totalCanvasArea = canvas.cols * canvas.rows; // Total area of the canvas
    double utilizedArea = 0.0; // Initialize utilized area to 0
    int nonZeroCount = countNonZero(canvas); // Count of non-zero elements

    // Check if nonZeroCount is greater than zero to avoid division by zero
    if (nonZeroCount > 0) {
        utilizedArea = (nonZeroCount / totalCanvasArea) * 100; // Calculate utilized area
    }

    return utilizedArea;
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
    else if (totalInitialArea == totalCombinedArea)
    {
        holder.validity = true;
        holder.image = binaryCanvas.clone();
        return holder;
    }
}
int areaUtilization = 0;
int endIndex = 0;
int startIndex = 0;
// Recursive function to find solutions for placing images on canvas
void placeFPs(const std::string& inputDir, int startIndex, int endIndex, const std::string& outputDir, int numOrient, Mat& canvas, std::vector<BMPInfo>& bmpList, int batchSize, double& areaUtilization) {
    std::vector<int> orientations = { 0, 45, 90, 135, 180, 225, 270, 315 };
    bool utilizationAchieved = false; // Flag to track if desired utilization is achieved
    for (int fileIndex = startIndex; fileIndex < endIndex; fileIndex++) {
        if (bmpList.size() == batchSize) {
            break; // Break if batch size is reached
        }
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

        for (int x = 0; x < canvas.cols - image.cols; x += 20) {
            for (int y = 0; y < canvas.rows - image.rows; y += 20) {
                struct tempSol solution = validSolution(canvas, image, bmpList, Point(x, y));
                if (solution.validity) {
                    BMPInfo bmpInfo;
                    bmpInfo.image = image.clone();
                    bmpInfo.position = Point(x, y);
                    bmpInfo.boundingBox = Rect(Point(x, y), image.size());
                    bmpList.push_back(bmpInfo);
                    Mat roi = canvas(bmpInfo.boundingBox);
                    Mat tempCanvas = canvas.clone();
                    canvas = solution.image.clone();
                    placeFPs(inputDir, fileIndex + 1, endIndex, outputDir, numOrient, canvas, bmpList, batchSize, areaUtilization); // Pass current fileIndex
                    areaUtilization = calculateUtilization(canvas);

                    if (areaUtilization >= 40) {
                        utilizationAchieved = true; // Set the flag to true if desired utilization is achieved
                        return; // Break out of inner loops
                    }
                    if (!utilizationAchieved && bmpList.size() > batchSize) {
                        bmpList.pop_back();
                    }
                    canvas = tempCanvas.clone();
                }
            }
            if (utilizationAchieved) {
                break; // Break out of outer loops
            }
        }
        if (utilizationAchieved) {
            break; // Break out of outer loops
        }
    }
 
}

int main() {
    std::string inputDir = "C:/cpp_code/Output_final";
    std::string outputDir = "C:/cpp_code/Final_output";
    srand(time(nullptr));
    std::vector<cv::String> filenames;
    cv::glob(inputDir + "/*.bmp", filenames, false);
    std::vector<BMPInfo> bmpList;
    int paperHeight = 400;
    int paperWidth = 400;
    int IncrementPaperWidth = 400;
    Scalar backgroundColor = Scalar(0);
    Mat canvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
    Mat finalCanvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
    int numOrient = 8;
    int initialPositionX = 0;
    int initialPositionY = 0;
    int filesPlaced = 0;
    int givenArea = canvas.rows * canvas.cols;
    int endIndex = 0;
    int remainingArea;
    double desiredUtilization = 40;
    double areaUtilization = 0;
    while (filesPlaced < filenames.size() / numOrient) {
        int batchSize = (filenames.size() / numOrient - filesPlaced);
        bool canPlaceNBmps = false;
        while (!canPlaceNBmps) {
            double requiredArea = calculateRequiredArea(inputDir, filesPlaced, batchSize);
            if (givenArea > requiredArea) {
                canPlaceNBmps = true;
                continue;
            }
            batchSize--;
        }
        std::cout << "Batch Size :" << batchSize << std::endl;
        placeFPs(inputDir, filesPlaced,batchSize + filesPlaced,outputDir, numOrient, canvas, bmpList, batchSize,areaUtilization);
        std::cout << "Area Utilization :" << areaUtilization << "%" << std::endl;
        cv::Mat tempCanvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
        finalCanvas.copyTo(tempCanvas(Rect(0, 0, canvas.cols, canvas.rows)));
        canvas.copyTo(tempCanvas(Rect(initialPositionX, 0, canvas.cols, canvas.rows)));
        finalCanvas = tempCanvas.clone();
        filesPlaced += bmpList.size();
        std::cout << "Files Placed :" << filesPlaced << std::endl;
        paperWidth += 400;
        std::string outputFile = outputDir + "/New canvas.jpg";
        imwrite(outputFile, finalCanvas);
        initialPositionX += IncrementPaperWidth;
        canvas.setTo(Scalar(0));
        bmpList.clear();

    }
    return 0;
}

