#pragma once
#include "opencv2\core\types.hpp"
#include "opencv2\core\matx.hpp"
#include <vector>
#include "CommonLayersUtils.h"
#include <ostream>
#include <chrono>

#define NOMINMAX
#include <Windows.h>

// SEE THE DESCRIPTION OF THESE IN THE CPP FILE !!
struct GlobalParameters
{
	///////// FEATURES IDENTIFICATION PARAMS 
	///////////////////////////////////////////////////////////////////////////////////////////////
	static float maxDistToLine;
	static float maxSlopeAngleDiff;
	static float maxSlopeAngleDiff_tighter;
	static float maxSlopeAngleDiff_inCluster;

	static float minAngleForVerticalCategory;

	static float minPercent_longLineUsableHeight;
	static float maxPercent_leftSide;
	static float minPercent_rightSide;

	static float minPercent_longSidelineUsableWidth;
	static float minPercent_downSide;


	static float SAFE_OFFSET;
	static float maxDistToConsiderAsIntersection;
	static float maxDistToConsiderAsIntersectionSq;
	///////////////////////////////////////////////////////////////////////////////////////////////


	// PREPROCESSING PARAMETERS
	static float minAxisDimForBoundingRect;
	static float maxAxisDimForPlayerMarker;
	static int MIN_PIXELS_WIDTH_FOR_PLAYER_HISTOGRAM;
	static int MIN_PIXELS_HEIGHT_FOR_PLAYER_HISTOGRAM;

	static int DILATION_SIZE_FOR_BALLDETECTION;
	static int EROSION_SIZE_FOR_BALLDETECTION;
	static int MAX_RECT_TO_SEARCH_BALL_LAYER_1;
	static int MAX_RECT_TO_SEARCH_BALL_LAYER_2;
	static int MIN_VOTES_TO_CONSIDER_BALL_CENTER;
	static int BOUND_IRREGULARITY_THRESHOLD;
	static int MIN_ENERGY_IRREGULARITY_XAXIS;
	static int MIN_ENERGY_IRREGULARITY_YAXIS;
	static int MIN_XAXIS_PIXELS_FOR_IRREGULARITY_CHECK;
	static int MIN_YAXIS_PIXELS_FOR_IRREGULARITY_CHECK;

	static bool DO_BOUNDARY_CHECK;
	static bool DO_PLAYER_MARKERS_CHECK;

	// This is used in the dilation process for finding the ball 
	static cv::Mat BALL_FIND_DILATE_ELEMENT;
	static int BALL_HIST_RADIUS;
	static const char* BASE_PROJECT_PATH;

	static bool USE_SLOPE_FILTERING;
	static int MAX_DISTANCE_TO_DOMINANT_BIN;

	static int MIN_USABLE_LEN_FOR_A_CLUSTER;
	static int MIN_USABLE_LEN_FOR_A_CLUSTER_CONSIDERED_AS_BOX;

	// Where to take shirts images from
	const static char* m_homeShirtPath;
	const static char* m_awayShirtPath;
	const static char* GlobalParameters::m_debugImgPath;

	static void loadData();

	static const char* getFullPath(const char* fileName);

	static std::chrono::milliseconds FRAME_TIME;
	static const float POWER_DECAY_PER_FRAME;
};

