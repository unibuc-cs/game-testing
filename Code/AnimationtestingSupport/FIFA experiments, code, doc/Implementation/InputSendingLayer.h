#pragma once

#include "stdafx.h"
#include "public.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "Math.h"
#include "vjoyinterface.h"
#include "Vector2D.h"
#include "Utils.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#define DEV_ID		1 	// Default device ID (Used when ID not specified)
#define MAX_AXIS_VALUE 32768
#define MIN_AXIS_VALUE 4096
#define NEUTRAL_AXIS_VALUE 16384
#define NEUTRAL_AXIS_HEX_VALUE 0x4000


class InputSendingLayerImpl
{
public:
	InputSendingLayerImpl();

	JOYSTICK_POSITION_V2 GetJoystickPositionData()
	{
		return m_iReport;
	}

	void ProcessActions(Actions& _actionsToDo);

	void init();

	void PrintVJoyStatus(UINT _devID);

	void DecimalToHexa(int _value, LONG &_outHexNum);
	void AngleToHexaValues(float _angle, LONG &_xValue, LONG &_yValue);

	void SendPositionDataToVJoyDevice(UINT _devID, JOYSTICK_POSITION_V2 &_iReport);

private:
	JOYSTICK_POSITION_V2 m_iReport; // The structure that holds the full position data
	Actions m_ActionsProcessed;
};