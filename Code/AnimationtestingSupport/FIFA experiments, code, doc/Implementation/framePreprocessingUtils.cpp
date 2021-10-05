#include "framePreprocessingUtils.h"
#include "ComputerVisionUtils.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/core/matx.hpp"
#include <iostream>


namespace ComputerVisionLayer
{
// Search the field pixels begin/end on a given image, row and direction (left / right)
// isFixedAxis_Y == true if we are testing each row for a function call
int FramePreprocessingTool::searchFiledPixels(cv::Mat& img, const int fixedAxisValue, const int dirSign, const bool isFixedAxis_Y)
{
	const int movingAxisLimit = isFixedAxis_Y ? (img.cols - 1) : (img.rows - 1);
	int movingAxisIter = (dirSign > 0 ? 0 : movingAxisLimit);
	int num_ConsecutiveGreenPixelsOnRowBegin = 0;
	int num_ConsecutiveWhitePixelsOnRowBegin = 0;
	int numTotalGreenPixelsOnRowBegin = 0;

	int totalPixelsIteratedOver = 0;
	while ((dirSign > 0 && movingAxisIter <= movingAxisLimit) || (dirSign < 0 && movingAxisIter >= 0))
	{
		totalPixelsIteratedOver++;

		/*static int debugRow = 300;
		static int debugCol = 400;
		if (y == debugRow && xBegin == debugCol)
		{
		int a = 3;
		a++;
		}
		*/

		// Update
		cv::Vec3b& pixel = isFixedAxis_Y ? img.at<cv::Vec3b>(fixedAxisValue, movingAxisIter) : img.at <cv::Vec3b>(movingAxisIter, fixedAxisValue);
		const bool bIsPixelGreen = ColorUtils::isHSVInCluster(pixel, ColorUtils::CL_GREEN);
		const bool bIsPixelWhite = ColorUtils::isHSVInCluster(pixel, ColorUtils::CL_WHITE);
		if (bIsPixelGreen || bIsPixelWhite)
		{
			if (bIsPixelGreen)
			{
				num_ConsecutiveGreenPixelsOnRowBegin++;
				numTotalGreenPixelsOnRowBegin++;
			}
			else
			{
				num_ConsecutiveWhitePixelsOnRowBegin++;
			}
		}
		else
		{
			num_ConsecutiveGreenPixelsOnRowBegin = 0;
			num_ConsecutiveWhitePixelsOnRowBegin = 0;
		}

		const int totalConsecutiveGreenAndWhitePixels = (num_ConsecutiveGreenPixelsOnRowBegin + num_ConsecutiveWhitePixelsOnRowBegin);
		if (numTotalGreenPixelsOnRowBegin >= MIN_TOTAL_GREEN_PIXELS_TO_PITCH ||
			num_ConsecutiveGreenPixelsOnRowBegin >= MIN_CONSECUTIVE_GREEN_PIXELS_TO_PITCH

			// Or if we are in the beginning of the iteration and we have only white pixels mostly..
			|| (totalConsecutiveGreenAndWhitePixels >= MIN_CONSECUTIVE_GREEN_PIXELS_TO_PITCH
				&& totalPixelsIteratedOver < (totalConsecutiveGreenAndWhitePixels * 2.0f))
			)
		{
			return std::min(std::max(movingAxisIter - (totalConsecutiveGreenAndWhitePixels + 1) * dirSign, -1), movingAxisLimit + 1);
		}

		movingAxisIter = movingAxisIter + dirSign;
	}

	return movingAxisIter;
}

void FramePreprocessingTool::removePitchBounds(cv::Mat& img, const ContourClassification& contourClassificationTool )
{
	
	auto recoverGoodPitchPixelsFunc = [&](const cv::Mat& img, std::vector<int>& pitchBounds_min, std::vector<int>& pitchBounds_max, const bool isTestinColumns)
	{

			/*// Early test
			if (isTestinColumns)
			{
				if (testBounds.width < GlobalParameters::MIN_XAXIS_PIXELS_FOR_IRREGULARITY_CHECK)
					return;
			}
			else
			{
				if (testBounds.height < GlobalParameters::MIN_YAXIS_PIXELS_FOR_IRREGULARITY_CHECK)
					return;
			}
			*/	

		//if (isTestinColumns)
		{
			std::vector<int>* inputPitchBounds[2] = {&pitchBounds_min, &pitchBounds_max};
			bool compareMax[2] = { true, false }; // TODO: pointer to function overload !
			const int IRREGULARITY_COUNT_THRESHOLD = isTestinColumns ? GlobalParameters::MIN_ENERGY_IRREGULARITY_XAXIS : GlobalParameters::MIN_ENERGY_IRREGULARITY_YAXIS;
			//compare* c = std::min<int>;
			for (int iPB = 0; iPB < 2; iPB++)
			{
				std::vector<int>& pitchBounds = *inputPitchBounds[iPB];
				const bool compMax = compareMax[iPB];

				// Find the median of the values to know how much to search for
				int median = 0;
				{
					std::vector<int> pitchBounds_copy(pitchBounds);
					const size_t n = pitchBounds_copy.size() / 2;
					std::nth_element(pitchBounds_copy.begin(), pitchBounds_copy.begin() + n, pitchBounds_copy.end());
					median = pitchBounds_copy[n];
				}
				
				/*
				int prevAxisIndex = 0;
				int prevAxisValue = pitchBounds[0];
				int extremeReplaceValue = -1;
				*/
				const int axisLimit = isTestinColumns ? img.cols : img.rows;
				int energyStabilization = IRREGULARITY_COUNT_THRESHOLD;

				for (int currAxisVal = 1; currAxisVal < axisLimit; currAxisVal++)
				{
					/*
					if (abs(prevAxisValue - pitchBounds[currAxisVal]) > (GlobalParameters::BOUND_IRREGULARITY_THRESHOLD - (currAxisVal - prevAxisIndex))) // We subtract some because as we progress we expect some 'slope' increase
					{				
						extremeReplaceValue = compMax ? std::max(extremeReplaceValue, pitchBounds[currAxisVal]) : std::min(extremeReplaceValue, pitchBounds[currAxisVal]);
					}
					else
					{
						// Check if the previous chunk contained a irregularity and it is a big enough
						if (currAxisVal - prevAxisIndex > GlobalParameters::MIN_XAXIS_PIXELS_FOR_IRREGULARITY_CHECK)
						{
							int axisMin = prevAxisValue;
							int axisMax = extremeReplaceValue;
							if (axisMin > axisMax)
							{
								std::swap(axisMin, axisMax);
							}

							const cv::Rect irregDetectedRect = isTestinColumns ? cv::Rect(cv::Point(prevAxisIndex, axisMin), cv::Point(currAxisVal - 1, axisMax)) :
							                                                     cv::Rect(cv::Point(axisMin, prevAxisIndex), cv::Point(axisMax, currAxisVal - 1));
							
							const int startAxis = irregDetectedRect.x;
							const int endAxis = irregDetectedRect.x + irregDetectedRect.width;
							for (int axisIter = startAxis; axisIter <= endAxis; axisIter++)
							{

								// TODO: test here each line by scanning
								const bool testRes = true; // Test Line (axisIter, prevAxisValue) - (axisIter, extremeReplaceValue)

								// If test is true, do not remove the line so set it back !
								if (testRes)
								{
									pitchBounds[axisIter] = prevAxisValue; 
								}
							}
												
						}

						prevAxisIndex = currAxisVal;
						prevAxisValue = pitchBounds[currAxisVal];
						extremeReplaceValue = -1;
					}
					*/

					// Simplified version
					if (abs(pitchBounds[currAxisVal] - pitchBounds[currAxisVal - 1]) > GlobalParameters::MIN_XAXIS_PIXELS_FOR_IRREGULARITY_CHECK)
					{
						energyStabilization = 0;
					}
					else
					{
						energyStabilization++;
					}

					if (energyStabilization >= IRREGULARITY_COUNT_THRESHOLD)
						continue;

					bool shouldRevertLine = false; 
					// Do the line scan and check if the predominant color is the same as in one of the histograms
					{
						Histogram lineHist;
						int otherAxis_min = median;
						int otherAxis_max = pitchBounds[currAxisVal];
						if (otherAxis_min > otherAxis_max)
						{
							std::swap(otherAxis_min, otherAxis_max);
						}

						otherAxis_min = std::max(otherAxis_min, 0);
						otherAxis_max = std::max(std::min(otherAxis_max, isTestinColumns ? img.rows - 1 : img.cols - 1), 0);
						

						cv::Rect testRect = isTestinColumns ? cv::Rect(cv::Point(currAxisVal, otherAxis_min), cv::Point(currAxisVal + 1, otherAxis_max))
															: cv::Rect(cv::Point(otherAxis_min, currAxisVal), cv::Point(otherAxis_max, currAxisVal + 1));
						lineHist.loadFromImage(img, testRect, nullptr, nullptr, ColorUtils::CL_GREEN);
						
						shouldRevertLine = (lineHist.m_predominantCluster == contourClassificationTool.m_homeHist.m_predominantCluster ||
											lineHist.m_predominantCluster == contourClassificationTool.m_awayHist.m_predominantCluster);
					}

					// Not stabilized, perform check
					// TODO:
					if (shouldRevertLine)
					{
						pitchBounds[currAxisVal] = median;
					}
				}				
			}					
		}
	};
	
	// Go over columns and rows and find the start of the pitch
	// The start is happening when > T consecutive pixels are green.
	//const float thresholdValue = 1.0f + (float)(green_threshold) / max_green_threshold;
	{
		std::vector<int> pitchBoundsPerRow_left;
		std::vector<int> pitchBoundsPerRow_right;
		pitchBoundsPerRow_left.resize(img.rows + 1);
		pitchBoundsPerRow_right.resize(img.rows + 1);
		for (int y = 0; y < img.rows; y++)
		{
			pitchBoundsPerRow_left[y] = -1;
			pitchBoundsPerRow_right[y] = img.cols;
			const int leftBound = searchFiledPixels(img, y, 1, true);
			const int rightBound = searchFiledPixels(img, y, -1, true);

			// DEBUG STUFF
			if (y < 100 && leftBound < 200)
			{
				int a = 3;
				a++;
			}

			pitchBoundsPerRow_left[y]	 = leftBound;
			pitchBoundsPerRow_right[y]	 = rightBound;
		}
		pitchBoundsPerRow_left[img.rows]	 = -1;
		pitchBoundsPerRow_right[img.rows]	 = img.cols;
		
		// Recover pixels which represent players
		if (GlobalParameters::DO_BOUNDARY_CHECK)
			recoverGoodPitchPixelsFunc(img, pitchBoundsPerRow_left, pitchBoundsPerRow_right, false);


		// Remove boundaries to pitch
		for (int y = 0; y < img.rows; y++)
		{
			const int start = pitchBoundsPerRow_left[y];
			const int end = pitchBoundsPerRow_right[y];

			for (int x = 0; x < start; x++)
			{
				cv::Vec3b& pixel = img.at<cv::Vec3b>(y, x);
				pixel = ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;
			}

			for (int x = img.cols - 1; x > end; x--)
			{
				cv::Vec3b& pixel = img.at<cv::Vec3b>(y, x);
				pixel = ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;
			}
		}
	}
	
	// Go over columns and do a similar thing
	{
		std::vector<int> pitchBoundsPerCol_up;
		std::vector<int> pitchBoundsPerCol_down;
		pitchBoundsPerCol_up.resize(img.cols + 1);
		pitchBoundsPerCol_down.resize(img.cols + 1);

		for (int x = 0; x < img.cols; x++)
		{
			pitchBoundsPerCol_up[x] = -1;
			pitchBoundsPerCol_down[x] = img.rows;
			
			const int topBound = searchFiledPixels(img, x, 1, false);
			const int downBound = searchFiledPixels(img, x, -1, false);

			pitchBoundsPerCol_up[x] = topBound;
			pitchBoundsPerCol_down[x] = downBound;
		}
		pitchBoundsPerCol_up[img.cols] = -1;
		pitchBoundsPerCol_down[img.cols] = img.rows;

		// Recover pixels which represent players
		if (GlobalParameters::DO_BOUNDARY_CHECK)
			recoverGoodPitchPixelsFunc(img, pitchBoundsPerCol_up, pitchBoundsPerCol_down, true);

		// Remove boundaries to pitch
		for (int x = 0; x < img.cols; x++)
		{
			const int start = pitchBoundsPerCol_up[x];
			const int end = pitchBoundsPerCol_down[x];

			// TODO: optimize for cache usage
			for (int y = 0; y < start; y++)
			{
				cv::Vec3b& pixel = img.at<cv::Vec3b>(y, x);
				pixel = ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;
			}

			for (int y = img.rows - 1; y > end; y--)
			{
				cv::Vec3b& pixel = img.at<cv::Vec3b>(y, x);
				pixel = ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;
			}
		}
	}
}

void FramePreprocessingTool::removeGreenColors(cv::Mat& imgSrc)
{
	//const float thresholdValue = 1.0f + (float)(green_threshold) / max_green_threshold;

	// Remove the green color (pitch) and make it black
	for (int i = 0; i < imgSrc.rows; i++)
	{
		for (int j = 0; j < imgSrc.cols; j++)
		{
			/*if (i == 226 && j == 838)
			{
			//cv::circle(imgSrc, cv::Point(400, 300), 10, Scalar(255, 0, 0), 4);

			int a = 3;
			a++;
			}*/
			cv::Vec3b& pixel = imgSrc.at<cv::Vec3b>(i, j);
			//Vec3b& pixel_rgb = src2.at<cv::Vec3b>(i, j);

			if (ColorUtils::isHSVInCluster(pixel, ColorUtils::CL_GREEN))
				pixel = ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;
		}
	}
}

/**
* @function main
*/

void FramePreprocessingTool::removeAllButWhitePixels(cv::Mat& img)
{
	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			cv::Vec3b& pixel = img.at<cv::Vec3b>(i, j);
			//const uchar& Red = pixel[0];
			//uchar& Green = pixel[1];
			//const uchar& Blue = pixel[2];

			const int th_perChannel = 180;
			if (!ColorUtils::isHSVInCluster(pixel, ColorUtils::CL_WHITE))
				pixel = ColorUtils::PURE_BLACK_THAT_SHOULD_USE_ALPHA;
		}
	}
}

