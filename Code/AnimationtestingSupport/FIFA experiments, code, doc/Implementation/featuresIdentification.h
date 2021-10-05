#pragma once

#define EPSILON 0.0001f

#include <opencv2/core/matx.hpp>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <opencv2\core\matx.hpp>
#include <opencv2\core\mat.hpp>
#include <opencv2\core\types.hpp>
#include <array>
#include "CommonLayersUtils.h"
#include "ComputerVisionUtils.h"

namespace ComputerVisionLayer
{



class Utils
{
public:
	static float radToDeg(const float rad)
	{
		return (rad * 180.0f) / (float)M_PI;
	}

	static float degToRad(const float deg)
	{
		return (deg * (float)M_PI) / 180.0f;
	}
};

// Stores informations about a single segment after a lines detection alg run
struct ImgSegment
{
	ImgSegment();

	void computeSlope();

	void transformEq();

	ImgSegment(int _x0, int _y0, int _x1, int _y1, int _origIndex);

	float distancePointToThisSegment(float _x0, float _y0) const;
	cv::Vec2f getProjectionPointOnThisSegment(float _x0, float _y0) const;
	void setPoints(int _x0, int _y0, int _x1, int _y1);

	float m_slope;
	float m_slopeAngle;
	float m_a, m_b, m_c; // Parameters for line distance
	float m_distanceDenomitorCached;
	float m_x0, m_y0;
	float m_x1, m_y1;

	int m_origIndex; // THe original index in the entire segments list
};

struct SidelineVisibleInfo
{
	bool isVisible; // Is visible on the screen on just cutting the edge of the screen and consider it there
	int horizontalSegmentIndex;  // If visible this will point to the segment
	cv::Vec2f startPos;
	cv::Vec2f endPos;
	bool isUp;

	SidelineVisibleInfo(const bool _isUp)
		: isUp(_isUp)
	{
		reset(0, 0);
	}

	void reset(const float totalWIDTH, const float totalHEIGHT)
	{
		isVisible = false;
		horizontalSegmentIndex = -1;

		startPos[0] = 0;
		endPos[0] = totalWIDTH;

		startPos[1] = isUp ? 0 : totalHEIGHT;
		endPos[1] = isUp ? 0 : totalHEIGHT;
	}
};

// Multiple segments that lay on the same line considering their 
// slope and small error distance to it 
struct ImgSegmentCluster
{
	explicit ImgSegmentCluster(const bool _isHorizontal) 
		: isHorizontal(_isHorizontal) 
		, m_xStart(FLT_MAX)
		, m_xEnd(FLT_MIN)
		, m_yMin(FLT_MAX)
		, m_yMax(FLT_MIN)
		, m_yStart(FLT_MAX)
		, m_yEnd(FLT_MIN)
		, m_xMin(FLT_MAX)
		, m_xMax(FLT_MIN)
		{}

	bool isHorizontal = false; // True if horizontal cluster

	// TODO: should be union or collapsed...
	// Horizontal lines data
	float m_xStart, m_xEnd;  // Where it starts and stops
	float m_yMin, m_yMax; // The y where it starts


	// Vertical lines data
	float m_yStart, m_yEnd; // Where it starts stops
	float m_xMin, m_xMax; // The x where it starts and ends


	// Indices into m_horizontalSegments / m_verticalSegments
	// of all segments that lie in this component
	std::vector<int>	m_segmentIndices;
	ImgSegment			m_representativeSegment; // Representative segment

	PitchEdgeFeatures   m_pitchEdgeFeature = PITCH_FEATURE_NONE;

	void addSegment(const ImgSegment& segment, const int index);
	bool isSegmentInCluster(const ImgSegment& otherSeg) const;
	float len() const
	{
		if (isHorizontal)
		{
			return m_xEnd - m_xStart;
		}
		else
		{
			return m_yEnd - m_yStart;
		}
	}

