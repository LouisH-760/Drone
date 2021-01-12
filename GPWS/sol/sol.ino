// pins
#define WARNING 12
#define DANGER 13
#define STATUS 4
#define TRIG 5
#define ECHO 4
// other constants
// delay for on-time for tests
#define TEST_DELAY 100
// delay for the blinking after a malfunction
#define MALFUNC_DELAY 500
// Speed of sound
// CENTIMETERS PER MICROSECOND
#define SOUND 0.034

#define BAUDS 9600
// heights for different stuff
// PLACEHOLDER VALUES!
// GROUND <-> danger zone <-> MAX_DANGER <-> warning zone <-> MAX_WARNING <-> normal flight <-> +inf
#define GROUND 0
#define MAX_DANGER 50
#define MAX_WARNING 100
#define MAX_START_HEIGHT 15
#define MIN_REAL_HEIGHT 0
// variables
int height;
long duration;

/**
 * Test Digital Outputs
 * 
 * args: int count (number of arguments given), ... (pin numbers, int expected)
 * 
 * go through all the pins given in the arguments
 * for each pin, set it to HIGH for 100ms then set it back to LOW
 * 
 * delay given by TEST_DELAY in define
 */
void test(int count, ...) {
  va_list args;
  int current;
  // start the argument list use
  va_start(args, count);
  // for each given pin
  for(int i = 0; i < count; i++) {
    // get the pin number as int from the argument list (pop)
    current = va_arg(args, int);
    // set to high
    digitalWrite(current, HIGH);
    // wait
    delay(TEST_DELAY);
    // set to low
    digitalWrite(current, LOW);
    // delay a bit between tests
    delay(TEST_DELAY);
  }
  // end the argument list use, should clear memory
  va_end(args);
}

/**
 * Permet, si le circuit malfonctionne, de bloquer toute éxcution autre que "faire clignoter" de beginPin à endPin (inclus)
 * args: beginPin(int), endPin(int)
 * 
 * NO PIN SKIPPING
 */
void malfunction(int beginPin, int endPin) {
  // actual code
  while(true) {
      for(int i = beginPin; i <= endPin; i++) {
          digitalWrite(i, HIGH);
      }
      delay(MALFUNC_DELAY);
      for(int i = beginPin; i <= endPin; i++) {
        digitalWrite(i, LOW);
      }
      delay(MALFUNC_DELAY);
  }

}

/**
 * Read the height from a sensor and update the value of the variable at the passed adress with it
 * args: h (pointer to the (int) height variable
 */
void updateHeight(int *h) {
  // clear Trig
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  // send 10 ms inpulse
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // read return delay
  duration = pulseIn(ECHO, HIGH);
  height = duration*SOUND/2;
}

void setup() {
  // remove for production code!
  // serial communication setup
  Serial.begin(BAUDS);
  Serial.println("START==========================================");
  
  // set WARNING as OUTPUT
  pinMode(WARNING, OUTPUT);

  // set DANGER as OUTPUT
  pinMode(DANGER, OUTPUT);
  
  // set STATUS as OUTPUT
  pinMode(STATUS, OUTPUT);

  // set trig as output
  pinMode(TRIG, OUTPUT);

  // set echo as input

 pinMode(ECHO, INPUT);

  // test all LEDS
  test(3, WARNING, DANGER, STATUS);

  // temp debugging inputs
  pinMode(12, INPUT);
  pinMode(13, INPUT);

  // temp debugging start height, replace by sensor read when available
  height = 0;

  // if the height is superior to something reasonable at startup,
  // or the height is inferior to a reasonable height
  // signal something might be wrong to the user and don't start up
  if(height > MAX_START_HEIGHT || height < MIN_REAL_HEIGHT) {
    malfunction(WARNING, STATUS);
  }
    
  // set STATUS to high, it should be at all times
  digitalWrite(STATUS, HIGH);
}

void loop() {
  updateHeight(&height);
  // if the sensor gives an unreasonable value during flight, go into malfunction mode
  // eventually replace by a malfunction counter if common, and go into failure mode after a certain amount
  if(height < MIN_REAL_HEIGHT) {
    malfunction(WARNING, STATUS);
  }
  // write the height to serial for debugging
  Serial.print("Altitude: ");
  Serial.print(height);
  Serial.print("\n");
  // actual processing
  if(height <= MAX_DANGER) {
    digitalWrite(WARNING, LOW);
    digitalWrite(DANGER, HIGH);
  } else if(height > MAX_DANGER && height <= MAX_WARNING) {
    digitalWrite(DANGER, LOW);
    digitalWrite(WARNING, HIGH);
  } else {
    digitalWrite(WARNING, LOW);
    digitalWrite(DANGER, LOW);
  }
}
