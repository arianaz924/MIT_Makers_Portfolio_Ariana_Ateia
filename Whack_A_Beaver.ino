#include <LiquidCrystal.h> 

const int switchPin = A4; 
const int buttonPin = 6;  

const int clusters = 4; 
const int pressThreshold = 400; 
const unsigned long INPUT_TIMEOUT_MS = 5000;

// 2D Array Mapping: Row = Cluster ID | Column = {Red Pin, Green Pin, Blue Pin}
const int clusterPins[clusters][3] = 
{
  {2,  3,  4},   // Cluster 0
  {5,  7,  8},   // Cluster 1 
  {9,  10, 11},  // Cluster 2
  {12, 13, A0}   // Cluster 3 
};

// Array Mapping for the 4 Analog Pressure Sensors
const int sensorPins[clusters] = {A1, A2, A3, A4}; // Re-mapped safely

// Separate Memory Preview Cluster Pins
const int memoryRed = 0;
const int memoryGreen = 1;
const int memoryBlue = A5; 

// Initializing LCD with completely clear, independent pins
// RS, EN, D4, D5, D6, D7 layout structure matching standard wiring
LiquidCrystal lcd(14, 15, 16, 17, 18, 19); // Uses analog lines as clean digital outs

// Game logic trackers
const int maxLevel = 5;
int gameSequence[maxLevel];
int currentLevel = 1;
bool gameRunning = false; // Flag to prevent the game from auto-running instantly

void setup() {
  // Set all 12 RGB LED controller channels as OUTPUT
  for (int c = 0; c < clusters; c++) 
  {
    pinMode(clusterPins[c][0], OUTPUT); // Red
    pinMode(clusterPins[c][1], OUTPUT); // Green
    pinMode(clusterPins[c][2], OUTPUT); // Blue
  }

  // Set all 4 pressure sensor lines as INPUT
  for (int s = 0; s < clusters; s++)
  {
    pinMode(sensorPins[s], INPUT);
  }
  
  pinMode(switchPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Using pullup for clean button handling
  
  pinMode(memoryRed, OUTPUT);
  pinMode(memoryGreen, OUTPUT);
  pinMode(memoryBlue, OUTPUT);

  // Use raw thermal static to generate random seed layouts
  randomSeed(analogRead(A5)); 

  // Initialize LCD parameters 
  lcd.begin(20, 4);
  
  // Master Switch evaluation layer
  if (digitalRead(switchPin) == HIGH)
  {
    turnOnMachine();
  }
}

void loop() {
  // If the machine switch is off, freeze everything
  if (digitalRead(switchPin) == LOW) {
    lcd.clear();
    lcd.print("Machine Off");
    delay(500);
    return;
  }

  //NOTE: Depends on our wiring (PUll UP vs PULL DOWN) 
  static int lastButtonState = HIGH; 
  int currentButtonState = digitalRead(buttonPin);
  
  // Watch for the physical push button action to launch standard initialization
  if (lastButtonState == HIGH && currentButtonState == LOW && !gameRunning) 
  {
    delay(50); // Simple debounce window
    gameStart();
  }
  lastButtonState = currentButtonState;

  // Only run the Simon loop sequences if the start sequence has officially cleared
  if (gameRunning) {
    // 1. Run the memorization countdown before showing the sequence
    runMemorizationCountdown();

    // 2. Pick a random number between 0 and 3
    gameSequence[currentLevel - 1] = random(0, clusters);
    
    // 3. Play back the memory sequence ONLY on the memory cluster
    showSequence();
  
    // 4. Collect and verify user step replies on the sensors
    if (getPlayerInput()) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Correct!");
      delay(1000);
      currentLevel++; 
      
      if(currentLevel > maxLevel) {
        lcd.clear();
        lcd.print("YOU WIN THE GAME!");
        delay(3000);
        resetGameDisplay();
      }
    } else {
      // Game over handling is safely contained inside input loops
    }
  }
}

void turnOnMachine(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Push Button to Start");
  lcd.setCursor(0, 1);
  lcd.print("System Ready...");
}

void gameStart(){
  gameRunning = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: 0");
  lcd.setCursor(0, 1);
  lcd.print("Get Ready!");
  
  // Cool visual effect: Flash random colors on the memory cluster before starting
  for(int i=0; i<10; i++){
    flashMemoryCluster(random(0,4), 100);
    delay(50);
  }
  delay(1000);
}

void runMemorizationCountdown() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Watch Memory Light");
  
  for (int i = 3; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Starting in: ");
    lcd.print(i);
    delay(1000); 
  }
}

