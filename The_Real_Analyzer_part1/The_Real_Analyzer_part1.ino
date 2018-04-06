/*
Original Code written by by FischiMc and SupaStefe

The original code used a 10x10 RGB LED-Matrix as a spectrum analyzer
It uses a FTT Library to analyze an audio signal connected to the
pin A7 of an Arduino nano. Everytime a column gets higher than
10 pixels the color of each column changes.

This code has been modified to work on 6 concentric WS2812b ring LEDs. 
Also modified to raise the noise floor to prevent flickering.
*/

#define LOG_OUT 0         //set output of FFT library to linear not logarithmical
#define LIN_OUT 1
#define FFT_N 256         //set to 256 point fft

#include <FFT.h>          //include the FFT library
#include <FastLED.h>      //include the FastLED Library
#include <math.h>         //include library for mathematic funcions
#define DATA_PIN 5        //DATA PIN WHERE YOUR LEDS ARE CONNECTED
#define NUM_LEDS 121      //amount of LEDs in your matrix
CRGB leds[NUM_LEDS];
float faktoren[6] = {1, 1.1, 2.0, 2.6, 3.2, 4.8};                              //factors to increase the height of each column
unsigned char hs[6] = {0,0,0,0,0,0};                                          //height of each column
float hue = 0;                                                                //hue value of the colors
unsigned char ringLed[6] = {30,12,8,6,4,1};
unsigned char numLeds[6] = {0,60,84,100,112,120}; //sum of leds before it


void setBalken(unsigned char column, unsigned char height){                   //calculation of the height of each column
    unsigned char h = (unsigned char)map(height, 0, 255, 0, ringLed[(column)]);
    h = (unsigned char)(h * faktoren[column]);
    if (height < 24 && column == 0){ //testing to reduce stable flickering
        h = 0;
    }
    if (h < hs[column]){
        hs[column]--;
    }
    else if (h > hs[column]){
        hs[column] = h;
    }
   if (height > 250){
      hue+=2;                     //CHANGE THIS VALUE IF YOU WANT THE DIFFERENCE BETWEEN THE COLORS TO BE BIGGER
      if(hue > 24) hue=0;
   }

    for(unsigned char y = 0; y < ringLed[column]; y++){                          //set colors of pixels according to column and hue
       if(hs[column] > y){ 
        float suby = y;
        float bright = (140 + (60*(suby/(ringLed[column]))));
        leds[y+numLeds[column]] = CHSV((hue*10)+(column*10), 255, bright ); ////may need to change second column to column+1
        leds[(numLeds[column+1]) -y] = CHSV((hue*10)+(column*10), 255, bright);
       } else {
        leds[y+numLeds[column]] = CRGB::Black;
        leds[(numLeds[column+1]) -y] = CRGB::Black;
       }
    }
}

unsigned char grenzen[7] = {0,3,5,8,14,100,120};          //borders of the frequency areas

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB> (leds, NUM_LEDS);
  Serial.begin(115200);                                             //use the serial port
  TIMSK0 = 0;                                                       //turn off timer0 for lower jitter
  ADCSRA = 0xe5;                                                    //set the adc to free running mode
  ADMUX = 0b01000111;                                               //use pin A7
  DIDR0 = 0x01;                                                     //turn off the digital input for 
  analogReference(EXTERNAL);                                        //set aref to external
}

void loop() {
  while(1) {                                                        //reduces jitter
    cli();                                                          //UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 512 ; i += 2) {                            //save 256 samples
      while(!(ADCSRA & 0x10));                                      //wait for adc to be ready
      ADCSRA = 0xf5;                                                //restart adc
      byte m = ADCL;                                                //fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m;                                         //form into an int
      k -= 0x0200;                                                  //form into a signed int
      k <<= 6;                                                      //form into a 16b signed int
      fft_input[i] = k;                                             //put real data into even bins
    }

    fft_window();                                                   // window the data for better frequency response
    fft_reorder();                                                  // reorder the data before doing the fft
    fft_run();                                                      // process the data in the fft
    fft_mag_lin();                                                  // take the output of the fft
    sei();

    fft_lin_out[0] = 0;
    fft_lin_out[1] = 0;

    for(unsigned char i = 0; i < 6; i++){
      unsigned char maxW = 0;
        for(unsigned char x = grenzen[i]; x < grenzen[i+1];x++){
 
           if((unsigned char)fft_lin_out[x] > maxW){
            maxW = (unsigned char)fft_lin_out[x];
           }
        }

      setBalken(i, maxW);
      Serial.print(maxW);
      Serial.print(" ");
    }
    Serial.println("");
    TIMSK0 = 1;
    FastLED.show();
    TIMSK0 = 0;
  }
}
