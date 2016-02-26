
#include <Wire.h>
#include <Time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// matrix of 12 - 16 bit ints (shorts) for displaying the leds
uint16_t mask[12];

// using masks for each row 16 bit mask which uses the msb as a row index and the
// next three bytes for the columnn index 
// TODO only using 3 of 4 bytes, maybe put color information in the first byte 
#define IT       mask[0]  |= 0x6000
#define IS       mask[0]  |= 0x1800
#define HALF     mask[0]  |= 0x03C0
#define BDAY     mask[1]  |= 0xF000
#define QUARTER  mask[1]  |= 0x0FE0
#define TWENTY   mask[2]  |= 0xFC00
#define MFIVE    mask[2]  |= 0x01E0
#define MTEN     mask[3]  |= 0x3800
#define PAST     mask[3]  |= 0x03C0
#define TO       mask[3]  |= 0x0030
#define TEN      mask[4]  |= 0xE000
#define SIX      mask[4]  |= 0x1C00
#define ONE      mask[4]  |= 0x00E0
#define NINE     mask[5]  |= 0xF000
#define THREE    mask[5]  |= 0x01F0
#define EIGHT    mask[6]  |= 0xF800
#define FIVE     mask[6]  |= 0x0780
#define TWO      mask[6]  |= 0x0070
#define FOUR     mask[7]  |= 0x7800
#define ELEVEN   mask[7]  |= 0x03F0
#define SEVEN    mask[8]  |= 0xF800
#define TWELVE   mask[8]  |= 0x03F0
#define OCLOCK   mask[9]  |= 0xFC00
#define KATIE    mask[9]  |= 0x07C0
#define IN       mask[9]  |= 0x0030
#define THE      mask[10] |= 0xE000
#define MORNING  mask[10] |= 0X0Fe0
#define EVENING  mask[11] |= 0x1FC0
#define MONE     mask[0]  |= 0x8000
#define MTWO     mask[0]  |= 0x0010
#define MTHREE   mask[11] |= 0x8000  
#define MFOUR    mask[11] |= 0x0010  

// define pins
#define NEOPIN 6  // connect to DIN on NeoMatrix 8x8
//#define RTCGND A2 // use this as DS1307 breakout ground TODO not using a RTC yet
//#define RTCPWR A3 // use this as DS1307 breakout power  TODO

// define delays
#define FLASHDELAY 500  // delay for startup "flashWords" sequence
#define SHIFTDELAY 100   // controls color shifting speed

int j;   // an integer for the color shifting effect

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//Adafruit_NeoPixel matrix = Adafruit_NeoPixel(64, NEOPIN, NEO_GRB + NEO_KHZ800);

// configure for 12x12 neopixel matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(12, 12, NEOPIN,
      NEO_MATRIX_BOTTOM  + NEO_MATRIX_LEFT +
      NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
      NEO_GRB         + NEO_KHZ800);

void rainbowCycle(uint8_t wait);
void flashWords(void);
void pickAPixel(uint8_t x, uint8_t y);

void setup(void) 
{
   // put your setup code here, to run once:

   //Serial for debugging
   Serial.begin(9600);

   //  setTime(__TIME__);
   matrix.begin();
   matrix.setBrightness(255);
}

