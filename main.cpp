#include<iostream>
#include<cstdio>
#include<string>
#include <utility>
#include<vector>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/objdetect/objdetect.hpp>
#include <fstream>
#include <stdlib.h>

using namespace cv;
using namespace std;

const char* _windowname = "Original Image";
const char* _dctwindow = "Decompressed Image";
const char* _filename = "../Images/2.ppm";
const char* _savefile = "../compressed.txt";

//Data for quantization matrix
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

void scaleQuant(int quality){
    int scaleFactor = 0;
    if (quality < 50){
        scaleFactor = 5000/quality;
    } else {
        scaleFactor = 200 - quality * 2;
    }

    for (int i = 0; i<8; i++){
        for (int j = 0; j<8; j++){
            dataLum[i][j] = (dataLum[i][j] * scaleFactor +50) /100;
            dataChrom[i][j] = (dataChrom[i][j] * scaleFactor +50) /100;
        }
    }
}

void zigZag(Mat input, vector<double>&output){
    int width = input.size().width - 1;
    int height = input.size().height - 1;
    double val = 0;

    int currX = 0;
    int currY = 0;
    //START (0,0)
    val = input.at<double>(currX,currY);
    output.push_back(val);

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

    //cout << "SIZE: " << output.size() << endl;
}

void UndozigZag(Mat &image, vector<double>values){
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
    for (int y = 0; y<input.size().height; y++) {
            for (int x = 0; x < input.size().width; x++) {
                double val = input.at<double>(x,y);
                val = (int)round(val);
                input.at<double>(x,y) = val;
            }
    }
}

void saveToFile (vector<double>y){
    ofstream outputFile;
    outputFile.open(_savefile, ofstream::app);

    for(int i = 0; i<y.size(); i++){
            outputFile << y[i] << ",";
    }
    outputFile.close();
}

void clearSaveFile(){
    ofstream outputFile;
    outputFile.open(_savefile, ofstream::out | ofstream::trunc);
    outputFile.close();
}

void saveDimensions(int width, int height){
    clearSaveFile();
    ofstream outputFile;
    outputFile.open(_savefile, ofstream::app);

    outputFile << width << "," << height << ",";

    outputFile.close();
}

void runLength (Mat image){
    vector<double> values;
    vector<double> ZigZagvalues;
    zigZag(std::move(image), values);

    for (int i = 0; i<values.size(); i++){
        int count = 0;
        ZigZagvalues.push_back(values[i]);
        count ++;
        while ((i+1)<values.size() && values[i] == values[i+1]){
            count++;
            i++;
        }
        ZigZagvalues.push_back((float)count);
    }
    saveToFile(ZigZagvalues);

}

