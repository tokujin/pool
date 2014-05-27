#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    guiSetup();
    // general settings
    // set background color to be black:
	ofBackground(0);
    ofSetVerticalSync(true);

    //grabber related
	video.initGrabber(312, 240);	// setup video grabber:
	width = video.getWidth(), height = video.getHeight(); // get the width and height
    
    
    //video.setDeviceID(2);
    for ( int i = 0; i< video.listDevices().size() ; i++){
        debug += video.listDevices()[i].deviceName;
        debug += "\n";
        debug += "id: " + ofToString(video.listDevices()[i].id) + "\n";
    
    }
    
    
	videoColorCvImage.allocate(width, height); 	// allocate color and grayscale images:
	videoGrayscaleCvImage.allocate(width, height);
	videoBgImage.allocate(width, height);

    //control panel related
	panel.setup("cv settings", 1350, 0, 300, 748);
	panel.addPanel("control", 1, false);
	
	panel.setWhichPanel("control");
	panel.setWhichColumn(0);
    panel.addToggle("learn background ", "B_LEARN_BG", true); //Actually I'm not using this but I might use this in the future, so I'll keep this
	panel.loadSettings("cvSettings.xml");
    
    
    //simulation
    for (int i=0; i<8; i++) {
        for (int j=0; j<72; j++) {
            led_matrix[i][j] = 0;
        }
    }
    
    //avoid janky image
    switch_go = false;
    
    //serial communication related
    //initialize array
    mySerial.listDevices();
    vector <ofSerialDeviceInfo> deviceList = mySerial.getDeviceList();

    for (int i = 0; i < 577; i++) {
        BytestoSend[i] = 0;
    }
    
    mySerial.setup("/dev/tty.usbmodemfa131", 115200);
    //mySerial.setup("/dev/tty.usbmodemfa131", 38400);
    mySerial.flush(); //flush the serial port once before we start

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
    
    //avoid janky image, prevent LEDs from burning
    if (switch_go==false) {
        if (ofGetElapsedTimeMillis() > 5000) {
            switch_go = true;
        }
    }

    //control panel related
	panel.update();
	bool bLearnBg			= panel.getValueB("B_LEARN_BG");
	int threshold			= panel.getValueI("THRESHOLD");
	
    
    //grabber update
	video.update();
    
    //main program
    pixels = videoGrayscaleCvImage.getPixels();
    //24x24
    for (int i = 0; i*13 < video.getWidth(); i++){
		for (int j = 0; j*10 < video.getHeight(); j++){
			leds[24*j+i] = pixels[(j*10) * (int)video.getWidth() + i*13];
		}
	}
    //8x72
    for (int k=0; k<576; k++) {
        int a = k/24;
        int b = a/8;
        int c = a%8;
        int d = k%24;
        led_matrix[c][24*b+d] = leds[k];
    }

    //caluculation
	if (video.isFrameNew()){
		
		videoColorCvImage.setFromPixels(video.getPixels(), width, height);
		videoGrayscaleCvImage = videoColorCvImage;
		
		if (bLearnBg ){
			videoBgImage = videoGrayscaleCvImage;
			panel.setValueB("B_LEARN_BG", false);
		}
		
        //do this to avoid janky image
		if (ofGetElapsedTimef() < 1.5){
			videoBgImage = videoGrayscaleCvImage;
		}
	}
    
    //serial communication related
    BytestoSend[0] = 255;     //set up header byte for serial communication
    for (int i=0; i<576; i++) {
        BytestoSend[i+1] = int(ofMap(leds[i], 0, 255, 0, 253)); //initialize the bytes to send
    }
        //write serial data
        mySerial.writeBytes(BytestoSend, 577);

    cout << "------------------------------" << endl;
    for(int i = 0; i < 9; i++){
        cout <<  "BytestoSend[i] "<< " is  " << (int)BytestoSend[i] << endl;
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
    
    if (switch_go) {
    ofSetColor(255, 255, 255);
    for (int i = 0; i*13 < video.getWidth(); i++){
		for (int j = 0; j*10 < video.getHeight(); j++){
			int pct = ofMap(leds[24*j+i], 0,255, 0,5);
			ofSetColor(255,255,255);
			ofCircle(120 + i*13 + 13/2, 240 + 40 + j*10 + 10/2, pct);
		}
	}
    
    //draw 8x72
    for (int i=0; i<72; i++) {
        for (int j=0; j<8; j++) {
            int pct = ofMap(led_matrix[j][i], 0, 255, 0, 5);
            ofCircle(120-1 + 13/2 + i*13, 280-1 + 240 + 20 + 10/2 + j*10, pct);
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
                ofRect(120-1 + 320/3*i,280-1 + 240/3*j, 320/3, 240/3);
            }
        }
        ofSetColor(0, 255, 0);
    
        //9 strip
        for (int i=0; i<9; i++) {
            ofRect(120-1 + 320/3*i,280-1 + 240 + 20, 320/3, 240/3);
        }
    
        //8 lines for casode
        ofSetColor(255, 0, 0);
        for (int i=0; i<8; i++) {
            ofLine(120-1 +13/2, 280-1 + 240 + 20 + 10/2 + 10*i, 120-1 +13/2 + 13*(72-1), 280-1 + 240 + 20 + 10/2 + 10*i);
        }
    
    ofPopStyle();
    }
    ofDrawBitmapStringHighlight(debug, 100,600);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
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
