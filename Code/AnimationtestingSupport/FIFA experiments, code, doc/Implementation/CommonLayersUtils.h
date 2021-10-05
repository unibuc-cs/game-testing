// Inter layers definitions
#pragma once
#include "opencv2/core/matx.hpp"
#include "opencv2/core/mat.hpp"

enum TeamSide
{
	TEAM_SIDE_HOME,
	TEAM_SIDE_AWAY,
	TEAM_SIDE_NEUTRAL,
	TEAM_SIDE_NUMS // i.e. we can't figure out side :)
};

enum FieldSide
{
	FIELD_SIDE_POSITIVE,
	FIELD_SIDE_NEGATIVE
};

namespace ComputerVisionLayer
{
	struct ContourClassification;
};

class CommonUtils
{
public:
	static FieldSide getOppositeFieldSide(const FieldSide side)
	{
		return side == FIELD_SIDE_POSITIVE ? FIELD_SIDE_NEGATIVE : FIELD_SIDE_POSITIVE;
	}

	static const char* fieldSideToString(const FieldSide side)
	{
		if (side == FIELD_SIDE_POSITIVE)
			return "POSITIVE";
		else
			return "NEGATIVE";
	}

	static void drawMultiLineText(cv::Mat& outImg, const char* stringToWrite, const int startCol, const int startRow, const int distBetweenRows
									, const cv::Scalar& color);
};


struct FrameParameters
{
	// Ball finding parameters - a recommended react, search outside if not found in recommended
	// And use 2 layer sizes - see the documentation function where these are used
	cv::Rect2i* m_recommendedRect = nullptr;
	bool m_searchBallOutsideHintRect = true;
	bool m_useBothLayersForBallDetection = true;

	// Debug params
	bool isInsideDebugEnabled = false; // Enable debug inside functions
	bool isTopLevelOutputDebugEnabled = false; // Final debug output for computer vision

	// Current image to process
	cv::Mat m_inputImg;
	cv::Mat m_originalImgCopy;

	const ComputerVisionLayer::ContourClassification* m_contourClassificationTool = nullptr;
};
