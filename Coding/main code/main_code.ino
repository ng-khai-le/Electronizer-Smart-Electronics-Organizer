// include buzzer pitches library
#include "pitches.h"
// include WS2812B RGB LED strips library
#include <Adafruit_NeoPixel.h>
// include Arduino Cloud library
#include "thingProperties.h"
// include OLED SSD1306_128_64 I2C display libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// declare hardware pins
#define buzzer_pin D5
#define LED_PIN_A  D3 // Main Grid
#define LED_PIN_1  D7 // Sub Grid
#define LED_PIN_2  D8 // Sub Grid
int joystick_SW = D6;

// Store the joystick switch state
int buttonState;
// Joystick x axis
int joy_x;
// Joystick y axis
int joy_y;
// Store the last message
String current_message;
String last_message;

int led_mode = 1;

// How many NeoPixels are attached?
#define NUMPIXELS_A 2 // Main Grid
#define NUMPIXELS_1 9 // Sub Grid
#define NUMPIXELS_2 9 // Sub Grid

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50 // Set BRIGHTNESS to about 1/5 (max = 255)

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter
Adafruit_NeoPixel pixels_A(NUMPIXELS_A, LED_PIN_A, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_1(NUMPIXELS_1, LED_PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_2(NUMPIXELS_2, LED_PIN_2, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

int melody[] = {
  NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4, 
  NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4, 
  NOTE_B4, NOTE_A4, NOTE_CS4, NOTE_E4,
  NOTE_A4
};

int durations[] = {
  8, 8, 4, 4,
  8, 8, 4, 4,
  8, 8, 4, 4,
  2
};

// This prototype has only 1 row and 2 columes
// if you wish to expand the row, add additional information at the end of the array, for example {"RES", "CAP", "MOS", "SW"};
// Array to store the category of the components, ALWAYS UPPERCASE !
String main_grid[] = {"RES", "CAP"};

// Array of the value of the components, ALWAYS UPPERCASE !
// If row has been expanded, you will need to add the new array, and change the code by yourself, I'm still working on simplier way to do this
String res[] = {"1M", "680K", "470K", "300K", "22", "45K", "64K", "110K", "220K"};
String cap[][2] = {{"ALU", "10U"}, {"NON", "22N"}, {"ALU", "100U"}, {"NON", "10N"}, {"NON", "1N"}, {"NON", "2.2N"}, {"NON", "3.3N"}, {"NON", "4.7N"}, {"NON", "1U"}}; 

int main_len = sizeof(main_grid)/sizeof(main_grid[0]);

int row = 1;
int colume = 2;


void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  pinMode(joystick_SW, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();

  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels_A.begin(); 
  pixels_1.begin(); 
  pixels_2.begin();
  // Turn OFF all pixels ASAP
  pixels_A.show(); 
  pixels_1.show(); 
  pixels_2.show(); 
  // Set the brightness of the LED Strips
  pixels_A.setBrightness(BRIGHTNESS);
  pixels_1.setBrightness(BRIGHTNESS);
  pixels_2.setBrightness(BRIGHTNESS);

  
  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo();

}


void loop() {

  ArduinoCloud.update();
  // Your code here 

  display.clearDisplay();

  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE);        
  display.setCursor(2,2);            
  display.print(F("Smart Organizer 1"));
  // Use full 256 char 'Code Page 437' font
  display.cp437(true);        
  // Draw Smile Face 
  display.setCursor(115,2);
  display.write(2);
  display.cp437(false);
  display.drawRect(0, 0, display.width(), 11, SSD1306_WHITE);

  display.display();

  if (current_message == last_message){
    if (led_mode == 2){
      led_mode = 1;
    }else{
      led_mode += 1;
    }
    switch (led_mode){
      case 1:
        colorWipe(pixels_1.Color(255,   0,   0), 50); // Red
        colorWipe(pixels_2.Color(255,   0,   0), 50); // Red
        colorWipe(pixels_1.Color(  0, 255,   0), 50); // Green
        colorWipe(pixels_2.Color(  0, 255,   0), 50); // Green
        colorWipe(pixels_1.Color(  0,   0, 255), 50); // Blue
        colorWipe(pixels_2.Color(  0,   0, 255), 50); // Blue
      break;
      case 2:
        rainbow(10);             // Flowing rainbow cycle along the whole strip
      break;
      default:
        rainbow(10);             // Flowing rainbow cycle along the whole strip
      break;
    }
  }
  // receive something
  else if (current_message != last_message){

    noColor();

    // Store the position of the component
    int index_main_grid;
    int sub_index=0;

    // Store the message into a new String variable from the Cloud
    String find_name = current_message;

    // Convert to Upper Case
    find_name.toUpperCase();

    last_message = current_message;
    current_message = last_message;

    // Store the types of components into a new String variable
    String types;
    // Store the sub-types of components, for example capacitor has two categories: Aluminium electrolytic and Ceramic (NON-polarized)
    String component_types;

    // Check if the message starts with the specific opening
    if (find_name.startsWith("RES,")){
      types = "RES";
    }else if (find_name.startsWith("CAP,")){
      types = "CAP";
    }

    // Reset the index position, use negative number to represent fault index
    index_main_grid=-1;
    // Find the main grid x y position
    for (int i=0; i<2; i++){
      if (types == main_grid[i]){
        index_main_grid=i;   
      }
    }
    // If either one of the position is -1, nothing has found
    if (index_main_grid==-1){
      fault_detected(find_name);
    }
    // Found the category of the component
    else{
      // Reset the index position, use negative number to represent fault index
      sub_index = -1;

      if (find_name.startsWith("RES,")){
        find_name = find_name.substring((find_name.indexOf(',')+1), (find_name.length()));
        for (int i=0; i<9; i++){
          if (find_name == res[i]){
            sub_index = i;
          }
        }
      }else if (find_name.startsWith("CAP,")){
        find_name = find_name.substring((find_name.indexOf(',')+1), (find_name.length()));
        if (find_name.startsWith("ALU,")){
          component_types = "ALU";
        }else if (find_name.startsWith("NON,")){
          component_types = "NON";
        }
        find_name = find_name.substring((find_name.indexOf(',')+1), (find_name.length()));
        for (int i=0; i<9; i++){
          // if the same type has occured
          if (component_types == cap[i][0]){
            // check if the value is the same
            if (find_name == cap[i][1]){
              sub_index = i;
            }
          }
        }
      }
      // See whether the index is fault
      if (sub_index == -1){
        fault_detected(find_name);
      }
      // Found the value of the component
      else{
        // make the position green

        display.clearDisplay();

        display.setTextSize(1);     
        display.setTextColor(SSD1306_WHITE);        
        display.setCursor(2,2);            
        display.print(F("Smart Organizer 1"));
        // Use full 256 char 'Code Page 437' font
        display.cp437(true);        
        // Draw Smile Face 
        display.setCursor(115,2);
        display.write(2);
        display.cp437(false);
        display.drawRect(0, 0, display.width(), 11, SSD1306_WHITE);

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);        
        display.setCursor(0,20);
        display.print(F("Type:"));
        display.println(types);

        display.print(F("Value:"));
        display.println(find_name);

        display.print(F("Main position at:"));
        display.println(index_main_grid);

        display.print(F("Sub Position at:"));
        display.println(sub_index);

        display.display();   

        // Set all pixel colors to 'off'
        pixels_A.clear(); 
        pixels_1.clear(); 
        pixels_2.clear(); 

        // The first NeoPixel in a strand is #0, second is 1, all the way up
        // to the count of pixels minus one.
        // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
        // Here we're using a moderately bright green color:
        for(int i=0; i<NUMPIXELS_A; i++) { // For each pixel...
          if (i == index_main_grid){
            pixels_A.setPixelColor(i, pixels_A.Color(0, 255, 0));
            pixels_A.show();   
          }else{
            pixels_A.setPixelColor(i, pixels_A.Color(0, 0, 0));
            pixels_A.show();   
          }
        }
        for(int i=0; i<NUMPIXELS_1; i++) { // For each pixel...
          // This LED strip 1 is connected to resistors box
          if (types == "RES"){
            // if current index equal to the actual index of the component
            if (i == sub_index){
              pixels_1.setPixelColor(i, pixels_1.Color(0, 255, 0));
              pixels_1.show();
            }else{
              pixels_1.setPixelColor(i, pixels_1.Color(0, 0, 0));
              pixels_1.show();
            }
          }else{
            pixels_1.setPixelColor(i, pixels_1.Color(0, 0, 0));
            pixels_1.show();
          }
        }
        for(int i=0; i<NUMPIXELS_2; i++) { // For each pixel...
        // This LED strip 2 is connected to capacitors box
          if (types == "CAP"){
            // if current index equal to the actual index of the component
            if (i == sub_index){
              pixels_2.setPixelColor(i, pixels_2.Color(0, 255, 0));
              pixels_2.show();
            }else{
              pixels_2.setPixelColor(i, pixels_2.Color(0, 0, 0));
              pixels_2.show();
            }
          }else{
            pixels_2.setPixelColor(i, pixels_2.Color(0, 0, 0));
            pixels_2.show();
          }
        }

        // turn on the buzzer
        int size = sizeof(durations) / sizeof(int);
        for (int note = 0; note < size; note++) {
          int duration = 1000 / durations[note];
          tone(buzzer_pin, melody[note], duration);
          int pauseBetweenNotes = duration * 1.30;
          delay(pauseBetweenNotes);
          noTone(buzzer_pin);
        }

        while (digitalRead(joystick_SW) != 0){}

        noColor();

      }

    }

  }


}


/*
  Since PartsName is READ_WRITE variable, onPartsNameChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onPartsNameChange()  {
  // Add your code here to act upon PartsName change
  current_message = parts_name;

  parts_name = PropertyActions::CLEAR;
}


void fault_detected(String find_name){
  last_message = current_message;
  current_message = last_message;

  display.clearDisplay();
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE);        
  display.setCursor(2,2);            
  display.print(F("Smart Organizer 1"));
  // Use full 256 char 'Code Page 437' font
  display.cp437(true);        
  // Draw Smile Face 
  display.setCursor(115,2);
  display.write(2);
  display.cp437(false);
  display.drawRect(0, 0, display.width(), 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);        
  display.setCursor(0,20);
  display.print(F("Cannot find item:"));
  display.print(find_name);
  display.display();   

  // Set all pixel colors to 'off'
  pixels_A.clear(); 
  pixels_1.clear(); 
  pixels_2.clear(); 
  
  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
  // Here we're using a moderately bright red color:
  for(int i=0; i<NUMPIXELS_A; i++) { // For each pixel...
    pixels_A.setPixelColor(i, pixels_A.Color(255, 0, 0));
    pixels_A.show();   
  }
  for(int i=0; i<NUMPIXELS_1; i++) { // For each pixel...
    pixels_1.setPixelColor(i, pixels_1.Color(255, 0, 0));
    pixels_1.show();   
  }
   for(int i=0; i<NUMPIXELS_2; i++) { // For each pixel...
    pixels_2.setPixelColor(i, pixels_2.Color(255, 0, 0));
    pixels_2.show();   
  }
  // turn on the buzzer
  tone(buzzer_pin, 350, 200);
  delay(400);
  tone(buzzer_pin, 255, 200);
  delay(400);
  tone(buzzer_pin, 350, 200);
  delay(400);
  tone(buzzer_pin, 255, 200);
  delay(400);
  noTone(buzzer_pin);
  delay(1000);

  noColor();
}


void noColor() {
  for(int i=0; i<NUMPIXELS_A; i++) { // For each pixel...
    pixels_A.setPixelColor(i, pixels_A.Color(0, 0, 0));
    pixels_A.show();   
  }
  for(int i=0; i<NUMPIXELS_1; i++) { // For each pixel...
    pixels_1.setPixelColor(i, pixels_1.Color(0, 0, 0));
    pixels_1.show();   
  }
   for(int i=0; i<NUMPIXELS_2; i++) { // For each pixel...
    pixels_2.setPixelColor(i, pixels_2.Color(0, 0, 0));
    pixels_2.show();   
  }
}


// Some functions for creating animated effects -----------------
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<pixels_1.numPixels(); i++) { // For each pixel in strip...
    pixels_1.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    pixels_2.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    pixels_1.show();                          //  Update strip to match
    pixels_2.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    pixels_1.rainbow(firstPixelHue);
    pixels_2.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    pixels_1.show(); // Update strip with new contents
    pixels_2.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

