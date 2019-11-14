#include "ofApp.h"

using namespace ::pplx;
using namespace concurrency::streams;
using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::json;
using namespace cv;

const int LEFT = 111;
const int RIGHT = 112;
string_t endpoint;
string_t key;

ofColor bgColor = ofColor::white;

//--------------------------------------------------------------
void ofApp::setup()
{
	//ofSetLogLevel(OF_LOG_VERBOSE);

	ofLogNotice(__FUNCTION__) << "Found " << ofxAzureKinect::Device::getInstalledCount() << " installed devices.";

	auto kinectSettings = ofxAzureKinect::DeviceSettings();
	kinectSettings.synchronized = false;
	kinectSettings.depthMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
	kinectSettings.updateIr = false;
	kinectSettings.updateColor = true;
	kinectSettings.updateBodies = true;
	kinectSettings.updateWorld = true;
	kinectSettings.updateVbo = false;
	if (this->kinectDevice.open(kinectSettings))
	{
		this->kinectDevice.startCameras();
	}

	// Load shader.
	auto shaderSettings = ofShaderSettings();
	shaderSettings.shaderFiles[GL_VERTEX_SHADER] = "shaders/render.vert";
	shaderSettings.shaderFiles[GL_FRAGMENT_SHADER] = "shaders/render.frag";
	shaderSettings.intDefines["BODY_INDEX_MAP_BACKGROUND"] = K4ABT_BODY_INDEX_MAP_BACKGROUND;
	shaderSettings.bindDefaults = true;
	if (this->shader.setup(shaderSettings))
	{
		ofLogNotice(__FUNCTION__) << "Success loading shader!";
	}

	// Setup vbo.
	std::vector<glm::vec3> verts(1);
	this->pointsVbo.setVertexData(verts.data(), verts.size(), GL_STATIC_DRAW);

	//Set up sound
	soundStomp.load("sounds/snap.wav");
	if (soundStomp.isLoaded())
	{
		printf("STOMP SOUND LOADED\n");
		soundStomp.setMultiPlay(true);
	}
	soundClap.load("sounds/sound1.wav");
	if (soundClap.isLoaded())
	{
		printf("CLAP SOUND LOADED\n");
		soundClap.setMultiPlay(true);
	}

	// Read in API endpoint and key from secrets.txt file in bin/data/keys
	ifstream secrets;
	secrets.open("../bin/data/keys/secrets.txt");
	string line;
	getline(secrets, line);
	endpoint = conversions::to_string_t(line);
	getline(secrets, line);
	key = conversions::to_string_t(line);

	this->elapsedTime = ofGetElapsedTimef();

	printf("Setup completed!\n");
}

//--------------------------------------------------------------
void ofApp::exit()
{
	this->kinectDevice.close();
}

