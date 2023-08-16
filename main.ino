#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9

byte btn_state;

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522 mfrc522(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

byte nuidPICC[4];

#define BTN1 A0
#define BTN2 A1
#define GREENLED 8
#define REDLED 7

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte newUid[] = {};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight();

  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(REDLED, OUTPUT);

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  lcd.clear();
  lcd.print("RFID Cloner");
  lcd.setCursor(0, 1);
  lcd.print("Y = READ");
  delay(1500);
  lcd.clear();
}

void loop() {

  if (analogRead(BTN1) == 0) {

    if (!rfid.PICC_IsNewCardPresent())
      return;

    if (!rfid.PICC_ReadCardSerial())
      return;

    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));

    if (analogRead(BTN2) != 0) {
      if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        digitalWrite(REDLED, HIGH);
        delay(1000);
        digitalWrite(REDLED, LOW);
        return;
      }
    }


    if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3]) {
      Serial.println(F("A new card has been detected."));

      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }

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

      digitalWrite(GREENLED, HIGH);
      delay(1000);
      digitalWrite(GREENLED, LOW);


    } else {
      Serial.println(F("Card read previously."));
      digitalWrite(GREENLED, HIGH);
      delay(1000);
      digitalWrite(GREENLED, LOW);
    }

    rfid.PICC_HaltA();

    rfid.PCD_StopCrypto1();
  }


  else {

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      delay(50);
      return;
    }

    Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    for (byte i = 0; i < 4; i++) {
      newUid[i] = rfid.uid.uidByte[i];
    }
    if (mfrc522.MIFARE_SetUid(newUid, (byte)4, true)) {
      Serial.println(F("Wrote new UID to card."));
    }

    mfrc522.PICC_HaltA();
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    Serial.println(F("New UID and contents:"));
    mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

    delay(2000);
  }
}



void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);

    lcd.print(buffer[i], HEX);
  }
}
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);

    lcd.print(buffer[i], DEC);
  }
}
