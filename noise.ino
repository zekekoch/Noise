#include<FastLED.h>

#define pin1 9
#define pin2 10
#define pin3 11
#define pin4 12
#define pin5 15
#define pin6 13

#define strip1 26
#define strip2 39
#define strip3 23
#define strip4 22
#define strip5 17
#define strip6 17

#define longestStrip strip2

const uint8_t kColumns = 23;
const uint8_t kRows = 9;
const uint8_t kMatrixWidth = kColumns;
const uint8_t kMatrixHeight = kRows;

byte inByte = 0;
boolean serialComplete = false;

const int myInput = A0;


// This example combines two features of FastLED to produce a remarkable range of
// effects from a relatively small amount of code.  This example combines FastLED's 
// color palette lookup functions with FastLED's Perlin/simplex noise generator, and
// the combination is extremely powerful.
//
// You might want to look at the "ColorPalette" and "Noise" examples separately
// if this example code seems daunting.
//
// 
// The basic setup here is that for each frame, we generate a new array of 
// 'noise' data, and then map it onto the LED matrix through a color palette.
//
// Periodically, the color palette is changed, and new noise-generation parameters
// are chosen at the same time.  In this example, specific noise-generation
// values have been selected to match the given color palettes; some are faster, 
// or slower, or larger, or smaller than others, but there's no reason these 
// parameters can't be freely mixed-and-matched.
//
// In addition, this example includes some fast automatic 'data smoothing' at 
// lower noise speeds to help produce smoother animations in those cases.
//
// The FastLED built-in color palettes (Forest, Clouds, Lava, Ocean, Party) are
// used, as well as some 'hand-defined' ones, and some proceedurally generated
// palettes.


#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)

uint16_t XY( uint8_t x, uint8_t y)
{
  //Serial.print("XY(");Serial.print(x);Serial.print(",");Serial.print(y);Serial.print(")");Serial.println();

  const uint16_t mappedTable[kRows * kColumns] = 
  {
//        0,   1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,    
/* 0 */  24,   24,   24,   24,   24,   24,   25,   25,   25,   25,   25,   25,   25,   25,   23,   23,   23,   23,   23,   23,   23,   25,   25,
/* 1 */  20,   19,   19,   18,   18,   18,   17,   17,   16,   16,   16,   15,   15,   14,   14,   14,   22,   22,   22,   21,   21,   21,   20,
/* 2 */  8,    7,    7,    6,    6,    5,    4,    4,    3,    3,    2,    1,    1,    0,    0,    13,   12,   12,   11,   10,   10,   9,    9,
/* 3 */  57,   56,   55,   54,   54,   53,   52,   52,   51,   50,   49,   48,   48,   47,   64,   63,   62,   62,   61,   60,   59,   59,   58,
/* 4 */  37,   36,   35,   34,   33,   32,   31,   31,   30,   29,   28,   27,   26,   46,   45,   44,   43,   42,   41,   40,   39,   39,   38,
/* 5 */  76,   75,   74,   73,   72,   71,   70,   69,   68,   67,   66,   65,   87,   86,   85,   84,   83,   82,   81,   80,   79,   78,   77,
/* 6 */  98,   97,   96,   95,   94,   93,   92,   91,   90,   89,   88,   109,  108,  107,  106,  105,  104,  103,  102,  101,  100,  100,  99,  
/* 7 */  116,  115,  114,  113,  113,  112,  111,  110,  143,  142,  126,  125,  124,  123,  123,  122,  121,  120,  119,  118,  118,  117,  117, 
/* 8 */  132,  131,  131,  130,  130,  129,  129,  128,  127,  141,  140,  139,  139,  138,  138,  137,  136,  135,  135,  134,  134,  133,  133,    
  };
  // lazy bounds checking
  if (x >= kColumns)
    x = 0;
  if (y >= kRows)
    y = 0;

  uint16_t i;
  i = (y * kColumns) + x;
  //Serial.print("i(");Serial.print(i);Serial.print(") = ");Serial.print(y);Serial.print(" * ");Serial.print(kColumns);Serial.print(" + ");Serial.print(x);Serial.print(" --- ");
  //Serial.flush();
  
  if( i >= sizeof(mappedTable) ) return 0;
 
  uint16_t j = mappedTable[i];
  //Serial.println(j);
  return j;

}

