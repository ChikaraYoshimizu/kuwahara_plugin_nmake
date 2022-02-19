#include "PIDefines.h"
#include "PIFilter.h"
#include "opencv2/opencv.hpp"
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>

FilterRecord *gFR = NULL;
int32 *gDataHandle = NULL;
int16 StartProc(void);
SPBasicSuite *sSPBasic = NULL;

// Entry point
DLLExport MACPASCAL void PluginMain(
    const int16 selector,
    FilterRecord *filterRecord,
    int32 *data,
    int16 *result)
{
    // update our global parameters
    gFR = filterRecord;
    gDataHandle = data;

    // do the command according to the selector
    switch (selector)
    {
    case filterSelectorAbout:
        break;
    case filterSelectorParameters:
        break;
    case filterSelectorPrepare:
        break;
    case filterSelectorStart:
        *result = StartProc();
        break;
    case filterSelectorContinue:
        break;
    case filterSelectorFinish:
        break;
    }
}

// to avoid Visual Studio C2589 error
#undef min

// This class is essentially a struct of 4 Kuwahara regions surrounding a pixel, along with each one's mean, sum and variance.
class Regions
{
    int *Area[4];
    int Size[4];
    unsigned long long Sum[4];
    double Var[4];
    int kernel;

public:
    Regions(int _kernel) : kernel(_kernel)
    {
        for (int i = 0; i < 4; i++)
        {
            Area[i] = new int[kernel * kernel];
            Size[i] = 0;
            Sum[i] = 0;
            Var[i] = 0.0;
        }
    }

    // Update data, increase the size of the area, update the sum
    void sendData(int area, int data)
    {
        Area[area][Size[area]] = data;
        Sum[area] += data;
        Size[area]++;
    }
    // Calculate the variance of each area
    double var(int area)
    {
        int __mean = (int)(Sum[area] / Size[area]);
        double temp = 0;
        for (int i = 0; i < Size[area]; i++)
        {
            temp += (Area[area][i] - __mean) * (Area[area][i] - __mean);
        }
        if (Size[area] == 1)
            return 1.7e38; // If there is only one pixel inside the region then return the maximum of double
                           // So that with this big number, the region will never be considered in the below minVar()
        return sqrt(temp / (Size[area] - 1));
    }
    // Call the above function to calc the variances of all 4 areas
    void calcVar()
    {
        for (int i = 0; i < 4; i++)
        {
            Var[i] = var(i);
        }
    }
    // Find out which regions has the least variance
    int minVar()
    {
        calcVar();
        int i = 0;
        double __var = Var[0];
        if (__var > Var[1])
        {
            __var = Var[1];
            i = 1;
        }
        if (__var > Var[2])
        {
            __var = Var[2];
            i = 2;
        }
        if (__var > Var[3])
        {
            __var = Var[3];
            i = 3;
        }
        return i;
    }

    // Return the mean of that regions
    uchar result()
    {
        int i = minVar();
        return cv::saturate_cast<uchar>((double)(Sum[i] * 1.0 / Size[i]));
    }
};

class Kuwahara
{
private:
    int wid, hei, pad, kernel;
    cv::Mat image;

public:
    Regions getRegions(int x, int y)
    {
        Regions regions(kernel);

        uchar *data = image.data;

        // Update data for each region, pixels that are outside the image's boundary will be ignored.

        // Area 1 (upper left)
        for (int j = (y - pad >= 0) ? y - pad : 0; j >= 0 && j <= y && j < hei; j++)
            for (int i = ((x - pad >= 0) ? x - pad : 0); i >= 0 && i <= x && i < wid; i++)
            {
                regions.sendData(1, data[(j * wid) + i]);
            }
        // Area 2 (upper right)
        for (int j = (y - pad >= 0) ? y - pad : 0; j <= y && j < hei; j++)
            for (int i = x; i <= x + pad && i < wid; i++)
            {
                regions.sendData(2, data[(j * wid) + i]);
            }
        // Area 3 (bottom left)
        for (int j = y; j <= y + pad && j < hei; j++)
            for (int i = ((x - pad >= 0) ? x - pad : 0); i <= x && i < wid; i++)
            {
                regions.sendData(3, data[(j * wid) + i]);
            }
        // Area 0 (bottom right)
        for (int j = y; j <= y + pad && j < hei; j++)
            for (int i = x; i <= x + pad && i < wid; i++)
            {
                regions.sendData(0, data[(j * wid) + i]);
            }
        return regions;
    }

