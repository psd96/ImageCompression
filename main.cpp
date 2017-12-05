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

void zigZag(Mat input, vector<float>&output, int channels){
    int width = input.size().width - 1;
    int height = input.size().height - 1;
    float val = 0;

    int currX = 0;
    int currY = 0;
    //START (0,0)
    val = (float)input.at<Vec3b>(currX,currY)[channels];
    output.push_back(val);

    while (currX < width || currY < height) {

        //X+1 IF X!=WIDTH ELSE Y+1
        if ((currX + 1) <= width) {
            currX++;
            val = (float)input.at<Vec3b>(currX,currY)[channels];
            output.push_back(val);
        } else {
            currY++;
            val = (float)input.at<Vec3b>(currX,currY)[channels];
            output.push_back(val);
        }

        //DOWNWARDS DIAGONAL - -X AND +Y UNTIL Y=0
        while ((currX > 0) && (currY < height)) {
            currX--;
            currY++;
            val = (float)input.at<Vec3b>(currX,currY)[channels];
            output.push_back(val);
        }

        //Y+1 IF Y!=HEIGHT ELSE X+1
        if ((currY + 1) <= height) {
            currY++;
            val = (float)input.at<Vec3b>(currX,currY)[channels];
            output.push_back(val);
        } else {
            currX++;
            val = (float)input.at<Vec3b>(currX,currY)[channels];
            output.push_back(val);
        }

        //UPWARDS DIAGONAL - +X AND -Y UNTIL X=0
        while ((currX < width) && (currY > 0)) {
            currX++;
            currY--;
            val = (float)input.at<Vec3b>(currX,currY)[channels];
            output.push_back(val);
        }

    }

    //cout << "SIZE: " << output.size() << endl;
}

void roundPixel (Mat &input){
    for (int y = 0; y<input.size().height; y++) {
            for (int x = 0; x < input.size().width; x++) {
                double val = input.at<double>(x,y);
                val = round(val);
                input.at<double>(x,y) = val;
            }
    }
}

void saveToFile (vector<float>y){
    ofstream outputFile;
    outputFile.open(_savefile, ofstream::app);

    for(int i = 0; i<y.size(); i++){
            outputFile << y[i] << ",";
    }
    outputFile << "|";
    outputFile << endl;

    outputFile.close();
}

void runLength (Mat image, int channels){
    vector<float> values;
    vector<float> Yvalues;
    zigZag(std::move(image), values, channels);

    for (int i = 0; i<values.size(); i++){
        int Ycount = 0;
        Yvalues.push_back(values[i]);
        Ycount ++;
        while ((i+1)<values.size() && values[i] == values[i+1]){
            Ycount++;
            i++;
        }
        Yvalues.push_back((float)Ycount);
    }

    saveToFile(Yvalues);

}

void compress(Mat &dctImage){
    scaleQuant(50);
    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    int newWidth = dctImage.size().width;
    int newHeight = dctImage.size().height;
    cvtColor(dctImage, dctImage, CV_BGR2YCrCb);

    vector<Mat> planes;
    split(dctImage, planes);

    for(int i=0; i < newHeight; i+=8) {
        for (int j = 0; j < newWidth; j += 8) {
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
                runLength(block, channel);
                //Vec3b temp = block.at<Vec3b>(j,i);
                //cout << (float)temp[channel] << endl;
                add(block, 128.0, block);
                block.convertTo(block, CV_8UC1);
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    merge(planes, dctImage);
    //runLength(dctImage);
}

void readCodedData(){
    string line;
    vector<string> temp;
    ifstream dataFile (_savefile);
    while (getline(dataFile, line)){
        temp.emplace_back(line);
    }
    dataFile.close();

    /*for (int i =0; i<temp.size(); i++){
        cout << temp[i] << endl;

    }*/

}

void deCompress(Mat &dctImage){
    //cvtColor(dctImage, dctImage, CV_BGR2YCrCb);
    readCodedData();
    int newWidth = dctImage.size().width;
    int newHeight = dctImage.size().height;

    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    vector<Mat> planes;
    split(dctImage, planes);

    for(int i=0; i < newHeight; i+=8) {
        for (int j = 0; j < newWidth; j += 8) {
            for (int channel = 0; channel < dctImage.channels(); channel++) {
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
    merge(planes, dctImage);
    cvtColor(dctImage, dctImage, CV_YCrCb2BGR);
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

    //Mat compImage = dctImage(Rect(0,0,width,height));
    //saveToFile(dctImage);
    //imwrite("../DCT.png",dctImage);

    getCR(CR);
    cout << CR << endl;

    deCompress(dctImage);
    Mat finalImage = dctImage(Rect(0,0,width,height));

    namedWindow(_dctwindow, CV_WINDOW_AUTOSIZE);
    moveWindow(_dctwindow, x,y);
    imshow(_dctwindow, finalImage);
    //imwrite("../Images/2_compression1.bmp",finalImage);

    waitKey(0);

    destroyWindow(_windowname);
    destroyWindow(_dctwindow);

    /*Mat quantLum = Mat(8,8,CV_8UC1,&dataLum);
    vector<int> temp;

    zigZag(quantLum, temp);

    for (int i = 0; i < temp.size(); i++){
        cout << temp[i] << endl;
    }*/
}