// The leds use this when I'm going serial
 CRGB leds[kMatrixWidth * kMatrixHeight];

// The leds use this when I'm going parallel
// even though I have 6 strips, it's faster to write 8 at a time
// than 6 one at a time
//CRGB leds[39 * 8];

// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
uint16_t speed = 20; // speed is set dynamically once we've started up

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t scale = 30; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];

CRGBPalette16 currentPalette( PartyColors_p );
uint8_t       colorLoop = 1;

void setup() {
  // uncomment the following lines if you want to see FPS count information
  // Serial.begin(38400);
  // Serial.println("resetting!");
  pinMode(A0, INPUT);
  delay(1000);

  LEDS.addLeds<WS2811_PORTC,8>(leds, longestStrip).setCorrection(TypicalSMD5050);
  /*
  LEDS.addLeds<WS2812B, pin1, GRB>(leds, strip1).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin2, GRB>(leds, strip1, strip2).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin3, GRB>(leds, strip1 + strip2, strip3).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin4, GRB>(leds, strip1 + strip2 + strip3, strip4).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin5, GRB>(leds, strip1 + strip2 + strip3 + strip4, strip5).setCorrection(TypicalSMD5050);
  LEDS.addLeds<WS2812B, pin6, GRB>(leds, strip1 + strip2 + strip3 + strip4 + strip5, strip6).setCorrection(TypicalSMD5050);
  */
  FastLED.setBrightness(255);
  set_max_power_in_volts_and_milliamps(5, 500);
  setAllColor(CRGB::Black);
  show_at_max_brightness_for_power();


  //Serial.begin(115200);
  x = random16();
  y = random16();
  z = random16();
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if( speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  
  for(int i = 0; i < MAX_DIMENSION; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < MAX_DIMENSION; j++) {
      int joffset = scale * j;
      
      uint8_t data = inoise8(x + ioffset,y + joffset,z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }
      
      noise[i][j] = data;
    }
  }
  
  z += speed;
  
  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri =   noise[i][j];

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);
      leds[XY(i,j)] = color;
    }
  }
  
  ihue+=1;
}

void setLedFromSerial()
{
  if (serialComplete)
  {
    setAllColor(CRGB::Black);
    Serial.print("is ");Serial.println(inByte);
    leds[inByte] = CRGB::Fuchsia;
    inByte = 0;
    serialComplete = false;
    show_at_max_brightness_for_power();
      //setRowColor(i, CRGB::Black);
  }
}

void setColFromSerial()
{
  if (serialComplete)
  {

    setAllColor(CRGB::Black);
    Serial.print("is ");Serial.println(inByte);
    if (inByte < kColumns)
      setColumnColor(inByte, CRGB::Red);
    leds[inByte] = CRGB::Blue;
    inByte = 0;
    serialComplete = false;
    show_at_max_brightness_for_power();
    //setRowColor(i, CRGB::Black);  
  }
}

// make all lights in a row the same color
void setRowColor(byte row, CRGB color)
{
  for(int col = 0;col < kColumns;col++)
  {
    leds[XY(col, row)] = color;
  }  
}

// make all lights in a column the same color
void setColumnColor(byte column, CRGB color)
{
  for(int row = 0;row < kRows;row++)
  {
    leds[XY(column, row)] = color;
  }
}

void listenToMic()
{
  if (analogRead(A0) > 800)
  {
    Serial.println("loud");
  }
  Serial.println(analogRead(A0));

}

void loop()
{
  //listenToMic();

  /*
  setRowColor(0, CRGB::Red);
  setRowColor(1, CRGB::Blue);
  setRowColor(2, CRGB::Green);
  setRowColor(3, CRGB::Fuchsia);
  setRowColor(4, CRGB::Yellow);
  setRowColor(5, CRGB::White);
  setRowColor(6, CRGB::Orange);
  setRowColor(7, CRGB::Teal);
  setRowColor(8, CRGB::Red);

  show_at_max_brightness_for_power(); 
  delay_at_max_brightness_for_power(500);
  */

  //setColFromSerial();
  drawNoise();
  show_at_max_brightness_for_power(); 
  delay_at_max_brightness_for_power(50);
}

