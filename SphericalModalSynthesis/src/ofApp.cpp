#include "ofApp.h"
#include "audioProperties.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetVerticalSync(true);

	ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL);

	// camera setup
	mVisCam.setPosition(ofGetWidth() * 0.7, ofGetHeight() * 0.5, ofGetHeight());
	glm::vec3 targetPosition = { ofGetWidth() * 0.5, ofGetHeight() * 0.5, 0 };
	mVisCam.setTarget(targetPosition);
	mVisCam.removeAllInteractions();
	mVisCam.addInteraction(ofEasyCam::TRANSFORM_ROTATE, OF_MOUSE_BUTTON_RIGHT);

	// sphere setup
	mVisSphere.setup(ROWS, COLS, 100);
	mVisSphere.verticies();
	mVisSphere.mesh();
	mVisSphere.distance();

	std::cout << "indiciesString " << mVisSphere.indiciesString.size() << std::endl;
	std::cout << "refPoint " << mVisSphere.ref_vert_pnt.size() << std::endl;

	mVisModalRouting.getParams(mAudioRawPickUpPos, mVisSphere.ref_vert_pnt);

	// osc setup
	setupOsc();

	// gui setup
	// Set Height in ofxDatGuiTheme.h Line 174
	mGui = new ofxDatGui(ofxDatGuiAnchor::TOP_LEFT);
	mGui->addSlider(mGuiFaderNames[0],  0.01,   100,  50.0  )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[1],  0.0001, 1.0,  0.001 )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[2],  0.0001, 1.0,  0.001 )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[3],  0.0001, 0.01, 0.001 )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[4],  1.0,    40.0, 10.0  )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[5],  0.01,   1.0,  0.01  )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[6],  0.0,    1.0,  0.0   )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[7],  1.0,    20.0, 10.0  )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[8],  0.01,   1.0,  0.5   )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[9],  0.01,   20.0, 20.0  )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[10], 0.0,    1.0,  0.1   )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[11], 0.0,    1.0,  0.5   )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[12], 0.01,   1.0,  0.3   )->setPrecision(7);
	mGui->addSlider(mGuiFaderNames[13], 1.0,    50.0, 50.0  )->setPrecision(7);
	mGui->onSliderEvent(this, &ofApp::onSliderEvent);

	mGui->addToggle("UNLOCK_PRESET");
	mGui->onToggleEvent(this, &ofApp::onToggleEvent);

	mGui->addButton("READ_WRITE_PRST");
	mGui->onButtonEvent(this, &ofApp::onButtonEvent);

	mGui->addTextInput("SEL_PRST_NR");
	mGui->onTextInputEvent(this, &ofApp::onTextInputEvent);
	
	// midi setup
	midiIn.listInPorts();
	midiIn.openPort(0);
	midiIn.ignoreTypes(false, false, false);
	midiIn.addListener(this);
	midiIn.setVerbose(true);

	midiOut.listOutPorts();
	midiOut.openPort(1);
	sendMidiMessage();
}

