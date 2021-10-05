#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include "InputSendingLayer.h"
#include "ComputerVisionLayer.h"
//#include "BrainTree.h"

class Strategy;

namespace ComputerVisionLayer
{
	struct VisionOutput;
};

class DecisionMakingLayer
{
public:
	enum StrategyType
	{
		Random,
		AIBehavior,
	};

	DecisionMakingLayer() { m_Strategy = NULL; }
	void SetStrategy(int _type);
	Actions Execute(ComputerVisionLayer::VisionOutput& input);

private:
	Strategy * m_Strategy;
};

class Strategy
{
public:
	virtual Actions Execute(ComputerVisionLayer::VisionOutput& output) = 0;
};

class RandomDecisionMaking: public Strategy
{
public:
	RandomDecisionMaking();// : Strategy() {}
	DecisionMakingActions GetCurrentAction();
private:
	Actions Execute(ComputerVisionLayer::VisionOutput& input);
};

#define BEHAVIORS_NUM 4

class UtilityAI : public Strategy
{
public:
	void ProcessBallHolder(ComputerVisionLayer::VisionOutput& _input);
	bool m_bIsOffensiveTeam;
	cv::Point2i m_BallPossessorCoordinates;
	cv::Point2i m_BallDefensiveHolder;

protected:
	//ComputerVisionLayer::VisionOutput _computerVisionOutput;
	//InputSendingLayerImpl m_InputSendingLayer;
private:
	cv::Vec2f m_PlayerToPass;
	cv::Vec2f m_PlayerToTackle;
	cv::Vec2f m_PositionToShoot;
	cv::Vec2f m_PositionToDribble;
	float m_fPowerToShoot;
	
	float PassBehavior(ComputerVisionLayer::VisionOutput& _input);
	float TackleBehavior(ComputerVisionLayer::VisionOutput& _input);
	float ShootBehavior(ComputerVisionLayer::VisionOutput& _input);
	float DribbleBehavior(ComputerVisionLayer::VisionOutput& _input);

	Actions Execute(ComputerVisionLayer::VisionOutput& _computerVisionOutput);
};

/*class Behavior
{
protected:
	ComputerVisionLayer::VisionOutput m_ComputerVisionOutput;
	InputSendingLayerImpl m_InputSendingLayer;
public:
	virtual float BehaviorImpl() = 0;
	void ProcessBallHolder();
	bool m_bIsOffensiveTeam;
	cv::Point2i m_BallPossessorCoordinates;
	cv::Point2i m_BallDefensiveHolder;
};

class PassBehavior : public Behavior
{
private:
	cv::Vec2f m_PlayerToPass;
public:
	virtual float BehaviorImpl();
};

class TackleBehavior : public Behavior
{
private:
	cv::Vec2f m_PlayerToTackle;
protected:
	float BehaviorImpl();
};

class ShootBehavior : public Behavior
{
private:
	cv::Vec2f m_PositionToShoot;
protected:
	float BehaviorImpl();
public:
	ShootBehavior()
	{
		m_PositionToShoot = cv::Vec2f(0.0f, 0.0f);
	}
};

class DribbleBehavior : public Behavior
{
private:
	cv::Vec2f m_PositionToDribble;
protected:
	float BehaviorImpl();
};*/