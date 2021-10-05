#include "FeaturesIdentification.h"
#include "CommonLayersUtils.h"
#include "ComputerVisionUtils.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/calib3d/calib3d_c.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>

#pragma warning(disable : 4100)

namespace ComputerVisionLayer
{

const char* getFeatureStr(const PitchKeyPoints vf)
{
	return g_KeypointsToStr[vf];
}

ImgSegment::ImgSegment()
	: m_slope(0.0f)
	, m_slopeAngle(0.0f)
	, m_a(0.0f)
	, m_b(0.0f)
	, m_c(0.0f)
	, m_distanceDenomitorCached(0.0f)
	, m_origIndex(-1)
{}

void ImgSegment::computeSlope()
{
	const float dy = m_y1 - m_y0;
	float dx = m_x1 - m_x0;
	if (dx == 0.0f)
		dx = EPSILON;

	m_slope = dy / dx;
	m_slopeAngle = Utils::radToDeg(atan(m_slope));
}

void ImgSegment::transformEq()
{
	m_a = m_slope;
	m_b = -1;

	const float n = (m_y0 - m_slope * m_x0);
	m_c = n;

	m_distanceDenomitorCached = sqrtf(m_a*m_a + m_b*m_b);
}

ImgSegment::ImgSegment(int _x0, int _y0, int _x1, int _y1, int _origIndex)
{
	m_origIndex = _origIndex;
	setPoints(_x0, _y0, _x1, _y1);
}

void ImgSegment::setPoints(int _x0, int _y0, int _x1, int _y1)
{
	m_x0 = (float)_x0;
	m_y0 = (float)_y0;
	m_x1 = (float)_x1;
	m_y1 = (float)_y1;

	computeSlope();
	transformEq();
}

float ImgSegment::distancePointToThisSegment(float _x0, float _y0) const
{
	const float x0 = (float)_x0;
	const float y0 = (float)_y0;

	const float nominator = fabs(m_a * x0 + m_b * y0 + m_c);
	const float res = nominator / m_distanceDenomitorCached;
	return res;
}

cv::Vec2f ImgSegment::getProjectionPointOnThisSegment(float _x0, float _y0) const
{
	cv::Vec2f P; // Point to project 
	P[0] = _x0;
	P[1] = _y0;

	cv::Vec2f v1, v2; // Line to project on
	v1[0] = m_x0; v1[1] = m_y0;
	v2[0] = m_x1; v2[1] = m_y1;

	// Line dir (e1) and start to Point  (e1)
	const cv::Vec2f e1 = v2 - v1;
	const cv::Vec2f e2 = P - v1;
	
	const float valDp = e2.dot(e1);
	const float e1LenSq = e1.dot(e1);
	
	cv::Vec2f res;
	res[0] = v1[0] + (valDp * e1[0]) / e1LenSq;
	res[1] = v1[1] + (valDp * e1[1]) / e1LenSq;

	return res;
}

void ImgSegmentCluster::addSegment(const ImgSegment& segment, const int index)
{
	bool updateReprSegmentEnds = true;
	if (m_segmentIndices.empty())
	{
		m_representativeSegment = segment;		
		updateReprSegmentEnds = false;
	}

	// Get the projection points on the current representative segment, and extend it if needed !
	cv::Vec2f startProj, endProj;
	if (updateReprSegmentEnds)
	{
		startProj = m_representativeSegment.getProjectionPointOnThisSegment(segment.m_x0, segment.m_y0);
		endProj = m_representativeSegment.getProjectionPointOnThisSegment(segment.m_x1, segment.m_y1);

		const cv::Vec2f segmentStart(m_representativeSegment.m_x0, m_representativeSegment.m_y0);
		const cv::Vec2f segmentEnd(m_representativeSegment.m_x1, m_representativeSegment.m_y1);

		const cv::Vec2f segmentStartToEnd = segmentEnd - segmentStart;
		const cv::Vec2f segmentEndToStart = -segmentStartToEnd;

		const cv::Vec2f segmentStartToProjStart = startProj - segmentStart;
		const cv::Vec2f segmentEndToProjEnd = endProj - segmentEnd;

		if (segmentStartToProjStart.dot(segmentStartToEnd) < 0)
		{
			m_representativeSegment.m_x0 = startProj[0];
			m_representativeSegment.m_y0 = startProj[1];
		}

		if (segmentEndToProjEnd.dot(segmentEndToStart) < 0)
		{
			m_representativeSegment.m_x1 = endProj[0];
			m_representativeSegment.m_y1 = endProj[1];
		}
	}

	// Update detected lines
	if (isHorizontal)
	{
		m_xStart = std::min(m_xStart, segment.m_x0);
		m_xEnd = std::max(m_xEnd, segment.m_x1);
		
		m_yMin = std::min(m_yMin, std::min(segment.m_y0, segment.m_y1));
		m_yMax = std::max(m_yMax, std::max(segment.m_y0, segment.m_y1));	
	}
	else
	{
		m_yStart = std::min(m_yStart, segment.m_y0);
		m_yEnd = std::max(m_yEnd, segment.m_y1);
		
		m_xMin = std::min(m_xMin, std::min(segment.m_x0, segment.m_x1));
		m_xMax = std::max(m_xMax, std::max(segment.m_x0, segment.m_x1));
	}

	m_segmentIndices.push_back(index);
}

bool ImgSegmentCluster::isSegmentInCluster(const ImgSegment& otherSeg) const
{
	if (fabsf(m_representativeSegment.m_slopeAngle - otherSeg.m_slopeAngle) > GlobalParameters::maxSlopeAngleDiff_inCluster)
		return false;

	const float distP0 = m_representativeSegment.distancePointToThisSegment(otherSeg.m_x0, otherSeg.m_y0);
	if (distP0 > GlobalParameters::maxDistToLine)
		return false;

	const float distP1 = m_representativeSegment.distancePointToThisSegment(otherSeg.m_x1, otherSeg.m_y1);
	if (distP1 > GlobalParameters::maxDistToLine)
		return false;

	return true;
}

struct SlopeBin
{
	static const int SLOPE_BIN_FACTOR = 5;
	static const int NUM_BINS = 360 / SLOPE_BIN_FACTOR + 1;

	static int degToBin(float val) { return ((int)(val / SLOPE_BIN_FACTOR) + NUM_BINS / 2); }
	static int binToDeg(int bin) { return (bin - NUM_BINS/2) * SLOPE_BIN_FACTOR; }

	void addBinVal(float val)
	{
		const int bin = degToBin(val);

		assert(bin < NUM_BINS);

		slopeBin[bin]++;
	}

	bool getArgmax(int& outBin, int& outDeg) const
	{
		outBin = -1;
		outDeg = -900;
		int max = -1;

		for (int i = 0; i < NUM_BINS; i++)
		{
			if (slopeBin[i] > max)
			{
				max		= slopeBin[i];
				outBin	= i;
			}
		}

		if (max == -1)
			return false;

		outDeg = binToDeg(outBin);
		return true;
	}

