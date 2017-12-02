#include<iostream>
#include<cstdio>
#include<string>
#include<vector>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/objdetect/objdetect.hpp>

using namespace cv;
using namespace std;

const char* _windowname = "Original Image";
const char* _dctwindow = "DCT Image";
const char* _filename = "../Images/Fish.jpg";

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

void zigZag(Mat input){
    int lastVal = (input.size().height * input.size().width) - 1;
    int lastRow = input.rows - 1;
    int lastCol = input.cols - 1;

}

void getPixelVals(Mat input) {
    float val[64];
    int x = 0;
    for (int i = 0; i < input.size().height; i++) {
        for (int j = 0; j < input.size().width; j++) {
            //GET PIXEL VALUES AND DO SOMETHING
            val[x] = input.at<char>(j,i);
            x++;
        }
    }

    for (int i = 0; i < 64; i++) {
        cout << "" << val[i] << ",";
    }
    cout << ". \n" << endl;
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



/*void huffman(float &array[], int size){
    for (int i = 0; i<size; i++) {
        int freq = 0;
        int curr = array[i];
        for (int j = 0; j < size; j++) {
            if (array[j] == curr){
                freq++;
                //ONCE MATCHED REMOVE IT
            }
        }
    }
}*/

void compress(Mat image, Mat &dctImage){
    int height = image.size().height;
    int width = image.size().width;
    int border_w = width % 8;
    int border_h = height % 8;

    scaleQuant(10);
    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);

    copyMakeBorder(image,dctImage,0,border_h,0,border_w,BORDER_CONSTANT);
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

                add(block, 128.0, block);
                block.convertTo(block, CV_8UC1);
                block.copyTo(planes[channel](Rect(j, i, 8, 8)));
            }
        }
    }
    merge(planes, dctImage);
}

void deCompress(Mat &dctImage){
    //cvtColor(dctImage, dctImage, CV_BGR2YCrCb);
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

int main()
{
    Mat image = imread(_filename, 1);
    if(image.empty()) {
        cout<<"Can't load the image from "<<_filename<<endl;
        return -1;
    }

    int x = 0;
    int y = 0;

    namedWindow(_windowname, CV_WINDOW_AUTOSIZE);
    moveWindow(_windowname, x, y);
    imshow(_windowname, image);

    x += 400;
    y += 0;

    Mat dctImage;
    compress(image, dctImage);
    deCompress(dctImage);

    namedWindow(_dctwindow, CV_WINDOW_AUTOSIZE);
    moveWindow(_dctwindow, x,y);
    imshow(_dctwindow, dctImage);
    //imwrite("../Images/2_compression1.ppm",dctImage);

    waitKey(0);

    destroyWindow(_windowname);
    destroyWindow(_dctwindow);
}