// takes an 8-bit height and maps it to column
void setColumnHeight(byte column, byte height, CRGB color)
{
  // map the height to bins to save computation (lazy lazy zeke)
  const byte heightTable[9] = {0, 23, 52, 81, 110, 139, 168, 197, 226};
  for(int row = 0;row<kRows;row++)
  {
    if (height < heightTable[row])
    {
      leds[XY(column,row)] = color;
    }
    else
    {
      //leds[XY(column,row)] = CRGB::Fuchsia; 
      leds[XY(column,row)].fadeToBlackBy( 64 );
//      if (!leds[XY(column,row)])
//        leds[XY(column,row)] = CHSV(height, 255, 255); 
    }
  }

}


void drawNoise() {
  // Periodically choose a new palette, speed, and scale
  ChangePaletteAndSettingsPeriodically();

  // generate noise data
  fillnoise8();
  
  // convert the noise data to colors in the LED array
  // using the current palette
  mapNoiseToLEDsUsingPalette();

  show_at_max_brightness_for_power(); 
  delay_at_max_brightness_for_power(1);
  //LEDS.show();
  // delay(10);
}

void setAllColor(CRGB color)
{
  for(int col = 0;col<kColumns;col++)
  {
    for (int row = 0;row<kRows;row++)
    {
      leds[XY(col, row)] = color;
    }
  }
}




// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.

// 1 = 5 sec per palette
// 2 = 10 sec per palette
// etc
#define HOLD_PALETTES_X_TIMES_AS_LONG 1

void ChangePaletteAndSettingsPeriodically()
{
  static byte col = 0;
  uint8_t secondHand = ((millis() / 1000) / HOLD_PALETTES_X_TIMES_AS_LONG) % 60;
  static uint8_t lastSecond = 99;
  
  if( lastSecond != secondHand) {
    lastSecond = secondHand;
    if( secondHand ==  0)  
    { 
      currentPalette = RainbowColors_p;         
      speed = 20; 
      scale = 30; 
      colorLoop = 1; 
    }
    if( secondHand ==  5)  
    { 
      SetupPurpleAndGreenPalette();             
      speed = 10; 
      scale = 50; 
      colorLoop = 1; 
    }
    if( secondHand == 10)  
    { 
      SetupBlackAndWhiteStripedPalette();       
      speed = 20; 
      scale = 30; 
      colorLoop = 1; 
    }
    if( secondHand == 15)  
    { 
      currentPalette = ForestColors_p;          
      speed =  8; 
      scale =120; 
      colorLoop = 0; 
    }
    if( secondHand == 20)  { currentPalette = CloudColors_p;           speed =  4; scale = 30; colorLoop = 0; }
    if( secondHand == 25)  { currentPalette = LavaColors_p;            speed =  8; scale = 50; colorLoop = 0; }
    if( secondHand == 30)  { currentPalette = OceanColors_p;           speed = 20; scale = 90; colorLoop = 0; }
    if( secondHand == 35)  { currentPalette = PartyColors_p;           speed = 20; scale = 30; colorLoop = 1; }
    if( secondHand == 40)  { SetupRandomPalette();                     speed = 20; scale = 20; colorLoop = 1; }
    if( secondHand == 45)  { SetupRandomPalette();                     speed = 50; scale = 50; colorLoop = 1; }
    if( secondHand == 50)  { SetupRandomPalette();                     speed = 90; scale = 90; colorLoop = 1; }
    if( secondHand == 55)  { currentPalette = RainbowStripeColors_p;   speed = 30; scale = 20; colorLoop = 1; }
  }
}

// This function generates a random palette that's a gradient
// between four different colors.  The first is a dim hue, the second is 
// a bright hue, the third is a bright pastel, and the last is 
// another bright hue.  This gives some visual bright/dark variation
// which is more interesting than just a gradient of different hues.
void SetupRandomPalette()
{
  currentPalette = CRGBPalette16( 
                      CHSV( random8(), 255, 32), 
                      CHSV( random8(), 255, 255), 
                      CHSV( random8(), 128, 255), 
                      CHSV( random8(), 255, 255)); 
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;

}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;
  
  currentPalette = CRGBPalette16( 
    green,  green,  black,  black,
    purple, purple, black,  black,
    green,  green,  black,  black,
    purple, purple, black,  black );
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char c = (char)Serial.read(); 
    Serial.print(" ");Serial.print(c);
    if (c >= '0' && c <= '9')
    {
      inByte = (inByte * 10) + (c - '0');
    } 
    else if('\n' == c) // character is newline     
    {
      serialComplete = true;
    }
  }
}

