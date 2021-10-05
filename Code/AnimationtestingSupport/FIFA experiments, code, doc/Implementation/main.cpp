
// TODO:
// In test image - detecting player twice
// In test image - not finding the corners
// 3. profiling and optimizations | custom hough lines ?

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <algorithm>
#include "featuresIdentification.h"
#include "framePreprocessingUtils.h"
#include "CommonLayersUtils.h"
#include "ComputerVisionLayer.h"
#include "ComputerVisionUtils.h"
#include "DecisionMakingLayer.h"
#include <thread>

#include <ctime>
#include <chrono>


//////////////// DEBUGGING PARAMS /////////////////////////
#define IS_DEBUG_WINDOW_ENABLED        // TODO: DISABLE THIS WHEN NOT DEBUGGING DO NOT FORGET !!!
//#define IS_SINGLE_FRAME_CAPTURE_ENABLED // if enabled => capture a screen from a local file | else => capture from screen buffer (FIFA)


#ifdef IS_DEBUG_WINDOW_ENABLED
const bool g_enableTopLevelDebugging = true;
#else
const bool g_enableTopLevelDebugging = false;
#endif                                       


const bool g_enableInsideDebugging = false;
//////////////////////////////////////////////



bool g_shouldTerminate = false; // Signal to finish everything
ActionBuffer g_actionBuffer;

InputSendingLayerImpl g_inputLayer;
Actions g_cachedLastActionsToDoSentToInput;

FrameParameters g_lastFrameParams;
ComputerVisionLayer::ComputerVisionLayerImpl g_computerVisionLayerImpl; // true is for debug window
ComputerVisionLayer::VisionOutput g_visionOutput;
DecisionMakingLayer g_decisionMakingLayer;
using namespace cv;
using namespace std;

HWND g_hwndDesktop;

void processFrame(FrameParameters& params, ComputerVisionLayer::VisionOutput & visionOutput);
void processImageFrameGUIHook(int, void*);

void createDebugInterface()
{
//	ComputerVisionLayer::FramePreprocessingTool&			preProcessingTool = g_computerVisionLayerImpl.getPreProTool();
	//ComputerVisionLayer::PitchEdgesClassificationTool&		pitchEdgesClassificationTool = g_computerVisionLayerImpl.getPitchEdgesClassificationTool();

	//createTrackbar(" Green threshold:", "Source", &green_threshold, max_green_threshold, find_edges_callback);
	//cv::createTrackbar(" Hough line threshold:", "Source", &preProcessingTool.hough_threshold, preProcessingTool.max_hough_threshold, &processImageFrameGUIHook);
	//cv::createTrackbar(" Hough min line:", "Source", &preProcessingTool.hough_minLineLength, preProcessingTool.max_hough_minLineLength, &processImageFrameGUIHook);
	//cv::createTrackbar(" Hough max line:", "Source", &preProcessingTool.hough_maxLineGap, preProcessingTool.max_hough_maxLineGap, &processImageFrameGUIHook);

	/*
	createTrackbar(" LW_H:", "Source", &g_preProcessingTool.lw_h, 180, processImageFrameGUIHook);
	createTrackbar(" LW_S:", "Source", &g_preProcessingTool.lw_s, 255, processImageFrameGUIHook);
	createTrackbar(" LW_V:", "Source", &g_preProcessingTool.lw_v, 255, processImageFrameGUIHook);
	createTrackbar(" HW_H:", "Source", &g_preProcessingTool.hw_h, 180, processImageFrameGUIHook);
	createTrackbar(" HW_S:", "Source", &g_preProcessingTool.hw_s, 255, processImageFrameGUIHook);
	createTrackbar(" HW_V:", "Source", &g_preProcessingTool.hw_v, 255, processImageFrameGUIHook);

	createTrackbar(" LG_H:", "Source", &g_preProcessingTool.lg_h, 180, processImageFrameGUIHook);
	createTrackbar(" LG_S:", "Source", &g_preProcessingTool.lg_s, 255, processImageFrameGUIHook);
	createTrackbar(" LG_V:", "Source", &g_preProcessingTool.lg_v, 255, processImageFrameGUIHook);
	createTrackbar(" HG_H:", "Source", &g_preProcessingTool.hg_h, 180, processImageFrameGUIHook);
	createTrackbar(" HG_S:", "Source", &g_preProcessingTool.hg_s, 255, processImageFrameGUIHook);
	createTrackbar(" HG_V:", "Source", &g_preProcessingTool.hg_v, 255, processImageFrameGUIHook);
	*/
}