//--------------------------------------------------------------
void ofApp::update()
{
	ofSoundUpdate();

	// Update emotions every 4 seconds
	if (ofGetElapsedTimef() - this->elapsedTime > 4) {
		pplx::task<void> task = HTTPFaceAsync(this->kinectDevice.getImage());
		this->elapsedTime = ofGetElapsedTimef();
	}
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofBackground(bgColor);

	this->camera.begin();
	{
		ofPushMatrix();
		{
			ofRotateXDeg(180);

			ofEnableDepthTest();

			constexpr int kMaxBodies = 6;
			int bodyIDs[kMaxBodies];
			int i = 0;
			while (i < this->kinectDevice.getNumBodies())
			{
				bodyIDs[i] = this->kinectDevice.getBodyIDs()[i];
				++i;
			}
			while (i < kMaxBodies)
			{
				bodyIDs[i] = 0;
				++i;
			}

			this->shader.begin();
			{
				this->shader.setUniformTexture("uDepthTex", this->kinectDevice.getDepthTex(), 1);
				this->shader.setUniformTexture("uBodyIndexTex", this->kinectDevice.getBodyIndexTex(), 2);
				this->shader.setUniformTexture("uWorldTex", this->kinectDevice.getDepthToWorldTex(), 3);
				this->shader.setUniform2i("uFrameSize", this->kinectDevice.getDepthTex().getWidth(), this->kinectDevice.getDepthTex().getHeight());
				this->shader.setUniform1iv("uBodyIDs", bodyIDs, kMaxBodies);

				int numPoints = this->kinectDevice.getDepthTex().getWidth() * this->kinectDevice.getDepthTex().getHeight();
				this->pointsVbo.drawInstanced(GL_POINTS, 0, 1, numPoints);
			}
			this->shader.end();

			ofDisableDepthTest();

			uint64_t timeStamp = this->kinectDevice.getTimeStamp();
			auto& bodySkeletons = this->kinectDevice.getBodySkeletons();
			for (auto& skeleton : bodySkeletons)
			{
				// Draw joints.
				for (int i = 0; i < K4ABT_JOINT_COUNT; ++i)
				{
					auto joint = skeleton.joints[i];
					ofPushMatrix();
					{
						glm::mat4 transform = glm::translate(toGlm(joint.position)) * glm::toMat4(toGlm(joint.orientation));
						ofMultMatrix(transform);

						//ofDrawAxis(50.0f);
					}
					ofPopMatrix();
				}

				// Draw connections.
				this->skeletonMesh.setMode(OF_PRIMITIVE_LINES);
				auto& vertices = this->skeletonMesh.getVertices();
				vertices.resize(50);

				//Calculate velocity of left and right ankle
				double leftVelocity = calculateVelocity(skeleton.joints, LEFT, timeStamp);
				double rightVelocity = calculateVelocity(skeleton.joints, RIGHT, timeStamp);
				//Stomp?
				if (didStomp(leftVelocity, m_lastLeftVel)) {
					printf("LEFT STOMP\n");
					//TODO: Logic to register stomp/create stomp vfx
					soundStomp.play();
				}
				if (didStomp(rightVelocity, m_lastRightVel))
				{
					printf("RIGHT STOMP\n");
					//TODO: Logic to register stomp/create stomp vfx
					soundStomp.play();
				}
				printf("right vel = (last = %f) %f, left vel = (last = %f) %f\n", m_lastRightVel, rightVelocity, m_lastLeftVel, leftVelocity);

				//Clap?
				if (didClap(skeleton.joints)) {
					printf("CLAP!\n");
					soundClap.play();
					if (soundClap.isPlaying()) {
						printf("CLAP PLAYING\n");
					}
				}
				saveJointsAndVel(skeleton.joints, leftVelocity, rightVelocity, timeStamp);
				int vdx = 0;

				// Spine.
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_PELVIS].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SPINE_NAVAL].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SPINE_NAVAL].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SPINE_CHEST].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SPINE_CHEST].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NECK].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NECK].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_HEAD].position);

				// Head.
				/*vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_HEAD].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NOSE].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NOSE].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_EYE_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_EYE_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_EAR_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NOSE].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_EYE_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_EYE_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_EAR_RIGHT].position);*/

				// Left Leg.
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_PELVIS].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_HIP_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_HIP_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_KNEE_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_KNEE_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ANKLE_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ANKLE_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_FOOT_LEFT].position);

				// Right leg.
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_PELVIS].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_HIP_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_HIP_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_KNEE_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_KNEE_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ANKLE_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ANKLE_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_FOOT_RIGHT].position);

				// Left arm.
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NECK].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_CLAVICLE_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_CLAVICLE_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SHOULDER_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SHOULDER_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ELBOW_LEFT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ELBOW_LEFT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_WRIST_LEFT].position);

				// Right arm.
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_NECK].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_CLAVICLE_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_CLAVICLE_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SHOULDER_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_SHOULDER_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ELBOW_RIGHT].position);

				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_ELBOW_RIGHT].position);
				vertices[vdx++] = toGlm(skeleton.joints[K4ABT_JOINT_WRIST_RIGHT].position);

				// Draw skeleton
				ofSetColor(0);
				ofSetLineWidth(10);
				this->skeletonMesh.draw();

				int radius = 150;
				glm::vec3 head = toGlm(skeleton.joints[K4ABT_JOINT_HEAD].position);
				head.y -= radius;
				ofNoFill();
				ofDrawCircle(head, radius);

				glm::vec3 eyeLeft = toGlm(skeleton.joints[K4ABT_JOINT_EAR_LEFT].position);
				eyeLeft.z = head.z;
				eyeLeft.y -= 100;
				glm::vec3 eyeRight = toGlm(skeleton.joints[K4ABT_JOINT_EAR_RIGHT].position);
				eyeRight.z = head.z;
				eyeRight.y -= 100;
				ofFill();
				ofDrawCircle(eyeLeft, 20);
				ofDrawCircle(eyeRight, 20);
			}
		}
		ofPopMatrix();
	}
	this->camera.end();

	//ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate(), 2) + " FPS", 10, 20);
}

