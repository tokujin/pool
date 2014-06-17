
/*
Parsons the New School for Design
 Nori Yuki
 Original Code by Kevin Darrah
 www.kevindarrah.com
 
 Controlling 576(=24x24) individual LEDs
 
 OF(72+1(header))x8 data
 
 */

#include <SPI.h>// SPI Library used to clock data out to the shift registers

#define latch_pin 2// can use any pin you want to latch the shift registers
#define blank_pin 4// same, can use any pin you want for this, just make sure you pull up via a 1k to 5V
#define data_pin 11// used by SPI, must be pin 11
#define clock_pin 13// used by SPI, must be 13

//These variables are used by multiplexing and Bit Angle Modulation Code
int shift_out;//used in the code a lot in for(i= type loops
byte anode[8];//byte to write to the anode shift register, 8 of them, shifting the ON level in each byte in the array
byte BytesReceived[576]= {  //initial image

0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

}; //this 24x24 grid is replaced by 

//This is how the brightness for every LED is stored,  
//Each LED only needs a 'bit' to know if it should be ON or OFF, so 72 Bytes gives you 576 bits= 576 LEDs
//Since we are modulating the LEDs, using 4 bit resolution, each color has 4 arrays containing 72 bits each

byte white0[72], white1[72], white2[72], white3[72]; //nori

int level=0;//keeps track of which level we are shifting data to
int anodelevel=0;//this increments through the anode levels
int BAM_Bit, BAM_Counter=0; // Bit Angle Modulation variables to keep track of things

//These variables can be used for other things
unsigned long start;//for a millis timer to cycle through the animations

void setup(){

  SPI.setBitOrder(MSBFIRST);//Most Significant Bit First
  SPI.setDataMode(SPI_MODE0);// Mode 0 Rising edge of data, keep clock low
  SPI.setClockDivider(SPI_CLOCK_DIV2);//Run the data in at 16MHz/2 - 8MHz

  Serial.begin(57600);// if you need it?
  noInterrupts();// kill interrupts until everybody is set up

  //We use Timer 1 to refresh the cube
  TCCR1A = B00000000;//Register A all 0's since we're not toggling any pins
  TCCR1B = B00001011;//bit 3 set to place in CTC mode, will call an interrupt on a counter match
  //bits 0 and 1 are set to divide the clock by 72, so 16MHz/72=222kHz
  TIMSK1 = B00000010;//bit 1 set to call the interrupt on an OCR1A match
  OCR1A=30; // you can play with this, but I set it to 30, which means:
  //our clock runs at 222kHz, which is 1/222kHz = 4.5us
  //with OCR1A set to 30, this means the interrupt will be called every (30+1)x4.5us=140us, 
  // which gives a multiplex frequency of about 7.1kHz

  // here I just set up the anode array, this is what's written to the anode shift register, to enable each level
  anode[0]=B10000000;
  anode[1]=B01000000;
  anode[2]=B00100000;
  anode[3]=B00010000;
  anode[4]=B00001000;
  anode[5]=B00000100;
  anode[6]=B00000010;
  anode[7]=B00000001;

  // don't hate on how I assigned the values to this register! haha

  //finally set up the Outputs
  pinMode(latch_pin, OUTPUT);//Latch
  pinMode(data_pin, OUTPUT);//MOSI DATA
  pinMode(clock_pin, OUTPUT);//SPI Clock
  //pinMode(blank_pin, OUTPUT);//Output Enable  important to do this last, so LEDs do not flash on boot up
  SPI.begin();//start up the SPI library
  interrupts();//let the show begin, this lets the multiplexing start

}//***end setup***end setup***end setup***end setup***end setup***end setup***end setup***end setup***end setup***end setup