namespace ComputerVisionLayer
{

#define INVALID_INDEX -1

// Keypoints in the pitch (usually intersection of pitch white lines)
enum PitchKeyPoints
{
	FEATURE_MIDDLE_UP,
	FEATURE_MIDDLE_DOWN,
	FEATURE_BOX_UP,
	FEATURE_BOX_DOWN,
	FEATURE_SMALL_BOX_UP,
	FEATURE_SMALL_BOX_DOWN,
	FEATURE_ENDLINE_UP,
	FEATURE_ENDLINE_DOWN,
	FEATURE_BOX_ON_SIDELINE_UP,
	FEATURE_BOX_ON_SIDELINE_DOWN,
	FEATURE_SMALL_BOX_ON_SIDELINE_UP,
	FEATURE_SMALL_BOX_ON_SIDELINE_DOWN,
	FEATURES_COUNT,
};

// Pitch edges - all white lines enumeration
enum PitchEdgeFeatures
{
	PITCH_FEATURE_NONE,
	PITCH_FEATURE_SIDELINE_UP,
	PITCH_FEATURE_SIDELINE_DOWN,
	PITCH_FEATURE_ENDLINE_LEFT,
	PITCH_FEATURE_ENDLINE_RIGHT,
	PITCH_FEATURE_BOX_LEFT_TOP,
	PITCH_FEATURE_BOX_LEFT_BOTTOM,
	PITCH_FEATURE_SMALL_BOX_LEFT_TOP,
	PITCH_FEATURE_SMALL_BOX_LEFT_BOTTOM,
	PITCH_FEATURE_BOX_RIGHT_TOP,
	PITCH_FEATURE_BOX_RIGHT_BOTTOM,
	PITCH_FEATURE_SMALL_BOX_RIGHT_TOP,
	PITCH_FEATURE_SMALL_BOX_RIGHT_BOTTOM,
	PITCH_FEATURE_MIDDLE_LINE,
};

static const char* g_KeypointsToStr[FEATURES_COUNT]=
{
	"M_U",
	"M_D",
	"B_U",
	"B_D",
	"SB_U",
	"SB_D",
	"E_U",
	"E_D",
	"B_S_U",
	"B_S_D",
	"SB_S_U",
	"SB_S_D",
};

const char* getFeatureStr(const PitchKeyPoints vf);


using KeyPointData = std::pair<PitchKeyPoints, cv::Vec2f>;

using Contour = std::vector<cv::Point>;



// Can identify concrete color classes inside an image
struct ColorUtils
{
	enum ColorCluster
	{
		CL_RED,
		CL_GREEN,
		CL_BLUE,
		CL_YELLOW,
		CL_ORANGE,
		CL_WHITE,
		CL_BLACK,
		CL_NUM_CLUSTERS
	};

	// Intervals for each of the above colors.
	// m_limits[c][0]  - min, [1] max
	static const cv::Vec3b m_limitsPerColorCluster[CL_NUM_CLUSTERS][2];

	static bool isHSVInCluster(const cv::Vec3b& pixelColor, const ColorCluster clusterToTest);

	static ColorCluster findClosestCluster(const cv::Vec3b& pixelColor);

	// Hack - we currently use pure black (0,0,0) instead of alpha channel. TODO ...
	static const cv::Vec3b ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;

private:
	// Checks if a given color HSV space is within a range


	static bool isHSVInCluster(const int hue, const int saturation, const int value, const ColorCluster clusterToTest);

	static bool inHSVRange(const int hue, const int saturation, const int value,
		const int minHue, const int minSat, const int minVal,
		const int maxHue, const int maxSat, const int maxVal);

};

struct Histogram
{
	void loadFromImage(const cv::Mat& inputImage, const cv::Rect2i& imagePart, const cv::Mat* maskAlphaImg, const cv::Mat* maskBinaryImg, const ColorUtils::ColorCluster clusterToIgnore = ColorUtils::CL_NUM_CLUSTERS);
	void loadFromImage(const char* fileName);
	void loadFromBinaryFile(const char* fileName);
	void saveToBinaryFile(const char* fileName) const;
	float compareTo(const Histogram& otherHistogram) const; // Returns the error /cost

	Histogram() { reset(); }
	void reset()
	{
		memset(m_pixelsPerCluster, 0, sizeof(m_pixelsPerCluster));
		m_numValidPixels = 0;
		m_predominantCluster = ColorUtils::CL_NUM_CLUSTERS;
	}

	int m_numValidPixels = 0;
	cv::Rect2i m_validBoundingRect;
	float m_pixelsPerCluster[ColorUtils::CL_NUM_CLUSTERS];

