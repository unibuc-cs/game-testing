#include "ComputerVisionUtils.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgcodecs/imgcodecs_c.h"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/core/matx.hpp"

#include <iostream>
#include <string>

using namespace cv;
using namespace std;


//!!!!!!!!!!!!!! PARAMETERS !!!!!!!!!!!!!!
// Features identifications parameters
float GlobalParameters::maxDistToLine = 20; // Maximum distance from endpoints to a line to consider it there
float GlobalParameters::maxSlopeAngleDiff = 35; // The maximum angle diff to consider it on the same slope - for big pitch lines comparison
float GlobalParameters::maxSlopeAngleDiff_tighter = 10; // same as above but with tighter constraings
float GlobalParameters::maxSlopeAngleDiff_inCluster = 7.0f;  // The maximum angle diff to consider it on the same slope - for lines in the same cluster
float GlobalParameters::minAngleForVerticalCategory = 30; // The minimum slope angle for clustering in H/V category
float GlobalParameters::minPercent_longLineUsableHeight = 0.7f; // Min percent of height to consider it as a long end line or middle line
float GlobalParameters::maxPercent_leftSide = 0.4f; // The percent to consider it on left side
float GlobalParameters::minPercent_rightSide = 0.7f; // Same, right side
float GlobalParameters::minPercent_longSidelineUsableWidth = 0.7f;
float GlobalParameters::minPercent_downSide = 0.65f; // Down side
float GlobalParameters::SAFE_OFFSET = 10.0f; // Safe offset to consider position on the screen
float GlobalParameters::maxDistToConsiderAsIntersection = 30.0f;
float GlobalParameters::maxDistToConsiderAsIntersectionSq = maxDistToConsiderAsIntersection * maxDistToConsiderAsIntersection;


const char* GlobalParameters::m_homeShirtPath = "KIT_BARCELONA_HOME.png";
const char* GlobalParameters::m_awayShirtPath = "KIT_VILLAREAL.png";
const char* GlobalParameters::m_debugImgPath = "BarcaVillareal/fifa18_1.png";


// Preprocessing part parameters
float GlobalParameters::minAxisDimForBoundingRect = 7; // IF lower than this the bounding box of the respective contour is ignored totally
float GlobalParameters::maxAxisDimForPlayerMarker = 25; // If higher than this the bounding box suggesting a marker is not a valid marker

														// If the target image doesn't have at least this number of pixels we can't classify it !
int MIN_PIXELS_WIDTH_FOR_PLAYER_HISTOGRAM = 7;
int MIN_PIXELS_HEIGHT_FOR_PLAYER_HISTOGRAM = 10;

int GlobalParameters::DILATION_SIZE_FOR_BALLDETECTION = 7;
int GlobalParameters::EROSION_SIZE_FOR_BALLDETECTION = 3;
int GlobalParameters::MAX_RECT_TO_SEARCH_BALL_LAYER_1 = 20;
int GlobalParameters::MAX_RECT_TO_SEARCH_BALL_LAYER_2 = 100;

// The number of votes to consider a point as a circle in Hough circles
int GlobalParameters::MIN_VOTES_TO_CONSIDER_BALL_CENTER = 10;
int GlobalParameters::BALL_HIST_RADIUS = 6; // Pixels to test if ball is really the ball

int GlobalParameters::BOUND_IRREGULARITY_THRESHOLD = 12; // Min number of pixels to consider it a irregularity - a huge fall between consecutive pixels, not according to slope
int GlobalParameters::MIN_YAXIS_PIXELS_FOR_IRREGULARITY_CHECK = 10;
int GlobalParameters::MIN_XAXIS_PIXELS_FOR_IRREGULARITY_CHECK = 10;

// NUmber of pixels to consider that the irregularity is stabilized
int GlobalParameters::MIN_ENERGY_IRREGULARITY_XAXIS = 15;
int GlobalParameters::MIN_ENERGY_IRREGULARITY_YAXIS = 30;

bool GlobalParameters::DO_BOUNDARY_CHECK = true;
bool GlobalParameters::DO_PLAYER_MARKERS_CHECK = false;

// Slope stuff
bool GlobalParameters::USE_SLOPE_FILTERING = true;
int GlobalParameters::MAX_DISTANCE_TO_DOMINANT_BIN = 6;

