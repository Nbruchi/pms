#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

const int BALANCE_SIZE = 4;

// Get EEPROM address by hashing UID (simple, not secure but enough for demo)
int getAddressForUID(byte *uid, byte uidSize) {
  int address = 0;
  for (byte i = 0; i < uidSize; i++) {
    address += uid[i];
  }
  address = address % (EEPROM.length() - BALANCE_SIZE);  // Prevent overflow
  return address;
}

// Read balance from EEPROM
int readBalance(int address) {
  int balance = 0;
  EEPROM.get(address, balance);
  return balance;
}

// Write balance to EEPROM
void writeBalance(int address, int balance) {
  EEPROM.put(address, balance);
}

// Read command from Serial until newline
String readSerialCommand() {
  String input = "";
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') break;  // Wait for newline
      input += c;
    }
  }
  input.trim();
  return input;
}

// Handle the command logic
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
    ;           // Wait for Serial port (for Leonardo or similar)
  delay(2000);  // Give Serial Monitor time to open

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("🔐 RFID Wallet System Ready!");
  Serial.println("Scan a card to continue...");
}

void loop() {
  // Wait for new card
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Show UID
  byte *uid = mfrc522.uid.uidByte;
  byte uidSize = mfrc522.uid.size;

  Serial.print("\n🆔 Card UID: ");
  for (byte i = 0; i < uidSize; i++) {
    Serial.print(uid[i], HEX);
  }
  Serial.println();

  // Get address and balance
  int address = getAddressForUID(uid, uidSize);
  int balance = readBalance(address);

  Serial.println("==========================");
  Serial.print("💳 Card detected! Balance: ");
  Serial.println(balance);
  Serial.println("==========================");

  // Ask for command
  Serial.println("✍️ Enter command (e.g. add 100 or pay 50):");

  // Wait for and process serial input
  while (!Serial.available()) {
    // wait
  }

  String command = readSerialCommand();
  processCommand(command, balance);

  // Save updated balance
  writeBalance(address, balance);

  Serial.print("💾 New Balance: ");
  Serial.println(balance);
  Serial.println("📢 Remove card and scan again to continue.\n");

  // Halt RFID reading
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1500);  // Cooldown
}