	bool isParallelWith(const ImgSegmentCluster& otherCluster, bool tigther = false) const
	{
		return (fabsf(fabsf(m_representativeSegment.m_slopeAngle) - fabsf(otherCluster.m_representativeSegment.m_slopeAngle)) < ( tigther == false ? GlobalParameters::maxSlopeAngleDiff : GlobalParameters::maxSlopeAngleDiff_tighter));
	}
};


struct CoordinateConversionOutput
{
	CoordinateConversionOutput() { reset(); }

	void loadTargetKeyPoints();

	void reset()
	{
		m_hasAll4PointsReady = false;
		m_isRightSide = false;
		m_keyPointsOutput.clear();

		loadTargetKeyPoints();
	}

	void addKeyPoint(const PitchKeyPoints feature, const cv::Vec2f& pos)
	{
		m_keyPointsOutput.push_back(std::make_pair(feature, pos));
	}

	int getNumKeyPoints() const { return (int)m_keyPointsOutput.size(); }
	const KeyPointData& getKeyPointByIndex(const int index) const { return m_keyPointsOutput[index]; }


	//Should be called after keypoints are filled in
	void doPostcomputations(const bool isLeftSided);

	// Transforms a given image point to a real point coordinate
	bool transformImgPointToRealCoords(const cv::Point2f& imgPoint, const cv::Point2f& outRealPoint) const;

	bool hasHomographMarix() const { return m_hasAll4PointsReady; }

	const KeyPointData* getKeyPointDataByType(const PitchKeyPoints keyPointType)
	{
		// TODO: use a map ?
		for (const KeyPointData& data : m_keyPointsOutput)
		{
			if (data.first == keyPointType)
				return &data;
		}

		return nullptr;
	}

	bool isRightSidedView () const  { return m_isRightSide; }
private:

	bool m_hasAll4PointsReady;

	cv::Mat m_homographMatrix; // Used to convert from image to world coordinates
	bool m_isRightSide; // If true, the image is taken in the right side of the pitch

	// Pairs of (feature type, position of the feature)
	std::vector<KeyPointData> m_keyPointsOutput;

	// Static loaded data from real world stadium picture img with real keypoints
	std::vector<cv::Point2f> m_realStadiumTargetKeypoints;
};

// Processes a vector of lines from an algorithm's identification output such as Hough
// The output is obtained using classifyCluster - in a ClassificationOutput structure
struct PitchEdgesClassificationTool
{
	PitchEdgesClassificationTool()
		: m_upSideline(true)
		, m_downSideline(false)
	{}

	struct ClusterClassification
	{
		int index; // the index of the cluster in the list
		int id; // the id assigned to the cluster - check the image !

		ClusterClassification(int _index, int _id)
		{
			index = _index;
			id = _id;
		}
	};

	void processComponents(const std::vector<cv::Vec4i>& lines, 
						   const float usableWeight, const float usableHeight, const float totalWeight, const float totalHeight,
						   const bool isParentDebuggingEnabled,
						   CoordinateConversionOutput& output);

	void clusterizeSegments(const std::vector<ImgSegment>& segmentsList, 
							std::vector<ImgSegmentCluster>& components,
							std::vector<int>& clustersSortedByLenKey,
							std::vector<int>& clustersSortedByMinAxisKey,
							const bool isHorizontal) const;

	// Classifies the given cluster in pitch features

	// Finds the main visible clusters (possibly representing pitch edges)
	void findMainVisibleClusterFeatures(const std::vector<ImgSegmentCluster>& clusters,  
						  const std::vector<int>& clustersSortedByLenKey,
						  const std::vector<int>& clustersSortedByMinAxisKey,
						  std::vector<ClusterClassification>& outClassification,
						  const bool isHorizontal) const;

	// Stores the keypoints if found, homograph matrix and if the side is left or right
	

	// Classifies the previously find visible clusters
	void classifyClusters(const float usableWIDTH, const float usableHEIGHT, 
						  const float totalWIDTH, const float totalHEIGHT,
						  CoordinateConversionOutput &output);