void loop(void) 
{
   // put your main code here, to run repeatedly:

   matrix.fillScreen(0); // Initialize all pixels to 'off'
   matrix.show();
   flashWords(); // briefly flash each word in sequence
   //  adjustBrightness();
   //  displayTime();

  // rainbowCycle(20);
   //mode_moon(); // uncomment to show moon mode instead!
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {

   WheelPos = 255 - WheelPos;
   uint32_t wheelColor;

   if (WheelPos < 85) {
      wheelColor = matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
   } else if (WheelPos < 170) {
      WheelPos -= 85;
      wheelColor = matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
   } else {
      WheelPos -= 170;
      wheelColor = matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
   }

   // convert from 24-bit to 16-bit color - NeoMatrix requires 16-bit. perhaps there's a better way to do this.
   uint32_t bits = wheelColor;
   uint32_t blue = bits & 0x001F;     // 5 bits blue
   uint32_t green = bits & 0x07E0;    // 6 bits green
   uint32_t red = bits & 0xF800;      // 5 bits red

   // Return shifted bits with alpha set to 0xFF
   return (red << 8) | (green << 5) | (blue << 3) | 0xFF000000;
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {

   uint16_t i, j;

   for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
      for (i = 0; i < matrix.numPixels(); i++) {
         Serial.print("Pixel = ");
         Serial.println(i);
         matrix.setPixelColor(i, Wheel(((i * 256 / matrix.numPixels()) + j) & 255));
         matrix.show();
         delay(wait);
      }
   }
}

// show colorshift through the phrase mask. for each NeoPixel either show a color or show nothing!
void applyMask() {

   for (byte row = 0; row < 12; row++) 
   {
      for (byte col = 0; col < 16; col++) 
      {
         boolean masker = bitRead(mask[row], 15 - col); // bitread is backwards because bitRead reads rightmost digits first. could have defined the word masks differently
         switch (masker) 
         {
            case 0:
               matrix.drawPixel(col, row, 0);
               break;
            case 1:
              // matrix.drawPixel(col, row, Wheel(((col * 256 / matrix.numPixels()) + j) & 255));
               matrix.drawPixel(col, row, 255);
               break;
         }
      }
      // reset mask for next time
      mask[row] = 0;
   }


   matrix.show(); // show it!
   delay(SHIFTDELAY);
   j++; // move the colors forward
   j = j % (256 * 5);

}

void flashWords(void) {

   IT;
   applyMask();
   delay(FLASHDELAY);

   IS;
   applyMask();
   delay(FLASHDELAY);

   HALF;
   applyMask();
   delay(FLASHDELAY);

   QUARTER;
   applyMask();
   delay(FLASHDELAY);

   TWENTY;
   applyMask();
   delay(FLASHDELAY);

   MFIVE;
   applyMask();
   delay(FLASHDELAY);

   MTEN;
   applyMask();
   delay(FLASHDELAY);

   PAST;
   applyMask();
   delay(FLASHDELAY);

   TO;
   applyMask();
   delay(FLASHDELAY);

   TEN;
   applyMask();
   delay(FLASHDELAY);

   SIX;
   applyMask();
   delay(FLASHDELAY);

   ONE;
   applyMask();
   delay(FLASHDELAY);

   NINE;
   applyMask();
   delay(FLASHDELAY);

   THREE;
   applyMask();
   delay(FLASHDELAY);

   EIGHT;
   applyMask();
   delay(FLASHDELAY);

   FIVE;
   applyMask();
   delay(FLASHDELAY);

   TWO;
   applyMask();
   delay(FLASHDELAY);

   FOUR;
   applyMask();
   delay(FLASHDELAY);

   ELEVEN;
   applyMask();
   delay(FLASHDELAY);

   SEVEN;
   applyMask();
   delay(FLASHDELAY);

   TWELVE;
   applyMask();
   delay(FLASHDELAY);

   OCLOCK;
   applyMask();
   delay(FLASHDELAY);

   IN;
   applyMask();
   delay(FLASHDELAY);

   THE;
   applyMask();
   delay(FLASHDELAY);

   MORNING;
   applyMask();
   delay(FLASHDELAY);

   EVENING;
   applyMask();
   delay(FLASHDELAY);

   MONE;
   applyMask();
   delay(FLASHDELAY);

   MONE;
   MTWO;
   applyMask();
   delay(FLASHDELAY);

   MONE;
   MTWO;
   MTHREE;
   applyMask();
   delay(FLASHDELAY);

   MONE;
   MTWO;
   MTHREE;
   MFOUR;
   applyMask();
   delay(FLASHDELAY);
   // blank for a bit
   applyMask();
   delay(FLASHDELAY);

}
