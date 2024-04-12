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
    double givenArea = canvas.cols * canvas.rows;
    int areaUtilized = countNonZero(canvas);
    double utilization = (areaUtilized / givenArea) * 100;

    return utilization;
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
int areaUtilizationPrev = 0;
int startX = 0;
// Recursive function to find solutions for placing images on canvas
int placeFPs(const std::string& inputDir, const std::string& outputDir, int numOrient, int filesPlaced, Mat& canvas, std::vector<BMPInfo>& bmpList, int batchSize) {

    std::vector<int> orientations = { 0, 45, 90, 135, 180, 225, 270, 315 };
    BMPInfo bmpInfo;
    double areaUtilization = calculateUtilization(canvas);

    for (int fileIndex = filesPlaced; fileIndex < batchSize; fileIndex++) {
        if (bmpList.size() == batchSize) {
            break;
        }
        for (int angleOfRot = 0; angleOfRot < 360; angleOfRot += (360 / numOrient)) {
            std::ostringstream fileIndexStr;
            fileIndexStr << std::setw(3) << std::setfill('0') << fileIndex;
            std::ostringstream angleOfRotStr;
            angleOfRotStr << std::setw(3) << std::setfill('0') << angleOfRot;
            std::string filename = inputDir + "/" + fileIndexStr.str() + "_" + angleOfRotStr.str() + ".bmp";
            Mat image = imread(filename, IMREAD_GRAYSCALE);
            if (image.empty()) {
                continue;
            }

            for (int x = startX; x < canvas.cols - image.cols; x += 30) {
                for (int y = 0; y < canvas.rows - image.rows; y += 30) {
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
                        placeFPs(inputDir, outputDir, numOrient, fileIndex + 1, canvas, bmpList, batchSize); // Pass current fileIndex
                        std::cout << "Area Utilization :" << areaUtilization << "%" << std::endl;
                        bmpList.pop_back();
                        canvas = tempCanvas.clone();
                        if (areaUtilization > 10) {
                            std::cout << "Area Utilization exceeds 30%. Terminating loops." << std::endl;
                            return areaUtilization;
                        }



                    }
                }
                startX = x;
            }
        }

    }

    imshow("temp", canvas);
    waitKey(100);
    double areaUtilizationPrev = areaUtilization;
    //std::cout << "Area Utilization :" << areaUtilization << "%" << std::endl;
    return areaUtilization;
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
    Scalar backgroundColor = Scalar(0);
    Mat canvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
    Mat finalCanvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
    int numOrient = 8;
    int initialPositionX = 0;
    int initialPositionY = 0;
    int filesPlaced = 0;
    int batchSize = (filenames.size() / 8 - filesPlaced);
    int givenArea = canvas.rows * canvas.cols;
    int remainingArea;
    double desiredUtilization = 10;
    int areaUtilization = 0;
    int fileIndex = 0;
    while (filesPlaced < filenames.size() / numOrient) {

        bool canPlaceNBmps = false;
        while (!canPlaceNBmps) {
            double requiredArea = calculateRequiredArea(filenames, filesPlaced, batchSize);

            if (givenArea > requiredArea) {
                canPlaceNBmps = true;
                continue;
            }
            batchSize--;
        }
        batchSize += 5;
        std::cout << "Batch Size :" << batchSize << std::endl;

        while (areaUtilization < desiredUtilization) {
            areaUtilization = placeFPs(inputDir, outputDir, numOrient, filesPlaced, canvas, bmpList, batchSize);
            bmpList.clear();
            // If area utilization exceeds the desired utilization, break the loop
            if (areaUtilization > desiredUtilization) {
                break;
            }


        }

        std::cout << "Area Utilization :" << areaUtilization << "%" << std::endl;
        cv::Mat tempCanvas(paperHeight, paperWidth, CV_8UC1, backgroundColor);
        finalCanvas.copyTo(tempCanvas(Rect(0, 0, canvas.cols, canvas.rows)));
        imshow("trial", tempCanvas);
        waitKey(10);
        canvas.copyTo(tempCanvas(Rect(initialPositionX, 0, canvas.cols, canvas.rows)));
        finalCanvas = tempCanvas.clone();
        filesPlaced += batchSize;

        paperWidth += 400;
        std::string outputFile = outputDir + "/New canvas.jpg";
        imwrite(outputFile, finalCanvas);
        initialPositionX += 400;
        canvas.setTo(Scalar(0));
    }
    return 0;
}
