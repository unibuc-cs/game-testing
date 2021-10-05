#pragma once

// This files serves as utils for preprocessing the image before entering to clusterization process
#include "opencv2\core\types.hpp"
#include "opencv2\core\matx.hpp"
#include <vector>
#include "ComputerVisionUtils.h"

namespace ComputerVisionLayer
{

#define USE_HSV_COLOR_SPACE


struct FramePreprocessingOutput
{
	// Img size
	float m_usableWidth = 0, m_usableHeight = 0;
	float m_totalWidth = 0, m_totalHeight = 0;

	// Pitch lines features (start - end pos)
	std::vector<cv::Vec4i> m_outPitchLines; // Lines within the pitch (white ones)

	// Contours represented as rects. Internally we can output polys but probably not important as output
	std::vector<cv::Rect2i> m_playersContoursBBoxes;
	// The list of players markers...hopefully will be at maximum 2 in a 2 players game :))
	// In a TV video match we don't have any so we have to decide in a different layer in AI system
	std::vector<PlayerMarkerInfo> m_playerMarkers;

	// The potential ist of ball coordinates. Normally should have only one element but who knows :)
	std::vector<cv::Point2i> m_ballCoordinatesInImg;
};

class FramePreprocessingTool
{
public:
	

private:
	int searchFiledPixels(cv::Mat& img, const int fixedAxisValue, const int dirSign, const bool isFixedAxis_Y);
	void removePitchBounds(cv::Mat& img, const ContourClassification& contourClassificationTool);
	void removeGreenColors(cv::Mat& imgSrc);
	void removeAllButWhitePixels(cv::Mat& img);
	
	void fillPlayersBoundaries(const cv::Mat& hsvInputImage, FramePreprocessingOutput& output, const bool isParentDebuggingEnabled);
	void fillPitchEdges(cv::Mat& hsvInputImage, const FrameParameters& input, FramePreprocessingOutput& output, const bool isParentDebuggingEnabled);
	void fillPlayerMarkers(const cv::Mat& inputBinaryImage, const cv::Mat& hsvInputImage, const std::vector<Contour>& originalContours, const std::vector<std::pair<int, cv::Rect2i>>& potentialPlayerMarkersIndices,
							const bool isParentDebuggingEnabled, FramePreprocessingOutput& output);

	// Check the comments in the CPP file for all these
	void findBallInHsvMask(const cv::Mat& inputImg, const cv::Mat& hsvMask, 
		const bool isParentDebugingEnabled, 
		const cv::Rect2i* hintToSearchBallIn, 
		const bool searchBallOutsideHint,
		const bool useBothRectLayersForBallFinding,
		std::vector<cv::Point2i>& outBallCoordinatesInImg) const;

public:

	// Responsible for finding pitch lines and player's contours
	bool preProcessImageFrame(cv::Mat& inputImage, const FrameParameters& inputParams, FramePreprocessingOutput& output, const bool isParentDebuggingEnabled);

	static void unitTestContours();
	static void unitTestPitchEdges();

	// Parameters
	//-----------------------------------------------------------------------

	const int MIN_CONSECUTIVE_GREEN_PIXELS_TO_PITCH = 30;
	const int MIN_TOTAL_GREEN_PIXELS_TO_PITCH = 50;

	// Thresholds for hough lines identification
	// TODO: implement our own version instead of the slow hough !!
	//int max_hough_threshold = 255;
	int hough_threshold = 45;
	int hough_minLineLength = 10, hough_maxLineGap = 10;
	//int max_hough_minLineLength = 200;
	//int max_hough_maxLineGap = 60;
};

};