	int slopeBin[NUM_BINS] = { 0 };
};

void PitchEdgesClassificationTool::processComponents(const std::vector<cv::Vec4i>& lines
											, const float usableWeight, const float usableHeight, const float totalWidth, const float totalHeight
											, const bool isParentDebuggingEnabled
											, CoordinateConversionOutput& output)
{
	m_tempVerticalSegments.clear();
	m_verticalSegments.clear();
	m_tempHorizontalSegments.clear();
	m_horizontalSegments.clear();

	SlopeBin slopeBin_horizontal;
	SlopeBin slopeBin_vertical;

	const bool isLocalDebuggingEnabled = false || isParentDebuggingEnabled;
	for (size_t i = 0; i < lines.size(); i++)
	{
		const cv::Vec4i& line = lines[i];
		ImgSegment segment(line[0], line[1], line[2], line[3], (int)i);

		if (fabsf(segment.m_slopeAngle) > GlobalParameters::minAngleForVerticalCategory)
		{
			if (segment.m_y0 > segment.m_y1)
			{
				segment.setPoints((int)segment.m_x1, (int)segment.m_y1, (int)segment.m_x0, (int)segment.m_y0);
			}

			/*
			// DEBUG CODE
			if (segment.m_x0 >= 0 && segment.m_x1 <= 200 && segment.m_y0 >= 100 && segment.m_y1 < 332)
			{
				int a = 3;
				a++;
			}*/

			slopeBin_vertical.addBinVal(segment.m_slopeAngle);
			m_tempVerticalSegments.emplace_back(segment);
		}
		else
		{
			if (segment.m_x0 > segment.m_x1)
			{
				segment.setPoints((int)segment.m_x1, (int)segment.m_y1, (int)segment.m_x0, (int)segment.m_y0);
			}

			slopeBin_horizontal.addBinVal(segment.m_slopeAngle);
			m_tempHorizontalSegments.emplace_back(segment);
		}
	}

	if (GlobalParameters::USE_SLOPE_FILTERING)
	{

		// Add only the segment lines that respect the dominant slope criteria
		auto filterSlopeFunc = [&](const SlopeBin& slopeBin, std::vector<ImgSegment>& tempArr, std::vector<ImgSegment>& outArr)
		{
			int dominantBin = -1;
			int dominantDeg = 0;
			if (!slopeBin.getArgmax(dominantBin, dominantDeg))
			{
				assert(tempArr.empty() && "can't do classification because no bins registered !!");
				return;
			}
			for (const ImgSegment& item : tempArr)
			{
				const int segBin = slopeBin.degToBin(item.m_slopeAngle);
				if (abs(segBin - dominantBin) <= GlobalParameters::MAX_DISTANCE_TO_DOMINANT_BIN)
				{
					outArr.push_back(item);
				}
			}
		};
		filterSlopeFunc(slopeBin_horizontal, m_tempHorizontalSegments, m_horizontalSegments);
		filterSlopeFunc(slopeBin_vertical, m_tempVerticalSegments, m_verticalSegments);
	}
	else
	{
		m_horizontalSegments = m_tempHorizontalSegments;
		m_verticalSegments   = m_tempVerticalSegments;
	}


	// DEBUG CODE TO FIND SOME THINGS..
	for (int i = 0; i < m_horizontalSegments.size(); i++)
	{
		if (m_horizontalSegments[i].m_x0 <= 2)
		{
			int a = 3;
			a++;
		}
	}


	clusterizeSegments(m_horizontalSegments, m_horizontalClusters,
		m_horizontalClusters_sortLenKey, m_horizontalClusters_sortYMinKey, true);

	clusterizeSegments(m_verticalSegments, m_verticalClusters, 
					  m_verticalClusters_sortLenKey, m_verticalClusters_sortXMinKey, false);


	findMainVisibleClusterFeatures(m_horizontalClusters, m_horizontalClusters_sortLenKey, m_horizontalClusters_sortYMinKey, 
					 m_horizontalClustersObservable, true);

	findMainVisibleClusterFeatures(m_verticalClusters, m_verticalClusters_sortLenKey, m_verticalClusters_sortXMinKey,
					 m_verticalClustersObservable, false);

	
	classifyClusters(usableWeight, usableHeight, totalWidth, totalHeight, output);

	if (isLocalDebuggingEnabled)
	{
		// Draw clusters
		{
			cv::Mat drawing = cv::Mat::zeros(cv::Size((int)totalWidth, (int)totalHeight), CV_8UC3);
			drawClusters(drawing, m_horizontalClusters, m_horizontalSegments);
			drawClusters(drawing, m_verticalClusters, m_verticalSegments);
			cv::imshow("FeaturesIdentification_allClusters", drawing);
		}

		// Draw observable clusters
		{
			std::vector<ImgSegmentCluster> horizontalObservableClusters;
			std::vector<ImgSegmentCluster> verticalObservableClusters;

			auto fillClustersFromClassification = [&](std::vector<ClusterClassification>& classifiedClusters, std::vector<ImgSegmentCluster>& outObservableClusters)
			{
				for (const ClusterClassification& clusterData : m_horizontalClustersObservable)
				{
					const int clusterIndex = clusterData.index;
					if (0 <= clusterIndex && clusterIndex < m_horizontalClusters.size())
					{
						horizontalObservableClusters.push_back(m_horizontalClusters[clusterIndex]);
					}
				}
			};

			fillClustersFromClassification(m_horizontalClustersObservable, horizontalObservableClusters);
			fillClustersFromClassification(m_verticalClustersObservable, verticalObservableClusters);

			cv::Mat drawing = cv::Mat::zeros(cv::Size((int)totalWidth, (int)totalHeight), CV_8UC3);

			const cv::Scalar horizontalColor(255, 0, 0);
			const cv::Scalar verticalColor(0, 0, 255);

			drawClusters(drawing, m_horizontalClusters, m_horizontalSegments, &horizontalColor);
			drawClusters(drawing, m_verticalClusters, m_verticalSegments, &verticalColor);

			// Draw the observable keypoints
			const int numPoints = output.getNumKeyPoints();
			for (int i = 0; i < numPoints; i++)
			{
				const KeyPointData& keyPoint = output.getKeyPointByIndex(i);
				const cv::Vec2f& vec = keyPoint.second;
				cv::circle(drawing, cv::Point((int)vec[0], (int)vec[1]), 5, cv::Scalar(255, 0, 255), 2);

				// Write text at a safe region
				const int xText = std::min((int)vec[0], drawing.cols - 40);
				const int yText = std::min((int)vec[1], drawing.rows - 20);

				cv::putText(drawing, getFeatureStr(keyPoint.first), cv::Point(xText, yText), cv::FONT_HERSHEY_COMPLEX, 1, // font face and scale
					cv::Scalar(255, 255, 255), // white
					1, cv::LINE_AA); // line thickness and type
			}

			cv::imshow("FeaturesIdentification_observableClusters", drawing);

		}

	}

}

// Tests if the coordinate is safe on height
bool PitchEdgesClassificationTool::isCoordinateOnSafeHeight(const float usableHEIGHT, const float x0, const float y0) const
{
	return (y0 - GlobalParameters::SAFE_OFFSET) > 0 && (y0 + GlobalParameters::SAFE_OFFSET) < usableHEIGHT;
}

bool PitchEdgesClassificationTool::isClusterLongVerticalEdge(
	const ImgSegmentCluster& testCluster, const float usableHEIGHT,
	const SidelineVisibleInfo& upSideline, const SidelineVisibleInfo& downSideline,
	bool& outUpIntersect, cv::Vec2f& outUpSidelineIntersectPos,
	bool &outDownIntersect, cv::Vec2f& outDownSidelineIntersectPos,
	bool &outIsEndline) const
{
	// Is enough visible on the screen ?
	//if (testCluster.len() < (usableHEIGHT * GlobalParameters::minPercent_longLineUsableHeight))
	//	return false;

	// Does it intersect with one of the sidelines ??
	const ImgSegment& testSegment = testCluster.m_representativeSegment;

	// Check 
	const SidelineVisibleInfo* sidelineToCheck[2] = { &upSideline, &downSideline };
	bool* outIntersectRes[2] =						{ &outUpIntersect, &outDownIntersect};
	cv::Vec2f* outIntersectPos[2]=					{ &outUpSidelineIntersectPos, &outDownSidelineIntersectPos};

	for (int i = 0; i < 2; i++)
	{
		const SidelineVisibleInfo& sideline = *sidelineToCheck[i];

		if (!sideline.isVisible)
			continue;

		const ImgSegmentCluster& sidelineCluster = m_horizontalClusters[sideline.horizontalSegmentIndex];
		const ImgSegment& sidelineSegment = sidelineCluster.m_representativeSegment;

		
		const cv::Vec2f endpointsToTest[2] = { cv::Vec2f(testSegment.m_x0, testSegment.m_y0),	
												cv::Vec2f(testSegment.m_x1, testSegment.m_y1), 
											};

		for (int endpointIter = 0; endpointIter < 2; endpointIter++)
		{
			const cv::Vec2f& endPoint = endpointsToTest[endpointIter];

			cv::Vec2f projPoint = sidelineSegment.getProjectionPointOnThisSegment(endPoint[0], endPoint[1]);
			cv::Vec2f testPointToProj = projPoint - endPoint;
			const float dist = testPointToProj.dot(testPointToProj);
			if (dist < GlobalParameters::maxDistToConsiderAsIntersectionSq)
			{
				*outIntersectRes[i] = true;
				*outIntersectPos[i] = projPoint;

				// Test if one of the sidelines ends which means it is an endline !
				const cv::Vec2f sideLinePos1 = cv::Vec2f(sidelineSegment.m_x0, sidelineSegment.m_y0);
				const cv::Vec2f sideLinePos2 = cv::Vec2f(sidelineSegment.m_x1, sidelineSegment.m_y1);
				const cv::Vec2f* sideLineEndpointsTest[2] = { &sideLinePos1, &sideLinePos2 };
				for (int sidelineEndpointIter = 0; sidelineEndpointIter < 2; sidelineEndpointIter++)
				{
					const cv::Vec2f testEndPointToSidelineEndPoint = *sideLineEndpointsTest[sidelineEndpointIter] - endPoint;
					const float testEndPointToSidelineEndPoint_dist = testEndPointToSidelineEndPoint.dot(testEndPointToSidelineEndPoint);
					if (testEndPointToSidelineEndPoint_dist < GlobalParameters::maxDistToConsiderAsIntersectionSq)
					{
						outIsEndline = true;
						break;
					}
				}


				break; // Once we find one of the endpoints intersect it's enough
			}
		}
	}

	return (outUpIntersect || outDownIntersect);
}

bool PitchEdgesClassificationTool::checkHorizontalClusterIntersectEndline(const ImgSegmentCluster& endlineCluster, const ImgSegmentCluster& testCluster, cv::Vec2f& outIntersectPos, const float usableHEIGHT) const
{
	assert(endlineCluster.isHorizontal == false && testCluster.isHorizontal == true && "you've sent me wrong parameters !");

	const ImgSegment& testSegment = testCluster.m_representativeSegment;
	const cv::Vec2f endpointsToTest[2] =
	{	cv::Vec2f(testSegment.m_x0, testSegment.m_y0),
		cv::Vec2f(testSegment.m_x1, testSegment.m_y1),
	};

	const ImgSegment& endlineSegment = endlineCluster.m_representativeSegment;

	for (int endpointIter = 0; endpointIter < 2; endpointIter++)
	{
		const cv::Vec2f& endPoint = endpointsToTest[endpointIter];

		cv::Vec2f projPoint = endlineSegment.getProjectionPointOnThisSegment(endPoint[0], endPoint[1]);
		
		// Is the point in pitch ?
		if (isCoordinateOnSafeHeight(usableHEIGHT, projPoint[0], projPoint[1]) == false)
		{
			continue;
		}
		
		cv::Vec2f testPointToProj = projPoint - endPoint;
		const float dist = testPointToProj.dot(testPointToProj);
		if (dist < GlobalParameters::maxDistToConsiderAsIntersectionSq)
		{
			outIntersectPos = projPoint;
			return true;
		}
	}
	
	return false;
}

bool PitchEdgesClassificationTool::checkHorizontalAndVerticalClustersIntersectsInEndpoints(const ImgSegmentCluster& cluster_horizontal, const ImgSegmentCluster& cluster_vertical, cv::Vec2f& outIntersectPos)
{
	const ImgSegment& horizontalSegment = cluster_horizontal.m_representativeSegment;
	const ImgSegment& verticalSegment = cluster_vertical.m_representativeSegment;

	const cv::Vec2f endpointsHorizontal[2] =
	{ 
		cv::Vec2f(horizontalSegment.m_x0, horizontalSegment.m_y0),
		cv::Vec2f(horizontalSegment.m_x1, horizontalSegment.m_y1),
	};

	const cv::Vec2f endpointsVertical[2] =
	{
		cv::Vec2f(verticalSegment.m_x0, verticalSegment.m_y0),
		cv::Vec2f(verticalSegment.m_x1, verticalSegment.m_y1)
	};

	for (int horiIter = 0; horiIter < 2; horiIter++)
	{
		for (int vertiIter = 0; vertiIter < 2; vertiIter++)
		{
			const cv::Vec2f diff = endpointsHorizontal[horiIter] - endpointsVertical[vertiIter];
			const float diffLen = diff.dot(diff);

			if (diffLen < GlobalParameters::maxDistToConsiderAsIntersectionSq)
			{
				outIntersectPos = endpointsVertical[vertiIter];
				return true;
			}
		}
	}

	return false;
}

/*bool ImgSplitInComponents::isClusterBoxVerticalEdge(const ImgSegmentCluster& cluster, const float usableHEIGHT) const
{
	return isCoordinateOnSafeHeight(usableHEIGHT, cluster.)
}*/

bool PitchEdgesClassificationTool::isClusterOnLeftVerticalSide(const ImgSegmentCluster& cluster, const float usableWIDTH) const
{
	return cluster.m_xMin < usableWIDTH * GlobalParameters::maxPercent_leftSide;
}

bool PitchEdgesClassificationTool::isClusterOnDownHorizontalSide(const ImgSegmentCluster& cluster, const float totalHEIGHT) const
{
	return cluster.m_yMin > totalHEIGHT * GlobalParameters::minPercent_downSide;
}

bool PitchEdgesClassificationTool::isClusterOnRightVerticalSide(const ImgSegmentCluster& cluster, const float usableWIDTH) const
{
	return cluster.m_xMin > usableWIDTH * GlobalParameters::minPercent_rightSide;
}

bool PitchEdgesClassificationTool::isClusterOnCenterVerticalSide(const ImgSegmentCluster& cluster, const float usableWIDTH) const
{
	return !isClusterOnLeftVerticalSide(cluster, usableWIDTH) && !isClusterOnRightVerticalSide(cluster, usableWIDTH);
}

void PitchEdgesClassificationTool::classifyClusters(const float usableWIDTH, const float usableHEIGHT, 
											const float totalWIDTH, const float totalHEIGHT,
											CoordinateConversionOutput &output)
{
	output.reset();

	// Step 1 - classify the horizontal clusters first and detect the sidelines
	///////////////////////////////
	m_upSideline.reset(totalWIDTH, totalHEIGHT);
	m_downSideline.reset(totalWIDTH, totalHEIGHT);

	if (m_horizontalClustersObservable.empty() == false)
	{
		const int longestIndex = m_horizontalClustersObservable[0].index;
		const int secondaryLongestIndex = m_horizontalClustersObservable.size() > 1 ?
										     m_horizontalClustersObservable[1].index : -1;

		ImgSegmentCluster* longestCluster = &m_horizontalClusters[longestIndex];
		ImgSegmentCluster* secondaryLongestCluster = secondaryLongestIndex == -1 ? nullptr:
														&m_horizontalClusters[secondaryLongestIndex];
	

		ImgSegmentCluster* clustersToTest[2] = { longestCluster, secondaryLongestCluster };
		const int clustersIndices[2] = { longestIndex, secondaryLongestIndex };

		for (int i = 0; i < 2; i++)
		{
			ImgSegmentCluster* clusterToTest = clustersToTest[i];
			if (clusterToTest == nullptr)
				continue;

			// Is the longest long enough ?
			if (clusterToTest->len() > GlobalParameters::minPercent_longSidelineUsableWidth * totalWIDTH)
			{
				const ImgSegment& longestSegment = clusterToTest->m_representativeSegment;

				// Is its Y in the pitch ?
				if (isCoordinateOnSafeHeight(totalHEIGHT,
					longestSegment.m_x0,
					longestSegment.m_y0) ||

					isCoordinateOnSafeHeight(totalHEIGHT,
						longestSegment.m_x1,
						longestSegment.m_y1))
				{
					const bool isDown = isClusterOnDownHorizontalSide(*longestCluster, totalHEIGHT);
					SidelineVisibleInfo& outTarget =  isDown ? m_downSideline : m_upSideline;

					outTarget.isVisible = true;
					outTarget.horizontalSegmentIndex = clustersIndices[i];
					outTarget.startPos[0] = longestSegment.m_x0;
					outTarget.startPos[1] = longestSegment.m_y0;
					outTarget.endPos[0] = longestSegment.m_x1;
					outTarget.endPos[1] = longestSegment.m_y1;

					clusterToTest->m_pitchEdgeFeature = isDown ? PITCH_FEATURE_SIDELINE_DOWN : PITCH_FEATURE_SIDELINE_UP;
				}
			}
		}
	}

	///////////////////////////////

	// Step 2: try to find 4 intersecting points in the image between horizontal vertical points

#if 0
	// In the vertical clusters list there could be in this order by length:
	// 1-2 vertical edges (representing and end side and on middle side)
	// big box one
	// small box one
	// Try them in this order and check to match features
	int numNonVerticalEdges = 0;
	for (int i = 0; i < m_verticalClustersObservable.size(); i++)
	{
		const int clusterIndex = m_verticalClustersObservable[i].index;
		ImgSegmentCluster& cluster = m_verticalClusters[clusterIndex];

		// Check if this cluster is a long vertical one
		cv::Vec2f upSidelineIntersectPos;
		cv::Vec2f downSidelineIntersectPos;
		bool isIntersectingUpSideline = false;
		bool isIntersectingDownSideline = false;
		bool isEndline = false;
		if (isClusterLongVerticalEdge(cluster, usableHEIGHT, m_upSideline, m_downSideline, isIntersectingUpSideline, upSidelineIntersectPos, isIntersectingDownSideline, downSidelineIntersectPos, isEndline))
		{
			// Add points as the correct features
			if (isEndline)
			{
				if (isIntersectingUpSideline)
				{
					output.addKeyPoint(FEATURE_ENDLINE_UP, upSidelineIntersectPos);
				}
				else
				{
					output.addKeyPoint(FEATURE_ENDLINE_DOWN, downSidelineIntersectPos);
				}
			}
			else // end
			{
				if (isIntersectingUpSideline)
				{
					output.addKeyPoint(FEATURE_MIDDLE_UP, upSidelineIntersectPos);
				}
				else
				{
					output.addKeyPoint(FEATURE_MIDDLE_DOWN, downSidelineIntersectPos);
				}
			}
		}
		// If we already seen the small and big boxes don't do anything !
		else if (numNonVerticalEdges < 2)
		{
			// COnsidering that we can't see the small box if the big box is not visible !
			numNonVerticalEdges++;
			const bool isBigBoxVerticalEdge = numNonVerticalEdges == 1;

			// Test if endpoints are on the pitch
			const ImgSegment& clusterSegment = cluster.m_representativeSegment;
			if (isCoordinateOnSafeHeight(usableHEIGHT, clusterSegment.m_x0, clusterSegment.m_y0))
			{
				output.addKeyPoint(isBigBoxVerticalEdge ? FEATURE_BOX_UP : FEATURE_SMALL_BOX_UP, 
									cv::Vec2f(clusterSegment.m_x0, clusterSegment.m_y0));
			}

			if (isCoordinateOnSafeHeight(usableHEIGHT, clusterSegment.m_x1, clusterSegment.m_y1))
			{
				output.addKeyPoint(isBigBoxVerticalEdge ? FEATURE_BOX_DOWN : FEATURE_SMALL_BOX_DOWN,
									cv::Vec2f(clusterSegment.m_x1, clusterSegment.m_y1));
			}
		}
	}
#else
	// Step 1: check if there is any point intersecting the endline and midline with the sidelines
	std::vector<int> endlineClusterIndices;
	bool isMiddleLineVisible = false;
	{
		for (int i = 0; i < m_verticalClustersObservable.size(); i++)
		{
			const int clusterIndex = m_verticalClustersObservable[i].index;
			ImgSegmentCluster& cluster = m_verticalClusters[clusterIndex];

			// Check if this cluster is a long vertical one
			cv::Vec2f upSidelineIntersectPos;
			cv::Vec2f downSidelineIntersectPos;
			bool isIntersectingUpSideline = false;
			bool isIntersectingDownSideline = false;
			bool isEndline = false;

			if (isClusterLongVerticalEdge(cluster, usableHEIGHT, m_upSideline, m_downSideline, isIntersectingUpSideline, upSidelineIntersectPos, isIntersectingDownSideline, downSidelineIntersectPos, isEndline))
			{
				// Add points as the correct features
				if (isEndline)
				{
					if (isIntersectingUpSideline)
					{
						output.addKeyPoint(FEATURE_ENDLINE_UP, upSidelineIntersectPos);
					}
					else
					{
						output.addKeyPoint(FEATURE_ENDLINE_DOWN, downSidelineIntersectPos);
					}

					cluster.m_pitchEdgeFeature = isClusterOnLeftVerticalSide(cluster, usableWIDTH) ?
							PITCH_FEATURE_ENDLINE_LEFT : PITCH_FEATURE_ENDLINE_RIGHT;

					endlineClusterIndices.push_back(clusterIndex);
				}
				else // end
				{
					if (isIntersectingUpSideline)
					{
						output.addKeyPoint(FEATURE_MIDDLE_UP, upSidelineIntersectPos);
					}
					else
					{
						output.addKeyPoint(FEATURE_MIDDLE_DOWN, downSidelineIntersectPos);
					}

					cluster.m_pitchEdgeFeature = PITCH_FEATURE_MIDDLE_LINE;
					isMiddleLineVisible = true;
				}
			}
		}
	}

	// If the midline is visible, then we can't see the endlines but we might see the boxes ? 
	// Appeareantly not from all cases that i see
	bool isLeftSided = false;
	if (isMiddleLineVisible)
	{
		// Nothing to do
		// Hint for upper layer: attack in your normal attack direction using img coordinates
	}
	else if (endlineClusterIndices.size() > 0) // Is any endline visible ? Assumption: if yes, then we can see only a single endline !
	{
		// Step 2: Check if the horizontal lines other than sidelines intersects the endlines

		// First, check if we are left or right sided. Assert if both for now (see the asumption above)
		bool isRightSided = false;
		for (int iEndLineIter = 0; iEndLineIter < endlineClusterIndices.size(); iEndLineIter++)
		{
			ImgSegmentCluster& endlineCluster = m_verticalClusters[endlineClusterIndices[iEndLineIter]];
			assert((endlineCluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_LEFT || endlineCluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_RIGHT)
				&& "I thought this is an sideline !!");

			if (endlineCluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_LEFT)
			{
				isLeftSided = true;
			}
			else if (endlineCluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_RIGHT)
			{
				isRightSided = true;
			}
		}

		assert(((int)isLeftSided + (int)isRightSided) == 1 && "something is wrong either both endlines visible or none. You can safely comment/disable this assert however");

		// Step 2.5
		int assumedBigBoxClusterVerticalIndex = -1;
		int bigBoxVerticalLimit = -1; // Where it is safe to consider the small box vertical cluster 'inside'
		// Eliminate the noise produced by ellipses around big boxes
		// Hint: The small box is inside endline and big box
		{
			// Find the first vertical cluster which seems like the big box - it is the biggest one and NOT classified yet as sideline, and big enough
			for (int verticalClusterIter = 0; verticalClusterIter < m_verticalClustersObservable.size(); verticalClusterIter++)
			{
				const int clusterIndex = m_verticalClustersObservable[verticalClusterIter].index;
				ImgSegmentCluster& cluster = m_verticalClusters[clusterIndex];

				if (cluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_LEFT || cluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_RIGHT
					|| cluster.isParallelWith(m_verticalClusters[endlineClusterIndices[0]]) == false)
					continue;

				if (cluster.len() > GlobalParameters::MIN_USABLE_LEN_FOR_A_CLUSTER)
				{
					assumedBigBoxClusterVerticalIndex = clusterIndex;

					if (isLeftSided)
						bigBoxVerticalLimit = (int)cluster.m_xMin - 15;
					else // Right side
						bigBoxVerticalLimit = (int)cluster.m_xMax + 15;

					break;
				}

				else  // iF found one but not big enough...
					break;
			}

			// Eliminate all clusters that are in the right side of big box (if left sided), or clusters that are in the left side of the big box (if right sided)
			// This will eliminate the noise caused by ellipses
			if (assumedBigBoxClusterVerticalIndex != -1)
			{				
				auto it = m_verticalClustersObservable.begin();
				while (it != m_verticalClustersObservable.end())
				{

					const int clusterIndex = it->index;
					ImgSegmentCluster& cluster = m_verticalClusters[clusterIndex];

					if (assumedBigBoxClusterVerticalIndex != clusterIndex &&
						 (  (isLeftSided && cluster.m_xMin > bigBoxVerticalLimit) ||
						    (isRightSided && cluster.m_xMax < bigBoxVerticalLimit)) 
						 )
					{
						it = m_verticalClustersObservable.erase(it);
					}
					else
					{
						it++;
					}
				}
			}
		}

		std::vector<int> bigBoxClusterIndices_horizontal;
		bigBoxClusterIndices_horizontal.reserve(2);
		std::vector<int> smallBoxClusterIndices_horizontal;
		smallBoxClusterIndices_horizontal.reserve(2);
		std::vector<int> allBoxClusterIndices;

		if (assumedBigBoxClusterVerticalIndex != -1)
		{
			allBoxClusterIndices.reserve(4);
			{
				// Classify the other horizontal clusters first in small and big boxes
				bool bigBoxFound = false; // First found and not sideline => big box
				float minBigBoxLen = -1.0f;  // This is how we identify if big or small box ones
				ImgSegmentCluster* bigBoxRepresentativeHorizontalCluster = nullptr; // For angle / parallelism check (small and big boxes are parallel)

				for (int iHorizontalClusterIter = 0; iHorizontalClusterIter < m_horizontalClustersObservable.size(); iHorizontalClusterIter++)
				{
					const int clusterIndex = m_horizontalClustersObservable[iHorizontalClusterIter].index;
					ImgSegmentCluster& horizontalCluster = m_horizontalClusters[clusterIndex];

					if (horizontalCluster.m_pitchEdgeFeature == PITCH_FEATURE_SIDELINE_DOWN ||
						horizontalCluster.m_pitchEdgeFeature == PITCH_FEATURE_SIDELINE_UP)
						continue;

					// Cache stuff from the first cluster representing a big box cluster
					if (bigBoxFound == false)
					{
						minBigBoxLen = horizontalCluster.len() * 0.5f;
						bigBoxFound = true;
						bigBoxRepresentativeHorizontalCluster = &horizontalCluster;
					}
					else
					{
						// Ignore this if not parallel to the representative cluster
						if (bigBoxRepresentativeHorizontalCluster->isParallelWith(horizontalCluster, true) == false)
							continue;
					}

					const bool isDown = isClusterOnDownHorizontalSide(horizontalCluster, totalHEIGHT);

					// If long enough = > big box, else small box
					if (horizontalCluster.len() > minBigBoxLen)
					{
						// BIG BOX
						horizontalCluster.m_pitchEdgeFeature = isDown ?
							(isLeftSided ? PITCH_FEATURE_BOX_LEFT_BOTTOM : PITCH_FEATURE_BOX_RIGHT_BOTTOM) :
							(isLeftSided ? PITCH_FEATURE_BOX_LEFT_TOP : PITCH_FEATURE_BOX_RIGHT_TOP);

						bigBoxClusterIndices_horizontal.push_back(clusterIndex);
					}
					else
					{
						// SMALL BOX
						horizontalCluster.m_pitchEdgeFeature = isDown ?
							(isLeftSided ? PITCH_FEATURE_SMALL_BOX_LEFT_BOTTOM : PITCH_FEATURE_SMALL_BOX_RIGHT_BOTTOM) :
							(isLeftSided ? PITCH_FEATURE_SMALL_BOX_LEFT_TOP : PITCH_FEATURE_SMALL_BOX_RIGHT_TOP);

						smallBoxClusterIndices_horizontal.push_back(clusterIndex);
					}
				}

				allBoxClusterIndices.insert(std::end(allBoxClusterIndices),
					std::begin(bigBoxClusterIndices_horizontal), std::end(bigBoxClusterIndices_horizontal));
				allBoxClusterIndices.insert(std::end(allBoxClusterIndices),
					std::begin(smallBoxClusterIndices_horizontal), std::end(smallBoxClusterIndices_horizontal));


				// Check now the intersection between horizontal clusters representing  big/small box edges and sidelines
				// If they intersect and safe in screen space => new keypoints !
				for (int iEndLineIter = 0; iEndLineIter < endlineClusterIndices.size(); iEndLineIter++)
				{
					ImgSegmentCluster& endlineCluster = m_verticalClusters[endlineClusterIndices[iEndLineIter]];
					assert((endlineCluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_LEFT || endlineCluster.m_pitchEdgeFeature == PITCH_FEATURE_ENDLINE_RIGHT)
						&& "I thought this is an sideline !!");

					for (int iBoxesLineIter = 0; iBoxesLineIter < allBoxClusterIndices.size(); iBoxesLineIter++)
					{
						ImgSegmentCluster& boxLineCluster = m_horizontalClusters[allBoxClusterIndices[iBoxesLineIter]];
						//assert(); - boxes ?

						cv::Vec2f intersectPos;
						const bool res = checkHorizontalClusterIntersectEndline(endlineCluster, boxLineCluster, intersectPos, usableHEIGHT);
						if (res)
						{
							// We have an intersection ! add the corresponding key point !
							PitchKeyPoints keyPoint = FEATURES_COUNT;
							if (boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_BOX_LEFT_BOTTOM ||
								boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_BOX_RIGHT_BOTTOM)
							{
								keyPoint = FEATURE_BOX_ON_SIDELINE_DOWN;
							}
							else if (boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_BOX_LEFT_TOP ||
								boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_BOX_RIGHT_TOP)
							{
								keyPoint = FEATURE_BOX_ON_SIDELINE_UP;
							}
							else if (boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_LEFT_BOTTOM ||
								boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_RIGHT_BOTTOM)
							{
								keyPoint = FEATURE_SMALL_BOX_ON_SIDELINE_DOWN;
							}
							else if (boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_LEFT_TOP ||
								boxLineCluster.m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_RIGHT_TOP)
							{
								keyPoint = FEATURE_SMALL_BOX_ON_SIDELINE_UP;
							}

							output.addKeyPoint(keyPoint, intersectPos);
						}
					}
				}
			}
		}

		// Step 3: check the intersection between big and small boxes edges (on endpoints only)
		auto funcTestBigOrSmallBoxEdgeIntersect =[&](const std::vector<int>& horizontalClusterIndices, const bool isSmallBoxTest)
		{
			for (int i = 0; i < horizontalClusterIndices.size(); i++)
			{
				const ImgSegmentCluster& clusterA = m_horizontalClusters[horizontalClusterIndices[i]];
				for (int j = 0; j < m_verticalClustersObservable.size(); j++)
				{
					const ImgSegmentCluster& clusterB = m_verticalClusters[m_verticalClustersObservable[j].index];
					if (clusterB.m_pitchEdgeFeature != PITCH_FEATURE_NONE &&
						clusterB.m_pitchEdgeFeature != PITCH_FEATURE_BOX_LEFT_BOTTOM &&
						clusterB.m_pitchEdgeFeature != PITCH_FEATURE_BOX_LEFT_TOP &&
						clusterB.m_pitchEdgeFeature != PITCH_FEATURE_BOX_RIGHT_TOP &&
						clusterB.m_pitchEdgeFeature != PITCH_FEATURE_BOX_RIGHT_BOTTOM)
					{
						continue;
					}

					cv::Vec2f intersectPos;
					if (checkHorizontalAndVerticalClustersIntersectsInEndpoints(clusterA, clusterB, intersectPos))
					{
						PitchKeyPoints keyPoint = FEATURES_COUNT;

						// Find the intersecting keypoint feature 
						const ImgSegmentCluster* clustersFound[2] = { &clusterA, &clusterB };
						for (int foundClusterIter = 0; foundClusterIter < 2; foundClusterIter++)
						{
							if (clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_LEFT_TOP ||
								clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_RIGHT_TOP)
							{
								keyPoint = FEATURE_SMALL_BOX_UP;
							}
							else if (clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_LEFT_BOTTOM ||
								clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_SMALL_BOX_RIGHT_BOTTOM)
							{
								keyPoint = FEATURE_SMALL_BOX_DOWN;
							}
							else if (clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_BOX_LEFT_TOP ||
								clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_BOX_RIGHT_TOP)
							{
								keyPoint = FEATURE_BOX_UP;
							}
							else if (clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_BOX_LEFT_BOTTOM ||
								clustersFound[foundClusterIter]->m_pitchEdgeFeature == PITCH_FEATURE_BOX_RIGHT_BOTTOM)
							{
								keyPoint = FEATURE_BOX_DOWN;
							}

							if (keyPoint != FEATURES_COUNT)
								break;
						}

						assert(keyPoint != FEATURES_COUNT && "Could not find the keypoint classification :(");
						if (keyPoint != FEATURES_COUNT)
						{
							output.addKeyPoint(keyPoint, intersectPos);
						}
					}
				}
			}
		};

		funcTestBigOrSmallBoxEdgeIntersect(smallBoxClusterIndices_horizontal, true);
		funcTestBigOrSmallBoxEdgeIntersect(bigBoxClusterIndices_horizontal, false);
	}
#endif

	output.doPostcomputations(isLeftSided);


	/*
	///// Step 2: Check if we have 2 box edges in screen or 2 fully detected edges
	// Take the endpoints of those

	std::array<int, 8> clusterIndicesRepresentingLongEdges;
	std::array<int, 8> clusterIndicesRepresentingBoxEdges; // Small or big box doesn't matter now

	
	*/


	/* OLD METHOD
	// Now we have the indices of the visible clusters. Classify them by feature ID too (see the doc picture !)
	///////////////////////
	// Case E:
	if (m_verticalClustersObservable.size() == 1)
	{
		// TODO:
		// If it intersects h1 or h2 => v4 is visible
		// If on left side => v3
		// If on right side = > v5

		const int index = m_verticalClustersObservable[0].index;
		const ImgSegmentCluster& mainCluster = m_verticalClusters[index];
		if (isClusterLongVerticalEdge(mainCluster, usableHEIGHT))
		{
			m_verticalClustersObservable[0].id = FEATURE_V4;
		}
		else if (isClusterOnLeftVerticalSide(mainCluster, usableWIDTH))
		{
			m_verticalClustersObservable[0].id = FEATURE_V3;
		}
		else if (isClusterOnRightVerticalSide(mainCluster, usableWIDTH))
		{
			m_verticalClustersObservable[0].id = FEATURE_V5;
		}
	}
	// Cases C and D - Suspecting the middle and one of the box vertical edges are found
	else if (m_verticalClustersObservable.size() == 2)
	{
		const ImgSegmentCluster& mainCluster = m_verticalClusters[m_verticalClustersObservable[0].index];
		const ImgSegmentCluster& secondaryCluster = m_verticalClusters[m_verticalClustersObservable[1].index];

		// Is long vertical edge visibile and the second one is in the pitch ?
		if (isClusterLongVerticalEdge(mainCluster, usableHEIGHT) &&
			isCoordinateOnSafeHeight(usableHEIGHT, secondaryCluster.m_yStart, secondaryCluster.m_yEnd))
		{
			if (secondaryCluster.m_xMin < mainCluster.m_xMin)
			{
				m_verticalClustersObservable[0].id = FEATURE_V4;
				m_verticalClustersObservable[1].id = FEATURE_V3;
			}
			else
			{
				m_verticalClustersObservable[0].id = FEATURE_V4;
				m_verticalClustersObservable[1].id = FEATURE_V5;
			}
		}		
	}
	*/
}

bool PitchEdgesClassificationTool::isClusterEndline(const ImgSegmentCluster& cluster, 
										    const float usableHEIGHT, 
											const SidelineVisibleInfo& upSideline, 
											const SidelineVisibleInfo& downSideline) const
{
	// TEST IF THE endpoints of cluster are on up/down sidelines corners !
	assert(false);
	return false;
}

bool PitchEdgesClassificationTool::isClusterMiddleline(const ImgSegmentCluster& cluster, const float usableHEIGHT, const SidelineVisibleInfo& upSideline, const SidelineVisibleInfo& downSideline) const
{
	// TEST IF THE endpoints of cluster are NOT up/down sidelines corners !
	assert(false);
	return false;
}

void PitchEdgesClassificationTool::findMainVisibleClusterFeatures(const std::vector<ImgSegmentCluster>& clusters, 
											const std::vector<int>& clustersSortedByLenKey,
											const std::vector<int>& clustersSortedByMinAxisKey,
											std::vector<ClusterClassification>& outClassification,
											const bool isHorizontal) const
{
	outClassification.clear();
	if (clusters.empty())
		return;

	// Step 1: Find the visibile main clusters Take the highest len cluster then add the ones which are parallel with that 
	const int longestIndex = clustersSortedByLenKey[0];

	outClassification.emplace_back(ClusterClassification(longestIndex, -1));
	const ImgSegmentCluster& baseCluster = clusters[longestIndex];
	for (int i = 1; i < clustersSortedByLenKey.size(); i++)
	{
		const int index = clustersSortedByLenKey[i];
		const ImgSegmentCluster& thisCluster = clusters[index];
		if (baseCluster.isParallelWith(thisCluster))
		{
			outClassification.emplace_back(ClusterClassification(index, -1));
		}
	}	
}

void PitchEdgesClassificationTool::clusterizeSegments(	const std::vector<ImgSegment>& segmentsList, 
												std::vector<ImgSegmentCluster>& clusters, 
												std::vector<int>& clustersSortedByLenKey,
												std::vector<int>& clustersSortedByMinAxisKey,
												const bool isHorizontal) const
{
	static const int INVALID_CLUSTER_INDEX = -1;
	// Initially all segments are assigned to an invalid component
	std::vector<int> segmentToClusterIndex;

	const int numSegments = (int)segmentsList.size();
	
	segmentToClusterIndex.clear();
	segmentToClusterIndex.resize(numSegments);

	for (int i = 0; i < numSegments; i++)
	{
		segmentToClusterIndex[i] = INVALID_CLUSTER_INDEX;
	}

	for (int iSegIter = 0; iSegIter < numSegments; iSegIter++)
	{
		if (segmentToClusterIndex[iSegIter] != INVALID_CLUSTER_INDEX)
			continue;

		const ImgSegment& seg = segmentsList[iSegIter];
		
		const int thisClusterIndex = (int)clusters.size();
		ImgSegmentCluster dummyCluster(isHorizontal);
		clusters.emplace_back(dummyCluster);
		ImgSegmentCluster& newCluster = clusters.back();

		newCluster.addSegment(seg, iSegIter);
		segmentToClusterIndex[iSegIter] = thisClusterIndex;

		// Search for all other segments in this cluster
		// TODO: we can optimize this NlogN instead of N^2 don't now if it worths it
		for (int iNextSeg = iSegIter + 1; iNextSeg < numSegments; iNextSeg++)
		{			
			if (segmentToClusterIndex[iNextSeg] != INVALID_CLUSTER_INDEX)
				continue;

			const ImgSegment& otherSeg = segmentsList[iNextSeg];

			if (newCluster.isSegmentInCluster(otherSeg))
			{
				newCluster.addSegment(otherSeg, iNextSeg);
				segmentToClusterIndex[iNextSeg] = thisClusterIndex;
			}
		}

		if (newCluster.len() < GlobalParameters::MIN_USABLE_LEN_FOR_A_CLUSTER)
			clusters.pop_back();
	}

	const int numClusters = (int)clusters.size();
	clustersSortedByLenKey.clear();
	clustersSortedByLenKey.resize(numClusters);
	clustersSortedByMinAxisKey.clear();
	clustersSortedByMinAxisKey.resize(numClusters);
	for (int i = 0; i < numClusters; i++)
	{
		clustersSortedByLenKey[i] = i;
		clustersSortedByMinAxisKey[i] = i;
	}

	// Sort by length
	std::sort(clustersSortedByLenKey.begin(), clustersSortedByLenKey.end(),
		[&](const int& a, const int& b)
		{
			//const int clusterA = clustersSortedByLenKey[a];
			//const int clusterB = clustersSortedByLenKey[b];
			const float lenClusterA = clusters[a].len();
			const float lenClusterB = clusters[b].len();

			return (lenClusterA  >  lenClusterB);
		});

	// Sort by min axis	
	
	std::sort(clustersSortedByMinAxisKey.begin(), clustersSortedByMinAxisKey.end(),
		[&](const int& a, const int &b)
		{
			return (isHorizontal ? clusters[a].m_yMin < clusters[b].m_yMin
								 : clusters[a].m_xMin < clusters[b].m_xMin);
		});
}

void PitchEdgesClassificationTool::drawClusters(cv::Mat& outputImg,
	const std::vector<ImgSegmentCluster>& clusters,
	const std::vector<ImgSegment>& allSegments, const cv::Scalar* forcedColor /*= nullptr*/) const
{
	for (int i = 0; i < clusters.size(); i++)
	{
		const ImgSegmentCluster& cluster = clusters[i];

		const cv::Scalar color = (forcedColor != nullptr ? *forcedColor : cv::Scalar(rand() % 256, rand() % 256, rand() % 256));
		for (const int segIndexInClusters : cluster.m_segmentIndices)
		{
			const ImgSegment& segment = allSegments[segIndexInClusters];
			cv::line(outputImg, cv::Point((int)segment.m_x0, (int)segment.m_y0), 
					 cv::Point((int)segment.m_x1, (int)segment.m_y1), color, 4, cv::LINE_AA);
		}

		

		cv::line(outputImg, cv::Point((int)cluster.m_representativeSegment.m_x0, (int)cluster.m_representativeSegment.m_y0), 
							cv::Point((int)cluster.m_representativeSegment.m_x1, (int)cluster.m_representativeSegment.m_y1),
				cv::Scalar(255, 255, 255), 1, cv::LINE_4);
	}
}

void PitchEdgesClassificationTool::unitTestPitchFeatures()
{
	// Read test from file
	char* path = getenv("DISAIPATH");
	assert(path && "please add DISAIPATH to your environment variables!");
	std::ostringstream s;
	s << path << "\\Untitled.txt";

	FILE* f = fopen(s.str().c_str(), "r");
	std::vector<cv::Vec4i> lines;
	int numLines = 0;
	fscanf(f, "%d", &numLines);
	for (int i = 0; i < numLines; i++)
	{
		cv::Vec4i line;
		fscanf(f, "%d %d %d %d", &line[0], &line[1], &line[2], &line[3]);
		lines.emplace_back(line);
	}

	fclose(f);

	PitchEdgesClassificationTool test;
	CoordinateConversionOutput output;
	test.processComponents(lines, 1024, 768, 1024, 768, true, output);

	cv::Mat drawing = cv::Mat::zeros(cv::Size(1024, 768), CV_8UC3);
	test.drawClusters(drawing, test.m_horizontalClusters, test.m_horizontalSegments);
	test.drawClusters(drawing, test.m_verticalClusters, test.m_verticalSegments);

	cv::imshow("FeaturesIdentificationUnitTest", drawing);
	//cv::waitKey(0);
}

void PitchEdgesClassificationTool::unitTestKeypointsHomography()
{
	std::vector< cv::Point2f > obj = {
		cv::Point2f(54, 504), //endLineDown
		cv::Point2f(213, 80), //endLineUp
		cv::Point2f(321, 368), //boxLineDown
		cv::Point2f(379, 122), //boxLineUp
	};

	std::vector< cv::Point2f > scene = {
		cv::Point2f(17, 380), //endLineDown
		cv::Point2f(17, 3), //endLineUp
		cv::Point2f(129, 328), //boxLineDown
		cv::Point2f(129, 55), //boxLineUp
	};

	cv::Mat H = cv::findHomography(obj, scene, CV_RANSAC);
	H = H;

	cv::Point2f testP(246, 319); // low small box
	cv::Point2f testP2(297, 153); // high small box
	cv::Point2f testP3(153, 238); // goal center
	std::vector<cv::Point2f> testPoints;
	testPoints.push_back(testP);
	testPoints.push_back(testP2);
	testPoints.push_back(testP3);
	//std::vector<cv::Point2f> transformedPoints;

	cv::perspectiveTransform(testPoints, testPoints, H);
}

void CoordinateConversionOutput::loadTargetKeyPoints()
{
	// These are taken from pitch.png, left side
	m_realStadiumTargetKeypoints.resize(FEATURES_COUNT);

	m_realStadiumTargetKeypoints[FEATURE_MIDDLE_UP]			= cv::Point2f(282, 1);
	m_realStadiumTargetKeypoints[FEATURE_MIDDLE_DOWN]		= cv::Point2f(282, 378);

	m_realStadiumTargetKeypoints[FEATURE_BOX_UP]			= cv::Point2f(112, 53);
	m_realStadiumTargetKeypoints[FEATURE_BOX_DOWN]			= cv::Point2f(112, 326);

	m_realStadiumTargetKeypoints[FEATURE_SMALL_BOX_UP]		= cv::Point2f(38, 128);
	m_realStadiumTargetKeypoints[FEATURE_SMALL_BOX_DOWN]	= cv::Point2f(38, 252);

	m_realStadiumTargetKeypoints[FEATURE_ENDLINE_UP]		= cv::Point2f(1, 1);
	m_realStadiumTargetKeypoints[FEATURE_ENDLINE_DOWN]		= cv::Point2f(1, 378);

	m_realStadiumTargetKeypoints[FEATURE_BOX_ON_SIDELINE_UP]	= cv::Point2f(18, 55);
	m_realStadiumTargetKeypoints[FEATURE_BOX_ON_SIDELINE_DOWN]	= cv::Point2f(18, 326);

	m_realStadiumTargetKeypoints[FEATURE_SMALL_BOX_ON_SIDELINE_UP]		= cv::Point2f(18, 128);
	m_realStadiumTargetKeypoints[FEATURE_SMALL_BOX_ON_SIDELINE_DOWN]	= cv::Point2f(18, 253);
}

// Middle / box / small box / endline
int getFeatureBigIndex(const PitchKeyPoints feature)
{
	return feature / 2;
}

void CoordinateConversionOutput::doPostcomputations(const bool isLeftSide)
{
	const int NUM_POINTS_NEEDED_FOR_HOMOGRAPH = 4;

	// We need at least 4 points to do our job
	m_hasAll4PointsReady = m_keyPointsOutput.size() >= NUM_POINTS_NEEDED_FOR_HOMOGRAPH;
	if (!m_hasAll4PointsReady)
	{
		return;
	}

	// 1. Establish if we are left or right side of the pitch
	//////////////////////
	// Sort the points by feature order
	std::sort(m_keyPointsOutput.begin(), m_keyPointsOutput.end(),
		[&](const std::pair<PitchKeyPoints, cv::Point2f>& p1, const std::pair<PitchKeyPoints, cv::Point2f>& p2)
	{
		return (p1.first < p2.first);
	});
	
	m_isRightSide = !isLeftSide;
	/*
	m_isRightSide = false;
	for (int i = 0; i < m_keyPointsOutput.size() - 1; i++)
	{
		// Same line ?
		if (getFeatureBigIndex(m_keyPointsOutput[i].first) == getFeatureBigIndex(m_keyPointsOutput[i + 1].first))
			continue;

		// If different line but we see first coming before the other we are in the right side
		if (m_keyPointsOutput[i].second[0] < m_keyPointsOutput[i + 1].second[0])
		{
			m_isRightSide = true;
		}
	}
	*/

	//////////////////////


	// 2. Compute homography matrix if possible
	std::vector<cv::Point2f> imageKeypoints(NUM_POINTS_NEEDED_FOR_HOMOGRAPH);
	std::vector<cv::Point2f> realStadiumKeypoints(NUM_POINTS_NEEDED_FOR_HOMOGRAPH);
	for (int i = 0; i < NUM_POINTS_NEEDED_FOR_HOMOGRAPH; i++)
	{
		const PitchKeyPoints featureId = m_keyPointsOutput[i].first;
		const cv::Point2f& featurePos = m_keyPointsOutput[i].second;
		const cv::Point2f& featureRealStadiumPos = m_realStadiumTargetKeypoints[featureId];

		imageKeypoints[i] = featurePos;
		realStadiumKeypoints[i] = featureRealStadiumPos;
	}

	m_homographMatrix = cv::findHomography(imageKeypoints, realStadiumKeypoints, CV_RANSAC);

	// Maybe homography is not valid !
	if (m_homographMatrix.empty())
	{
		m_hasAll4PointsReady = false;
	}
}

bool CoordinateConversionOutput::transformImgPointToRealCoords(const cv::Point2f& imgPoint, const cv::Point2f& outRealPoint) const
{
	if (!m_hasAll4PointsReady)
		return false;

	std::vector<cv::Point2f> points{ imgPoint };
	cv::perspectiveTransform(points, points, m_homographMatrix);
	return true;
}

};