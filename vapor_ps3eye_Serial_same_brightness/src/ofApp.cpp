#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    // general settings
    // set background color to be black:
	ofBackground(0);
    ofSetVerticalSync(true);

    //grabber related
	video.initGrabber(312, 240);	// setup video grabber:
	width = video.getWidth(), height = video.getHeight(); // get the width and height
    
    for ( int i = 0; i< video.listDevices().size() ; i++){
        debug += video.listDevices()[i].deviceName;
        debug += "\n";
        debug += "id: " + ofToString(video.listDevices()[i].id) + "\n";
    
    }
	videoColorCvImage.allocate(width, height); 	// allocate color and grayscale images:
	videoGrayscaleCvImage.allocate(width, height);
    
    //maccam gui setup
    guiSetup();
    
    //code for simulation
    for (int i=0; i<8; i++) {
        for (int j=0; j<72; j++) {
            led_matrix[i][j] = 0;
        }
    }
    
    //avoid janky image
    switch_go = false;
    
    //serial communication related
	mySerial.listDevices();
	vector <ofSerialDeviceInfo> deviceList = mySerial.getDeviceList();
	// this should be set to whatever com port your serial device is connected to.
	// (ie, COM4 on a pc, /dev/tty.... on linux, /dev/tty... on a mac)
	// arduino users check in arduino app....
	int baud = 57600;
	mySerial.setup(0, baud); //open the first device
    
    //initialize array
    for (int i = 0; i < NUM_MSG_BYTES; i++) {
        BytestoSend[i] = 0;
    }
    mySerial.flush();
    isInitialized = false;
    
    ofSetFrameRate(10);
}


void ofApp::guiSetup(){
    
	gui.setup("PS3Eye", "ps3eye.xml");
	gui.setPosition(660,20);
	
    ofxToggle * autoGainAndShutter = new ofxToggle();
    autoGainAndShutter->setup("Auto Gain and Shutter", false);
    autoGainAndShutter->addListener(this, &ofApp::onAutoGainAndShutterChange);
    gui.add(autoGainAndShutter);
    
    ofxFloatSlider * gain = new ofxFloatSlider();
    gain->setup("Gain", 0.5, 0.0, 1.0);
    gain->addListener(this, &ofApp::onGainChange);
    gui.add(gain);
    
    ofxFloatSlider * shutter = new ofxFloatSlider();
    shutter->setup("Shutter", 0.5, 0.0, 1.0);
    shutter->addListener(this, &ofApp::onShutterChange);
    gui.add(shutter);
    
    ofxFloatSlider * gamma = new ofxFloatSlider();
    gamma->setup("Gamma", 0.5, 0.0, 1.0);
    gamma->addListener(this, &ofApp::onGammaChange);
    gui.add(gamma);
    
    ofxFloatSlider * brightness = new ofxFloatSlider();
    brightness->setup("Brightness", 0.5, 0.0, 1.0);
    brightness->addListener(this, &ofApp::onBrightnessChange);
    gui.add(brightness);
    
    ofxFloatSlider * contrast = new ofxFloatSlider();
    contrast->setup("Contrast", 0.5, 0.0, 1.0);
    contrast->addListener(this, &ofApp::onContrastChange);
    gui.add(contrast);
    
    ofxFloatSlider * hue = new ofxFloatSlider();
    hue->setup("Hue", 0.5, 0.0, 1.0);
    hue->addListener(this, &ofApp::onHueChange);
    gui.add(hue);
    
    ofxIntSlider * flicker = new ofxIntSlider();
    flicker->setup("Flicker Type", 0, 0, 2);
    flicker->addListener(this, &ofApp::onFlickerChange);
    gui.add(flicker);
    
    ofxIntSlider * wb = new ofxIntSlider();
    wb->setup("White Balance Mode", 4, 1, 4);
    wb->addListener(this, &ofApp::onFlickerChange);
    gui.add(wb);
	
	ofxToggle * led = new ofxToggle();
    led->setup("LED", true);
	led->addListener(this, &ofApp::onLedChange);
	gui.add(led);
	
	// Load initial values
    
    gui.loadFromFile("ps3eye.xml");
    bool b;
    float f;
    int i;
    b = gui.getToggle("Auto Gain and Shutter");
    onAutoGainAndShutterChange(b);
    f = gui.getFloatSlider("Gain");
    onGainChange(f);
    f = gui.getFloatSlider("Shutter");
    onShutterChange(f);
    f = gui.getFloatSlider("Gamma");
    onGammaChange(f);
    f = gui.getFloatSlider("Brightness");
    onBrightnessChange(f);
    f = gui.getFloatSlider("Contrast");
    onContrastChange(f);
    f = gui.getFloatSlider("Hue");
    onHueChange(f);
    b = gui.getToggle("LED");
    onLedChange(b);
    i = gui.getIntSlider("Flicker Type");
    onFlickerChange(i);
    i = gui.getIntSlider("White Balance Mode");
    onWhiteBalanceChange(i);
	

}

