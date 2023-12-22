#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Constants for display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define JUMP 12

// Objects for RFID and display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MFRC522 rfid(10, 9);  // SS_PIN, RST_PIN
MFRC522 mfrc522(10, 9);  // Second RFID module

// State variables
byte nuidPICC[4];  // Store the UID of the last scanned card
byte newUid[] = {};  // Store the new UID for writing to another card

// MIFARE key for authentication
MFRC522::MIFARE_Key key;

// Define digital pins for buttons and LEDs
#define MAIN_BTN A0    // Main button pin
#define BTN2 A1        // Second button pin
#define GREENLED 8     // Green LED pin
#define REDLED 7       // Red LED pin

// Function prototypes
void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  SPI.begin();

  // Initialize RFID module
  rfid.PCD_Init();

  // Define button and LED pin modes
  pinMode(MAIN_BTN, INPUT);
  pinMode(BTN2, INPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(REDLED, OUTPUT);

  // Set default key for MIFARE cards
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Display initialization message on display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  delay(500);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("RFID Cloner");
  display.setCursor(0, 16);
  display.println("Scan card...");
  display.display();
}

void loop() {
  // Check if the main button is pressed
  if (analogRead(MAIN_BTN) == 0) {
    display.fillRect(0, 64 - 12, 128, 64, SSD1306_BLACK);
    display.display();

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
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("RFID Cloner");
      display.setCursor(0, JUMP + 4);

      // Display the UID on the display
      display.println("HEX: ");
      display.setCursor(8 * 3, JUMP + 4);

      Serial.println(F("The NUID tag is:"));
      Serial.print(F("In hex: "));
      printHex(rfid.uid.uidByte, rfid.uid.size);
      Serial.println();

      display.setCursor(0, JUMP * 2 + 4);
      display.println("DEC: ");
      display.setCursor(8 * 3, JUMP * 2 + 4);

      Serial.print(F("In dec: "));
      printDec(rfid.uid.uidByte, rfid.uid.size);
      Serial.println();

      display.display();

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
    delay(1000);
  }
}

// Function to print a byte buffer in hexadecimal format
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);

    display.print(buffer[i], HEX);
  }
}

// Function to print a byte buffer in decimal format
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);

    display.print(buffer[i], DEC);
  }
}
