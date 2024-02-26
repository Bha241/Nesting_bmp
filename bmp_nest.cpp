#include <iostream>
#include <opencv2/opencv.hpp>
#include <iomanip>
#include <fstream>

using namespace cv;


struct BMPInfo {
    Mat image;
    Point position;
    Rect boundingBox;
};


// Function to check for overlap between two bounding boxes
bool checkOverlap(const Rect& box1, const Rect& box2) {
    return (box1 & box2).area() > 0;
}


// This function extracts the file name
std::string extractFileName(const std::string& filePath) {
    size_t found = filePath.find_last_of("/\\");
    return filePath.substr(found + 1);
}

void min_area() {

}

void placeBmp(const std::string& inputDir, const std::string& outputDir, int num_orient) {
    std::vector<cv::String> filenames;
    cv::glob(inputDir + "/*.bmp", filenames, false);
    int num_bmp_to_place = filenames.size() / 4; // Calculate the number of BMPs to place

    // Required parameters
    int paper_len = 350; // Working area length
    int paper_wid = 350; // Working area width
    int x_disp_rt = 2;   // Rate of movement in the x-direction if overlap occurs
    int y_disp_rt = 2;   // Rate of movement in the y-direction if overlap occurs


    // Create a blank canvas filled with the background color
    cv::Scalar backgroundColor = cv::Scalar(255, 255, 255); // White color in BGR format
    cv::Mat canvas(paper_len, paper_wid, CV_8UC3, backgroundColor);

    // Vector to store information about placed BMPs
    std::vector<BMPInfo> placedBMPs;

    // Iterate over the number of BMPs to place
    for (size_t i = 0; i < num_bmp_to_place; i++) {
        // Load the current BMP image
        Mat image = imread(filenames[i]);

        // Check if the image is loaded successfully
        if (!image.empty()) {
            // Extract the file name of the current image
            std::string filename = extractFileName(filenames[i]);

            // Process the image (placeholder)
            std::cout << "Processing Image: " << filename << std::endl;

            // Loop over orientations
            for (int angle_rot = 0; angle_rot < 360; angle_rot += (360 / num_orient)) {
                // Rotate the BMP image
                Mat rotatedImage;
                cv::warpAffine(image, rotatedImage, cv::getRotationMatrix2D(cv::Point2f(image.cols / 2, image.rows / 2), angle_rot, 1.0), image.size());

                // Loop over x-direction for placement
                for (int x = 0; x < paper_len; x += x_disp_rt) {
                    // Loop over y-direction for placement
                    for (int y = 0; y < paper_wid; y += y_disp_rt) {
                        // Check for overlap
                        bool overlap = false;
                        Rect bmpRect(x, y, rotatedImage.cols, rotatedImage.rows);
                        for (const auto& placedBMP : placedBMPs) {
                            if (checkOverlap(placedBMP.boundingBox,bmpRect)) {
                                overlap = true;
                                break;
                            }
                        }

                        if (!overlap) {

                            // Place the BMP on the canvas if its bounding box is within the canvas bounds
                            if (bmpRect.x >= 0 && bmpRect.y >= 0 && bmpRect.x + bmpRect.width <= canvas.cols && bmpRect.y + bmpRect.height <= canvas.rows) {
                                // Copy the rotated image to the canvas
                                rotatedImage.copyTo(canvas(bmpRect));

                                // Store information about the placed BMP
                                BMPInfo bmpInfo;
                                bmpInfo.image = rotatedImage.clone();
                                bmpInfo.position = Point(x, y);
                                bmpInfo.boundingBox = bmpRect;
                                placedBMPs.push_back(bmpInfo);

                                // Break out of the loop since the BMP is successfully placed
                                break;
                            }

                            // Store information about the placed BMP
                            BMPInfo bmpInfo;
                            bmpInfo.image = rotatedImage.clone();
                            bmpInfo.position = Point(x, y);
                            bmpInfo.boundingBox = bmpRect;
                            placedBMPs.push_back(bmpInfo);

                            // Break out of the loop since the BMP is successfully placed
                            break;
                        }
                    }
                }
            }
        }
    }

    // Save or display the canvas
    cv::imshow("Final Canvas", canvas);
    cv::waitKey(0);
    cv::imwrite(outputDir + "/final_canvas.bmp", canvas);
}




int main() {
    std::string inputDir = "C:/cpp_code/nest_bmp/Overlap_output";
    std::string outputDir = "C:/cpp_code/nest_bmp/Final_output"; // Output directory for combined image

    // Call placeBmp function
    placeBmp(inputDir, outputDir, 2); // Assuming 4 orientations

    return 0;
}
