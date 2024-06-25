// rduino-based Car Obstacle Game
// Group 15
// Abhinav Thakur and Saima Tazreen

#include <LiquidCrystal.h>

#define CAR_MOVING 1
#define LANE_EMPTY ' '      // User the ' ' character

#define CAR_HORIZONTAL_POSITION 1    // Horizontal position of CAR on screen

#define ROAD_WIDTH 16
#define ROAD_EMPTY 0
#define ROAD_LOWER_BLOCK 1
#define ROAD_UPPER_BLOCK 2

#define PIN_SENSOR 2
#define PIN_AUTOPLAY 1
#define PIN_READWRITE 10
#define PIN_CONTRAST 12

#define CAR_POSITION_OFF 0          // CAR is invisible
#define CAR_POSITION_RUN_LOWER 1  // CAR is running on right lane //
#define CAR_POSITION_RUN_UPPER 2 // CAR is running on left lane
#define Truck_incoming 3
#define Truck_incoming_RIGHT 4
#define Truck_incoming_LEFT 5

LiquidCrystal lcd(11, 9, 6, 5, 4, 3);
static char roadUpper[ROAD_WIDTH + 1];
static char roadLower[ROAD_WIDTH + 1];
static bool sensorTouched = false;
static byte CARPos;
int HapticMotor = 7;

static byte newROAD = ROAD_EMPTY;
static byte TrafficDuration = 1;
static bool playing = false;
static bool blinking = false;
static unsigned int distance = 0;
int change = 0;

void initializeGraphics(){
  static byte graphics[] = { 
  // 1: Car on left(upper) side of road
    B00000,
    B11110,
    B11111,
    B01010,
    B00000,
    B00000,
    B00000,
    B00000,
    // 2: Car on right(bottom) side of road
    B00000,
    B11110,
    B11111,
    B01010,
    B00000,
    B00000,
    B00000,
    B00000,
    // Incoming vehicle
    B00000,
    B00000,
    B11111,
    B11111,
    B01010,
    B00000,
    B00000,
    B00000,
    // vehicle front side
    B00000,
    B00000,
    B00000,
    B00011,
    B00000,
    B00000,
    B00000,
    B00000,
    // vehicle back side
    B00000,
    B00000,
    B00000,
    B11000,
    B00000,
    B00000,
    B00000,
    B00000,

};

  int i;
  // Drawing multiple characters on LCD together
  for (i = 0; i < 7; ++i) {
	  lcd.createChar(i + 1, &graphics[i * 8]);
  }
  
  for (i = 0; i < ROAD_WIDTH; ++i) {
  	roadUpper[i] = LANE_EMPTY;
    roadLower[i] = LANE_EMPTY;
  }
  
}

// Slide the terrain to the left in half-character increments
void advanceTerrain(char* terrain, byte newTerrain){
  for (int i = 0; i < ROAD_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == ROAD_WIDTH-1) ? newTerrain : terrain[i+1];
    // No two traffic vehicles should appear together
    switch (current){
      case LANE_EMPTY:
        terrain[i] = (next == Truck_incoming) ? Truck_incoming_RIGHT : LANE_EMPTY;
        break;
      case Truck_incoming:
        terrain[i] = (next == LANE_EMPTY) ? Truck_incoming_LEFT : Truck_incoming;
        break;
      case Truck_incoming_RIGHT:
        terrain[i] = Truck_incoming;
        break;
      case Truck_incoming_LEFT:
        terrain[i] = LANE_EMPTY;
        break;
    }
  }
}

//Drawing the player's car on road
bool drawCAR(byte position, char* roadUpper, char* roadLower, unsigned int score, int lane_change) {
  bool crash = false;
  char leftSave = roadUpper[CAR_HORIZONTAL_POSITION];
  char rightSave = roadLower[CAR_HORIZONTAL_POSITION];
  byte upper, lower;
  // Check if the sensor is pushed
  if(lane_change == 0){
    // If yes, then stay on the same lane
  	switch (position) {
    	case CAR_POSITION_OFF:
      		upper = lower = LANE_EMPTY;
      		break;
    	case CAR_POSITION_RUN_UPPER:
      		upper = CAR_MOVING;
      		lower = LANE_EMPTY;
      		break;
    	case CAR_POSITION_RUN_LOWER:
      		upper = LANE_EMPTY;
      		lower = CAR_MOVING;
      		break;
  	}
  } else {
     // Else, change the lane
      if (position == CAR_POSITION_OFF) {
      	  upper = lower = LANE_EMPTY;
      } else if(position == CAR_POSITION_RUN_UPPER) {
      	  upper = LANE_EMPTY;
          lower = CAR_MOVING;  
      } else if(position == CAR_POSITION_RUN_LOWER){
          upper = CAR_MOVING;
      	  lower = LANE_EMPTY;
      }
  }
  // Check if a collision happened / car crashed
  if (upper != ' ') {
    roadUpper[CAR_HORIZONTAL_POSITION] = upper;
    crash = (leftSave == LANE_EMPTY) ? false : true;
  }
  if (lower != ' ') {
    roadLower[CAR_HORIZONTAL_POSITION] = lower;
    crash = (rightSave == LANE_EMPTY) ? false : true;
  }
  
  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;
  
  // Draw the scene
  roadUpper[ROAD_WIDTH] = '\0';
  roadLower[ROAD_WIDTH] = '\0';
  char temp = roadUpper[16-digits];
  roadUpper[16-digits] = '\0';
  lcd.setCursor(0,0);
  lcd.print(roadUpper);
  roadUpper[16-digits] = temp;  
  lcd.setCursor(0,1);
  lcd.print(roadLower);
  
  lcd.setCursor(16 - digits,0);
  lcd.print(score);

  roadUpper[CAR_HORIZONTAL_POSITION] = leftSave;
  roadLower[CAR_HORIZONTAL_POSITION] = rightSave;
  return crash;
}