// If something changed on the debug interface, process the frame again
// Used for debugging
void processImageFrameGUIHook(int, void*)
{	
	processFrame(g_lastFrameParams, g_visionOutput);
}

void processFrame(FrameParameters& params, ComputerVisionLayer::VisionOutput & visionOutput)
{
	visionOutput.reset();
	//std::clock_t startcputime = std::clock();

	g_computerVisionLayerImpl.processImageFrame(params, visionOutput, params.isInsideDebugEnabled);

	//double cpu_duration = (std::clock() - startcputime) / (double)CLOCKS_PER_SEC;
	//std::cout << "Finished in " << cpu_duration << " seconds [CPU Clock] " << std::endl;
}

void drawVisionOutputDebug(FrameParameters& params, ComputerVisionLayer::VisionOutput & visionOutput, const bool force = false)
{
	// Write the computer vision output
	if (params.isTopLevelOutputDebugEnabled || force)
	{
		const char* source_window = "Source";
		cv::namedWindow(source_window, cv::WINDOW_NORMAL);

		// Draw some info text
		std::ostringstream visionOutputString;
		visionOutputString << visionOutput;

		CommonUtils::drawMultiLineText(params.m_originalImgCopy, visionOutputString.str().c_str(), 50, 50, 20, cv::Scalar(255, 255, 255));

		// Draw players locations
		auto drawPlayerPosFunc = [&](auto& players, const cv::Scalar& color)
		{
			const int rectangleRadius = 10;
			for (const cv::Point2i& playerPos : players)
			{
				const cv::Rect rect(playerPos.x - rectangleRadius, playerPos.y + rectangleRadius, 20, 20);

				cv::rectangle(params.m_originalImgCopy, rect, color, 2);
			}
		};
		drawPlayerPosFunc(visionOutput.m_homePlayers_img, cv::Scalar(255, 0, 0));
		drawPlayerPosFunc(visionOutput.m_awayPlayers_img, cv::Scalar(0, 0, 255));


		// Draw balls detected
		for (const cv::Point2i& ballPos : visionOutput.m_ballCoordinates_img)
		{
			cv::circle(params.m_originalImgCopy, ballPos, 8, cv::Scalar(255, 255, 255), 5);
		}

		// Draw the first ball
		if (visionOutput.m_ballCoordinates_img.empty() == false)
		{
			cv::circle(params.m_originalImgCopy, visionOutput.m_ballCoordinates_img[0], 10, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
		}

		// Draw markers detected
		for (const ComputerVisionLayer::PlayerMarkerInfo& marker : visionOutput.m_playerMarkers)
		{
			cv::rectangle(params.m_originalImgCopy, marker.rect, marker.teamSide == TEAM_SIDE_HOME ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 0, 255));
		}
	}
}

// This function gets a decision for the current frame
int getFrameDecision(Actions& outDecision)
{
	outDecision.reset();

	// Step 1: Take the image from screen and run the vision layer to compute features
	// Processing one frame from source
	// Instantiate params and get the image to process this frame
	{
#ifdef IS_SINGLE_FRAME_CAPTURE_ENABLED 
		cv::String imageName(GlobalParameters::getFullPath(GlobalParameters::m_debugImgPath));
		g_lastFrameParams.m_inputImg = imread(imageName, cv::IMREAD_COLOR);
		if (g_lastFrameParams.m_inputImg.empty())
		{
			std::cout << "No image supplied ..." << endl;
			return -1;
		}
#else
		g_lastFrameParams.m_inputImg = ComputerVisionLayer::captureFullScreenDesktop(g_hwndDesktop);
		if (g_lastFrameParams.m_inputImg.empty())
		{
			std::cout << "No image supplied ..." << endl;
			return -1;
		}
#endif 

		if (g_lastFrameParams.isTopLevelOutputDebugEnabled)
		{
			g_lastFrameParams.m_originalImgCopy = g_lastFrameParams.m_inputImg.clone();
		}

		// Process the image loaded above
		processImageFrameGUIHook(0, 0);
	}

	// Step 2: Using the features that we got in Step 1 send the input
	outDecision = g_decisionMakingLayer.Execute(g_visionOutput);
	return 0;
}

void drawDebugActions(Actions& actions)
{
	if (g_lastFrameParams.isTopLevelOutputDebugEnabled)
	{
		// Draw some info text
		std::ostringstream visionOutputString;
		visionOutputString << "Stick: " << actions.fStickAngle << " power: " << actions.fPower << endl;
		visionOutputString << "Shoot ? " << actions.bShoot << " TODO: add others !" << endl;
		CommonUtils::drawMultiLineText(g_lastFrameParams.m_originalImgCopy, visionOutputString.str().c_str(), 50, 250, 20, cv::Scalar(255, 255, 255));
	}
}

