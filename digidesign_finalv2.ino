#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>

// password
const String defaultPassword = "ABC12345";
const String configurePassword = "CBA54321";
const byte passwordLength = 9; // 8 chars plus terminator
String password;
String input;
String oldPass;
char customKey;

// keypad stuff
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'D', '#', '0', '*'},
  {'C', '9', '8', '7'},
  {'B', '6', '5', '4'},
  {'A', '3', '2', '1'}
};

byte rowPins[ROWS] = {4, 5, 6, 7}; 
byte colPins[COLS] = {8, 9, 10, 11}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

// lcd stuff
LiquidCrystal_I2C lcd(0x27, 16, 2);

// buzzer
const byte buzzerPin = 13;

// counters
byte charNum = 0;
byte tryCount = 0;

// servo
Servo servo;
const byte angle = 95;

// sms
const byte messagePin = 2;
const byte stopPin = 12;

void setup(){
  Serial.begin(9600);
  
  // lcd setup
  lcd.init();
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on

  // buzzer setup
  pinMode(buzzerPin, OUTPUT);

  // Greeting message
  lcd.setCursor(5, 0);
  lcd.print("Hello!");
  tone(buzzerPin, 600);
  delay(2000);

  // set lcd text for input
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter password:");

  // servo
  servo.attach(3);
  servo.write(angle);
  delay(50);

  // password setup
  if (isEmptyEEPROM()) { // if EEPROM is empty, set EEPROM values to default password
    char passwordChars[passwordLength];
    defaultPassword.toCharArray(passwordChars, passwordLength);

    for(int x = 0; x < passwordLength; x++) {
      EEPROM.update(x, passwordChars[x]);
    }
  }

  char readPass[passwordLength];

  for(int x = 0; x < passwordLength; x++) {
    readPass[x] = EEPROM.read(x);
  }

  password = String(readPass);
  input = "";

}
  
void loop(){
  if (tryCount > 4) { // starts alarm sequence

    servo.write(angle); // make sure container is locked
    delay(50);
    digitalWrite(messagePin, HIGH); // send signal to gsm module

    while (true) {

      // modified confirm password method for continuous alarm
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try limit maxxed");
      lcd.setCursor(1, 1);
      lcd.print("Pwd to disable");
      
      for (int x = 0; x < 3; x++) {
        alarm();
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Old Password:");

      charNum = 0;
      oldPass = "";

      while (charNum < passwordLength - 1) {
        customKey = customKeypad.getKey();

        if(customKey) {

          lcd.setCursor(charNum, 1);
          lcd.print(customKey);

          oldPass = oldPass + customKey;
          charNum++;

        }
      }

      if (oldPass.equals(password)) {
        noTone(buzzerPin);

        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Alarm disabled");
        delay(2000);

        tryCount = 0;
        digitalWrite(messagePin, LOW); // turn off signal to gsm module
        digitalWrite(stopPin, HIGH); // reset gsm module
        delay(100);
        digitalWrite(stopPin, LOW); // reset gsm module
        break;
      }

    }

      // resets inputs and lcd after alarm
      input = "";
      charNum = 0;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter password:");
  }

  noTone(buzzerPin);
  customKey = customKeypad.getKey();
  
  if (customKey) {

    tone(buzzerPin, 440); // A4
    delay(100);
    noTone(buzzerPin);

    lcd.setCursor(charNum, 1);
    lcd.print(customKey);

    input = input + customKey;
    charNum++;

    // if input is 8 characters long, compare against password string
    if (charNum == 8) {
      
      tryCount++;

      if (input.equals(configurePassword)) {

        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("New/Reset PWD?");
        lcd.setCursor(0, 1);
        lcd.print("A = New  B = RST");

        while (true) {
          customKey = customKeypad.getKey();

          if (customKey) {
            tone(buzzerPin, 440); // A4
            delay(100);
            noTone(buzzerPin);

            if (customKey == 65) {
              createNewPassword();
              break;
            } else if (customKey == 66) {
              resetPassword();
              break;
            } else if (customKey != 0) {
              break;
            }
          }
        }

        tryCount = 0;

      } else if (!input.equals(password)) {

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong password!");

        tone(buzzerPin, 2000);
        delay(1000);
        noTone(buzzerPin);

      } else {

        unlock();

        while(true) { // keeps system unlocked until user presses key
          char lockKey = customKeypad.getKey();

          if (lockKey) {
            break;
          }
        }

        lock();

        tryCount = 0;

      }

      input = "";
      charNum = 0;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter password:");

    }
  }
}

void unlock() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Unlocking...");

  tone(buzzerPin, 523);

  servo.write(0);
  delay(50);

  noTone(buzzerPin);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Unlocked! Press");
  lcd.setCursor(0, 1);
  lcd.print("any btn to lock");
}