	ColorUtils::ColorCluster m_predominantCluster;
};

// This helps loading / binarizing the histograms of home/away shirts
// And compare a given part of image to one of these
// This is a kNN classification with 3 classes - None, HOME,AWAY
struct ContourClassification
{
	// First 2 params - the path to the data
	// If binaryLoading = true, data must be binary saved histograms and it will attempt to load the binaries
	// If saveBinary is true, data will be saved as binary after loading
	void loadHistograms(const char* homeShirt, const char* awayShirt, const bool binaryLoadingIfExists, const bool saveBinary);

	struct ClassificationScores
	{
		float m_scorePerCLass[TEAM_SIDE_NUMS];
		TeamSide m_predominantClass;

		ClassificationScores() { reset(); }
		void reset() 
		{ 
			memset(m_scorePerCLass, 0, sizeof(m_scorePerCLass)); 
			m_predominantClass = TEAM_SIDE_NUMS; 
		}
	};

	void compareHistograms(const cv::Mat& inputImage, const cv::Rect2i& imagePart, ClassificationScores& outScores) const;

//private:

	// TODO: add GK and referee too !
	Histogram m_homeHist;
	Histogram m_awayHist;
};

// Marker above controlled player detection
struct PlayerMarkerInfo
{
	PlayerMarkerInfo(const ColorUtils::ColorCluster _predColor, const cv::Rect2i& _rect)
	{
		predominantColor = _predColor;
		rect = _rect;
		teamSide = TEAM_SIDE_NUMS;
	}

	cv::Rect2i rect;							// the rect covering it
	ColorUtils::ColorCluster predominantColor;	// the predominant color cluster in the marker
	TeamSide teamSide;							// the team side owning this marker
};

// This is the output of processing a frame . Add this more features if you need it for other layers
struct VisionOutput
{
	void reset()
	{
		m_ballCoordinates_img.clear();
		m_ballCoordinates_real.clear();
		m_homePlayers_img.clear();
		m_homePlayers_real.clear();
		m_awayPlayers_img.clear();
		m_awayPlayers_real.clear();
		m_fieldSideToAttack = FIELD_SIDE_POSITIVE;

		m_isGoalVisible = false;
		m_keyPointsOutput_img.clear();
		m_keyPointsOutput_real.clear();

		m_isRealCoordinates = false;
	}


	// If this is true, we have real coordinates (i.e. pitch ones)
	// If not, then we have only relative coordinates. In this case you might want to compare positions relative to each other :)
	// The real ones are marked with _real, the img relative are marked with _img
	bool m_isRealCoordinates = false;

	using BallInfo = cv::Point2i;

	// Ball coordinates
	std::vector<BallInfo> m_ballCoordinates_img;	// Can be multiple balls or 0 :) You decide what you do with this output
	std::vector<BallInfo> m_ballCoordinates_real;
												
	// Markers - they might not exist and in that case hold the img! - only img coordinates of course
	std::vector<PlayerMarkerInfo> m_playerMarkers;

	using PlayerInfo = cv::Point2i;

	// Positions of home and away players
	std::vector<PlayerInfo> m_homePlayers_img;
	std::vector<PlayerInfo> m_homePlayers_real;

	std::vector<PlayerInfo> m_awayPlayers_img;
	std::vector<PlayerInfo> m_awayPlayers_real;

	// TODO: get current score and time and fill the direction to attack also
	FieldSide m_fieldSideToAttack = FIELD_SIDE_POSITIVE; 

	// Which side is visible ?? negative (left ) or positive (right)
	FieldSide m_visibleFieldSide = FIELD_SIDE_POSITIVE;

	// The keypoints visible - type and coordinate
	// This is how you can find out the goalZ position even if it's not visible, or real goal pos if all needed keypoints are available
	std::vector<KeyPointData> m_keyPointsOutput_img;
	std::vector<KeyPointData> m_keyPointsOutput_real;

	// Goal position stuff. If visible we have either img or real coordinates (depending on the variable switch at the top of this struct)
	bool m_isGoalVisible = false;
	cv::Point2f m_goalPos_img;
	cv::Point2f m_goalPos_real;



	friend std::ostream & operator<< (std::ostream &out, VisionOutput const &t);
};

cv::Mat captureFullScreenDesktop(HWND hwnd);

};