void loop(){//***start loop***start loop***start loop***start loop***start loop***start loop***start loop***start loop***start loop

  if (Serial.available()>0) {
    for (int i=0; i< 72; i++){ //parse out all three bytes
      BytesReceived[i] = int(Serial.read());
    }    
   delay(10);
  //AT THE END OF THIS FUNCTION, I AM REQUESTING THE NEXT UPDATE FROM OF:
  Serial.write('N');
//  Serial.println(BytesReceived[3]);
  }
  pool();


//pool();

}//***end loop***end loop***end loop***end loop***end loop***end loop***end loop***end loop***end loop***end loop***end loop***end loop


void LED(int level, int column, byte brightness){ 

  //****LED Routine****LED Routine****LED Routine****LED Routine
  //This is where it all starts
  //This routine is how LEDs are updated, with the inputs for the LED location and its R G and B brightness levels

  // First, check and make sure nothing went beyond the limits, just clamp things at either 0 or 7 for location, and 0 or 15 for brightness
  
  if(level<0)
    level=0;
  if(level>7)
    level=7;
  if(column<0)
    column=0;
  if(column>71)
    column=71;  

  if(brightness<0)
    brightness=0;
  if(brightness>15)
    brightness=15;  

  //There are 576 LEDs in the grid, so when we write to level 2, column 20, that needs to be translated into a number from 0 to 576
  int whichbyte = int(((level*72)+column)/8);

  // The first level LEDs are first in the sequence, then 2nd level, then third, and so on
  //the (level*72) is what indexes the level's starting place, so level 0 are LEDs 0-72, level 1 are LEDs 72-143, and so on

  //Now, what about the divide by 8 in there?
  //...well, we have 8 bits per byte, and we have 72 bytes in memory for all 576 bits needed for each LED, so
  //we divide the number we just found by 8, and take the integer of it, so we know which byte, that bit is located
  //confused? that's ok, let's take an example, if we wanted to write to the LED to the last LED in the cube, we would write a 7, 20
  // giving (7*72)+ 20 = 524, which is right, but now let's divide it by 8, 724/8 = 65.5, and take the int of it so, we get 65,
  //this is the last byte in the array, which is right since this is the last LED

  // This next variable is the same thing as before, but here we don't divide by 8, so we get the LED number 0-576
  int wholebyte=(level*72)+column;
  //This will all make sense in a sec

  //This is 4 bit color resolution, so contains x4 64 byte arrays, explanation below:
  bitWrite(white0[whichbyte], wholebyte-(8*whichbyte), bitRead(brightness, 0));
  bitWrite(white1[whichbyte], wholebyte-(8*whichbyte), bitRead(brightness, 1));
  bitWrite(white2[whichbyte], wholebyte-(8*whichbyte), bitRead(brightness, 2)); 
  bitWrite(white3[whichbyte], wholebyte-(8*whichbyte), bitRead(brightness, 3)); 

  //Are you now more confused?  You shouldn't be!  It's starting to make sense now.  Notice how each line is a bitWrite, which is,
  //bitWrite(the byte you want to write to, the bit of the byte to write, and the 0 or 1 you want to write)
  //This means that the 'whichbyte' is the byte from 0-71 in which the bit corresponding to the LED from 0-575
  //Is making sense now why we did that? taking a value from 0-575 and converting it to a value from 0-71, since each LED represents a bit in 
  //an array of 72 bytes.
  //Then next line is which bit 'wholebyte-(8*whichbyte)'  
  //This is simply taking the LED's value of 0-575 and subracting it from the BYTE its bit was located in times 8
  //Think about it, byte 63 will contain LEDs from 504 to 511, so if you took 505-(8*63), you get a 1, meaning that,
  //LED number 505 is is located in bit 1 of byte 63 in the array

  //is that it?  No, you still have to do the bitRead of the brightness 0-15 you are trying to write,
  //if you wrote a 15, all 4 arrays for that LED would have a 1 for that bit, meaning it will be on 100%
  //This is why the four arrays read 0-4 of the value entered in for the LEDs

}//****LED routine end****LED routine end****LED routine end****LED routine end****LED routine end****LED routine end****LED routine end

