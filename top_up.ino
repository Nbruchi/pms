#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

const int BALANCE_SIZE = 4;

int getAddressForUID(byte *uid, byte uidSize) {
  int address = 0;
  for (byte i = 0; i < uidSize; i++) {
    address += uid[i];
  }
  address = address % (EEPROM.length() - BALANCE_SIZE);
  return address;
}

int readBalance(int address) {
  int balance = 0;
  EEPROM.get(address, balance);
  return balance;
}

void writeBalance(int address, int balance) {
  EEPROM.put(address, balance);
}

String readSerialCommand() {
  String input = "";
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') break;  
      input += c;
    }
  }
  input.trim();
  return input;
}

void processCommand(String command, int &balance) {
  command.trim();
  command.toLowerCase();

  if (command.startsWith("add ")) {
    int amount = command.substring(4).toInt();
    if (amount > 0) {
      balance += amount;
      Serial.print("✅ Added ");
      Serial.print(amount);
      Serial.println(" units.");
    } else {
      Serial.println("⚠️ Invalid amount to add.");
    }

  } else if (command.startsWith("pay ")) {
    int amount = command.substring(4).toInt();
    if (amount > 0) {
      if (balance >= amount) {
        balance -= amount;
        Serial.print("✅ Paid ");
        Serial.print(amount);
        Serial.println(" units.");
      } else {
        Serial.println("❌ Insufficient balance.");
      }
    } else {
      Serial.println("⚠️ Invalid amount to pay.");
    }

  } else {
    Serial.println("⚠️ Unknown command. Use 'add 100' or 'pay 50'");
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;           
  delay(2000); 

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("🔐 RFID Wallet System Ready!");
  Serial.println("Scan a card to continue...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte *uid = mfrc522.uid.uidByte;
  byte uidSize = mfrc522.uid.size;

  Serial.print("\n🆔 Card UID: ");
  for (byte i = 0; i < uidSize; i++) {
    Serial.print(uid[i], HEX);
  }
  Serial.println();

  int address = getAddressForUID(uid, uidSize);
  int balance = readBalance(address);

  Serial.println("==========================");
  Serial.print("💳 Card detected! Balance: ");
  Serial.println(balance);
  Serial.println("==========================");

  Serial.println("✍️ Enter command (e.g. add 100 or pay 50):");
  String command = readSerialCommand();
  processCommand(command, balance);

  writeBalance(address, balance);

  Serial.print("💾 New Balance: ");
  Serial.println(balance);
  Serial.println("📢 Remove card and scan again to continue.\n");

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1500); 
}