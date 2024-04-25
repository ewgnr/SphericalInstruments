#pragma once

#include "ofMain.h"
#include "uvSphere.h"
#include "sfPlayer.h"
#include "metro.h"
#include "dab_osc_receiver.h"
#include "dab_osc_sender.h"
#include <memory>
#include "fileIO.h"
#include "ofxDatGui.h"

class ofApp : public ofBaseApp, public dab::OscListener
{
	public:
		void setup();
		void audioSetup();
		void setupOsc() throw (dab::Exception);
		void notify(std::shared_ptr<dab::OscMessage> pMessage);
		void updateOsc();
		void updateOsc(std::shared_ptr<dab::OscMessage> pMessage);
		void updateOscSender() throw (dab::Exception);
		void detectCollisions(const std::vector<glm::vec3>& pJointPositions);
		void drawAudioTrigger(std::vector<std::size_t> pCollisionIndices);
		void triggerAudio(std::vector<std::size_t> pCollisionIndices);
		void update();
		void draw();
		void audioOut(ofSoundBuffer& outBuffer);
		void writeFileSetup(const std::string& pSfNumber);
		void writeFileToBuffer(const double& pInput);
		void writeFileToDisk(const std::vector<double>& pSampleBuffer);
		void keyPressed(int key);
		void onSliderEvent(ofxDatGuiSliderEvent e);
		void onToggleEvent(ofxDatGuiToggleEvent e);
		void onButtonEvent(ofxDatGuiButtonEvent e);
		void onTextInputEvent(ofxDatGuiTextInputEvent e);
		void saveGuiValuesAsFile(const std::string& pGuiSelectPresetNumber);
		void readGuiValuesFromFile(const std::string& pGuiSelectPresetNumber);
		void exit();

		ofxDatGui* mGui;

		// RETRIGGER_AT_INDEX works with RETRIGGER_AT_END or CONTINOUS_TRIGGER
		bool RETRIGGER_AT_INDEX = false;
		bool RETRIGGER_AT_END = false;
		bool CONTINOUS_TRIGGER = false;

		bool METRONOME_TRIGGER = true;
		double METRO_FREQUENCY = 100;
		double METRO_RAND_FREQ = 0;

		float PITCH_RAND_MIN = 0.5;
		float PITCH_RAND_MAX = 1.0;

		float SAMPLE_POS_START = 0.0;
		float SAMPLE_POS_END = 0.9;

		float MIN_COL_DISTANCE = 50;

		std::size_t ROWS = 10;
		std::size_t COLS = 10;
		std::size_t NUMBER_OF_SOUNDFILES = 291;

		bool mGuiUnlockPresets, mGuiReadOrWritePreset;
		std::string mGuiSelectPresetNumber;

		std::vector<string> mGuiFaderNames =
		{
			"METRO_FREQUENCY",
			"METRO_RAND_FREQ",
			"PITCH_RAND_MIN",
			"PITCH_RAND_MAX",
			"SAMPLE_POS_START",
			"SAMPLE_POS_END",
			"MIN_COL_DISTANCE"
		};

		fileIO mGuiFileIO;

		uvSphere mVisSphere;
		ofEasyCam mVisCam;

		bool drawVerticies = true, drawMesh = true, drawIndicies = false, drawExitingPos = true;

		std::vector<std::string> fileNames;

		std::vector<std::string> directoryIterator(std::string pPath)
		{
			cout << pPath << endl;
			auto dirIter = std::filesystem::directory_iterator(pPath);

			for (auto& entry : dirIter)
			{
				std::cout << "entry path: " << entry.path().string() << std::endl;
				fileNames.push_back(entry.path().filename().string());
			}

			std::sort(fileNames.begin(), fileNames.end());

			return fileNames;
		}

		ofSoundStream soundStream;

		sfPlayer* mAudioPlayers = new sfPlayer[NUMBER_OF_SOUNDFILES];
		metro* mAudioPlayerMetros = new metro[NUMBER_OF_SOUNDFILES];
		
		metro mMetro;

		double output;

		SndfileHandle soundFileBuffer;
		std::vector<double> writeFileSampleBuffer;

		bool mAudioTrigger = false;
		unsigned int mAudioSoundStreamOnOffToggle = 0;

		std::atomic<bool> mAudioThreadSync = { false };

		dab::OscSender* mOscSender;
		std::string mOscSendAddress;
		int mOscSendPort;

		dab::OscReceiver* mOscReceiver;
		unsigned int mMaxOscMessageQueueLength = 1000;
		std::deque< std::shared_ptr<dab::OscMessage> > mOscMessageQueue;

		std::shared_ptr<ofApp> mSelf;
		std::mutex mOscLock;

		std::vector<glm::vec3> mJointPositions = std::vector<glm::vec3>(32, glm::vec3(0, 0, 0));

		std::size_t exitingIndex;
		std::vector<std::size_t> mCollisionIndices;
		std::vector<std::size_t> mThreadedCollisionIndices;
		std::vector<std::size_t> mCollisionIndicesBefore;

		std::vector<std::size_t> set_difference(std::vector<std::size_t> const& pVector_1, std::vector<std::size_t> const& pVector_2)
		{
			std::vector <size_t> diff;
			std::set_difference(std::begin(pVector_1), std::end(pVector_1), std::begin(pVector_2), std::end(pVector_2), std::back_inserter(diff));
			return diff;
		}

		std::size_t colIndexTestCount = -1;
};
