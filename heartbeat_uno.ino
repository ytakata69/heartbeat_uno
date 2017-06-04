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
//  8, 7, 6, 5,  9, 10, 11, 12 // Dotsduino 1.2d flipped
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

const int N_SHAPE = 2;

const byte shapeA[] = {
  B00000000,
  B00000001,
  B00000011,
  B00000111,
  B00001111,
  B00000111,
  B00000011,
  B00000001,
};

const byte shapeB[] = {
  B00010000,
  B00110000,
  B01110000,
  B11110000,
  B01110000,
  B00110000,
  B00010000,
  B00000000,
};

const byte *shape[] = { shapeA, shapeB };

const byte shapeH[] = {
  B00000000,
  B01100110,
  B11111111,
  B11111111,
  B11111111,
  B01111110,
  B00111100,
  B00011000,
};

const byte shapeO[] = {
  B01111110,
  B10000001,
  B10100101,
  B10000001,
  B10011101,
  B10000001,
  B01111110,
  B00000000,
};

const byte *shapeHO[] = { shapeH, shapeO };

// The current lighting row. Only one row is lighting at every moment.
int row;

// Variables for controlling the brightness of LEDs
int brightnessCount = 0;
int fadeDelayCount = 0;
const int fadeDelay = 30;
int valleyCount = 0;

// accumulator for software PWM
int pwmAcc[N_SHAPE];
int pwmCarry[N_SHAPE];
const int pwmBits = 5;

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

inline int brightness(int t) {
  t += 128;
  return t < 512 ? t - 128 : 896 - t;
}

void loop()
{
  int i, j;
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
    for (i = 0; i < N_SHAPE; i++) {
      int b = brightness(brightnessCount - i * 256);

      // Shrink the range of brightness to 0..255.
      int out = (b > 256 ? 256 :
                 b <   0 ?   0 : b);
      // Use only 5 bits.
      out = out >> (8 - pwmBits);
      // The following code makes the PWM smoother.
      if (out == 1) {
        pwmAcc[i] = (1 << pwmBits) / 2;
      }
      // Accumulate the value and obtain the carry.
      pwmAcc[i] += out;
      pwmCarry[i] = pwmAcc[i] & (1 << pwmBits);
      pwmAcc[i] &= (1 << pwmBits) - 1;
    }
  }

  // Set the brightness of each column.
  mask = 0x80;
  for (i = 0; i < numOfCols; i++) {
    int out = ROWS_ARE_ANODES;
    if (valleyCount < 3) {
      for (j = 0; j < N_SHAPE; j++) {
        if (shape[j][row] & mask) {
          out = (pwmCarry[j] ? ! out : out);
          break;
        }
      }
    } else {
        if (shapeHO[valleyCount - 3][row] & mask) {
          out = (pwmCarry[0] ? ! out : out);
        }
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
      brightnessCount++;
      if (brightnessCount >= 1024 + (valleyCount < 3 ? 256 : 0)) {
        brightnessCount = 0;
        valleyCount++;
      }
      fadeDelayCount = 0;
    }
  } else {
    brightnessCount = 512;
  }

  int val = (USE_VOLUME ? analogRead(volumePin) : 1);
  for (i = 0; i < val; i++) {
    delayMicroseconds(50);
  }

  if (valleyCount >= 5) {
    valleyCount = 0;
    delay(4000);
  }
}