ISR(TIMER1_COMPA_vect){//***MultiPlex BAM***MultiPlex BAM***MultiPlex BAM***MultiPlex BAM***MultiPlex BAM***MultiPlex BAM***MultiPlex BAM

  //This routine is called in the background automatically at frequency set by OCR1A
  //In this code, I set OCR1A to 30, so this is called every 140us, giving each level in the cube 140us of ON time
  //There are 8 levels, so we have a maximum brightness of 1/8, since the level must turn off before the next level is turned on
  //The frequency of the multiplexing is then 140us*8=1120us, or 1/1120us= about 0.9kHz


  PORTD |= 1<<blank_pin;//The first thing we do is turn all of the LEDs OFF, by writing a 1 to the blank pin
  //Note, in my bread-boarded version, I was able to move this way down in the cube, meaning that the OFF time was minimized
  //do to signal integrity and parasitic capcitance, my rise/fall times, required all of the LEDs to first turn off, before updating
  //otherwise you get a ghosting effect on the previous level

  //This is 4 bit 'Bit angle Modulation' or BAM, There are 8 levels, so when a '1' is written to the  brightness, 
  //each level will have a chance to light up for 1 cycle, the BAM bit keeps track of which bit we are modulating out of the 4 bits
  //Bam counter is the cycle count, meaning as we light up each level, we increment the BAM_Counter
  if(BAM_Counter==8)
    BAM_Bit++;
  else
    if(BAM_Counter==24)
      BAM_Bit++;
    else
      if(BAM_Counter==56)
        BAM_Bit++;

  BAM_Counter++;//Here is where we increment the BAM counter

  switch (BAM_Bit){//The BAM bit will be a value from 0-3, and only shift out the arrays corresponding to that bit, 0-3
    //Here's how this works, each case is the bit in the Bit angle modulation from 0-4, 
    //Next, it depends on which level we're on, so the byte in the array to be written depends on which level, but since each level contains 72 LED,
    //we only shift out 8 bytes for each color
  case 0:
    for(shift_out=level; shift_out<level+9; shift_out++)
      SPI.transfer(white0[shift_out]);
    break;
  case 1:
    for(shift_out=level; shift_out<level+9; shift_out++)
      SPI.transfer(white1[shift_out]);
    break;
  case 2:
    for(shift_out=level; shift_out<level+9; shift_out++)
      SPI.transfer(white2[shift_out]);
    break;
  case 3:
    for(shift_out=level; shift_out<level+9; shift_out++)
      SPI.transfer(white3[shift_out]);
    //Here is where the BAM_Counter is reset back to 0, it's only 4 bit, but since each cycle takes 8 counts,
    //, it goes 0 8 16 32, and when BAM_counter hits 64 we reset the BAM
    if(BAM_Counter==120){
      BAM_Counter=0;
      BAM_Bit=0;
    }
    break;
  }//switch_case

  SPI.transfer(anode[anodelevel]);//finally, send out the anode level byte

  PORTD |= 1<<latch_pin;//Latch pin HIGH
  PORTD &= ~(1<<latch_pin);//Latch pin LOW
  PORTD &= ~(1<<blank_pin);//Blank pin LOW to turn on the LEDs with the new data

  anodelevel++;//inrement the anode level
  level = level+9;//increment the level variable by 8, which is used to shift out data, since the next level woudl be the next 8 bytes in the arrays

  if(anodelevel==8)//go back to 0 if max is reached
    anodelevel=0;
  if(level==72)//if you hit 72 on level, this means you just sent out all 71 bytes, so go back
    level=0;
  pinMode(blank_pin, OUTPUT);//moved down here so outputs are all off until the first call of this function
}//***MultiPlex BAM END***MultiPlex BAM END***MultiPlex BAM END***MultiPlex BAM END***MultiPlex BAM END***MultiPlex BAM END***MultiPlex BAM END


void pool(){

  for(int i=0; i<576;i++){
  
     int a = i/24;
     int b = a/8;
     int c = a%8;
     int d = i%24;
     LED(7-c,71-(24*b+d), BytesReceived[i]);
  
  }
  
}
