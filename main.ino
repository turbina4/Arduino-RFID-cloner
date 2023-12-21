#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

// RFID module pin definitions
#define SS_PIN 10
#define RST_PIN 9

// State variables
byte btn_state;
byte nuidPICC[4];  // Store the UID of the last scanned card
byte newUid[] = {};  // Store the new UID for writing to another card

// RFID module instances
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// MIFARE key for authentication
MFRC522::MIFARE_Key key;

// I2C LCD instance
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Function prototypes
void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);

// Define digital pins for buttons and LEDs
#define MAIN_BTN A0    // Main button pin
#define BTN2 A1        // Second button pin
#define GREENLED 8     // Green LED pin
#define REDLED 7       // Red LED pin

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  SPI.begin();

  // Initialize RFID module
  rfid.PCD_Init();

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Define button and LED pin modes
  pinMode(MAIN_BTN, INPUT);
  pinMode(BTN2, INPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(REDLED, OUTPUT);

  // Set default key for MIFARE cards
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Display initialization message on LCD
  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  lcd.clear();
  lcd.print("RFID Cloner");
  delay(1500);
  lcd.clear();
  lcd.print("Scan card...");
}

void loop() {
  // Check if the main button is pressed
  if (analogRead(MAIN_BTN) == 0) {
    // Check if a new card is present
    if (!rfid.PICC_IsNewCardPresent())
      return;

    // Read the card's UID
    if (!rfid.PICC_ReadCardSerial())
      return;

    // Get the type of the card
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.print(F("PICC type: "));
    Serial.println(rfid.PICC_GetTypeName(piccType));

    // Check for the second button press and card type
    if (analogRead(BTN2) != 0) {
      if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        digitalWrite(REDLED, HIGH);
        delay(1000);
        digitalWrite(REDLED, LOW);
        return;
      }
    }

    // Check if the card is different from the last one
    if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3]) {
      Serial.println(F("A new card has been detected."));

      // Update the stored UID
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }
      lcd.clear();
      // Display the UID on the LCD
      lcd.print("HEX: ");
      lcd.setCursor(5, 0);

      Serial.println(F("The NUID tag is:"));
      Serial.print(F("In hex: "));
      printHex(rfid.uid.uidByte, rfid.uid.size);
      Serial.println();

      lcd.setCursor(0, 1);
      lcd.print("DEC: ");
      lcd.setCursor(5, 1);

      Serial.print(F("In dec: "));
      printDec(rfid.uid.uidByte, rfid.uid.size);
      Serial.println();

      // Provide visual feedback with the green LED
      digitalWrite(GREENLED, HIGH);
      delay(1000);
      digitalWrite(GREENLED, LOW);
    } else {
      // Display a message if the card has been read previously
      Serial.println(F("Card read previously."));
      digitalWrite(GREENLED, HIGH);
      delay(1500);
      digitalWrite(GREENLED, LOW);
    }

    // Halt communication and stop crypto for the RFID module
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  } else {
    // Check if a new card is present for the second RFID module
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      delay(50);
      return;
    }

    // Display the UID of the second card
    Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Store the UID for writing to another card
    for (byte i = 0; i < 4; i++) {
      newUid[i] = rfid.uid.uidByte[i];
    }

    // Write the new UID to another card
    if (mfrc522.MIFARE_SetUid(newUid, (byte)4, true)) {
      Serial.println(F("Wrote new UID to card."));
    }

    // Halt communication for the second RFID module
    mfrc522.PICC_HaltA();
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    // Display the new UID and contents
    Serial.println(F("New UID and contents:"));
    mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

    // Delay before reading another card
    delay(2000);
  }
}

// Function to print a byte buffer in hexadecimal format
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);

    lcd.print(buffer[i], HEX);
  }
}

// Function to print a byte buffer in decimal format
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);

    lcd.print(buffer[i], DEC);
  }
}