//--------------------------------------------------------------
void ofApp::update(){
    

    //grabber update
	video.update();
    
    //main program
    pixels = videoGrayscaleCvImage.getPixels();
    //24x24
    for (int i = 0; i*10 < video.getWidth()-72; i++){
		for (int j = 0; j*10 < video.getHeight(); j++){
			leds[24*j+i] = pixels[(j*10) * (int)video.getWidth() + i*10];
		}
	}
    
    for (int i=0; i<576; i++) {
        int a = i/24;
        int b = i%24;
        leds_flip[i] = leds[a*24 + (23-b)];
        arr[i] = leds_flip[i] > 200 ? true:false;
    }
    
    //8x72
    for (int k=0; k<576; k++) {
        int a = k/24;
        int b = a/8;
        int c = a%8;
        int d = k%24;
        led_matrix[c][24*b+d] = leds_flip[k];
    }

    //caluculation
	if (video.isFrameNew()){
		
		videoColorCvImage.setFromPixels(video.getPixels(), width, height);
		videoGrayscaleCvImage = videoColorCvImage;
				
        //do this to avoid janky image
		if (ofGetElapsedTimef() < 1.5){
			videoBgImage = videoGrayscaleCvImage;
		}
	}

    BytestoSend[0]=255;
    
    for (int i=0; i<576; i++) {
        if (arr[i]==true) {
            BytestoSend[1+(i/7)] |= (0x00000001 << (i%7));
        }
    }
    
    //serial communication related
    if(mySerial.available() >0){ //something came in over serial port
        
        if(!isInitialized) isInitialized = true; //we heard from Arduino, we're initialized.
        
        char incoming = mySerial.readByte();
        cout << "message received: "<< incoming << endl;
        
        mySerial.flush(); //flush whatever messages were received, clean slate next frame
        
		mySerial.writeBytes(BytestoSend, NUM_MSG_BYTES); //send out current bytes to send
        numMsgSent++; //for our own count
	}
    
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){
	
    
    gui.draw();
    
	videoGrayscaleCvImage.draw(120,20, 312,240);
    //	videoBgImage.draw(312+40, 20, 312, 240);

    //	this is where the point image comes(20,240+40);
	panel.draw();

    //draw 24x24
    
    ofSetColor(255, 255, 255);
    for (int i = 0; i*10 < video.getWidth()-72; i++){
		for (int j = 0; j*10 < video.getHeight(); j++){
			int pct = ofMap(leds_flip[24*j+i], 0,255, 0,5);
			ofSetColor(255,255,255);
			ofCircle(120 + i*10 + 13/2, 240 + 40 + j*10 + 10/2, pct);
		}
    
    //draw 8x72
    for (int i=0; i<72; i++) {
        for (int j=0; j<8; j++) {
            int pct = ofMap(led_matrix[j][i], 0, 255, 0, 5);
            ofCircle(120-1 + 13/2 + i*10, 280-1 + 240 + 20 + 10/2 + j*10, pct);
        }
    }
    

        for (int i=0; i<24; i++) {
            for (int j=0; j<24; j++) {
                ofCircle(i*10, j*10, arr[i]*3);
            }
        }
        
        

    //additional line
    ofPushStyle();
        ofNoFill();
        ofSetColor(255, 0, 0);
        ofSetLineWidth(1.5);
        ofSetRectMode(OF_RECTMODE_CORNER);
    
        //3x3 grid
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++) {
                ofRect(120-1 + 240/3*i,280-1 + 240/3*j, 240/3, 240/3);
            }
        }
        ofSetColor(0, 255, 0);
    
        //9 strip
        for (int i=0; i<9; i++) {
            ofRect(120-1 + 240/3*i,280-1 + 240 + 20, 240/3, 240/3);
        }
    
        //8 lines for casode
        ofSetColor(255, 0, 0);
        for (int i=0; i<8; i++) {
            ofLine(120-1 +13/2, 280-1 + 240 + 20 + 10/2 + 10*i, 120-1 +13/2 + 10*(72-1), 280-1 + 240 + 20 + 10/2 + 10*i);
        }
    
    ofPopStyle();
    }
    
    
    if(!isInitialized)
        ofDrawBitmapString("PRESS 'S' TO SEND KICKOFF BYTE", 400, 400);
    else {
        //just for debug
        ofDrawBitmapString("num messages sent total: "+ ofToString(numMsgSent), 400, 450);
    }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    switch(key){
        case 's':
            cout << "SENDING KICK-OFF MESSAGE" << endl;
            mySerial.writeBytes(BytestoSend, NUM_MSG_BYTES);
            break;
        default:
            cout << "UNRECOGNIZED BUTTON PRESS"<<endl;
    }
    
}


//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	panel.mouseDragged(x,y,button);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	panel.mousePressed(x,y,button);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	panel.mouseReleased();
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

// macam stuffs
//--------------------------------------------------------------
void ofApp::onAutoGainAndShutterChange(bool & value){
	video.setAutoGainAndShutter(value);
}

//--------------------------------------------------------------
void ofApp::onGainChange(float & value){
	// Only set if auto gain & shutter is off
	if(!(bool&)gui.getToggle("Auto Gain and Shutter")){
        video.setGain(value);
	}
}

//--------------------------------------------------------------
void ofApp::onShutterChange(float & value){
	// Only set if auto gain & shutter is off
	if(!(bool&)gui.getToggle("Auto Gain and Shutter")){
        video.setShutter(value);
	}
}

//--------------------------------------------------------------
void ofApp::onGammaChange(float & value){
	video.setGamma(value);
}

//--------------------------------------------------------------
void ofApp::onBrightnessChange(float & value){
	video.setBrightness(value);
}

//--------------------------------------------------------------
void ofApp::onContrastChange(float & value){
	video.setContrast(value);
}

//--------------------------------------------------------------
void ofApp::onHueChange(float & value){
	video.setHue(value);
}

//--------------------------------------------------------------
void ofApp::onLedChange(bool & value){
	video.setLed(value);
}

//--------------------------------------------------------------
void ofApp::onFlickerChange(int & value){
	video.setFlicker(value);
}

//--------------------------------------------------------------
void ofApp::onWhiteBalanceChange(int & value){
	video.setWhiteBalance(value);
}