    // Constructor
    Kuwahara(const cv::Mat &_image, int _kernel) : kernel(_kernel)
    {
        image = _image.clone();
        wid = image.cols;
        hei = image.rows;
        pad = kernel - 1;
    }

    // Create new image and replace its pixels by the results of Kuwahara filter on the original pixels
    cv::Mat apply()
    {
        cv::Mat temp;
        temp.create(image.size(), CV_8U);
        uchar *data = temp.data;

        for (int j = 0; j < hei; j++)
        {
            for (int i = 0; i < wid; i++)
                data[j * wid + i] = getRegions(i, j).result();
        }
        return temp;
    }
};

//  main processing
int16 StartProc(void)
{
    // image size & depth.
    int16 width = gFR->filterRect.right - gFR->filterRect.left;
    int16 height = gFR->filterRect.bottom - gFR->filterRect.top;
    int16 planes = gFR->planes;

    // set all regions and channels to get whole image
    gFR->inLoPlane = 0;
    gFR->inHiPlane = planes - 1;
    gFR->outLoPlane = 0;
    gFR->outHiPlane = planes - 1;
    gFR->outRect = gFR->filterRect;
    gFR->inRect = gFR->filterRect;

    // get image copy
    int16 res = gFR->advanceState();
    if (res != noErr)
        return res;

    // head pointers to source & dest & mask image
    uint8 *inpix;
    uint8 *outpix;
    // uint8 * maskpix; // Need study: haveMask is always zero even if mask is set by photoshop.

    // filtering
    cv::Mat in = cv::Mat::zeros(width, height, CV_8UC3);
    cv::Mat out = cv::Mat::zeros(width, height, CV_8UC3);

    // process each pixel
    for (int y = 0; y < height; y++)
    {
        inpix = (uint8 *)gFR->inData + (y * gFR->inRowBytes);
        for (int x = 0; x < width; ++x)
        {
            for (int ch = 0; ch < planes; ++ch)
            {
                in.at<cv::Vec3b>(x, y)[ch] = inpix[ch];
            }
            inpix += planes;
        }
    }

    // cv::Mat img = imread("input.jpg", 1);
    int kernel = 15;

    cv::Mat dest, channels[3];
    // cv::split(img, channels);
    cv::split(in, channels);
    Kuwahara filter0(channels[0], kernel);
    Kuwahara filter1(channels[1], kernel);
    Kuwahara filter2(channels[2], kernel);

    std::vector<cv::Mat> color_shuffle;
    color_shuffle.push_back(filter0.apply());
    color_shuffle.push_back(filter1.apply());
    color_shuffle.push_back(filter2.apply());
    cv::merge(color_shuffle, out);

    for (int y = 0; y < height; y++)
    {
        outpix = (uint8 *)gFR->outData + (y * gFR->outRowBytes);
        // maskpix = (uint8 *) gFR->maskData + (y * gFR->maskRowBytes);
        for (int x = 0; x < width; ++x)
        {
            for (int ch = 0; ch < planes; ++ch)
            {
                outpix[ch] = out.at<cv::Vec3b>(x, y)[ch];
            }
            outpix += planes;
        }
    }

    // Setting rect to 0 means "processing done"
    memset(&(gFR->outRect), 0, sizeof(Rect));
    memset(&(gFR->inRect), 0, sizeof(Rect));

    return noErr;
}