//TODO: Refactor into MOVE Detection Class
double ofApp::calculateDistance(k4a_float3_t pos1, k4a_float3_t pos2)
{
	auto xDistance = pow(pos2.xyz.x - pos1.xyz.x, 2);
	auto yDistance = pow(pos2.xyz.y - pos1.xyz.y, 2);
	auto zDistance = pow(pos2.xyz.z - pos1.xyz.z, 2);
	return sqrt(xDistance + yDistance + zDistance);
}

double ofApp::calculateYDist(k4a_float3_t pos1, k4a_float3_t pos2)
{
	double dp = pos2.xyz.y - pos1.xyz.y;
	return abs(dp);
}

// TODO: Refactor into Stomp/Clap Detection class
void ofApp::saveJointsAndVel(const k4abt_joint_t joints[32],
	double leftVel,
	double rightVel,
	uint64_t timeStamp)
{
	//STOMP DETECTION
	m_leftAnkle = joints[K4ABT_JOINT_ANKLE_LEFT].position;
	m_rightAnkle = joints[K4ABT_JOINT_ANKLE_RIGHT].position;
	m_lastLeftVel = leftVel;
	m_lastRightVel = rightVel;
	m_lastTimeStamp = timeStamp;
}

//TODO: Refactor into Stomp Detection class
double ofApp::calculateVelocity(const k4abt_joint_t joints[32], int leg, uint64_t timeStamp)
{
	//KNEE_LEFT 19, ANKLE_LEFT 20, KNEE_RIGHT 23, ANKLE_RIGHT 24
	double dp = 0;
	if (leg == LEFT) {
		dp = calculateYDist(joints[K4ABT_JOINT_ANKLE_LEFT].position, m_leftAnkle);
	}
	if (leg == RIGHT) {
		dp = calculateYDist(joints[K4ABT_JOINT_ANKLE_RIGHT].position, m_rightAnkle);
	}
	double dt = double(timeStamp - m_lastTimeStamp) / 1000000;
	return dp / dt;
}

//TODO: Refactor into Stomp Detection Class
int ofApp::didStomp(double velocity, double lastVelocity)
{
	if (velocity <= 700.0) {
		if (lastVelocity > 1500.0) {
			return 1;
		}
	}
	return 0;
}

double ofApp::calculateVelocityClap(const k4abt_joint_t joints[32], int arm, uint64_t timeStamp)
{
	//HAND_LEFT 8, HAND_RIGHT 15
	double dp = 0;
	if (arm == LEFT) {
		dp = calculateDistance(joints[K4ABT_JOINT_HAND_LEFT].position, m_leftHand);
	}
	if (arm == RIGHT) {
		dp = calculateDistance(joints[K4ABT_JOINT_HAND_RIGHT].position, m_rightHand);
	}
	uint64_t dt = (timeStamp - m_lastTimeStamp) / 1000000;
	return dp / dt;
}

int ofApp::didClap(const k4abt_joint_t joints[32])
{
	int didClap = 0;
	double distance = calculateDistance(joints[K4ABT_JOINT_HAND_LEFT].position, joints[K4ABT_JOINT_HAND_RIGHT].position);
	if (distance < 110.0  && m_lastHandDist >= 150.0) {
		didClap = 1;
	}
	//printf("last = %f, current = %f\n", m_lastHandDist, distance);
	m_lastHandDist = distance;
	return didClap;
}

