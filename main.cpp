#include <iostream>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <fstream>
#include <stdlib.h>

using namespace cv;
using namespace std;

const char* _windowname = "Original Image";
const char* _dctwindow = "Decompressed Image";
const char* _filename = "../Images/2.ppm";
const char* _savefile = "../compressed.txt";

//Quantization matrix for Luminance
double dataLum[8][8] = {
        {16, 11, 10, 16, 24, 40, 51, 61},
        {12, 12, 14, 19, 26, 58, 60, 55},
        {14, 13, 16, 24, 40, 57, 69, 56},
        {14, 17, 22, 29, 51, 87, 80, 62},
        {18, 22, 37, 56, 68, 109, 103, 77},
        {24, 35, 55, 64, 81, 104, 113, 92},
        {49, 64, 78, 87, 103, 121, 120, 101},
        {72, 92, 95, 98, 112, 100, 103, 99}
};

//Quantization matrix for Chrominance
double dataChrom[8][8] = {
        {17, 18, 24, 27, 99, 99, 99, 99},
        {18, 21, 26, 66, 99, 99, 99, 99},
        {24, 26, 56, 99, 99, 99, 99, 99},
        {47, 66, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99},
        {99, 99, 99, 99, 99, 99, 99, 99}
};

void scaleQuant(int quality, bool undo){
    //Adjust quantization matrix based on quality level
    int scaleFactor = 0;
    if (quality < 50){
        scaleFactor = 5000/quality;
    } else {
        scaleFactor = 200 - quality * 2;
    }

    for (int i = 0; i<8; i++){
        for (int j = 0; j<8; j++){
            if (undo){
                dataLum[i][j] = (dataLum[i][j] * 100  - 50) / scaleFactor;
                dataChrom[i][j] = (dataChrom[i][j] * 100  - 50) / scaleFactor;
            } else {
                dataLum[i][j] = (dataLum[i][j] * scaleFactor + 50) / 100;
                dataChrom[i][j] = (dataChrom[i][j] * scaleFactor + 50) / 100;
            }
        }
    }
}

void zigZag(Mat input, vector<double>&output){
    //Collect pixels in a zig-zag way
    int width = input.size().width - 1;
    int height = input.size().height - 1;
    double val = 0;

    int currX = 0;
    int currY = 0;

    //START (0,0)
    val = input.at<double>(currX,currY);
    output.push_back(val);

    //Loop through block
    while (currX < width || currY < height) {

        //X+1 IF X!=WIDTH ELSE Y+1
        if ((currX + 1) <= width) {
            currX++;
            val = input.at<double>(currX,currY);
            output.push_back(val);
        } else {
            currY++;
            val = input.at<double>(currX,currY);
            output.push_back(val);
        }

        //DOWNWARDS DIAGONAL - -X AND +Y UNTIL Y=0
        while ((currX > 0) && (currY < height)) {
            currX--;
            currY++;
            val = input.at<double>(currX,currY);
            output.push_back(val);
        }

        //Y+1 IF Y!=HEIGHT ELSE X+1
        if ((currY + 1) <= height) {
            currY++;
            val = input.at<double>(currX,currY);
            output.push_back(val);
        } else {
            currX++;
            val = input.at<double>(currX,currY);
            output.push_back(val);
        }

        //UPWARDS DIAGONAL - +X AND -Y UNTIL X=0
        while ((currX < width) && (currY > 0)) {
            currX++;
            currY--;
            val = input.at<double>(currX,currY);
            output.push_back(val);
        }

    }
}

