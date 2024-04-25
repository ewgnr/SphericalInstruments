#pragma once

#include "ofMain.h"
#include "ofxDatGui.h"
#include "ofxMidi.h"
#include "uvSphere.h"
#include "modalFilterRouting.h"
#include "modalFilter.h"
#include "oscillators.h"
#include "analysis.h"
#include "phasor.h"
#include "windowing.h"
#include "noise.h"
#include "lowPassFilter.h"
#include "metro.h"
#include "fileIO.h"
#include "highPassFilter.h"
#include "dab_osc_receiver.h"


class ofApp : public ofBaseApp, public ofxMidiListener, public dab::OscListener
{
	public:
		void setup();
		void audioSetup();
		void setupOsc() throw (dab::Exception);
		void notify(std::shared_ptr<dab::OscMessage> pMessage);
		void updateOsc();
		void updateOsc(std::shared_ptr<dab::OscMessage> pMessage);
		void detectCollisions(const std::vector<glm::vec3>& pJointPositions);
		void drawAudioTrigger(std::vector<std::size_t> pCollisionIndices);
		void triggerAudio(std::vector<std::size_t> pCollisionIndices, double pAudioEFreqLowerBound, double pAudioEFreqUpperBound);
		void update();
		void draw();
		void audioOut(ofSoundBuffer& buffer);
		void keyPressed(int key);
		void onSliderEvent(ofxDatGuiSliderEvent e);
		void onToggleEvent(ofxDatGuiToggleEvent e);
		void onButtonEvent(ofxDatGuiButtonEvent e);
		void onTextInputEvent(ofxDatGuiTextInputEvent e);
		void saveGuiValuesAsFile(const std::string& pGuiSelectPresetNumber);
		void readGuiValuesFromFile(const std::string& pGuiSelectPresetNumber);
		void newMidiMessage(ofxMidiMessage& eventArgs);
		void sendMidiMessage();
		void exit();

		ofxDatGui* mGui;

		float EXITER_ENV_RANGE = 10.0f;
		float FREQ_UPPER_BOUND = 0.001f;
		float FREQ_LOWER_BOUND = 0.001f;
		float EXITER_AMPLITUDE = 0.001f;
		float STIFFNESS_LAYER1 = 10.0f;
		float DEVIATION_LAYER1 = 0.01f;
		float AMPLITUDE_LAYER1 = 0.0f;
		float STIFFNESS_LAYER2 = 10.0f;
		float DEVIATION_LAYER2 = 0.5f;
		float SPREADING_LAYER2 = 20.0f;
		float AMPLITUDE_LAYER2 = 0.1f;
		float AMPLITUDE_EXITER = 0.5f;
		float SCALING_FACTOR = 0.3f;
		float MIN_COL_DISTANCE = 50;

		std::size_t ROWS = 10;
		std::size_t COLS = 10;

		bool mGuiUnlockPresets, mGuiReadOrWritePreset;
		std::string mGuiSelectPresetNumber;

		std::vector<string> mGuiFaderNames =
		{
			"EXITER_ENV_RANGE",
			"FREQ_UPPER_BOUND",
			"FREQ_LOWER_BOUND",
			"EXITER_AMPLITUDE",
			"STIFFNESS_LAYER1",
			"DEVIATION_LAYER1",
			"AMPLITUDE_LAYER1",
			"STIFFNESS_LAYER2",
			"DEVIATION_LAYER2",
			"SPREADING_LAYER2",
			"AMPLITUDE_LAYER2",
			"AMPLITUDE_EXITER",
			"SCALING_FACTOR",
			"MIN_COL_DISTANCE"
		};

		fileIO mGuiFileIO;

		uvSphere mVisSphere;
		ofEasyCam mVisCam;

		bool mVisDrawVerticies = true, mVisDrawMesh = true, mVisDrawIndicies = true, mVisDrawDistances = true;
		bool mVisDrawRawPickupPos = true, mVisDrawExitingPos = true;
		
		modalFilterRouting mVisModalRouting;
		std::vector<size_t> mAudioRawPickUpPos = {
								  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
								  30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
								  50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
								  70, 71, 72, 73, 74, 75, 76, 77, 78, 79
		};

		double mScalingFactor = 0.3321;

		ofSoundStream soundStream;

		std::vector<std::shared_ptr<std::size_t>> mAudioIndex;
		std::vector<std::shared_ptr<phasor>> mAudioPhasor;
		std::vector<std::shared_ptr<hanningWindow>> mAudioWindow;
		std::vector<std::shared_ptr<noiseGenerator>> mAudioNoises;
		std::vector<std::shared_ptr<loPassFilter>> mAudioLoPass;

		highPassFilter mAudioHighPass;
		
		std::array<modalFilter, 90> mAudioModes1, mAudioModes2;
		std::array<double, 90> mAudioLConnection, mAudioRConnection, mAudioUConnection, mAudioDConnection; 
		std::array<double, 90> mAudioRawConnections, mAudioWeightedConnections, mAudioExiter, mAudioResonateOutput, mAudioSecondLayer;
		double mAudioOutput, mAudioSelectedExiterOut;

		bool mAudioTrigger = false;
		unsigned int mAudioSoundStreamOnOffToggle = 0;

		std::atomic<bool> mAudioThreadSync = { false };

		dab::OscReceiver* mOscReceiver;
		unsigned int mMaxOscMessageQueueLength = 1000;
		std::deque< std::shared_ptr<dab::OscMessage> > mOscMessageQueue;

		std::shared_ptr<ofApp> mSelf;
		std::mutex mOscLock;

		std::vector<glm::vec3> mJointPositions = std::vector<glm::vec3>(32, glm::vec3(0, 0, 0));

		std::vector<std::size_t> mCollisionIndices;
		std::vector<std::size_t> mThreadedCollisionIndices;
		std::vector<std::size_t> mCollisionIndicesBefore;
		std::vector<std::size_t> mColChangesBefore;

		std::vector<std::size_t> set_difference(std::vector<std::size_t> const& pVector_1, std::vector<std::size_t> const& pVector_2)
		{
			std::vector <size_t> diff;
			std::set_difference(std::begin(pVector_1), std::end(pVector_1), std::begin(pVector_2), std::end(pVector_2), std::back_inserter(diff));
			return diff;
		}

		ofxMidiIn midiIn;
		ofxMidiOut midiOut;

		void midiRouteReceive(int pMidiSelGuiSlider, int pMidiSelChannel, int pMidiSelCtrl, float pMidiMaxValue, int pMidiMsgChnl, int pMidiMsgCtrl, int pMidiMsgVal)
		{
			if (pMidiMsgChnl == pMidiSelChannel && pMidiSelCtrl == pMidiMsgCtrl)
			{
				mGui->getSlider(mGuiFaderNames[pMidiSelGuiSlider])->setValue(pMidiMaxValue * float(pMidiMsgVal / 127.0f));
			}
		}

		void midiRouteSend(int pMidiSelGuiSlider, int pMidiSelChannel, int pMidiSelCtrl, float pMidiMaxValue)
		{
			midiOut.sendControlChange(pMidiSelChannel, pMidiSelCtrl, (mGui->getSlider(mGuiFaderNames[pMidiSelGuiSlider])->getValue() / pMidiMaxValue) * 127);
		}

		std::size_t colIndexTestCount = -1;
};