void inputThreadRun()
{
	while (g_shouldTerminate == false)
	{
		Actions data;
		const bool res = g_actionBuffer.readItem(data);

		if (res)
		{
			// If power is > 0 then it will loop several times until the power is consumed. we simulate this way how user is using power
			do
			{
				g_inputLayer.ProcessActions(data);
				data.fPower -= GlobalParameters::POWER_DECAY_PER_FRAME;
				std::this_thread::sleep_for(GlobalParameters::FRAME_TIME);
			} while (data.fPower > 0.0f);
		}
		else
		{
			// We dont have any input from computer vision....
			// Dummy press buttons like in the attacking direction or last known dir
			// For now press in attack dir: ToDo

			data.bDribble = true;
			data.fStickAngle = 90; // Replace the 90 with attack dir

			g_inputLayer.ProcessActions(data);
			std::this_thread::sleep_for(GlobalParameters::FRAME_TIME);
		}
	}
}

void processFrameRun()
{
	int numItemsLeftToProduce = -1; // Num Items to produce before exit. neg means infinite

	while (true)
	{
		if (g_shouldTerminate)
		{
			numItemsLeftToProduce = 1;
		}

		// Fill in g_frameParameters with new frame info
		Actions actionsToDo;
		if (getFrameDecision(actionsToDo) < 0)
		{
			assert(false && "couldn't take decision for frame");
			break;
		}

		g_cachedLastActionsToDoSentToInput = actionsToDo;

		bool suceed = false;
		do {
			suceed = g_actionBuffer.writeItem(actionsToDo);
			if (suceed == false)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep a little while to let the other thread finish
			}
		} while (suceed == false);

		std::this_thread::sleep_for(GlobalParameters::FRAME_TIME);

		numItemsLeftToProduce--;
		if (numItemsLeftToProduce == 0)
		{
			break;
		}
	}
}

int main(/* int argc, char** argv */)
{
	//PitchEdgesClassificationTool::unitTestPitchFeatures();
	//PitchEdgesClassificationTool::unitTestKeypointsHomography();

	//FramePreprocessingTool::unitTestContours();
	//FramePreprocessingTool::unitTestPitchEdges();

	// When pressing Alt+ A key stop everything and save the image to confirm that we see something good
	// TODO: to the image saved, it is good to write the features and input applied as a debugging strategy
	RegisterHotKey(NULL, 1, MOD_ALT, 0x41); // Alt + A key

	g_lastFrameParams.isTopLevelOutputDebugEnabled	= g_enableTopLevelDebugging;
	g_lastFrameParams.isInsideDebugEnabled			= g_enableInsideDebugging;

	// Init the input layer
	g_inputLayer.init();

	// Init the computer vision layer
	g_computerVisionLayerImpl.init();

	g_hwndDesktop = GetDesktopWindow();

	// Init the decision making layer
	g_decisionMakingLayer.SetStrategy(DecisionMakingLayer::StrategyType::Random);

	std::thread inputThread(inputThreadRun);
	//std::thread processFrameThread(processFrameRun);

	// Let's run the simulation - start it AFTEr you start the game
	while (true)
	{
	
		// Fill in g_frameParameters with new frame info
		Actions actionsToDo;
		if (getFrameDecision(actionsToDo) < 0)
		{
			assert(false && "couldn't take decision for frame");
			break;
		}

		g_cachedLastActionsToDoSentToInput = actionsToDo;

		bool suceed = false;
		do {
			suceed = g_actionBuffer.writeItem(actionsToDo);
			if (suceed == false)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep a little while to let the other thread finish
			}
		} while (suceed == false);

		std::this_thread::sleep_for(GlobalParameters::FRAME_TIME);

		// Check termination conditions
		{
#ifdef IS_SINGLE_FRAME_CAPTURE_ENABLED
			g_shouldTerminate = true;
#else
			MSG msg = { 0 };
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
			{
				if (msg.message == WM_HOTKEY)
				{
					g_shouldTerminate = true;
					break;
				}
			}
#endif

			if (g_shouldTerminate)
			{
				inputThread.join();

				drawVisionOutputDebug(g_lastFrameParams, g_visionOutput, true);
				drawDebugActions(g_cachedLastActionsToDoSentToInput);
				if (g_lastFrameParams.isTopLevelOutputDebugEnabled)
				{
					cv::imshow("Source", g_lastFrameParams.m_originalImgCopy);
				}

				break;
			}
		}
	}

	cv::waitKey(0);
	return(0);
}