	// Returns if the intersection is with up / down items
	bool isClusterLongVerticalEdge(const ImgSegmentCluster& cluster, const float usableHEIGHT,
								   const SidelineVisibleInfo& upSideline, const SidelineVisibleInfo& downSideline,
									bool& outUpIntersect, cv::Vec2f& outUpSidelineIntersectPos, 
									bool &outDownIntersect, cv::Vec2f& outDownSidelineIntersectPos, 
									bool &outIsEndline) const;

	// Check if the test (horizontal) cluster given as parameter intersects the given endline cluster
	// If it does, returns ture and the intersection pos
	bool checkHorizontalClusterIntersectEndline(const ImgSegmentCluster& endlineCluster, const ImgSegmentCluster& testCluster,
		cv::Vec2f& outIntersectPos, const float usableHEIGHT) const;

	// Check if the two given clusters intersects in one of their endpoints
	bool checkHorizontalAndVerticalClustersIntersectsInEndpoints(const ImgSegmentCluster& cluster_horizontal, const ImgSegmentCluster& vertical,
		cv::Vec2f& outIntersectPos);

	bool isClusterEndline(const ImgSegmentCluster& cluster, const float usableHEIGHT,
		const SidelineVisibleInfo& upSideline, const SidelineVisibleInfo& downSideline) const;

	bool isClusterMiddleline(const ImgSegmentCluster& cluster, const float usableHEIGHT,
		const SidelineVisibleInfo& upSideline, const SidelineVisibleInfo& downSideline) const;

	bool isClusterOnLeftVerticalSide(const ImgSegmentCluster& cluster, const float usableWIDTH) const;
	bool isClusterOnRightVerticalSide(const ImgSegmentCluster& cluster, const float usableWIDTH) const;
	bool isClusterOnCenterVerticalSide(const ImgSegmentCluster& cluster, const float usableWIDTH) const;
	//bool isClusterBoxVerticalEdge(const ImgSegmentCluster& cluster, const float usableHEIGHT) const;

	bool isClusterOnDownHorizontalSide(const ImgSegmentCluster& cluster, const float totalHEIGHT) const;

	bool isCoordinateOnSafeHeight(const float usableHEIGHT, const float x0, const float y0) const;

	void drawClusters(cv::Mat& outputImg, 
					  const std::vector<ImgSegmentCluster>& clusters, 
					  const std::vector<ImgSegment>& segments, const cv::Scalar* forcedColor = nullptr) const;

	static void unitTestPitchFeatures();
	static void unitTestKeypointsHomography();

	// Details about all horizontal components
	// -------------------------------------------------
	std::vector<ImgSegmentCluster>	m_horizontalClusters; // The real clusters
	std::vector<int>				m_horizontalClusters_sortLenKey; // clusters sorted by len key
	std::vector<int>				m_horizontalClusters_sortYMinKey; // clusters sorted by the Y min key
	std::vector<ImgSegment>			m_horizontalSegments; // Segments classified as horizontal
	std::vector<ImgSegment>			m_tempHorizontalSegments;

	std::vector<ClusterClassification>	m_horizontalClustersObservable; // The List of maximum 6 observable horizontal clusters parallel defining pitch edges
	
	// Details about sidelines visibility
	SidelineVisibleInfo				m_upSideline;
	SidelineVisibleInfo				m_downSideline;
	// -------------------------------------------------

	// Details about all vertical components
	// -------------------------------------------------
	std::vector<ImgSegmentCluster>	m_verticalClusters; // The real clusters
	std::vector<int>				m_verticalClusters_sortLenKey; // clusters sorted by len key
	std::vector<int>				m_verticalClusters_sortXMinKey; // clusters sorted by the X min key
	std::vector<ImgSegment>			m_verticalSegments; 
	std::vector<ImgSegment>			m_tempVerticalSegments;

	std::vector<ClusterClassification> m_verticalClustersObservable; // The list of maximum 3 observable vertical clusters parallel defining pitch edges
	//--------------------------------------------------
};

};