// Given the input image (colored in hsv space) and the white mask associated
// Find the ball coordinates. There might be multiple balls and you have to decide further which one is correct 
// after applying a tracking mechanism
void FramePreprocessingTool::findBallInHsvMask(
	const cv::Mat& inputImg, const cv::Mat& hsvMask_,
	const bool isParentDebugingEnabled,
	const cv::Rect2i* hintToSearchBallIn,		// This is a hint given to search the ball only within that rect.
	const bool searchBallOutsideHint,			// If not found in the rect, can i search it outside ?
	const bool useBothRectLayersForBallFinding, // Use both layers (parameters for size of contour rect) for finding the ball
	std::vector<cv::Point2i>& outBallCoordinatesInImg) const // the output ball coordinates in the image
{
	std::vector <std::pair<cv::Point2i, float>> tempBallCoordinatesAndTrust;

	// Main idea: find the contours and search within them the ball
	// Search within the recommended rect first then if allowed, in the rest of the screen
	// Using both layer sizes for contour rect if specified so. 
	// The reason why we use two layer sizes: first layer checks only small bbox of contours which will get the ball if not attached to foot
	// If attached to foot we won't find the ball unless we try to find it in bigger contours
	// But bigger contours means more cycles to spend !! a lot more. so there is a tradeoff between prediction power and quality of recognition that can be specified by user !
	// Note that the algorithm below doesn't duplicate any efforts for different region images.
	// TODO: add a picture

	// Fill some paramters in
	const int numLayersToSearch = useBothRectLayersForBallFinding ? 2 : 1;
	const int numHintsRects = hintToSearchBallIn != nullptr ? (searchBallOutsideHint ? 2 : 1) : (searchBallOutsideHint ? 1 : 0);
	const int layersSize[2] = { GlobalParameters::MAX_RECT_TO_SEARCH_BALL_LAYER_1, GlobalParameters::MAX_RECT_TO_SEARCH_BALL_LAYER_2 };
	std::array<const cv::Rect2i*, 2> rectsToSearchBallIn = { hintToSearchBallIn , nullptr };

	tempBallCoordinatesAndTrust.clear();
	const bool isLocalDebuggingEnabled = false || isParentDebugingEnabled;

	// Cloning the image (for debugging) to write stuff on it and present it to user
	cv::Mat* inputImgToUse = nullptr;
	cv::Mat inputImgCloned_;
	if (isLocalDebuggingEnabled)
	{
		inputImgCloned_ = inputImg.clone();
		inputImgToUse = const_cast<cv::Mat*>(&inputImgCloned_);
	}
	else
	{
		inputImgToUse = const_cast<cv::Mat*>(&inputImg);
	}
	
	// Step 1: the ball is known to be mostly white (at least in our prototype...)
	// dilate its pixel to obtain a regular contour - fill the black / orange markers on the ball
	cv::Mat hsvMask = hsvMask_.clone();
	cv::dilate(hsvMask, hsvMask, GlobalParameters::BALL_FIND_DILATE_ELEMENT);
	
	if (isLocalDebuggingEnabled)
	{
		cv::imshow("hsvMask_canny_or_dilate", hsvMask);
		cv::imshow("inputImg", inputImg);
	}

	// Step 2: Segment mask given and find all the contours
	std::vector<Contour> contoursInMask;
	std::vector<cv::Vec4i> hierarchyInSubImage;
	cv::findContours(hsvMask, contoursInMask, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	cv::RNG rng(1234);
	for (int rectHintIter = 0; rectHintIter < numHintsRects; rectHintIter++)
	{
		if (tempBallCoordinatesAndTrust.empty() == false)
			break;

		const cv::Rect2i* rectToSearch = rectsToSearchBallIn[rectHintIter];
		const bool alreadySearchedInRecommendedRect = rectHintIter == 1;
		const cv::Rect2i* userRecommendedRect = rectsToSearchBallIn[0];

		for (int layerSizeIter = 0; layerSizeIter < numLayersToSearch; layerSizeIter++)
		{
			const int layerSize = layersSize[layerSizeIter];

			if (tempBallCoordinatesAndTrust.empty() == false)
				break;

			// For all contour
			for (size_t i = 0; i < contoursInMask.size(); i++)
			{
				if (isLocalDebuggingEnabled)
				{
					const cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
					drawContours(*inputImgToUse, contoursInMask, (int)i, color, 2, 8, hierarchyInSubImage, 0, cv::Point());
				}

				// Get the bounding box of the contour
				cv::Rect2i contourRect = cv::boundingRect(contoursInMask[i]);

				// Check the contour size
				if (contourRect.width > layerSize || contourRect.height > layerSize
					// || contourRect.x <= 730 || contourRect.x >= 780 || contourRect.y < 340 || contourRect.y > 360  // HAAAAAAAAAACk for debug
					)
					continue;

				// Check if contour is in the target rect
				const int isContourInTargetRect = rectToSearch == nullptr || rectToSearch->contains(contourRect.tl());
				const int alreadySearchedInThisContour = alreadySearchedInRecommendedRect && userRecommendedRect->contains(contourRect.tl());

				if (isContourInTargetRect == false || alreadySearchedInThisContour)
					continue;


				// See if this is a circle in the initial image
				cv::Mat inputImgInChannels[3];
				cv::split(*inputImgToUse, inputImgInChannels);
				cv::Mat ballImgGrey = inputImgInChannels[2](contourRect);
				//cv::Mat ballImgGrey = ballImgGreyO.clone();
				//cv::resize(ballImgGrey, ballImgGrey, cv::Size(), 5.0f, 5.0f);
				//medianBlur(ballImgGrey, ballImgGrey, 5);

				// TODO: optimize parameters for speed
				std::vector<cv::Vec3f> circles;
				const int minRadiusForCircle = 2;
				const int maxRadiusForCicle = -1; // If negative, radius will not be returned, only the center - which we are actually interested in ! We use negative to speed up things
				const int cannyThreshold = 50; // Edge threshold to consider as point 
				cv::HoughCircles(ballImgGrey, circles, cv::HOUGH_GRADIENT, 1, 50, cannyThreshold, GlobalParameters::MIN_VOTES_TO_CONSIDER_BALL_CENTER, minRadiusForCircle, maxRadiusForCicle);
			

				for (size_t ci = 0; ci < circles.size(); ci++)
				{
					if (isLocalDebuggingEnabled)
					{
						//cv::imshow("ballImgGrey", ballImgGrey);
						//cv::waitKey(0);
					}

					const cv::Point2i circleXY((int)circles[ci][0] + contourRect.x, (int)circles[ci][1] + contourRect.y);

					// First, check the area around ball - we expect a white color for now
					// TODO: parametrize this or do as we did with the shirts - each ball could have a histogram of colors..
					// Now we consider it white:)
					Histogram potentialBallHist;
					cv::Rect2i potentialBallRect;
					potentialBallRect.x = circleXY.x - contourRect.width / 2; // GlobalParameters::BALL_HIST_RADIUS;
					potentialBallRect.y = circleXY.y - contourRect.height / 2; // GlobalParameters::BALL_HIST_RADIUS;
					potentialBallRect.width = contourRect.width; // GlobalParameters::BALL_HIST_RADIUS * 2;
					potentialBallRect.height = contourRect.height; // GlobalParameters::BALL_HIST_RADIUS * 2;

					// Check if not outside image !
					if (potentialBallRect.x < 0 || potentialBallRect.y < 0 ||
						(potentialBallRect.x + potentialBallRect.width) >= inputImgToUse->cols ||
						(potentialBallRect.y + potentialBallRect.height) >= inputImgToUse->rows)
						continue;


					potentialBallHist.loadFromImage(*inputImgToUse, potentialBallRect, nullptr, nullptr);

					const float fillPercent = (float)potentialBallHist.m_numValidPixels / (potentialBallRect.width * potentialBallRect.height);

					if (potentialBallHist.m_pixelsPerCluster[ColorUtils::CL_WHITE] < 0.70f)
						continue;

					if (fillPercent < 0.5f)
						continue;

					tempBallCoordinatesAndTrust.push_back(std::make_pair(circleXY, fillPercent));

					if (isLocalDebuggingEnabled)
					{
						// circle center
						cv::circle(*inputImgToUse, circleXY, 1, cv::Scalar(0, 100, 100), 2, cv::LINE_AA);
						// circle outline
						int radius = contourRect.width / 2;
						circle(*inputImgToUse, circleXY, radius, cv::Scalar(255, 0, 255), 1, cv::LINE_AA);
					}
				}

				if (outBallCoordinatesInImg.empty() == false)
				{
					//break;
				}
			}
		}
	}	

	// sort the ball coordinates and output again the most probable ball
	std::sort(tempBallCoordinatesAndTrust.begin(), tempBallCoordinatesAndTrust.end(), [](const auto&a, const auto&b) -> bool { return a.second < b.second; });
	for (const auto& it : tempBallCoordinatesAndTrust)
	{
		outBallCoordinatesInImg.push_back(it.first);
	}

	// Debug final output in image
	if (isLocalDebuggingEnabled)
	{
		cv::imshow("ballDetectionAndContours", *inputImgToUse);
		//cv::waitKey(0);
	}
}

void FramePreprocessingTool::fillPitchEdges(cv::Mat& hsvInputImage, const FrameParameters& input, FramePreprocessingOutput& output, const bool isParentDebuggingEnabled)
{
	const bool isLocalDebuggingEnabled = false || isParentDebuggingEnabled;
	// A. Remove pitch boundaries and green color from pitch
	{
		removePitchBounds(hsvInputImage, *input.m_contourClassificationTool);
		if (isLocalDebuggingEnabled)
			imshow("NoBoundaries", hsvInputImage);

		removeGreenColors(hsvInputImage);
		if (isLocalDebuggingEnabled)
			imshow("nogreen", hsvInputImage);

		//waitKey(0);
	}

	// B. Find the white edges inside pitch and remove them from initial image
	{
		cv::Mat src2_hsv_mask;

		const cv::Vec3b& minWhite = ColorUtils::m_limitsPerColorCluster[ColorUtils::CL_WHITE][0];
		const cv::Vec3b& maxWhite = ColorUtils::m_limitsPerColorCluster[ColorUtils::CL_WHITE][1];

		cv::Scalar lower_white(minWhite[0], minWhite[1], minWhite[2]);
		cv::Scalar upper_white(maxWhite[0], maxWhite[1], maxWhite[2]);
		cv::inRange(hsvInputImage, lower_white, upper_white, src2_hsv_mask);

		if (isLocalDebuggingEnabled)
			cv::imshow("hsvMask", src2_hsv_mask);

		// Find the potential ball positions using this mask - avoiding to recompute it each time
		// This function doesn't modify inputs sent as parameters
		output.m_ballCoordinatesInImg.clear();
		findBallInHsvMask(hsvInputImage, src2_hsv_mask, isLocalDebuggingEnabled,
						  input.m_recommendedRect, input.m_searchBallOutsideHintRect, input.m_useBothLayersForBallDetection,
						  output.m_ballCoordinatesInImg);

		//cv::Mat inversed_src2_hsv_mask;
		//cv::bitwise_not(src2_hsv_mask, inversed_src2_hsv_mask);

		//cv::Mat res;
		//cv::bitwise_and(hsvInputImage, hsvInputImage, res, inversed_src2_hsv_mask);

		if (isLocalDebuggingEnabled)
		{
			//cv::imshow("hsvWithoutWhiteLines", res);
			
			// This is used for unit testing to take the saved image and continue from here
			//	cv::imwrite(GlobalParameters::BASE_PROJECT_PATH"hsvWithoutWhiteLines.png", res);
		}

		// !!!!!!!!! TODO: Apply canny before !!!!!!!!

		// Find edges using hough
		{				
			//vector<float> p_linesDistances;
			/// 2. Use Probabilistic Hough Transform
			cv::HoughLinesP(src2_hsv_mask, output.m_outPitchLines, 1, CV_PI / 180, hough_threshold, hough_minLineLength, hough_maxLineGap);

			/// Show the result
			//p_linesDistances.resize(p_lines.size());
			for (size_t i = 0; i < output.m_outPitchLines.size(); i++)
			{
				cv::Vec4i l = output.m_outPitchLines[i];
				/*int x0 = l[0];
				int y0 = l[1];
				int x1 = l[2];
				int y1 = l[3];
				*/
				//p_linesDistances[i] = sqrtf((float)((x0 - x1)*(x0 - x1) + (y0 - y1)*(y0 - y1)));

				if (isLocalDebuggingEnabled)
				{
					cv::line(src2_hsv_mask, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(110, 220, 255), 2, cv::LINE_AA);
				}

				// 3. Very important step !!!!!!!
				// All the pitch lines are written with black color to the input given for the rest of the program
				// THis is needed to avoid considering the pitch edges as lines. SO once detected, remove them. 
				// As a concrete example, if we don't do this then if a player contour is overalling a pitch edge then the contour detection will consider the pitch edge and player from the same component contour.
				cv::line(hsvInputImage, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
			}

			if (isLocalDebuggingEnabled)
			{
				cv::imshow("houglLines", src2_hsv_mask);
			}
		}
	}
}

void FramePreprocessingTool::fillPlayerMarkers(const cv::Mat& inputImage, const cv::Mat& hsvInputImage, const std::vector<Contour>& originalContours, const std::vector<std::pair<int, cv::Rect2i>>& potentialPlayerMarkersIndices,
											   const bool isParentDebuggingEnabled, FramePreprocessingOutput& output)
{
	const bool isLocalDebuggingEnabled = false || isParentDebuggingEnabled;

	// Try to detect the controlled player markers by looking at the potential small bounding boxes.
	// We'll do a contour search in the subimage for optimal performance
	// Then we check if that looks like a triangle and we'll return the most representative color cluster 
	///////
	{
		for (int i = 0; i < potentialPlayerMarkersIndices.size(); i++)
		{
			const int originalContourIndex = potentialPlayerMarkersIndices[i].first;
			const cv::Rect& counterRect = potentialPlayerMarkersIndices[i].second;

			assert(counterRect.area() > 0.0);
			cv::Mat subImageWithMarker = inputImage(counterRect); // This is not a deep copy !

			//cv::imshow("subImageMarker", subImageWithMarker);
			//cv::waitKey(0);

			// Find the contours in the subimage
			std::vector<Contour> contoursInSubImage;
			std::vector<cv::Vec4i> hierarchyInSubImage;
			cv::findContours(subImageWithMarker, contoursInSubImage, hierarchyInSubImage, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

			// Take the most consistent one if there are more
			int subContourIndex = INVALID_INDEX;
			if (contoursInSubImage.size() > 1)
			{
				double maxArclen = INVALID_INDEX;
				for (int cntIdx = 0; cntIdx < contoursInSubImage.size(); cntIdx++)
				{
					const Contour& contour = contoursInSubImage[cntIdx];
					const double contourArcLen = cv::arcLength(contour, true);
					if (contourArcLen > maxArclen)
					{
						maxArclen = contourArcLen;
						subContourIndex = cntIdx;
					}
				}
			}
			else
			{
				subContourIndex = 0;
			}

			if (subContourIndex == INVALID_INDEX)
			{
				assert(false && "we coldn't identify any subcontour for player marker !! this is a hard error place check");
				continue;
			}

			// Approximate the contour poly and see if we can get a triangle out of it
			const Contour& selectedContour = contoursInSubImage[subContourIndex];
			Contour postProcessedSubContour(selectedContour.size());
			cv::approxPolyDP(selectedContour, postProcessedSubContour, 0.07f * cv::arcLength(selectedContour, true), true);

			// TODO: maybe we should check again here the dimensions..
			if (postProcessedSubContour.size() == 3)
			{
				const cv::Rect2i selectedContourBBox = cv::boundingRect(originalContours[originalContourIndex]);

				PlayerMarkerInfo markerInfo(ColorUtils::CL_NUM_CLUSTERS, selectedContourBBox);
				Histogram hist;
				hist.loadFromImage(hsvInputImage, selectedContourBBox, nullptr, nullptr);
				if (hist.m_pixelsPerCluster[ColorUtils::CL_RED] > 0.65f)
				{
					markerInfo.teamSide = TEAM_SIDE_HOME;
				}
				else if (hist.m_pixelsPerCluster[ColorUtils::CL_BLUE] > 0.5f)
				{
					markerInfo.teamSide = TEAM_SIDE_AWAY;
				}
				else
				{
					// Fake !
					continue;
				}

				output.m_playerMarkers.push_back(markerInfo); // TODO: might be usefully to find the predominant color inside marker..

#if 0
				cv::putText(inputImage, "tri", postProcessedSubContour[0],
					cv::FONT_HERSHEY_COMPLEX, 1, // font face and scale
					Scalar(255, 255, 255), // white
					1, LINE_AA); // line thickness and type
#endif
			}
		}
	}
}

void FramePreprocessingTool::fillPlayersBoundaries(const cv::Mat& hsvInputImage, FramePreprocessingOutput& output, const bool isParentDebuggingEnabled)
{
	const bool isLocalDebuggingEnabled = false || isParentDebuggingEnabled;

	cv::RNG rng(1234);

	///////////////////////////////////////////////////////////////////
	cv::Mat canny_output;
	std::vector<Contour > contours;
	std::vector<cv::Vec4i> hierarchy;

	if (isLocalDebuggingEnabled)
		cv::imshow("hsvInput", hsvInputImage);

	cv::Mat binary_input;
	cv::inRange(hsvInputImage, cv::Scalar(0, 0, 0, 0), cv::Scalar(180, 255, 30, 0), binary_input);

	if (isLocalDebuggingEnabled)
		cv::imshow("binaryInRange", binary_input);

	cv::bitwise_not(binary_input, binary_input);

	if (isLocalDebuggingEnabled)
		cv::imshow("binaryNOT", binary_input);

	// Cloning the image - the purpose is to have the unprocessed image available for some contour detection that could be affected by postprocessed stuff
	// E.g. ball detection + marker detection
	cv::Mat binary_input_notPostProcessed = binary_input.clone();

#if 1
	{	
		cv::Mat element_erode = getStructuringElement(cv::MORPH_RECT, cv::Size(4, 4));
		cv::Mat element_dilate = getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
		
		/// Apply the erosion and dilation operations
		cv::erode(binary_input, binary_input, element_erode);

		if (isLocalDebuggingEnabled)
		{
			cv::imshow("erode", binary_input);
		}

		cv::dilate(binary_input, binary_input, element_dilate);

		if (isLocalDebuggingEnabled)
		{
			cv::imshow("dilate", binary_input);
		}

		// TODO: do we really need this ???
		cv::GaussianBlur(binary_input, binary_input, cv::Size(9, 9), 2, 2);
	}
#endif

	//cv::threshold(inputImage, inputImage, 1, 255, THRESH_BINARY);
	if (isLocalDebuggingEnabled)
	{
		cv::imshow("thresholded", binary_input);
	}

	//cv::waitKey(0);

	canny_output = hsvInputImage.clone();
	cv::Mat canny_output_copy = canny_output.clone();

	/// Find contours
	cv::findContours(binary_input, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	//cv::waitKey(0);

	/// Draw contours
	cv::Mat drawing = cv::Mat::zeros(canny_output.size(), CV_8UC3);

	output.m_playersContoursBBoxes.clear();
	output.m_playersContoursBBoxes.reserve(contours.size());

	std::vector<std::pair<int, cv::Rect>> potentialPlayerMarkersIndices; // indices of contours that could look like a potential area for a player marker
	potentialPlayerMarkersIndices.reserve(2);
	for (size_t i = 0; i < contours.size(); i++)
	{
		const cv::Rect rect = cv::boundingRect(contours[i]);
		if (rect.width <= GlobalParameters::minAxisDimForBoundingRect || rect.height <= GlobalParameters::minAxisDimForBoundingRect)
			continue;

		// Is rectangle a potential marker ? Add it for later analysis
		if (rect.width <= GlobalParameters::maxAxisDimForPlayerMarker && rect.height <= GlobalParameters::maxAxisDimForPlayerMarker)
		{
			potentialPlayerMarkersIndices.push_back(std::make_pair((int)i, rect));
		}

		if (isLocalDebuggingEnabled)
		{
			cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			drawContours(drawing, contours, (int)i, color, 2, 8, hierarchy, 0, cv::Point());
		}

		output.m_playersContoursBBoxes.push_back(cv::boundingRect(contours[i]));
	}

	if (GlobalParameters::DO_PLAYER_MARKERS_CHECK)
	{
		fillPlayerMarkers(binary_input_notPostProcessed, hsvInputImage, contours, potentialPlayerMarkersIndices, isLocalDebuggingEnabled, output);
	}

	if (isLocalDebuggingEnabled)
	{
		cv::imshow("contours", drawing);

		cv::imshow("FeaturesIdentificationUnitTest", drawing);
	}

	


	//cv::waitKey(0);
	///////////////////////////////////////////////////////////////////
}

bool FramePreprocessingTool::preProcessImageFrame(cv::Mat& inputImage, const FrameParameters& inputParams, FramePreprocessingOutput& output, const bool isParentDebuggingEnabled)
{
	const bool isLocalDebuggingEnabled = false || isParentDebuggingEnabled;

	// TODO: not really correct but leave it like this for now :)
	output.m_totalHeight = output.m_usableHeight = (float)inputImage.rows;
	output.m_totalWidth = output.m_usableWidth = (float)inputImage.cols;

	// Step 1: Transform image to HSV format - note that input image is modified between functions
	cvtColor(inputImage, inputImage, CV_BGR2HSV);

	if (isLocalDebuggingEnabled)
		cv::imshow("hsv", inputImage);
	
	{
		fillPitchEdges(inputImage, inputParams, output, isLocalDebuggingEnabled);

		fillPlayersBoundaries(inputImage, output, isLocalDebuggingEnabled);
	}

	return true;
}

void FramePreprocessingTool::unitTestContours()
{
	/// Load source image - fake
	cv::String imageName(GlobalParameters::getFullPath("contoursUnitTest.png")); 
	cv::Mat inputImage = cv::imread(imageName, cv::IMREAD_COLOR);

	if (inputImage.empty())
	{
		std::cout << "No image supplied ..." << std::endl;
		return;
	}

	{
		const char* source_window = "Source";
		cv::namedWindow(source_window, cv::WINDOW_AUTOSIZE);
		cv::imshow(source_window, inputImage);
	}

	FramePreprocessingTool concrete;
	FramePreprocessingOutput output;
	concrete.fillPlayersBoundaries(inputImage, output, true);

	ContourClassification contoursClassifyTool;
	contoursClassifyTool.loadHistograms(GlobalParameters::getFullPath("KIT_BARCELONA_HOME.png"),
										GlobalParameters::getFullPath("KIT_VILLAREAL.png"),
										 false,
										 false);

	//cv::imshow("ImageAfterBoundaries", inputImage);
	//cv::waitKey(0);

	ContourClassification::ClassificationScores scores;
	for (const cv::Rect2i& bbox : output.m_playersContoursBBoxes)
	{
		// Classify the bbox in the input image
		// The input image must have only the players with real colors.
		contoursClassifyTool.compareHistograms(inputImage, bbox, scores);

		cv::Scalar colorsByContourCLass[TEAM_SIDE_NUMS] =
		{
			cv::Scalar(255, 0, 0),
			cv::Scalar(0, 255, 0),
			cv::Scalar(255, 255, 255)
		};

		cv::rectangle(inputImage, bbox, colorsByContourCLass[scores.m_predominantClass], 3);
	}

	cv::imshow("PlayerClassification", inputImage);
	//cv::waitKey(0);
}

void FramePreprocessingTool::unitTestPitchEdges()
{
	/// Load source image - fake
	cv::String imageName(GlobalParameters::getFullPath("fifa18_g3.png"));
	cv::Mat inputImage = cv::imread(imageName, cv::IMREAD_COLOR);
	cv::cvtColor(inputImage, inputImage, CV_BGR2HSV);

	if (inputImage.empty())
	{
		std::cout << "No image supplied ..." << std::endl;
		return;
	}

	{
		const char* source_window = "Source";
		namedWindow(source_window, cv::WINDOW_AUTOSIZE);
		imshow(source_window, inputImage);
	}

	FramePreprocessingTool concrete;
	FramePreprocessingOutput output;
	FrameParameters inputParams;
	concrete.fillPitchEdges(inputImage, inputParams, output, true);
	//cv::waitKey(0);
}

};