void lock() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Locking...");

  tone(buzzerPin, 523);

  servo.write(angle);
  delay(50);

  noTone(buzzerPin);

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Locked!");
  delay(1000);
  lcd.clear();
}

void alarm() {
  tone(buzzerPin, 2500);
  delay(800);
  tone(buzzerPin, 800);
  delay(800);
}

bool isEmptyEEPROM() {

  for (int x = 0; x < passwordLength; x++) {
    if(EEPROM.read(x) == 255) {
      return true;
    }
  }

  return false;
}

bool confirmOldPass() {
  
  // input old passsword for confirmation
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter old pwd to");
  lcd.setCursor(0, 1);
  lcd.print("confirm");
  
  noTone(buzzerPin);
  tone(buzzerPin, 523);
  delay(1000);
  noTone(buzzerPin);

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Old Password:");

  charNum = 0;
  oldPass = "";

  while (charNum < passwordLength - 1) {
    customKey = customKeypad.getKey();

    if(customKey) {
      tone(buzzerPin, 440); // A4
      delay(100);

      lcd.setCursor(charNum, 1);
      lcd.print(customKey);

      oldPass = oldPass + customKey;
      charNum++;

      noTone(buzzerPin);
    }
  }

  // update password if confirmed
  if (oldPass.equals(password)) {
    return true;
  } else {
    return false;
  }
}

void createNewPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("New pass, 8 char");
  
  noTone(buzzerPin);
  tone(buzzerPin, 523);
  delay(1000);
  noTone(buzzerPin);

  input = "";
  charNum = 0;

  // input new password
  while (charNum < passwordLength - 1) {
    customKey = customKeypad.getKey();

    if(customKey) {
      tone(buzzerPin, 440); // A4
      delay(100);

      lcd.setCursor(charNum, 1);
      lcd.print(customKey);

      input = input + customKey;
      charNum++;

      noTone(buzzerPin);
    }
  }

  if (confirmOldPass()) {

    char newPass[passwordLength];
    input.toCharArray(newPass, passwordLength);

    for (int x = 0; x < passwordLength; x++) {
      EEPROM.update(x, newPass[x]);
    }

    password = input;

    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Password");
    lcd.setCursor(4, 1);
    lcd.print("updated!");
    
    tone(buzzerPin, 523);
    delay(1000);
    noTone(buzzerPin);

  } else {

    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Password");
    lcd.setCursor(5, 1);
    lcd.print("wrong!");
    
    tone(buzzerPin, 2000);
    delay(1000);
    noTone(buzzerPin);

  }

  oldPass = "";
  charNum = 0;
}

void resetPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reset password?");
  lcd.setCursor(0, 1);
  lcd.print("Old pass needed");
  
  noTone(buzzerPin);
  tone(buzzerPin, 523);
  delay(1000);
  noTone(buzzerPin);

  delay(1000);

  if (confirmOldPass()) {
    char passwordChars[passwordLength];
    defaultPassword.toCharArray(passwordChars, passwordLength);

    for(int x = 0; x < passwordLength; x++) {
      EEPROM.update(x, passwordChars[x]);
    }

    password = defaultPassword;

    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Password");
    lcd.setCursor(5, 1);
    lcd.print("reset!");

    tone(buzzerPin, 523);
    delay(1000);
    noTone(buzzerPin);

  } else {

    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Password");
    lcd.setCursor(5, 1);
    lcd.print("wrong!");
    
    tone(buzzerPin, 2000);
    delay(1000);
    noTone(buzzerPin);

  }
}