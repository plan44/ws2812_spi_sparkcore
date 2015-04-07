/*
 * Spark Core library to control WS2812 based RGB LED devices
 * using SPI to create bitstream.
 * Future plan is to use DMA to feed the SPI, so WS2812 bitstream
 * can be produced without CPU load and without blocking IRQs
 *
 * (c) 2014-2015 by luz@plan44.ch (GPG: 1CC60B3A)
 * Licensed as open source under the terms of the MIT License
 * (see LICENSE.TXT)
 */


// Declaration (would go to .h file once library is separated)
// ===========================================================

class p44_ws2812 {

  typedef struct {
    unsigned int red:5;
    unsigned int green:5;
    unsigned int blue:5;
  } __attribute((packed)) RGBPixel;

  uint16_t numLeds; // number of LEDs
  RGBPixel *pixelBufferP; // the pixel buffer
  uint16_t ledsPerRow; // number of LEDs per row
  bool xReversed; // even (0,2,4...) rows go backwards, or all if not alternating
  bool alternating; // direction changes after every row


public:
  /// create driver for a WS2812 LED chain
  /// @param aNumLeds number of LEDs in the chain
  /// @param aLedsPerRow number of LEDs in a row (x size in a X/Y arrangement of the LEDs)
  /// @param aXReversed X direction is reversed
  /// @param aAlternating X direction is reversed in first row, normal in second, reversed in third etc..
  p44_ws2812(uint16_t aNumLeds, uint16_t aLedsPerRow=0, bool aXReversed=false, bool aAlternating=false);

  /// destructor
  ~p44_ws2812();

  /// begin using the driver
  void begin();

  /// transfer RGB values to LED chain
  /// @note this must be called to update the actual LEDs after modifying RGB values
  /// with setColor() and/or setColorDimmed()
  void show();

  /// set color of one LED
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  void setColorXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue);
  void setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue);

  /// set color of one LED, scaled by a visible brightness (non-linear) factor
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  /// @param aBrightness brightness, will be converted non-linear to PWM duty cycle for uniform brightness scale, 0..255
  void setColorDimmedXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue, byte aBrightness);
  void setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness);

  /// get current color of LED
  /// @param aRed set to intensity of red component, 0..255
  /// @param aGreen set to intensity of green component, 0..255
  /// @param aBlue set to intensity of blue component, 0..255
  /// @note for LEDs set with setColorDimmed(), this returns the scaled down RGB values,
  ///   not the original r,g,b parameters. Note also that internal brightness resolution is 5 bits only.
  void getColorXY(uint16_t aX, uint16_t aY, byte &aRed, byte &aGreen, byte &aBlue);
  void getColor(uint16_t aLedNumber, byte &aRed, byte &aGreen, byte &aBlue);

  /// @return number of LEDs
  int getNumLeds();

private:

  uint16_t ledIndexFromXY(uint16_t aX, uint16_t aY);


};



// Implementation (would go to .cpp file once library is separated)
// ================================================================

static const uint8_t pwmTable[32] = {0, 1, 1, 2, 3, 4, 6, 7, 9, 10, 13, 15, 18, 21, 24, 28, 33, 38, 44, 50, 58, 67, 77, 88, 101, 115, 132, 150, 172, 196, 224, 255};

p44_ws2812::p44_ws2812(uint16_t aNumLeds, uint16_t aLedsPerRow, bool aXReversed, bool aAlternating)
{
  numLeds = aNumLeds;
  if (aLedsPerRow==0)
    ledsPerRow = aNumLeds; // single row
  else
    ledsPerRow = aLedsPerRow; // set row size
  xReversed = aXReversed;
  alternating = aAlternating;
  // allocate the buffer
  if((pixelBufferP = new RGBPixel[numLeds])!=NULL) {
    memset(pixelBufferP, 0, sizeof(RGBPixel)*numLeds); // all LEDs off
  }
}

p44_ws2812::~p44_ws2812()
{
  // free the buffer
  if (pixelBufferP) delete pixelBufferP;
}


int p44_ws2812::getNumLeds()
{
  return numLeds;
}


void p44_ws2812::begin()
{
  // begin using the driver
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8); // System clock is 72MHz, we need 9MHz for SPI
  SPI.setBitOrder(MSBFIRST); // MSB first for easier scope reading :-)
  SPI.transfer(0); // make sure SPI line starts low (Note: SPI line remains at level of last sent bit, fortunately)
}

