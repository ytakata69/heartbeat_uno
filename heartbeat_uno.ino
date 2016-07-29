// We assume that the controller is Arduino UNO.
// The ports 2 to 19 are for Digital I/O.
// The ports 14 to 19 are also for Analog input.

// Settings for Dotsduino:
//   Board: Arduino Pro or Pro Mini
//   Processor: ATmega328 (3.3V, 8MHz)
// Dots::init12d() etc. in Dots.cpp describes the
// Dotsduino pin assignment.

// The output port for each row
const byte rowPins[10] = {
//  2, 3, 4, 5,  6, 7, 8, 9 // Arduino UNO
    2, 3, 4, 13,  14, 15, 16, 17 // Dotsduino 1.2d
};
// The output port for each column
const byte colPins[8] = {
//  10, 11, 12, 13,  17, 16, 15, 14 // Arduino UNO
    12, 11, 10, 9,  5, 6, 7, 8 // Dotsduino 1.2d
};
const boolean ROWS_ARE_ANODES = 
//  false;  // Arduino UNO
    true;   // Dotsduino 1.2d

// Input pins
const int volumePin = 4;  // Analog 4
const int switchPin = 19; // Analog 5 = Digital 19
const boolean USE_VOLUME =
//  true;  // Arduino connected with a volume
    false; // Dotsduino

const int numOfRows = 8;
const int numOfCols = 8;

const byte shapeRegular[] = {
  B00000000,
  B01100110,
  B11111111,
  B11111111,
  B11111111,
  B01111110,
  B00111100,
  B00011000,
};
const byte shapeTumble[] = {
  B00111000,
  B01111100,
  B01111110,
  B00111111,
  B00111111,
  B01111110,
  B01111100,
  B00111000,
};

const byte *shape = shapeRegular;
//const byte *shape = shapeTumble;

// The current lighting row. Only one row is lighting at every moment.
int row;

// Variables for controlling the brightness of LEDs
int brightness = 0;
int fadeAmount = 1;
int fadeDelayCount = 0;
const int fadeDelay = 30;
int valleyCount = 0;

// accumulator for DSM
int dsmAcc = 0;
int dsmCarry = 0;
const int dsmBits = 5;

void setup()
{
  int i;
  // Set up the pin mode for output.
  for (i = 0; i < numOfRows; i++) {
    pinMode(rowPins[i], OUTPUT);
  }
  for (i = 0; i < numOfCols; i++) {
    pinMode(colPins[i], OUTPUT);
  }
  pinMode(switchPin, INPUT);
  row = 0;
  
  // Turn off all LEDs.
  if (ROWS_ARE_ANODES) {
    for (i = 0; i < numOfRows; i++) {
      digitalWrite(rowPins[i], LOW);
    }
    for (i = 0; i < numOfCols; i++) {
      digitalWrite(colPins[i], HIGH);
    }
  } else {
    for (i = 0; i < numOfCols; i++) {
      digitalWrite(colPins[i], LOW);
    }
    for (i = 0; i < numOfRows; i++) {
      digitalWrite(rowPins[i], HIGH);
    }
  }
}

void loop()
{
  int i;
  uint8_t mask;
  
  // Write ROWS_ARE_ANODE to a rowPin and its complement to a colPin,
  // then the LED lights.

  // Shutdown the previous row.
  digitalWrite(rowPins[row], ! ROWS_ARE_ANODES);

  // Go to the next row.
  row++;
  if (row >= numOfRows) {
    row = 0;
  }

  // Compute the carry bit per one frame.
  if (row == 0) {
    // Shrink the range of brightness to 0..255.
    int out = (brightness > 256 ? 256 :
               brightness <   0 ?   0 : brightness);
    // Use only 5 bits.
    out = out >> (8 - dsmBits);
    // The following code makes the DSM smoother.
    if (out == 1) {
      dsmAcc = (1 << dsmBits) / 2;
    }
    // Accumulate the value and obtain the carry.
    dsmAcc += out;
    dsmCarry = dsmAcc & (1 << dsmBits);
    dsmAcc &= (1 << dsmBits) - 1;
  }

  // Set the brightness of each column.
  mask = 0x80;
  for (i = 0; i < numOfCols; i++) {
    int out = ROWS_ARE_ANODES;
    if (shape[row] & mask) {
      out = (dsmCarry ? ! out : out);
    }
    digitalWrite(colPins[i], out);
    mask >>= 1;
  }
  
  // Light the row.
  digitalWrite(rowPins[row], ROWS_ARE_ANODES);

  // Change the brightness
  if (! USE_VOLUME || digitalRead(switchPin)) {
    fadeDelayCount++;
    if (fadeDelayCount >= fadeDelay) {
      brightness = brightness + fadeAmount;
      if (brightness == -128 || brightness == 256 + 128) {
        fadeAmount = -fadeAmount;
        if (brightness == -128) valleyCount++;
      }
      fadeDelayCount = 0;
    }
  } else {
    brightness = 256;
  }

  int val = (USE_VOLUME ? analogRead(volumePin) : 1);
  for (i = 0; i < val; i++) {
    delayMicroseconds(50);
  }

  if (valleyCount >= 3) {
    valleyCount = 0;
    delay(4000);
  }
}

