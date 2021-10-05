#include "ComputerVisionLayer.h"
#include "ComputerVisionUtils.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>


using namespace std;


namespace ComputerVisionLayer
{
ComputerVisionLayerImpl* g_this = nullptr;
ContourClassification g_contoursClassifyTool;
ComputerVisionLayerImpl::ComputerVisionLayerImpl()
{
	// Must be the first line to load the cached data and parameters
	GlobalParameters::loadData();

	g_this = this;
}

//#define RUN_UNIT_TESTS

void ComputerVisionLayerImpl::processImageFrame(FrameParameters inputParams, VisionOutput& finalVisionOutput, const bool parentDebuggingEnabled)
{
	inputParams.m_contourClassificationTool = &g_contoursClassifyTool;
	const bool isLocalDebuggingEnabled = false || parentDebuggingEnabled;

	// PIPELINE technique with feedback!
	// We get as input FramePreprocessingInput and write finalVisionOutput.
	// Basically after other systems are computing stuff, they can send us some feedback
	// This feedback can be used in our computer vision layer pipeline to oriend our search better
	// (E.g. where to search the ball ? If we know the previous pos of the ball, how much can it move from frame to frame ? it isn't a space ship, right ?!


	// Big 1: Preprocess the image and find:
	// - active player markers if any
	// - ball coordinates
	// - players's bounding boxes
	// See FrameProcessingOutput data structure. You can customize the input or leave it as default below
	// ###########################################
	FramePreprocessingOutput frameProcessingOutput;
	g_preProcessingTool.preProcessImageFrame(inputParams.m_inputImg, inputParams, frameProcessingOutput, isLocalDebuggingEnabled);

	// Big 2: Preprocess the above's output and get the homograph matrix - a conversion between image coordinates to real pitch coordinates
	// The process works like this:
	// Process the edge lines identified above and classify them (e.g. middle of pitch line, endlines, box edges etc)
	// Get the corner points: intersection of the features above - see PitchKeyPoints enum for a list of these)
	// If there are at least 4 points we compute a homograph matrix by comparing our detected points coordinates with real pitch coordinates of the same features 
	CoordinateConversionOutput coordinateConversionOutput;
	g_pitchEdgesClassificationTool.processComponents(frameProcessingOutput.m_outPitchLines, frameProcessingOutput.m_usableWidth, frameProcessingOutput.m_usableHeight, frameProcessingOutput.m_totalWidth, frameProcessingOutput.m_totalHeight, isLocalDebuggingEnabled, coordinateConversionOutput);

	{
		const bool playerClassificationLocalDebugging = false;

		finalVisionOutput.reset();
		finalVisionOutput.m_isRealCoordinates = coordinateConversionOutput.hasHomographMarix();
		finalVisionOutput.m_playerMarkers = frameProcessingOutput.m_playerMarkers;

		finalVisionOutput.m_ballCoordinates_img = frameProcessingOutput.m_ballCoordinatesInImg;

		// Write the ball coordinates
		if (finalVisionOutput.m_isRealCoordinates)
		{
			for (const VisionOutput::BallInfo& imgBall : finalVisionOutput.m_ballCoordinates_img)
			{
				VisionOutput::BallInfo realBall;
				coordinateConversionOutput.transformImgPointToRealCoords(imgBall, realBall);
			}
		}

		// Classify the contour players using the bbox of contours found in the previous steps: 
		ContourClassification::ClassificationScores scores;
		for (const cv::Rect2i& bbox : frameProcessingOutput.m_playersContoursBBoxes)
		{
			// Classify the bbox in the input image
			// The input image must have only the players with real colors.

			// Debug to detect certain bboxes
			/*
			if ((cv::Rect2i(cv::Point2i(900, 210), cv::Point2i(1000, 300)) & bbox).area() > 10)
			{
				int a = 3;
				a++;
			}*/

			g_contoursClassifyTool.compareHistograms(inputParams.m_inputImg, bbox, scores);
			const TeamSide resultSide = scores.m_predominantClass;
			if (resultSide != TEAM_SIDE_HOME && resultSide != TEAM_SIDE_AWAY)
				continue;

			auto& targetToAddPlayer_img = resultSide == TEAM_SIDE_HOME ?	finalVisionOutput.m_homePlayers_img :
																			finalVisionOutput.m_awayPlayers_img;

			auto& targetToAddPlayer_real = resultSide == TEAM_SIDE_HOME ?	finalVisionOutput.m_homePlayers_real :
																			finalVisionOutput.m_awayPlayers_real;

			
			// Get the center of the bbox. If using real coordinates, convert them using homograph otherwise keep the image coordintes
			const cv::Point2f centerPos(bbox.x + bbox.width * 0.5f, bbox.y + bbox.height * 0.5f);
			targetToAddPlayer_img.push_back(centerPos);
			
			cv::Point2i pointToAdd = centerPos;
			if (finalVisionOutput.m_isRealCoordinates)
			{
				coordinateConversionOutput.transformImgPointToRealCoords(centerPos, pointToAdd);
				targetToAddPlayer_real.push_back(pointToAdd);
			}
			

			if (isLocalDebuggingEnabled || playerClassificationLocalDebugging)
			{
				cv::Scalar debugColorsByContourCLass[TEAM_SIDE_NUMS] =
				{
					cv::Scalar(255, 0, 0),
					cv::Scalar(0, 255, 0),
					cv::Scalar(255, 255, 255)
				};

				cv::rectangle(inputParams.m_inputImg, bbox, debugColorsByContourCLass[resultSide], 3);
			}
		}

		// Add the  goal position if possible
		// For this, we have to see the features from the small box projections on endline:
		{
			const KeyPointData* smallBoxOnSidelineUp	= coordinateConversionOutput.getKeyPointDataByType(FEATURE_SMALL_BOX_ON_SIDELINE_UP);
			const KeyPointData* smallBoxOnSidelineDown	= coordinateConversionOutput.getKeyPointDataByType(FEATURE_SMALL_BOX_ON_SIDELINE_DOWN);

			finalVisionOutput.m_isGoalVisible = false;
			if (smallBoxOnSidelineDown && smallBoxOnSidelineUp)
			{
				finalVisionOutput.m_goalPos_img = (smallBoxOnSidelineUp->second + smallBoxOnSidelineDown->second) * 0.5f;

				if (finalVisionOutput.m_isRealCoordinates)
				{
					const bool res = coordinateConversionOutput.transformImgPointToRealCoords(finalVisionOutput.m_goalPos_img, finalVisionOutput.m_goalPos_real);
					assert(res);
				}
				else
				{
					finalVisionOutput.m_goalPos_real = cv::Vec2f(0, 0);
				}

				finalVisionOutput.m_isGoalVisible = true;
			}

			finalVisionOutput.m_visibleFieldSide = coordinateConversionOutput.isRightSidedView() ? FIELD_SIDE_POSITIVE : FIELD_SIDE_NEGATIVE;
		}

		if (isLocalDebuggingEnabled || playerClassificationLocalDebugging)
		{
			if (finalVisionOutput.m_isGoalVisible)
			{
				cv::circle(inputParams.m_inputImg, finalVisionOutput.m_goalPos_img, 10, cv::Scalar(0, 0, 0), 3);
			}

			cv::imshow("PlayerClassification", inputParams.m_inputImg);
			cv::waitKey(0);
		}
	}
}

void ComputerVisionLayerImpl::init()
{
	// Step 0: Load shirts histograms
	g_contoursClassifyTool.loadHistograms(GlobalParameters::m_homeShirtPath,
		GlobalParameters::m_awayShirtPath,
		false,
		false);
}

std::ostream & operator<< (std::ostream &out, VisionOutput const &t)
{
	out << "Real coordinates -  " << t.m_isRealCoordinates << std::endl;
	out << "Field side to attack - " << CommonUtils::fieldSideToString(t.m_fieldSideToAttack) << std::endl;
	out << "Ball detected - " << (!t.m_ballCoordinates_img.empty()) << std::endl;
	out << "# Players markers - " << t.m_playerMarkers.size() << std::endl;
	out << "Field side in view - " << CommonUtils::fieldSideToString(t.m_visibleFieldSide) << std::endl;
	out << "Goal pos visible - " << t.m_isGoalVisible << std::endl;

	return out;
}

};