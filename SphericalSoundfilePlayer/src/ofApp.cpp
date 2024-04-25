#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() 
{    
    // load audio files
    std::vector<string> tempFilePath;
    tempFilePath = directoryIterator("data/audio/");

    for (size_t sfPlayerIndex = 0; sfPlayerIndex < NUMBER_OF_SOUNDFILES; sfPlayerIndex++)
    {
        mAudioPlayers[sfPlayerIndex].read(ofToDataPath("audio/" + tempFilePath[sfPlayerIndex], true));
    }

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

    std::cout << "indiciesString " << mVisSphere.indiciesString.size() << std::endl;
    std::cout << "refPoint " << mVisSphere.ref_vert_pnt.size() << std::endl;

    // osc setup
    setupOsc();

    // GUI setup
    // Set Height in ofxDatGuiTheme.h Line 174
    mGui = new ofxDatGui(ofxDatGuiAnchor::TOP_LEFT);
    mGui->addSlider(mGuiFaderNames[0], 0.001, 100.0, 1.0)->setPrecision(7);
    mGui->addSlider(mGuiFaderNames[1], 0.001, 100.0, 1.0)->setPrecision(7);
    mGui->addSlider(mGuiFaderNames[2], 0.001, 12.0, 0.5)->setPrecision(7);
    mGui->addSlider(mGuiFaderNames[3], 0.001, 12.0, 1.0)->setPrecision(7);
    mGui->addSlider(mGuiFaderNames[4], 0.0, 0.9, 0.0)->setPrecision(7);
    mGui->addSlider(mGuiFaderNames[5], 0.0, 0.9, 0.9)->setPrecision(7);
    mGui->addSlider(mGuiFaderNames[6], 1.0, 100.0, 50.0)->setPrecision(7);
    mGui->onSliderEvent(this, &ofApp::onSliderEvent);

    mGui->addToggle("UNLOCK_PRESET");
    mGui->onToggleEvent(this, &ofApp::onToggleEvent);

    mGui->addButton("READ_WRITE_PRST");
    mGui->onButtonEvent(this, &ofApp::onButtonEvent);

    mGui->addTextInput("SEL_PRST_NR");
    mGui->onTextInputEvent(this, &ofApp::onTextInputEvent);

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
    settings.numOutputChannels = 1;
    settings.numInputChannels = 2;
    settings.sampleRate = 48000;
    settings.bufferSize = 1024;
    settings.numBuffers = 4;
    settings.setOutListener(this);
    settings.setInListener(this);
    soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::setupOsc()
{
    try
    {
        mSelf = std::shared_ptr<ofApp>(this);
        mOscReceiver = new dab::OscReceiver("MocapReceiver", 23456);
        mOscReceiver->registerOscListener(std::weak_ptr<ofApp>(mSelf));
        mOscReceiver->start();

        mOscSendAddress = "127.0.0.1";
        mOscSendPort = 9003;
        mOscSender = new dab::OscSender("MocapSender", mOscSendAddress, mOscSendPort);
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

        if (address == "/mocap/joint/pos")
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
void ofApp::updateOscSender() throw (dab::Exception)
{
    try
    {
        // send joint positions
        {
            std::string messageAddress = "/mocap/pos";
            std::shared_ptr<dab::OscMessage> message(new dab::OscMessage(messageAddress));

            for (int pI = 0; pI < mJointPositions.size(); ++pI)
            {
                message->add(mJointPositions[pI].x);
                message->add(mJointPositions[pI].y);
                message->add(mJointPositions[pI].z);
            }

            mOscSender->send(message);
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

//--------------------------------------------------------------
void ofApp::triggerAudio(std::vector<std::size_t> pCollisionIndices)
{
    float playbackSpeed = ofRandom(PITCH_RAND_MIN, PITCH_RAND_MAX);

    if (RETRIGGER_AT_INDEX)
    {
        std::sort(pCollisionIndices.begin(), pCollisionIndices.end());
        std::sort(mCollisionIndicesBefore.begin(), mCollisionIndicesBefore.end());

        std::vector<std::size_t> intersection;
        std::set_intersection(std::begin(pCollisionIndices), std::end(pCollisionIndices), std::begin(mCollisionIndicesBefore), std::end(mCollisionIndicesBefore), std::back_inserter(intersection));

        std::vector colChanges = set_difference(mCollisionIndicesBefore, intersection);
        
        for (std::size_t colChangeIndex : colChanges)
        {
            if (RETRIGGER_AT_END)
            {
                if (mAudioPlayers[colChangeIndex].isPlaying() == false)
                {
                    mAudioPlayers[colChangeIndex].start(playbackSpeed, SAMPLE_POS_START, SAMPLE_POS_END);
                }
            }

            if (CONTINOUS_TRIGGER)
            {
                mAudioPlayers[colChangeIndex].start(playbackSpeed, SAMPLE_POS_START, SAMPLE_POS_END);
            }
        }

        mCollisionIndicesBefore.resize(pCollisionIndices.size());
        std::copy_n(pCollisionIndices.begin(), pCollisionIndices.size(), mCollisionIndicesBefore.begin());
    }

    if (!RETRIGGER_AT_INDEX)
    {
        for (std::size_t colIndex : pCollisionIndices)
        {
            if (RETRIGGER_AT_END)
            {
                if (mAudioPlayers[colIndex].isPlaying() == false)
                {
                    mAudioPlayers[colIndex].start(playbackSpeed, SAMPLE_POS_START, SAMPLE_POS_END);
                }
            }

            if (CONTINOUS_TRIGGER)
            {
                mAudioPlayers[colIndex].start(playbackSpeed, SAMPLE_POS_START, SAMPLE_POS_END);
            }

            if (METRONOME_TRIGGER)
            {
                if (mAudioPlayerMetros[colIndex].play(METRO_FREQUENCY + (ofRandom(0, 1) * 2 - 1) * METRO_RAND_FREQ) == true)
                {
                    mAudioPlayers[colIndex].start(playbackSpeed, SAMPLE_POS_START, SAMPLE_POS_END);
                }
            }
        }
    }
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
    std::size_t checkForActivePlayers = 0;

    for (size_t sfPlayerIndex = 0; sfPlayerIndex < NUMBER_OF_SOUNDFILES; sfPlayerIndex++)
    {
        if (mAudioPlayers[sfPlayerIndex].isPlaying() == true)
        {
            checkForActivePlayers += 1;
        }
    }
    // std::cout << checkForActivePlayers << std::endl;
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofEnableDepthTest();

    mVisCam.begin();

    ofPushMatrix();

    ofTranslate(ofGetWidth() * 0.5, ofGetHeight() * 0.5, 0);

    if (drawVerticies) mVisSphere.drawVerticies(ofColor(255, 0, 0));
    if (drawMesh) mVisSphere.drawMesh(ofColor(0));
    if (drawIndicies) mVisSphere.drawIndicies(ofColor(255));

    mVisSphere.drawExitingPosition(colIndexTestCount, ofColor(255));

    if (mAudioThreadSync.load(memory_order_acquire))
    {
        mThreadedCollisionIndices = mCollisionIndices;
        mAudioThreadSync.store(false, memory_order_release);
    }
    drawAudioTrigger(mThreadedCollisionIndices);

    ofSetColor(ofColor(0, 255, 0));
    for (auto jointPos : mJointPositions)
    {
        ofDrawSphere(jointPos, 1.0);
    }

    ofPopMatrix();

    ofDisableDepthTest();

    mVisCam.end();

    updateOscSender();
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& outBuffer)
{
    updateOsc();
    detectCollisions(mJointPositions);
    if (mAudioTrigger) triggerAudio(mCollisionIndices);

    for (size_t buffer_count = 0; buffer_count < outBuffer.size(); buffer_count++)
    {
        for (size_t player_count = 0; player_count < NUMBER_OF_SOUNDFILES; player_count++)
        {
            output += (mAudioPlayers[player_count].play() * 0.5);
        }
        outBuffer[buffer_count] = output;
        writeFileToBuffer(outBuffer[buffer_count]);
        output = 0;
    }

    mAudioThreadSync.store(true, memory_order_release);
}

//--------------------------------------------------------------
void ofApp::writeFileSetup(const std::string& pSfName)
{
    soundFileBuffer = SndfileHandle(ofToDataPath(pSfName), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_DOUBLE, 1, 48000);
}

//--------------------------------------------------------------
void ofApp::writeFileToBuffer(const double& pInput)
{
    writeFileSampleBuffer.push_back(pInput);
}

//--------------------------------------------------------------
void ofApp::writeFileToDisk(const std::vector<double>& pSampleBuffer)
{
    soundFileBuffer.write(pSampleBuffer.data(), pSampleBuffer.size());
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    // Sphere
    if (key == 'y') drawVerticies = !drawVerticies;
    if (key == 'x') drawMesh = !drawMesh;
    if (key == 'c') drawIndicies = !drawIndicies;
    if (key == 'v') drawExitingPos = !drawExitingPos;
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

    if (key == 'r') writeFileSetup("audioTest.wav");;
    if (key == 'w') writeFileToDisk(writeFileSampleBuffer);
    // test
    if (key == ' ')
    {
        colIndexTestCount = (colIndexTestCount += 1) % mVisSphere.ref_vert_pnt.size();
        mVisSphere.drawExitingPosition(colIndexTestCount, ofColor(255));
        mAudioPlayers[colIndexTestCount].start(ofRandom(PITCH_RAND_MIN, PITCH_RAND_MAX), SAMPLE_POS_START, SAMPLE_POS_END);
    }
}

//--------------------------------------------------------------
void ofApp::onSliderEvent(ofxDatGuiSliderEvent e)
{
    if (e.target->is(mGuiFaderNames[0])) METRO_FREQUENCY = e.value;
    if (e.target->is(mGuiFaderNames[1])) METRO_RAND_FREQ = e.value;
    if (e.target->is(mGuiFaderNames[2])) PITCH_RAND_MIN = e.value;
    if (e.target->is(mGuiFaderNames[3])) PITCH_RAND_MAX = e.value;
    if (e.target->is(mGuiFaderNames[4])) SAMPLE_POS_START = e.value;
    if (e.target->is(mGuiFaderNames[5])) SAMPLE_POS_END = e.value;
    if (e.target->is(mGuiFaderNames[6])) MIN_COL_DISTANCE = e.value;
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
void ofApp::exit()
{
    delete[] mAudioPlayers;
    delete[] mAudioPlayerMetros;
}