void compress(Mat &dctImage){
    scaleQuant(10);
    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    int width = dctImage.size().width;
    int height = dctImage.size().height;
    saveDimensions(width, height);
    cvtColor(dctImage, dctImage, CV_BGR2YCrCb);

    vector<Mat> planes;
    split(dctImage, planes);

    for(int i=0; i < height; i+=8) {
        for (int j = 0; j < width; j += 8) {
            for (int channel = 0; channel < dctImage.channels(); channel++) {
                Mat block = planes[channel](Rect(j, i, 8, 8));
                block.convertTo(block, CV_64FC1);
                subtract(block, 128.0, block);
                dct(block, block);

                if (channel == 0){
                    divide(block, quantLum, block);
                } else {
                    divide(block, quantChrom, block);
                }

                roundPixel(block);
                runLength(block);
                add(block, 128.0, block);
                block.convertTo(block, CV_8UC1);
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    merge(planes, dctImage);
}

vector<double> readCodedData() {
    vector<double> data;
    string line;
    ifstream dataFile(_savefile);

    while (getline(dataFile, line)) {
        stringstream ss(line);
        string value;

        while(getline(ss, value, ',')){
            data.push_back(atof(value.c_str()));
        }
    }
    dataFile.close();

    return data;
}

void rebuildVector(vector<double> data, vector<double>&values) {

    for (int i = 0; i < data.size() - 1; i+=2){
        double pixel = data[i];
        double amount = data[i+1];

        for(int j = 0; j < amount; j++){
            values.push_back(pixel);
        }
    }

}

void reBuildImage(Mat &image, vector<double>values){
    int width = image.size().width;
    int height = image.size().height;

    vector<Mat> planes;
    split(image, planes);

    for(int i=0; i < height; i+=8) {
        for (int j = 0; j < width; j += 8) {
            for (int channel = 0; channel < 3; channel++) {
                vector<double> data;
                for (int x = 0; x<64; x++){
                    data.push_back(values[x]);
                }
                values.erase(values.begin(), values.begin() + 64);
                Mat block = planes[channel](Rect(j, i, 8, 8));
                block.convertTo(block, CV_64FC1);
                UndozigZag(block, data);
                add(block, 128.0, block);
                block.convertTo(block, CV_8UC1);
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    merge(planes, image);
}

void deCompress(Mat &dctImage){
    vector<double> data;
    vector<double> values;
    data = readCodedData();
    auto width = (int)data[0];
    auto height = (int)data[1];
    data.erase(data.begin(), data.begin()+2);

    rebuildVector(data, values);
    Mat blank = Mat(height, width, CV_8UC3, CV_RGB(1,1,1));
    cvtColor(blank, blank, CV_BGR2YCrCb);
    reBuildImage(blank, values);

    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    vector<Mat> planes;
    split(blank, planes);

    for(int i=0; i < height; i+=8) {
        for (int j = 0; j < width; j += 8) {
            for (int channel = 0; channel < blank.channels(); channel++) {
                Mat block = planes[channel](Rect(j, i, 8, 8));
                block.convertTo(block, CV_64FC1);
                subtract(block, 128.0, block);

                if (channel == 0){
                    multiply(block, quantLum, block);
                } else {
                    multiply(block, quantChrom, block);
                }

                idct(block, block);
                add(block, 128.0, block);

                block.convertTo(block, CV_8UC1);
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    merge(planes, blank);
    cvtColor(blank, blank, CV_YCrCb2BGR);
    imshow(_dctwindow, blank);

}

void getCR(double &size){
    ifstream compressed(_savefile, ifstream::in | ifstream::binary);
    compressed.seekg(0, ios::end);
    double compSize = compressed.tellg();
    cout << compSize <<endl;
    compressed.close();

    ifstream orginal(_filename, ifstream::in | ifstream::binary);
    orginal.seekg(0, ios::end);
    double orginalSize = orginal.tellg();
    cout << orginalSize <<endl;
    orginal.close();

    size = orginalSize / compSize;
}

int main()
{
    Mat image = imread(_filename, 1);
    if(image.empty()) {
        cout<<"Can't load the image from "<<_filename<<endl;
        return -1;
    }

    int x = 0;
    int y = 0;
    double CR = 0;

    namedWindow(_windowname, CV_WINDOW_AUTOSIZE);
    moveWindow(_windowname, x, y);
    imshow(_windowname, image);

    x += 400;
    y += 0;

    int height = image.size().height;
    int width = image.size().width;
    int border_w = width % 8;
    int border_h = height % 8;

    Mat dctImage;
    copyMakeBorder(image,dctImage,0,border_h,0,border_w,BORDER_CONSTANT);
    compress(dctImage);

    getCR(CR);
    cout << CR << endl;

    deCompress(dctImage);
    Mat finalImage = dctImage(Rect(0,0,width,height));

    namedWindow(_dctwindow, CV_WINDOW_AUTOSIZE);
    moveWindow(_dctwindow, x,y);
    //imshow(_dctwindow, finalImage);
    //imwrite("../Images/2_compression1.bmp",finalImage);

    waitKey(0);

    destroyWindow(_windowname);
    destroyWindow(_dctwindow);
}