int GlobalParameters::MIN_USABLE_LEN_FOR_A_CLUSTER = 50; // 150;
int GlobalParameters::MIN_USABLE_LEN_FOR_A_CLUSTER_CONSIDERED_AS_BOX = 220;

// Connected variables !!:
std::chrono::milliseconds GlobalParameters::FRAME_TIME = std::chrono::milliseconds(33);
const float GlobalParameters::POWER_DECAY_PER_FRAME = float(33) / 1000;

cv::Mat GlobalParameters::BALL_FIND_DILATE_ELEMENT;
const char* GlobalParameters::BASE_PROJECT_PATH = nullptr;

//-----------------------------------------------------------------------


void GlobalParameters::loadData()
{
	// TODO: save this globally somewhere !!! doesn't need to be recomputed each time
	GlobalParameters::BALL_FIND_DILATE_ELEMENT = getStructuringElement(MORPH_RECT, Size(GlobalParameters::DILATION_SIZE_FOR_BALLDETECTION, GlobalParameters::DILATION_SIZE_FOR_BALLDETECTION));

	GlobalParameters::BASE_PROJECT_PATH = std::getenv("OPENCV_PROJECT_BASE");
}

const char* GlobalParameters::getFullPath(const char* fileName)
{
	static char filePath[1024];
	sprintf_s(filePath, 1024, "%s%s", GlobalParameters::BASE_PROJECT_PATH, fileName);
	return filePath;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


namespace ComputerVisionLayer
{



// TODO: we really need to tune these :)
const cv::Vec3b ColorUtils::m_limitsPerColorCluster[CL_NUM_CLUSTERS][2] =
{
	{ cv::Vec3b(0, 60, 27), cv::Vec3b(180, 100, 75) }, // RED  -- THIS HAS A SPECIAL CASE !!!
	{ cv::Vec3b(38, 102, 0), cv::Vec3b(76, 255, 255) }, // GREEN
	{ cv::Vec3b(96, 30, 30), cv::Vec3b(145, 255, 255) }, // BLUE
	{ cv::Vec3b(25, 100, 100), cv::Vec3b(37, 255, 255) }, // YELLOW
	{ cv::Vec3b(10, 100, 20), cv::Vec3b(25, 255, 255) }, // ORANGE
	{ cv::Vec3b(0, 0, 77), cv::Vec3b(180, 45, 255) }, // WHITE
	{ cv::Vec3b(0, 0, 0), cv::Vec3b(180, 255, 70) }, // BLACK
};

const cv::Vec3b ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA = cv::Vec3b(0, 0, 0);

bool ColorUtils::isHSVInCluster(const cv::Vec3b& pixelColor, const ColorCluster clusterToTest)
{
	return isHSVInCluster(pixelColor[0], pixelColor[1], pixelColor[2], clusterToTest);
}

bool ColorUtils::isHSVInCluster(const int hue, const int saturation, const int value, const ColorCluster clusterToTest)
{
	if (clusterToTest == CL_RED)
	{
		Mat1b mask1, mask2;
		Vec3b minRed1(0, 30, 50), maxRed1(10, 255, 255);
		Vec3b minRed2(145, 30, 50), maxRed2(180, 255, 255);

		return (inHSVRange(hue, saturation, value, minRed1[0], minRed1[1], minRed1[2], maxRed1[0], maxRed1[1], maxRed1[2]) ||
			    inHSVRange(hue, saturation, value, minRed2[0], minRed2[1], minRed2[2], maxRed2[0], maxRed2[1], maxRed2[2]));
	}
	else
	{
		const Vec3b& min = m_limitsPerColorCluster[clusterToTest][0];
		const Vec3b& max = m_limitsPerColorCluster[clusterToTest][1];
		return inHSVRange(hue, saturation, value, min[0], min[1], min[2], max[0], max[1], max[2]);
	}
}

bool ColorUtils::inHSVRange(const int hue, const int saturation, const int value, const int minHue, const int minSat, const int minVal, const int maxHue, const int maxSat, const int maxVal)
{
	return (minHue <= hue && hue <= maxHue &&
		minSat <= saturation && saturation <= maxSat &&
		minVal <= value && value <= maxVal);
}

// Knn search
ColorUtils::ColorCluster ColorUtils::findClosestCluster(const cv::Vec3b& pixelColor)
{
	for (int i = 0; i < CL_NUM_CLUSTERS; i++)
	{
		// TODO - normally i should collect the clusters in range and select with NN
		if (isHSVInCluster(pixelColor, (ColorCluster)i))
			return (ColorCluster)i;
	}

	//assert("No cluster for this color !");
	return CL_NUM_CLUSTERS;
}



void ContourClassification::loadHistograms(const char* homeShirt, const char* awayShirt, const bool binaryLoadingIfExists, const bool saveBinary)
{
	std::string homeStr(GlobalParameters::getFullPath(homeShirt));
	std::string awayStr(GlobalParameters::getFullPath(awayShirt));

	std::string binaryHomeStr(homeStr.substr(0, homeStr.find_last_of('.')) + ".bin");
	std::string binaryAwayStr(awayStr.substr(0, awayStr.find_last_of('.')) + ".bin");


	if (binaryLoadingIfExists)
	{		
		m_homeHist.loadFromBinaryFile(binaryHomeStr.c_str());
		m_awayHist.loadFromBinaryFile(binaryAwayStr.c_str());
	}
	else
	{
		m_homeHist.loadFromImage(homeStr.c_str());
		m_awayHist.loadFromImage(awayStr.c_str());
	}

	if (saveBinary)
	{
		m_homeHist.saveToBinaryFile(binaryHomeStr.c_str());
		m_awayHist.saveToBinaryFile(binaryAwayStr.c_str());
	}
}

void ContourClassification::compareHistograms(const cv::Mat& inputImage, const cv::Rect2i& imagePart, ClassificationScores& outScores) const
{
	outScores.reset();

	// If the closest neighbor's error is higher than this we can't say which class it is
	static float ERR_THRESHOLD_FOR_UNKNOWN = 1.0f; 

	Histogram queryHist;
	queryHist.loadFromImage(inputImage, imagePart, nullptr, &inputImage);

	if (queryHist.m_validBoundingRect.width < MIN_PIXELS_WIDTH_FOR_PLAYER_HISTOGRAM ||
		queryHist.m_validBoundingRect.height < MIN_PIXELS_HEIGHT_FOR_PLAYER_HISTOGRAM)
	{
		outScores.m_predominantClass = TEAM_SIDE_NEUTRAL;
		return;
	}

	outScores.m_scorePerCLass[TEAM_SIDE_HOME] = m_homeHist.compareTo(queryHist);
	outScores.m_scorePerCLass[TEAM_SIDE_AWAY] = m_awayHist.compareTo(queryHist);

	outScores.m_predominantClass = outScores.m_scorePerCLass[TEAM_SIDE_HOME] < outScores.m_scorePerCLass[TEAM_SIDE_AWAY] ? TEAM_SIDE_HOME : TEAM_SIDE_AWAY;
	if (outScores.m_scorePerCLass[outScores.m_predominantClass] > ERR_THRESHOLD_FOR_UNKNOWN)
	{
		outScores.m_predominantClass = TEAM_SIDE_NEUTRAL;
		return;
	}

	assert(0 <= outScores.m_predominantClass && outScores.m_predominantClass < TEAM_SIDE_NUMS);
}

#define HIST_DEBUG_PIXEL_ENABLED 1

void Histogram::loadFromImage(const cv::Mat& inputImage, const cv::Rect2i& imagePart, const cv::Mat* maskAlphaImg, const cv::Mat* maskBinaryImg, const ColorUtils::ColorCluster clusterToIgnore)
{
	const int x0 = imagePart.x, x1 = x0 + imagePart.width, 
			  y0 = imagePart.y, y1 = y0 + imagePart.height;

	int xmin = inputImage.cols, xmax = -1;
	int ymin = inputImage.rows, ymax = -1;

	static int debugPixelX = 771;
	static int debugPixelY = 12;

	// Count the pixel's cluster in the region image
	int totalValidPixels = 0;
	for (int y = y0; y < y1; y++)
	{
		for (int x = x0; x < x1; x++)
		{
#if HIST_DEBUG_PIXEL_ENABLED
			if (x == debugPixelX && y == debugPixelY)
			{
				int a = 3;
				a++;
			}
#endif
		
			// TODO: use template to fix performance issue because of branching
			Vec3b pixel;
			if (maskAlphaImg)
			{
				const Vec4b pixelWithAlpha = maskAlphaImg->at<Vec4b>(Point2i(x, y));
				if (pixelWithAlpha[3] < 255)
					continue;
			}
			else if (maskBinaryImg)
			{
				const Vec3b binaryPixel = maskBinaryImg->at<Vec3b>(Point2i(x, y));
				if (binaryPixel == ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA)
					continue;
			}

			// Valid pixel, augment min/max bounding rect
			xmin = std::min(xmin, x);
			ymin = std::min(ymin, y);
			xmax = std::max(xmax, x);
			ymax = std::max(ymax, y);

			pixel = inputImage.at<Vec3b>(Point2i(x, y));			
			if (pixel == ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA) // Using pure black instead of alpha..of course, a hack that would be difficult to debug later :)
				continue;

			const ColorUtils::ColorCluster closestCluster = ColorUtils::findClosestCluster(pixel);

#if HIST_DEBUG_PIXEL_ENABLED // Debug stuff to compare this with GIMP output
			//{
				const int gimpHue = pixel[0] * 2;
				const int gimpSat = (int)(((pixel[1] + 0.0f) / 255.0f)*100.0f);
				const int gimpVal = (int)(((pixel[2] + 0.0f) / 255.0f)*100.0f);

			//}
#endif
			if (closestCluster != ColorUtils::CL_NUM_CLUSTERS && closestCluster != clusterToIgnore)
			{
				totalValidPixels++;
				m_pixelsPerCluster[closestCluster] += 1.0f;
			}
		}
	}

	// Normalize the histogram
	m_numValidPixels			= totalValidPixels;
	m_validBoundingRect.x		= xmin;
	m_validBoundingRect.y		= ymin;
	m_validBoundingRect.width	= xmax - xmin + 1;
	m_validBoundingRect.height	= ymax - ymin + 1;

	if (m_numValidPixels > 0)
	{
		for (int i = 0; i < ColorUtils::CL_NUM_CLUSTERS; i++)
		{
			m_pixelsPerCluster[i] /= m_numValidPixels;
		}
	}

	m_predominantCluster = ColorUtils::CL_NUM_CLUSTERS;
	float predominantClusterVal = -1.0f;
	for (int i = 0; i < ColorUtils::CL_NUM_CLUSTERS; i++)
	{
		if (m_pixelsPerCluster[i] > predominantClusterVal)
		{
			predominantClusterVal = m_pixelsPerCluster[i];
			m_predominantCluster = (ColorUtils::ColorCluster)i;
		}
	}
}

void Histogram::loadFromBinaryFile(const char* fileName)
{
	FILE *f = fopen(fileName, "rb");
	fread(m_pixelsPerCluster, sizeof(m_pixelsPerCluster[0]), sizeof(m_pixelsPerCluster) / sizeof(m_pixelsPerCluster), f);
	fclose(f);
}

float Histogram::compareTo(const Histogram& otherHistogram) const
{
	// L1 diff 
	float totalDiff = 0.0f;
	for (int i = 0; i < ColorUtils::CL_NUM_CLUSTERS; i++)
	{
		float localDiff = fabsf(m_pixelsPerCluster[i] - otherHistogram.m_pixelsPerCluster[i]);
		totalDiff += localDiff;
	}

	return totalDiff;
}

void Histogram::saveToBinaryFile(const char* fileName) const
{
	FILE* f = fopen(fileName, "wb");
	fwrite(m_pixelsPerCluster, sizeof(char), sizeof(m_pixelsPerCluster), f);

	fclose(f);	
}

void Histogram::loadFromImage(const char* fileName)
{
	cv::Mat img = cv::imread(fileName, CV_LOAD_IMAGE_UNCHANGED);
	const int numChannels = img.channels();

	// Convert images to HSV and keep the mask
	cv::Mat hsvImg;
	cv::cvtColor(img, hsvImg, CV_BGRA2BGR);
	cv::cvtColor(hsvImg, hsvImg, CV_BGR2HSV);

	loadFromImage(hsvImg, cv::Rect2i(0, 0, hsvImg.cols, hsvImg.rows), (numChannels == 4 ? &img : nullptr), nullptr);
}

cv::Mat captureFullScreenDesktop(HWND hwnd)
{
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	cv::Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom / 1;  //change this to whatever size you want to resize to
	width = windowsize.right / 1;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

																									   // avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

};
