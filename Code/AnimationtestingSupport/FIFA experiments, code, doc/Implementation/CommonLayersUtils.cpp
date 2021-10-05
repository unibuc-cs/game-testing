#include "CommonLayersUtils.h"
#include <sstream>
#include <opencv2\imgproc.hpp>

void CommonUtils::drawMultiLineText(cv::Mat& outImg, const char* stringToWrite, const int startCol, const int startRow, const int distBetweenRows
	, const cv::Scalar& color)
{
	std::stringstream ss(stringToWrite);
	std::string line;
	int currentRow = startRow;

	while (std::getline(ss, line, '\n'))
	{
		cv::putText(outImg, line, cv::Point(startCol, currentRow),
					cv::FONT_HERSHEY_COMPLEX, 1, // font face and scale
					color, // white
					1, cv::LINE_AA); // line thickness and type

		currentRow += distBetweenRows;
	}

}