void showSequence() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(currentLevel);
  lcd.setCursor(0, 1);
  lcd.print("Memorizing...");
  delay(500);

  for (int i = 0; i < currentLevel; i++) {
    //Flashes ONLY the memory cluster module
    flashMemoryCluster(gameSequence[i], 600);
    delay(250); 
  }
}

bool getPlayerInput() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Your turn!");

  for (int i = 0; i < currentLevel; i++) {
    int pressedCluster = -1;
    unsigned long stepStartTime = millis(); 
    long lastDisplayedSeconds = -1;

    while (pressedCluster == -1) {
      unsigned long timeElapsed = millis() - stepStartTime;
      long secondsLeft = (INPUT_TIMEOUT_MS - timeElapsed) / 1000;
      if (secondsLeft < 0) secondsLeft = 0;

      if (secondsLeft != lastDisplayedSeconds) {
        lcd.setCursor(0, 1);
        lcd.print("Time Left: ");
        lcd.print(secondsLeft);
        lcd.print("s  "); 
        lastDisplayedSeconds = secondsLeft;
      }

      if (timeElapsed >= INPUT_TIMEOUT_MS) {
        timeOutGameOver();
        return false; 
      }

      for (int c = 0; c < clusters; c++) {
        if (analogRead(sensorPins[c]) > pressThreshold) {
          pressedCluster = c;
          break;
        }
      }
    }

    // Displays the color match layout sequence explicitly across elements
    flashCluster(pressedCluster, 300);

    bool padIsHeld = true;
    while (padIsHeld) {
      padIsHeld = false;
      for (int c = 0; c < clusters; c++) {
        if (analogRead(sensorPins[c]) > (pressThreshold - 100)) {
          padIsHeld = true; 
        }
      }
      delay(10);
    }
    delay(100); 

    if (pressedCluster != gameSequence[i]) {
      wrongChoiceGameOver();
      return false; 
    }
  }
  return true; 
}

void flashMemoryCluster(int patternID, int duration) {
  digitalWrite(memoryRed, LOW);
  digitalWrite(memoryGreen, LOW);
  digitalWrite(memoryBlue, LOW);

  if (patternID == 0)      digitalWrite(memoryRed, HIGH);                 // 0 = Red
  else if (patternID == 1) digitalWrite(memoryBlue, HIGH);                // 1 = Blue
  else if (patternID == 2) digitalWrite(memoryGreen, HIGH);               // 2 = Green
  else if (patternID == 3) { digitalWrite(memoryRed, HIGH); digitalWrite(memoryGreen, HIGH); } // 3 = Yellow
  
  delay(duration);
  digitalWrite(memoryRed, LOW);
  digitalWrite(memoryGreen, LOW);
  digitalWrite(memoryBlue, LOW);
}

void setClusterRGB(int clusterID, int r, int g, int b) {
  digitalWrite(clusterPins[clusterID][0], r);
  digitalWrite(clusterPins[clusterID][1], g);
  digitalWrite(clusterPins[clusterID][2], b);
}

void flashCluster(int clusterID, int duration) {
  setClusterRGB(clusterID, LOW, LOW, LOW);
  
  // Beaver clusters color mix perfectly matches the memory cluster rules
  if (clusterID == 0)      setClusterRGB(clusterID, HIGH, LOW, LOW);  // Red
  else if (clusterID == 1) setClusterRGB(clusterID, LOW, LOW, HIGH);  // Blue
  else if (clusterID == 2) setClusterRGB(clusterID, LOW, HIGH, LOW);  // Green
  else if (clusterID == 3) setClusterRGB(clusterID, HIGH, HIGH, LOW); // Yellow
  
  delay(duration);
  setClusterRGB(clusterID, LOW, LOW, LOW);
}

void timeOutGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TIME OUT!");
  lcd.setCursor(0, 1);
  lcd.print("Too slow!");
  delay(2000);
  resetGameDisplay();
}

void wrongChoiceGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WRONG BEAVER!");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(currentLevel - 1);
  
  for (int i = 0; i < 4; i++) {
    digitalWrite(memoryRed, HIGH);
    for (int c = 0; c < clusters; c++) setClusterRGB(c, HIGH, LOW, LOW);
    delay(200);
    digitalWrite(memoryRed, LOW);
    for (int c = 0; c < clusters; c++) setClusterRGB(c, LOW, LOW, LOW);
    delay(200);
  }
  resetGameDisplay();
}

void resetGameDisplay() {
  currentLevel = 1; 
  gameRunning = false;
  delay(1500);
  turnOnMachine();
}