void UndozigZag(Mat &image, vector<double>values){
    //Place pixel values back into block by doing reverse zig-zag
    int width = image.size().width - 1;
    int height = image.size().height - 1;
    double val = 0;
    int i = 0;

    int currX = 0;
    int currY = 0;

    //START (0,0)
    val = values[i];
    image.at<double>(currX,currY) = val;
    i++;

    while (currX < width || currY < height) {

        //X+1 IF X!=WIDTH ELSE Y+1
        if ((currX + 1) <= width) {
            currX++;
            val = values[i];
            image.at<double>(currX,currY) = val;
            i++;
        } else {
            currY++;
            val = values[i];
            image.at<double>(currX,currY) = val;
            i++;
        }

        //DOWNWARDS DIAGONAL - -X AND +Y UNTIL Y=0
        while ((currX > 0) && (currY < height)) {
            currX--;
            currY++;
            val = values[i];
            image.at<double>(currX,currY) = val;
            i++;
        }

        //Y+1 IF Y!=HEIGHT ELSE X+1
        if ((currY + 1) <= height) {
            currY++;
            val = values[i];
            image.at<double>(currX,currY) = val;
            i++;
        } else {
            currX++;
            val = values[i];
            image.at<double>(currX,currY) = val;
            i++;
        }

        //UPWARDS DIAGONAL - +X AND -Y UNTIL X=0
        while ((currX < width) && (currY > 0)) {
            currX++;
            currY--;
            val = values[i];
            image.at<double>(currX,currY) = val;
            i++;
        }

    }
}

void roundPixel (Mat &input){
    //Round the pixel vales to a whole number
    for (int y = 0; y<input.size().height; y++) {
            for (int x = 0; x < input.size().width; x++) {
                double val = input.at<double>(x,y);
                //Used to stop getting negative zero (-0)
                val = (int)round(val);
                //Place rounded value back into block
                input.at<double>(x,y) = val;
            }
    }
}

void clearSaveFile(){
    //Clear out file of compressed data.
    ofstream outputFile;
    outputFile.open(_savefile, ofstream::out | ofstream::trunc);
    outputFile.close();
}

void saveDimensions(int width, int height, int scale){
    //Clear file of any old data.
    clearSaveFile();

    //Save width, height and scale factor used in compression of image
    ofstream outputFile;
    outputFile.open(_savefile, ofstream::app);
    outputFile << width << "," << height << "," << scale << ",";
    outputFile.close();
}

void saveToFile (vector<double>values){
    //Save values into file separated by comma.
    ofstream outputFile;
    //Open file in append mode
    outputFile.open(_savefile, ofstream::app);

    for(int i = 0; i<values.size(); i++){
        outputFile << values[i] << ",";
    }
    outputFile.close();
}

void runLength (Mat image){
    vector<double> values;
    vector<double> ZigZagvalues;
    zigZag(std::move(image), values);

    for (int i = 0; i<values.size(); i++){
        int count = 0;
        //Get the first value and add one to count
        ZigZagvalues.push_back(values[i]);
        count ++;
        //While the next value is the same as the current
        //Increment count and add one to iteration.
        while ((i+1)<values.size() && values[i] == values[i+1]){
            count++;
            i++;
        }
        //Place count value next to pixel value in vector
        ZigZagvalues.push_back((float)count);
    }
    //Save the values into file
    saveToFile(ZigZagvalues);

}

