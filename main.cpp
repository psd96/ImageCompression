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
const char* _filename = "../Images/2.ppm";

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

}

int main()
{
    Mat image = imread(_filename, CV_LOAD_IMAGE_COLOR);
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

    int height = image.size().height;
    int width = image.size().width;
    Mat dctImage = image.clone();
    cvtColor(dctImage, dctImage, CV_BGR2YCrCb);
    /*dctImage.convertTo(dctImage, CV_64FC1);
    dct(dctImage, dctImage);
    imshow("DCT", dctImage);
    idct(dctImage, dctImage);
    dctImage.convertTo(dctImage, CV_8UC1);*/



    /*
    //Changing Quality Level
    for (int x = 0; x<8; x++){
        for (int y = 0; y<8; y++){
            //For more compression, lower image quality. QUALITY_LEVEL lower than 50
            data[x][y] = (50/QUALITY_LEVEL) * data[x][y];
            //For less compression, higher image quality. QUALITY_LEVEL higher than 50
             data[x][y] = ((100-QUALITY_LEVEL)/50) * data[x][y];

        }
    }
     */

    //Convert the 2D array data to image matrix
    Mat quantLum = Mat(8,8,CV_64FC1,&dataLum);
    Mat quantChrom = Mat(8,8,CV_64FC1,&dataChrom);



    //Discrete Cosine Transform
    //Splits Image into blocks 8x8 and performs DCT on each of these blocks.
    for(int i=0; i < height; i+=8) {
        for(int j=0; j < width; j+=8) {
            //Takes a block from Image of size 8x8 starting at (i,j)
            Mat block = dctImage(Rect(j,i,8,8));
            vector<Mat> planes;

            //Splits image into single channel array, planes.
            split(block,planes);
            vector<Mat> outplanes(planes.size());

            //Loop through all channels and perform DCT
            for(int k=0; k < planes.size(); k++) {
                planes[k].convertTo(planes[k],CV_64FC1);
                subtract(planes[k], 128.0, planes[k]);
                dct(planes[k],outplanes[k]);
                //Quantization
                divide(outplanes[k],quantLum,outplanes[k]);
                outplanes[k].convertTo(outplanes[k],CV_8UC1);
            }
            //Merges channel arrays into one
            merge(outplanes,block);

        }
    }
    //imshow("COMPRESSION", dctImage);

    //Inverse Discrete Cosine Transform
    for(int i=0; i < height; i+=8) {
         for(int j=0; j < width; j+=8) {
             Mat block = dctImage(Rect(j,i,8,8));
             vector<Mat> planes;
             split(block,planes);
             vector<Mat> outplanes(planes.size());
             for(int k=0; k < planes.size(); k++) {
                 planes[k].convertTo(planes[k],CV_64FC1);
                 multiply(planes[k],quantLum,planes[k]);
                 idct(planes[k],outplanes[k]);
                 add(planes[k], 128.0, planes[k]);
                 outplanes[k].convertTo(outplanes[k],CV_8UC1);
             }
             merge(outplanes,block);
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