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
int dataLum[8][8] = {
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

void round (vector<Mat> &input){
    for (int i = 0; i<input.size(); i++){
        for (int y = 0; y<input[i].size().height; y++) {
            for (int x = 0; x < input[i].size().width; x++) {
                round(input[i].at<double>(x,y));
            }
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

int main()
{
    Mat image = imread(_filename, 1);
    if(image.empty()) {
        cout<<"Can't load the image from "<<_filename<<endl;
        return -1;
    }

    int x = 0;
    int y = 0;


    /*for (int x = 0; x<8; x++){
        for (int y = 0; y<8; y++){
            //For more compression, lower image quality. QUALITY_LEVEL lower than 50
            dataLum[x][y] = (50/QUALITY_LEVEL) * dataLum[x][y];
            //For less compression, higher image quality. QUALITY_LEVEL higher than 50
            dataLum[x][y] = ((100-QUALITY_LEVEL)/50) * dataLum[x][y];
        }
    }*/



    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64F,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64F,&dataChrom);

    /*double scale = (200-2*80) / 100.0;
    multiply(quantLum, scale, quantLum);
    multiply(quantChrom, scale, quantChrom);*/

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
    cvtColor(dctImage, dctImage, CV_BGR2YCrCb);

    //Discrete Cosine Transform
    //Splits Image into blocks 8x8 and performs DCT on each of these blocks.
    for(int i=0; i < height; i+=8) {
        for(int j=0; j < width; j+=8) {
            Mat block = dctImage(Rect(j,i,8,8));
            //block.convertTo(block, CV_64F);
            vector<Mat>planes;
            split(block,planes);
            for (int k = 0; k < planes.size(); k++){
                planes[k].convertTo(planes[k], CV_64FC1);
                subtract(planes[k], 128.0, planes[k]);
                divide(planes[k], 255.0, planes[k]);
                dct(planes[k], planes[k]);

                if (k==0) {
                    divide(planes[k], quantLum, planes[k]);
                } else {
                    divide(planes[k], quantChrom, planes[k]);
                }

                //planes[k].convertTo(planes[k],CV_8UC1, 1.0, 0.0);
                //planes[k].convertTo(planes[k], CV_64FC1);

                if (k==0) {
                    multiply(planes[k], quantLum, planes[k]);
                } else {
                    multiply(planes[k], quantChrom, planes[k]);
                }

                idct(planes[k], planes[k]);
                multiply(planes[k], 255.0, planes[k]);
                add(planes[k], 128, planes[k]);
                planes[k].convertTo(planes[k], CV_8UC1);

            }
            merge(planes, block);
            //imshow(_dctwindow, dctImage);
            //multiply(block, 255.0, block);
            //add(block, 128, block);
            //block.convertTo(block, CV_8U);

        }
    }



    cvtColor(dctImage, dctImage, CV_YCrCb2BGR);
    namedWindow(_dctwindow, CV_WINDOW_AUTOSIZE);
    moveWindow(_dctwindow, x,y);
    imshow(_dctwindow, dctImage);
    //imwrite("../Images/2_compression1.ppm",dctImage);

    waitKey(0);

    destroyWindow(_windowname);
    destroyWindow(_dctwindow);
}