void compress(Mat &dctImage, int scale){
    //Scale quantization matrices.
    scaleQuant(scale, false);

    //Convert the 2D array's for quantization to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    //Get dimensions of input image
    int width = dctImage.size().width;
    int height = dctImage.size().height;

    //Save dimensions and scale factor to file
    saveDimensions(width, height, scale);

    //Convert image to YCrCb colour scheme
    cvtColor(dctImage, dctImage, CV_BGR2YCrCb);

    //Split image into multiple channels
    vector<Mat> planes;
    split(dctImage, planes);

    cout << "\t-->MAPPING...";
    cout <<" DONE"<<endl;
    cout << "\t-->QUANTISATION...";
    //Loop through each pixel in the image
    for(int i=0; i < height; i+=8) {
        for (int j = 0; j < width; j += 8) {
            //Loop through each channel of that pixel
            for (int channel = 0; channel < dctImage.channels(); channel++) {
                //Create an block 8x8
                Mat block = planes[channel](Rect(j, i, 8, 8));

                //Conver to 64-Bit Floating Point with 1 Channel
                block.convertTo(block, CV_64FC1);

                //Centre pixels around zero (0), so subtract 128
                subtract(block, 128.0, block);

                //Perform DCT on each 8x8 block
                dct(block, block);

                //Quantization
                if (channel == 0){
                    //Divide by Luminance if Y channel
                    divide(block, quantLum, block);
                } else {
                    //Divide by Chrominance if Cr, Cb channel
                    divide(block, quantChrom, block);
                }

                //Round each pixel in the block to the nearest whole number
                roundPixel(block);

                //Perform Run Length(RLC) on the block
                runLength(block);

                //Add 128 to each pixel
                add(block, 128.0, block);

                //Convert to 8-bit unassigned char
                block.convertTo(block, CV_8UC1);

                //Copy block back to image
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    //Merge channels back into one.
    merge(planes, dctImage);

    //Undo scaling of matrices
    scaleQuant(scale, true);

    cout <<" DONE" <<endl;
    cout << "\t-->ENTROPY ENCODING...";
    cout <<" DONE" <<endl;
    cout << "\t-->SAVING TO FILE: " << _savefile <<" ...";
    cout <<" DONE" <<endl;
    cout << endl;

}

vector<double> readCodedData() {
    //Read data from file
    vector<double> data;
    string line;

    //Open file
    ifstream dataFile(_savefile);

    //While there is a line in the file
    while (getline(dataFile, line)) {
        stringstream ss(line);
        string value;

        //Use comma as delimiter
        while(getline(ss, value, ',')){
            //Conver string to double and place in vector
            data.push_back(atof(value.c_str()));
        }
    }

    //Close file
    dataFile.close();

    //Return Vector
    return data;
}

void undoRLC(vector<double> data, vector<double> &values) {

    //Used to create a vector with all the values (Undoing the RLC)
    for (int i = 0; i < data.size() - 1; i+=2){
        //Get the first value and how many times it is repeated
        double pixel = data[i];
        double amount = data[i+1];

        //Place that pixel values in the vector the amount of times it is repeated
        for(int j = 0; j < amount; j++){
            values.push_back(pixel);
        }
    }

}

void reBuildImage(Mat &image, vector<double>values){
    int width = image.size().width;
    int height = image.size().height;

    //Split image into multiple channels
    vector<Mat> planes;
    split(image, planes);

    //Loop through pixels in the image
    for(int i=0; i < height; i+=8) {
        for (int j = 0; j < width; j += 8) {
            //Loop through each channel of the pixel
            for (int channel = 0; channel < image.channels(); channel++) {
                vector<double> data;

                //Get the first 64 pixel values, as each block is 8x8
                for (int x = 0; x<64; x++){
                    data.push_back(values[x]);
                }

                //No longer need those values, so remove them from vector holding all values
                values.erase(values.begin(), values.begin() + 64);

                //Create block 8x8
                Mat block = planes[channel](Rect(j, i, 8, 8));

                //Convert to 64-bit floating point
                block.convertTo(block, CV_64FC1);

                //Place values into block by doing reverse zig-zag
                UndozigZag(block, data);

                //Add 128 to each pixel in the block, to centre around 128
                add(block, 128.0, block);

                //Convert block to 8-bit unassigned char
                block.convertTo(block, CV_8UC1);

                //Copy block back to original image
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }

    //Merge channels back into one
    merge(planes, image);
}

void inverseDCT (Mat &image){
    int width = image.size().width;
    int height = image.size().height;
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    //Split image into multiple channels
    vector<Mat> planes;
    split(image, planes);

    //Loop through pixels in the image
    for(int i=0; i < height; i+=8) {
        for (int j = 0; j < width; j += 8) {
            //Loop through each channel of the pixel
            for (int channel = 0; channel < image.channels(); channel++) {
                //Create block 8x8
                Mat block = planes[channel](Rect(j, i, 8, 8));

                //Convert to 64-bit floating point
                block.convertTo(block, CV_64FC1);

                //Centre pixel value around zero (0), so subtract 128
                subtract(block, 128.0, block);

                //Undo quantization
                if (channel == 0){
                    //If Y channel
                    multiply(block, quantLum, block);
                } else {
                    //If Cr, Cb channel
                    multiply(block, quantChrom, block);
                }

                //Perform Inverse DCT
                idct(block, block);

                //Add 128 to each pixel in the block, to centre around 128
                add(block, 128.0, block);

                //Convert block to 8-bit unassigned char
                block.convertTo(block, CV_8UC1);

                //Copy block back to original image
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    //Merge channels back into one
    merge(planes, image);

    //Convert back to BGR colour scheme
    cvtColor(image, image, CV_YCrCb2BGR);
}

Mat deCompress(){
    vector<double> data;
    vector<double> values;

    //Read data from compressed data file
    cout << "\t-->READING CODED DATA...";
    data = readCodedData();
    cout <<" DONE"<<endl;

    //Get the dimensions and scale
    auto width = (int)data[0];
    auto height = (int)data[1];
    auto scale = (int)data[2];

    //Remove these from data values, the rest are pixel values
    data.erase(data.begin(), data.begin()+3);

    //Scale the quantization matrices
    scaleQuant(scale, false);

    //Undo Run Length Encoding
    undoRLC(data, values);

    //Blank Mat with 3 channels to rebuild the image
    Mat decodedImage = Mat(height, width, CV_8UC3, CV_RGB(1,1,1));
    //Convert to YCrCb colour scheme
    cvtColor(decodedImage, decodedImage, CV_BGR2YCrCb);

    cout << "\t-->SYMBOL DECODER...";

    //Rebuild Image
    reBuildImage(decodedImage, values);

    cout <<" DONE"<<endl;

    cout << "\t-->INVERSE MAPPER...";

    //Inverse DCT and undo quantization
    inverseDCT(decodedImage);

    cout <<" DONE"<<endl;

    //Return Compressed-Decompressed Image
    return decodedImage;

}

void getCR(double &size){
    //Get size of compressed file in bytes
    ifstream compressed(_savefile, ifstream::in | ifstream::binary);
    compressed.seekg(0, ios::end);
    double compSize = compressed.tellg();
    cout << "Size of Compressed Image (int bytes): " << compSize <<endl;
    compressed.close();

    //Get size of original file in bytes
    ifstream orginal(_filename, ifstream::in | ifstream::binary);
    orginal.seekg(0, ios::end);
    double orginalSize = orginal.tellg();
    cout << "Size of Original Image (int bytes): " << orginalSize <<endl;
    orginal.close();

    //Calculate Compression Ratio
    size = orginalSize / compSize;
}

int main() {
    //Load image to compress
    Mat image = imread(_filename, 1);
    if(image.empty()) {
        cout<<"Can't load the image from "<<_filename<<endl;
        return -1;
    }

    int x = 0;
    int y = 0;
    int scale;
    char decompress;
    double CR = 0;

    namedWindow(_windowname, CV_WINDOW_AUTOSIZE);
    moveWindow(_windowname, x, y);
    imshow(_windowname, image);

    x += 400;
    y += 0;

    //Dimension of original image
    int height = image.size().height;
    int width = image.size().width;

    //Calculate the amount the original image should be extended so it can be split into 8x8 blocks.
    int border_w = width % 8;
    int border_h = height % 8;

    Mat compressedImage;
    Mat deCompressedImage;

    //Copy original image and add border so it can be split into blocks of 8.
    copyMakeBorder(image,compressedImage,0,border_h,0,border_w,BORDER_CONSTANT);

    cout << "Compressing image: " << _filename << endl;
    cout << "Please enter the quality value you would like to compress the image too (enter value in the range 1-100): " << endl;
    cin >> scale;
    if (scale > 100){
        scale -= 100;
        cout << "VALUE OVER 100, SUBTRACTING SO NEW VALUE IS: "<< scale << endl;
    }
    cout << endl;
    cout << "COMPRESSING: "<<endl;

    //Compress Image
    compress(compressedImage, scale);

    cout << "Compressed into file: "<<_savefile<<endl;

    cout<<"Would you like to decompress the image (enter Y or y): " << endl;
    cin >> decompress;
    if (decompress != 'y' && decompress != 'Y') {
        cout << "WELL WE ARE DOING IT ANYWAY!!!"<<endl;
    }
    cout << endl;
    cout << "DECOMPRESSING: " << endl;

    //Decompress Image
    deCompressedImage = deCompress();

    cout << "\nImage has been decompressed.\n" << endl;

    //Calculate and print Compression Ratio
    getCR(CR);
    cout << "Compression Ration is: " << CR << endl;

    //Remove border
    Mat finalImage = deCompressedImage(Rect(0,0,width,height));

    namedWindow(_dctwindow, CV_WINDOW_AUTOSIZE);
    moveWindow(_dctwindow, x,y);
    //Show Compressed-Decompressed Image
    imshow(_dctwindow, finalImage);

    waitKey(0);

    destroyWindow(_windowname);
    destroyWindow(_dctwindow);
}