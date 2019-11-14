#pragma once

#include "ofMain.h"
#include <k4a/k4a.hpp>
#include <k4abt.h>
#include "ofxAzureKinect.h"

#include "cpprest/http_client.h"
#include <opencv2/opencv.hpp>

class ofApp
	: public ofBaseApp
{
public:
	void setup();
	void exit();

	void update();
	void draw();

	//TODO: Move to move detection class
	double calculateDistance(k4a_float3_t pos1, k4a_float3_t pos2);

	//TODO: Refactor to Stomp Detection Class 
	void saveJointsAndVel(const k4abt_joint_t joints[32],
		double leftVel,
		double rightVel,
		uint64_t timeStamp);
	//double leftHandVel,
	//double rightHandVel);
	double calculateVelocity(const k4abt_joint_t joints[32], int leg, uint64_t timeStamp);
	int didStomp(double velocity, double lastVelocity);
	double calculateYDist(k4a_float3_t pos1, k4a_float3_t pos2);

	//TODO: Refactor to Clap Detection Class
	double calculateVelocityClap(const k4abt_joint_t joints[32], int arm, uint64_t timeStamp);
	int didClap(const k4abt_joint_t joints[32]);

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);


private:
	ofxAzureKinect::Device kinectDevice;

	ofEasyCam camera;

	ofVbo pointsVbo;
	ofShader shader;
	ofMesh mesh;

	ofSoundPlayer soundStomp;
	ofSoundPlayer soundClap;

	ofVboMesh skeletonMesh;

	uint64_t m_lastTimeStamp;
	// TODO: Refactor into Stomp Detection Class
	k4a_float3_t m_leftAnkle;
	k4a_float3_t m_rightAnkle;
	double m_lastRightVel;
	double m_lastLeftVel;
	//---------------------

	//TODO: Refactor into Clap Detection Class
	k4a_float3_t m_leftHand;
	k4a_float3_t m_rightHand;
	double m_lastRightHandVel;
	double m_lastLeftHandVel;
	double m_lastHandDist;
	//-----------------------

	// Elapsed time in seconds from application start
	float elapsedTime;

	pplx::task<void> HTTPFaceAsync(k4a::image img);
	concurrency::streams::istream k4aImageToIstream(k4a::image img, bool DEBUG);
};
