#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxControlPanel.h"
#include "ofxMacamPs3Eye.h"
#include "ofxGui.h"

#define NUM_MSG_BYTES 84 //how many bytes we send to Arduino 576/7+1(header)


class ofApp : public ofBaseApp{

    public:
        void setup();
    void guiSetup();
        void update();
        void draw();
        
        void keyPressed  (int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void windowResized(int w, int h);
        
        // create a variable of the type ofImage
        ofxControlPanel			panel;
        
        int						width, height;
        ofxMacamPs3Eye			video;
        ofxCvColorImage			videoColorCvImage;
        ofxCvGrayscaleImage		videoGrayscaleCvImage;
        ofxCvGrayscaleImage		videoBgImage;
        
        ofxCvGrayscaleImage		videoDiffImage;
        ofxCvGrayscaleImage     modifiedImage;
        
        float value, valuetemp;
        
        unsigned char * pixels;
        unsigned char * mpixels;
        
        float pct = 0;

        
        //we'll use this to count how many msgs have been received so far
        //(for debugging)
        long numMsgSent;
        unsigned char leds[576];  //output to
        unsigned char leds_flip[576];  //output to
        unsigned char arr[576];

        int led_matrix[8][72];
        int led_strip[576];
    
        //timer to avoid janky image
        bool switch_go;
    
        //serial related
        ofSerial mySerial;
        unsigned char BytestoSend[NUM_MSG_BYTES];  //output to
        bool bSendSerialMessage;			// a flag for sending serial
        bool isInitialized;
    
        string debug;
    
        //gui related macam functions
        void onAutoGainAndShutterChange(bool & value);
        void onGainChange(float & value);
        void onShutterChange(float & value);
        void onGammaChange(float & value);
        void onBrightnessChange(float & value);
        void onContrastChange(float & value);
        void onHueChange(float & value);
        void onLedChange(bool & value);
        void onFlickerChange(int & value);
        void onWhiteBalanceChange(int & value);

        ofxPanel gui;
    
        unsigned long serial_counter;

};