//--------------------------------------------------------------
void ofApp::audioSetup()
{
	ofSoundStreamSettings settings;
	settings.setApi(ofSoundDevice::Api::MS_ASIO);
	auto devices = soundStream.getDeviceList(ofSoundDevice::Api::MS_ASIO);
	for (int i = 0; i < devices.size(); ++i) 
	{
		std::cout << devices[i] << "\n";
	}

	settings.setOutDevice(devices[0]);
	settings.setInDevice(devices[0]);
	settings.setOutListener(this);
	settings.numOutputChannels = 1;
	settings.sampleRate = sampleRate;
	settings.bufferSize = bufferSize;
	settings.numBuffers = 4;
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::setupOsc()
{
	try
	{
		mSelf = std::shared_ptr<ofApp>(this);
		mOscReceiver = new dab::OscReceiver("MocapReceiver", 9003);
		mOscReceiver->registerOscListener(std::weak_ptr<ofApp>(mSelf));
		mOscReceiver->start();
	}
	catch (dab::Exception& e)
	{
		std::cout << e << "\n";
	}
}

//--------------------------------------------------------------
void ofApp::notify(std::shared_ptr<dab::OscMessage> pMessage)
{
	mOscLock.lock();

	mOscMessageQueue.push_back(pMessage);
	if (mOscMessageQueue.size() > mMaxOscMessageQueueLength) mOscMessageQueue.pop_front();

	mOscLock.unlock();
}

//--------------------------------------------------------------
void ofApp::updateOsc()
{
	mOscLock.lock();

	while (mOscMessageQueue.size() > 0)
	{
		std::shared_ptr< dab::OscMessage > oscMessage = mOscMessageQueue[0];

		updateOsc(oscMessage);

		mOscMessageQueue.pop_front();
	}

	mOscLock.unlock();
}

//--------------------------------------------------------------
void ofApp::updateOsc(std::shared_ptr<dab::OscMessage> pMessage)
{
	try
	{
		std::string address = pMessage->address();

		const std::vector<dab::_OscArg*>& arguments = pMessage->arguments();

		if (address == "/mocap/pos")
		{
			size_t jointCount = mJointPositions.size();
			size_t argCount = arguments.size();

			jointCount = std::min(jointCount, argCount / 3);

			for (int jI = 0, aI = 0; jI < jointCount; ++jI, aI += 3)
			{
				mJointPositions[jI].x = *arguments[aI];
				mJointPositions[jI].y = *arguments[aI + 1];
				mJointPositions[jI].z = *arguments[aI + 2];
			}
		}
	}
	catch (dab::Exception& e)
	{
		std::cout << e << "\n";
	}
}

//--------------------------------------------------------------
void ofApp::detectCollisions(const std::vector<glm::vec3>& pJointPositions)
{
	mCollisionIndices.clear();

	int refPointCount = mVisSphere.ref_vert_pnt.size();
	float minColDistance = MIN_COL_DISTANCE;

	for (int iPI = 0; iPI < refPointCount; ++iPI)
	{
		const glm::vec3& instrPoint = mVisSphere.ref_vert_pnt[iPI];

		for (auto jointPoint : pJointPositions)
		{
			float dist = glm::length(jointPoint - instrPoint);

			if (dist <= minColDistance)
			{
				mCollisionIndices.push_back(iPI);
			}
		}
	}
}

void ofApp::triggerAudio(std::vector<std::size_t> pCollisionIndices, double pAudioEFreqLowerBound, double pAudioEFreqUpperBound)
{
	std::sort(pCollisionIndices.begin(), pCollisionIndices.end());
	std::sort(mCollisionIndicesBefore.begin(), mCollisionIndicesBefore.end());

	std::vector<std::size_t> intersection;
	std::set_intersection(std::begin(pCollisionIndices), std::end(pCollisionIndices), std::begin(mCollisionIndicesBefore), std::end(mCollisionIndicesBefore), std::back_inserter(intersection));

	std::vector colChanges = set_difference(mCollisionIndicesBefore, intersection);

	for (std::size_t colChangeIndex : colChanges)
	{
		mAudioIndex.push_back(std::make_shared<std::size_t>(colChangeIndex));
		mAudioPhasor.push_back(std::make_shared<phasor>((double)EXITER_ENV_RANGE, 0, 1));
		mAudioWindow.push_back(std::make_shared<hanningWindow>());
		mAudioNoises.push_back(std::make_shared<noiseGenerator>());
		mAudioLoPass.push_back(std::make_shared<loPassFilter>((double)ofRandom(0, 1)));
	}

	mCollisionIndicesBefore.resize(pCollisionIndices.size());
	std::copy_n(pCollisionIndices.begin(), pCollisionIndices.size(), mCollisionIndicesBefore.begin());
}

//--------------------------------------------------------------
void ofApp::drawAudioTrigger(std::vector<std::size_t> pCollisionIndices)
{
	for (auto colIndex = pCollisionIndices.rbegin(); colIndex != pCollisionIndices.rend(); ++colIndex)
	{
		mVisSphere.drawExitingPosition(*colIndex, ofColor(255));
	}
}

//--------------------------------------------------------------
void ofApp::update()
{
	// std::cout << mAudioIndex.size() << std::endl;
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofEnableDepthTest();

	mVisCam.begin();

	ofPushMatrix();
	ofTranslate(ofGetWidth() * 0.5, ofGetHeight() * 0.5, 0);
	// Sphere
	if (mVisDrawVerticies) mVisSphere.drawVerticies(ofColor(255, 0, 0));
	if (mVisDrawMesh) mVisSphere.drawMesh(ofColor(0));
	if (mVisDrawIndicies) mVisSphere.drawIndicies(ofColor(255));
	if (mVisDrawDistances) mVisSphere.drawDistances(ofColor(125));
	
	if (mAudioThreadSync.load(memory_order_acquire))
	{
		mThreadedCollisionIndices = mCollisionIndices;
		mAudioThreadSync.store(false, memory_order_release);
	}

	if (mVisDrawExitingPos) drawAudioTrigger(mThreadedCollisionIndices);
	
	// Instrument Parameter
	if (mVisDrawRawPickupPos) mVisModalRouting.drawRawPickUpPos(ofColor(0, 255, 0));

	// Draw keyInput trigger for debugging
	mVisSphere.drawExitingPosition(colIndexTestCount, ofColor(255));

	ofPopMatrix();

	ofDisableDepthTest();

	mVisCam.end();
	mGui->draw();
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& buffer)
{
	double audioEFreqUpperBound = (double)FREQ_UPPER_BOUND;
	double audioEFreqLowerBound = (double)FREQ_LOWER_BOUND;
	double audioExiterAmplitude = (double)EXITER_AMPLITUDE;
	double audioStiffnessLayer1 = (double)STIFFNESS_LAYER1;
	double audioDeviationLayer1 = (double)DEVIATION_LAYER1;
	double audioAmplitudeLayer1 = (double)AMPLITUDE_LAYER1;
	double audioStiffnessLayer2 = (double)STIFFNESS_LAYER2;
	double audioDeviationLayer2 = (double)DEVIATION_LAYER2;
	double audioSpreadValLayer2 = (double)SPREADING_LAYER2;
	double audioAmplitudeLayer2 = (double)AMPLITUDE_LAYER2;
	double audioAmplitudeExiter = (double)AMPLITUDE_EXITER;

	double sphereScalingFactor = (double)SCALING_FACTOR;
	double mAudioFundamental = (1 - sphereScalingFactor) * 200.0;

	updateOsc();
	detectCollisions(mJointPositions);
	if (mAudioTrigger) triggerAudio(mCollisionIndices, 0, 1);

	for (size_t i = 1; i < mVisSphere.ref_vert_pnt.size() - 2; ++i)
	{
		mAudioModes1[i].init((double)mAudioFundamental * 1.0 + (int)ofRandom(0, audioDeviationLayer1), mAudioFundamental * audioStiffnessLayer1, 1.0);
		mAudioModes2[i].init((double)mAudioFundamental * 2.0 + (int)ofRandom(0, audioDeviationLayer2), mAudioFundamental * audioStiffnessLayer2, 1.0);
	}

	for (size_t bI = 0; bI < buffer.size(); ++bI)
	{
		if (mAudioIndex.size() > 0)
		{
			for (int cS = mAudioIndex.size() - 1; cS >= 0; --cS)
			{
				mAudioExiter[*mAudioIndex[cS]] = mAudioLoPass[cS]->play(mAudioNoises[cS]->play()) * mAudioWindow[cS]->play(sampleRate, mAudioPhasor[cS]->play() * sampleRate) * 0.001;
				
				if (mAudioPhasor[cS]->phase >= 1 && mAudioPhasor.size() != 0)
				{
					mAudioIndex.erase(mAudioIndex.begin() + cS);
					mAudioPhasor.erase(mAudioPhasor.begin() + cS);
					mAudioWindow.erase(mAudioWindow.begin() + cS);
					mAudioNoises.erase(mAudioNoises.begin() + cS);
					mAudioLoPass.erase(mAudioLoPass.begin() + cS);
				}
			}
		}
		
		for (size_t mI = 1; mI < mVisSphere.ref_vert_pnt.size() - 2; ++mI)
		{
			size_t nbIndex_1 = mI < 1 ? 1 : mI;
			nbIndex_1 = nbIndex_1 > mVisSphere.euclidianDistSlices.size() - 2 ? mVisSphere.euclidianDistSlices.size() - 2 : nbIndex_1;

			size_t nbIndex_2 = mI < 10 ? 10 : mI;
			nbIndex_2 = nbIndex_2 > mVisSphere.euclidianDistStacks.size() - 12 ? mVisSphere.euclidianDistStacks.size() - 12 : nbIndex_2;
			
			mAudioRawConnections[mI] = mAudioModes1[mI].play(mAudioExiter[mI]);
			mAudioResonateOutput[mI] = mAudioModes2[mI].play(mAudioHighPass.play(mAudioWeightedConnections[mI], 0.9) * audioSpreadValLayer2);

			mAudioLConnection[mI] = mAudioRawConnections[nbIndex_2 - 10] * mVisSphere.euclidianDistStacks[nbIndex_2 - 1];
			mAudioRConnection[mI] = mAudioRawConnections[nbIndex_2 + 10] * mVisSphere.euclidianDistStacks[nbIndex_2 + 1];
			mAudioUConnection[mI] = mAudioRawConnections[nbIndex_1 - 1] * mVisSphere.euclidianDistSlices[nbIndex_1 - 1];
			mAudioDConnection[mI] = mAudioRawConnections[nbIndex_1 + 1] * mVisSphere.euclidianDistSlices[nbIndex_1 + 1];

			mAudioWeightedConnections[mI] = ((mAudioLConnection[mI] + mAudioRConnection[mI] + mAudioUConnection[mI] + mAudioDConnection[mI]) * 0.25) / (double(mVisSphere.radius) * 2.0);

			for (size_t i = 0; i < mAudioRawPickUpPos.size(); ++i)
			{
				mAudioSelectedExiterOut += mAudioExiter[mAudioRawPickUpPos[i]];
			}

			mAudioOutput += ((mAudioRawConnections[mI] * audioAmplitudeLayer1 * 20.0) + (mAudioResonateOutput[mI] * audioAmplitudeLayer2 * 10.0) + (mAudioSelectedExiterOut * audioAmplitudeExiter * 0.1));
			// mAudioOutput += mAudioExiter[mI];
			// mAudioOutput += mAudioSelectedExiterOut * audioAmplitudeExiter;
			// mAudioOutput += mAudioRawConnections[mI] * 2;
		}

		buffer[bI] = (float)mAudioOutput;
		
		mAudioOutput = mAudioSelectedExiterOut = 0;

		mAudioThreadSync.store(true, memory_order_release);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	// sphere
	if (key == 'y') mVisDrawVerticies = !mVisDrawVerticies;
	if (key == 'x') mVisDrawMesh = !mVisDrawMesh;
	if (key == 'c') mVisDrawIndicies = !mVisDrawIndicies;
	if (key == 'v') mVisDrawDistances = !mVisDrawDistances;
	if (key == 'b') mVisDrawExitingPos = !mVisDrawExitingPos;
	if (key == 'n') mVisDrawRawPickupPos = !mVisDrawRawPickupPos;
	// audio parameter
	if (key == 't') mAudioTrigger = !mAudioTrigger;
	if (key == 'o')
	{
		mAudioSoundStreamOnOffToggle = (mAudioSoundStreamOnOffToggle + 1) % 2;
		if (mAudioSoundStreamOnOffToggle == 0)
		{
			soundStream.close();
			std::cout << "audio engine close" << std::endl;
		}
		if (mAudioSoundStreamOnOffToggle == 1)
		{
			audioSetup();
			std::cout << "audio engine start" << std::endl;
		}
	}
	// test
	if (key == ' ')
	{
		colIndexTestCount = (colIndexTestCount += 1) % mVisSphere.ref_vert_pnt.size();
	}
}

void ofApp::onSliderEvent(ofxDatGuiSliderEvent e)
{
	if (e.target->is(mGuiFaderNames[0]))  EXITER_ENV_RANGE = e.value;
	if (e.target->is(mGuiFaderNames[1]))  FREQ_UPPER_BOUND = e.value;
	if (e.target->is(mGuiFaderNames[2]))  FREQ_LOWER_BOUND = e.value;
	if (e.target->is(mGuiFaderNames[3]))  EXITER_AMPLITUDE = e.value;
	if (e.target->is(mGuiFaderNames[4]))  STIFFNESS_LAYER1 = e.value;
	if (e.target->is(mGuiFaderNames[5]))  DEVIATION_LAYER1 = e.value;
	if (e.target->is(mGuiFaderNames[6]))  AMPLITUDE_LAYER1 = e.value;
	if (e.target->is(mGuiFaderNames[7]))  STIFFNESS_LAYER2 = e.value;
	if (e.target->is(mGuiFaderNames[8]))  DEVIATION_LAYER2 = e.value;
	if (e.target->is(mGuiFaderNames[9]))  SPREADING_LAYER2 = e.value;
	if (e.target->is(mGuiFaderNames[10])) AMPLITUDE_LAYER2 = e.value;
	if (e.target->is(mGuiFaderNames[11])) AMPLITUDE_EXITER = e.value;
	if (e.target->is(mGuiFaderNames[12])) SCALING_FACTOR   = e.value;
	if (e.target->is(mGuiFaderNames[13])) MIN_COL_DISTANCE = e.value;
	
	sendMidiMessage();
}

void ofApp::onToggleEvent(ofxDatGuiToggleEvent e)
{
	if (e.target->is("UNLOCK_PRESET")) mGuiUnlockPresets = e.checked;
}

void ofApp::onTextInputEvent(ofxDatGuiTextInputEvent e)
{
	if (e.target->is("SEL_PRST_NR")) mGuiSelectPresetNumber = e.text;
}

void ofApp::onButtonEvent(ofxDatGuiButtonEvent e)
{
	if (e.target->is("READ_WRITE_PRST")) mGuiReadOrWritePreset = e.target;

	if (mGuiUnlockPresets && mGuiReadOrWritePreset)
	{
		saveGuiValuesAsFile(mGuiSelectPresetNumber);
	}
	else if (!mGuiUnlockPresets && mGuiReadOrWritePreset)
	{
		readGuiValuesFromFile(mGuiSelectPresetNumber);
	}
}

void ofApp::saveGuiValuesAsFile(const std::string& pGuiSelectPresetNumber)
{
	std::ostringstream guiValues;

	for (int i = 0; i < mGuiFaderNames.size(); ++i)
	{
		std::cout << mGuiFaderNames[i] << " " << mGui->getSlider(mGuiFaderNames[i])->getValue() << " " << std::endl;
		guiValues << mGuiFaderNames[i] << " " << mGui->getSlider(mGuiFaderNames[i])->getValue() << " " << std::endl;
	}

	std::string guiWriteFileName = "preset_number_" + pGuiSelectPresetNumber;
	mGuiFileIO.write(guiValues.str(), ofToDataPath(guiWriteFileName));
}

void ofApp::readGuiValuesFromFile(const std::string& pGuiSelectPresetNumber)
{
	std::string fileValues = {};

	std::string guiReadFileName = "preset_number_" + pGuiSelectPresetNumber;
	mGuiFileIO.read(ofToDataPath(guiReadFileName), fileValues);

	const char delimiter = ' ';
	std::vector<std::string> outputArray;
	std::istringstream streamData(fileValues);
	std::string token;

	while (std::getline(streamData, token, delimiter))
	{
		token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
		outputArray.push_back(token);
	}
	
	for (int i = 0; i < outputArray.size() - 1; i += 2)
	{
		mGui->getSlider(outputArray[i])->setValue(std::stof(outputArray[i + 1]));
	}	
}

//--------------------------------------------------------------

void ofApp::newMidiMessage(ofxMidiMessage& pMsg) 
{
	if (pMsg.status == MIDI_CONTROL_CHANGE)
	{
		std::cout << "channel " << pMsg.channel << std::endl;
		std::cout << "msg ctrl " << pMsg.control << std::endl;
		std::cout << "msg value " << pMsg.value << std::endl;
	}
	midiRouteReceive(0,  1,  7, 10.0f,  pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(1,  2,  7, 100.0f, pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(2,  3,  7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(3,  4,  7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(4,  5,  7, 0.01f,  pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(5,  6,  7, 40.0f,  pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(6,  7,  7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(7,  8,  7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(8,  9,  7, 20.0f,  pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(9,  10, 7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(10, 11, 7, 20.0f,  pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(11, 12, 7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(12, 13, 7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
	midiRouteReceive(13, 14, 7, 1.0f,   pMsg.channel, pMsg.control, pMsg.value);
}

void ofApp::sendMidiMessage()
{
	midiRouteSend(0,  1, 7, 10.0f);
	midiRouteSend(1,  2, 7, 100.0f);
	midiRouteSend(2,  3, 7, 1.0f);
	midiRouteSend(3,  4, 7, 1.0f);
	midiRouteSend(4,  5, 7, 0.01f);
	midiRouteSend(5,  6, 7, 40.0f);
	midiRouteSend(6,  7, 7, 1.0f);
	midiRouteSend(7,  8, 7, 1.0f);
	midiRouteSend(8,  9, 7, 20.0f);
	midiRouteSend(9,  10, 7, 1.0f);
	midiRouteSend(10, 11, 7, 20.0f);
	midiRouteSend(11, 12, 7, 1.0f);
	midiRouteSend(12, 13, 7, 1.0f);
	midiRouteSend(13, 14, 7, 1.0f);
}

//--------------------------------------------------------------
void ofApp::exit() 
{
	// clean up
	midiIn.closePort();
	midiIn.removeListener(this);
}