void p44_ws2812::show()
{
  // Note: on the spark core, system IRQs might happen which exceed 50uS
  // causing WS2812 chips to reset in midst of data stream.
  // Thus, until we can send via DMA, we need to disable IRQs while sending
  __disable_irq();
  // transfer RGB values to LED chain
  for (uint16_t i=0; i<numLeds; i++) {
    RGBPixel *pixP = &(pixelBufferP[i]);
    byte b;
    // Order of PWM data for WS2812 LEDs is G-R-B
    // - green
    b = pwmTable[pixP->green];
    for (byte j=0; j<8; j++) {
      SPI.transfer(b & 0x80 ? 0x7E : 0x70);
      b = b << 1;
    }
    // - red
    b = pwmTable[pixP->red];
    for (byte j=0; j<8; j++) {
      SPI.transfer(b & 0x80 ? 0x7E : 0x70);
      b = b << 1;
    }
    // - blue
    b = pwmTable[pixP->blue];
    for (byte j=0; j<8; j++) {
      SPI.transfer(b & 0x80 ? 0x7E : 0x70);
      b = b << 1;
    }
  }
  __enable_irq();
}


uint16_t p44_ws2812::ledIndexFromXY(uint16_t aX, uint16_t aY)
{
  uint16_t ledindex = aY*ledsPerRow;
  bool reversed = xReversed;
  if (alternating) {
    if (aY & 0x1) reversed = !reversed;
  }
  if (reversed) {
    ledindex += (ledsPerRow-1-aX);
  }
  else {
    ledindex += aX;
  }
  return ledindex;
}


void p44_ws2812::setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue)
{
  int y = aLedNumber / ledsPerRow;
  int x = aLedNumber % ledsPerRow;
  setColorXY(x, y, aRed, aGreen, aBlue);
}


void p44_ws2812::setColorXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue)
{
  uint16_t ledindex = ledIndexFromXY(aX,aY);
  if (ledindex>=numLeds) return;
  RGBPixel *pixP = &(pixelBufferP[ledindex]);
  // linear brightness is stored with 5bit precision only
  pixP->red = aRed>>3;
  pixP->green = aGreen>>3;
  pixP->blue = aBlue>>3;
}


void p44_ws2812::setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness)
{
  int y = aLedNumber / ledsPerRow;
  int x = aLedNumber % ledsPerRow;
  setColorDimmedXY(x, y, aRed, aGreen, aBlue, aBrightness);
}


void p44_ws2812::setColorDimmedXY(uint16_t aX, uint16_t aY, byte aRed, byte aGreen, byte aBlue, byte aBrightness)
{
  setColorXY(aX, aY, (aRed*aBrightness)>>8, (aGreen*aBrightness)>>8, (aBlue*aBrightness)>>8);
}


void p44_ws2812::getColor(uint16_t aLedNumber, byte &aRed, byte &aGreen, byte &aBlue)
{
  int y = aLedNumber / ledsPerRow;
  int x = aLedNumber % ledsPerRow;
  getColorXY(x, y, aRed, aGreen, aBlue);
}


void p44_ws2812::getColorXY(uint16_t aX, uint16_t aY, byte &aRed, byte &aGreen, byte &aBlue)
{
  uint16_t ledindex = ledIndexFromXY(aX,aY);
  if (ledindex>=numLeds) return;
  RGBPixel *pixP = &(pixelBufferP[ledindex]);
  // linear brightness is stored with 5bit precision only
  aRed = pixP->red<<3;
  aGreen = pixP->green<<3;
  aBlue = pixP->blue<<3;
}



// Main program, example showing a color cycle
// ===========================================




// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void wheel(byte WheelPos, byte &red, byte &green, byte &blue) {
  if(WheelPos < 85) {
    red = WheelPos * 3;
    green = 255 - WheelPos * 3;
    blue = 0;
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    red = 255 - WheelPos * 3;
    green = 0;
    blue = WheelPos * 3;
  } else {
    WheelPos -= 170;
    red = 0;
    green = WheelPos * 3;
    blue = 255 - WheelPos * 3;
  }
}

byte cnt = 0;


p44_ws2812 leds(240); // for 4m strip with 240 LEDs

void setup() {
  leds.begin();
}


void loop() {

  byte r,g,b;

  // create color cycle
  for(int i=0; i<leds.getNumLeds(); i++) {
    wheel(((i * 256 / leds.getNumLeds()) + cnt) & 255, r, g, b);
    leds.setColorDimmed(i, r, g, b, 128);
  }

  leds.show();

  cnt++;
  delay(1); // latch & reset needs 50 microseconds pause, at least.
}