// Handle the Sensor touch as an interrupt
void touch() {
  sensorTouched = true;
}

void setup(){
  // Mapping pins
  pinMode(PIN_READWRITE, OUTPUT);
  digitalWrite(PIN_READWRITE, LOW);
  pinMode(PIN_CONTRAST, OUTPUT);
  digitalWrite(PIN_CONTRAST, LOW);
  pinMode(PIN_SENSOR, INPUT);
  digitalWrite(PIN_SENSOR, HIGH);
  pinMode(PIN_AUTOPLAY, OUTPUT);
  digitalWrite(PIN_AUTOPLAY, HIGH);
  pinMode(HapticMotor, OUTPUT);
  digitalWrite(HapticMotor, LOW);
  
  // Digital pin 2 maps to interrupt 0
  attachInterrupt(0/*PIN_BUTTON*/, touch, FALLING);
  
  initializeGraphics();
  
  lcd.begin(16, 2);
}

void loop(){
  
  if (!playing) { // when game not started
    drawCAR((blinking) ? CAR_POSITION_OFF : CARPos, roadUpper, roadLower, distance >> 3, change);
    if (blinking) {
      lcd.setCursor(0,0);
      lcd.print("Touch To Play");
    }
    delay(250);
    blinking = !blinking;
    if (sensorTouched) {  // If sensor is touched, game starts
      initializeGraphics();
      CARPos = CAR_POSITION_RUN_LOWER;
      playing = true;
      sensorTouched = false;
      distance = 0;
      change = 0;
    }
    return;
  }

  // Shift the terrain to the left
  advanceTerrain(roadLower, newROAD == ROAD_LOWER_BLOCK ? Truck_incoming : LANE_EMPTY);
  advanceTerrain(roadUpper, newROAD == ROAD_UPPER_BLOCK ? Truck_incoming : LANE_EMPTY);

  // Add traffic randomly to the road
  if (--TrafficDuration == 0) {
    if (newROAD == ROAD_EMPTY) {
      newROAD = (random(2) == 0) ? ROAD_UPPER_BLOCK : ROAD_LOWER_BLOCK;
      TrafficDuration = 2 + random(2);
    } else {
      newROAD = ROAD_EMPTY;
      TrafficDuration = 10 + random(2);
    }
  }
  
  if (sensorTouched) { // If sensor touched, change the lane
    change = 1;
    sensorTouched = false;
  }  

  if (drawCAR(CARPos, roadUpper, roadLower, distance >> 3, change)) {
    digitalWrite(HapticMotor, HIGH); // start vibrating if crash happened
    delay(1000);  // delay one second
    digitalWrite(HapticMotor, LOW);  //stop vibrating
    playing = false; // Crash happened!!
  } else { // The game keeps going
    // Change the car position variable as per the change in car's lane
    if (change == 1 && CARPos == CAR_POSITION_RUN_LOWER) {
     	CARPos = CAR_POSITION_RUN_UPPER;
    } else if (change == 1 && CARPos == CAR_POSITION_RUN_UPPER) {
      	CARPos = CAR_POSITION_RUN_LOWER;
    } else if ((change == 1) && roadLower[CAR_HORIZONTAL_POSITION] != LANE_EMPTY) {
      	CARPos = CAR_POSITION_RUN_UPPER;
    } else if ((change == 1) && roadUpper[CAR_HORIZONTAL_POSITION] != LANE_EMPTY) {
      	CARPos = CAR_POSITION_RUN_LOWER;
    } else if (CARPos == CAR_POSITION_RUN_LOWER) {
        CARPos = CAR_POSITION_RUN_LOWER;
    } else if (CARPos == CAR_POSITION_RUN_UPPER) {
        CARPos = CAR_POSITION_RUN_UPPER;
    } else {
      ++CARPos;
    }
    ++distance;
    change = 0;
    
    digitalWrite(PIN_AUTOPLAY, roadLower[CAR_HORIZONTAL_POSITION + 2] == LANE_EMPTY ? HIGH : LOW);
  }
  delay(50);
}