//--------------------------------------------------------------
concurrency::streams::istream ofApp::k4aImageToIstream(k4a::image img, bool DEBUG = false) {
	// Retrieve Azure Kinect image data
	cout << "Get Kinect Image" << endl;
	int nSize = img.get_size();
	const uint8_t* pcBuffer = img.get_buffer();

	// Create a Size(1, nSize) Mat object of 8-bit, single-byte elements
	Mat rawData(1, nSize, CV_8UC1, (void*)pcBuffer);

	// Read image from buffer in memory
	Mat decodedImage = imdecode(rawData, IMREAD_COLOR);

	if (decodedImage.data == NULL)
	{
		ofLogError(__FUNCTION__) << "Unable to decodeImage";
	}
	else if (DEBUG) {
		cout << "Display Image" << endl;
		imshow("Display window", decodedImage);
	}

	// Encode image to jpg format for Azure Face API
	cout << "Encode Image" << endl;
	std::vector<uchar> buff;
	cv::imencode(".jpg", decodedImage, buff);
	std::stringstream ssbuff;
	copy(buff.begin(), buff.end(), std::ostream_iterator<unsigned char>(ssbuff, ""));

	// Convert image into an istream format
	concurrency::streams::bytestream byteStream = concurrency::streams::bytestream();
	concurrency::streams::istream inputStream = byteStream.open_istream(ssbuff.str());
	return inputStream;
}

pplx::task<void> ofApp::HTTPFaceAsync(k4a::image img) {
	ofLogNotice(__FUNCTION__) << "----GET FACE INFORMATION----";

	http_client client(endpoint);

	cout << "Building URL" << endl;
	// Build request URI and start the request.
	auto query = uri_builder()
		.append_query(U("subscription-key"), key)
		.append_query(U("returnFaceAttributes"), U("emotion"))
		.to_string();

	cout << "Get istream from image buffer" << endl;
	// Set DEBUG to true to show the frame that the kinect is capturing
	concurrency::streams::istream fileStream = k4aImageToIstream(img, false);

	cout << "Send Post Request" << endl;
	return client
		.request(methods::POST, query, fileStream)
		.then([fileStream](pplx::task<http_response> previousTask)
	{
		cout << "We got a response!" << endl;
		fileStream.close();

		return previousTask.get().extract_json();
	})
		.then([](pplx::task<json::value> previousTask)
	{
		try
		{
			// Parse JSON response
			json::value const& value = previousTask.get();
			auto faces = value.as_array();

			if (faces.size() <= 0) return;

			// TODO: Apply emotion detection to multiple faces. Currently only to the first face
			auto emotions = faces[0].at(U("faceAttributes")).at(U("emotion")).as_object();

			// Retrieve the estimated emotion
			cout << "=========================" << endl;
			string emotion = "";
			double max = 0;
			for (auto i = emotions.cbegin(); i < emotions.cend(); ++i)
			{
				auto &emotionKey = i->first;
				auto &emotionValue = i->second;
				if (emotionValue.as_double() > max)
				{
					max = emotionValue.as_double();
					emotion = conversions::to_utf8string(emotionKey);
				}
				std::wcout << "Property: " << emotionKey << ", Value: " << emotionValue << std::endl;
			}
			cout << "=========================" << endl;

			// Change background color based on emotion
			if (emotion.compare("anger") == 0)
			{
				bgColor = ofColor::red;
			}
			else if (emotion.compare("contempt") == 0)
			{
				bgColor = ofColor::purple;
			}
			else if (emotion.compare("disgust") == 0)
			{
				bgColor = ofColor::brown;
			}
			else if (emotion.compare("fear") == 0)
			{
				bgColor = ofColor::black;
			}
			else if (emotion.compare("happiness") == 0)
			{
				bgColor = ofColor::yellow;
			}
			else if (emotion.compare("neutral") == 0)
			{
				bgColor = ofColor::grey;
			}
			else if (emotion.compare("sadness") == 0)
			{
				bgColor = ofColor::blue;
			}
			else if (emotion.compare("surprise") == 0)
			{
				bgColor = ofColor::green;
			}
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	});
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
