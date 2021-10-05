#pragma once

#include "framePreprocessingUtils.h"
#include "featuresIdentification.h"

namespace ComputerVisionLayer
{

class ComputerVisionLayerImpl
{
public:

	ComputerVisionLayerImpl();
	void processImageFrame(FrameParameters inputParams, VisionOutput& output, const bool parentDebuggingEnabled);
	void init();

	FramePreprocessingTool& getPreProTool() { return g_preProcessingTool; }
	PitchEdgesClassificationTool& getPitchEdgesClassificationTool() { return g_pitchEdgesClassificationTool; }

private:
	FramePreprocessingTool			g_preProcessingTool;
	PitchEdgesClassificationTool	g_pitchEdgesClassificationTool;
};

};