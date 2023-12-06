#include <Servo.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <MFRC522.h>

#define SS_PIN 10  //SS pin of RFID module
#define RST_PIN 9  //RST pin of RFID module

bool e;
int input_funct;
int add_funct;
int delete_funct;
int frequency = 500; // initialize the starting frequency
int increment = 10; // set the frequency increment
const int buzzerPin = 1;
// Define a string array to hold the RFID card data
String rfid_cards[10];
int num_cards = 0;
String mastercard = "B1 B8 72 1D";  //rfid master card

char password_nhap[] = { '0', '0', '0', '0', '0' };
char password[] = "12345";
char add_rfid[] = "*****";
char delete_rfid[] = "00000";

// Define keypad pin configuration and password
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
// connect the pins from right to left to pin 2, 3, 4, 5, 6,7,8
byte rowPins[ROWS] = { 8, 7, 6, 5 };
byte colPins[COLS] = { 4, 3, 2 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Define servo pin configuration
Servo myservo;
int servoPin = 0;

// Define variables for password and card ID
String cardID = "";
// String password = "12345";

// Define variables for EEPROM memory addresses
int address = 0;
int num_cards_address = 100;

// Define RFID module instance
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {

  // Initialize RFID module
  SPI.begin();
  rfid.PCD_Init();

  // Attach servo to its pin and set initial position
  myservo.attach(servoPin);
  myservo.write(0);
}

void loop() {
  // Wait for a card to be detected
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Get the ID of the detected card
    cardID = "";
    for (int i = 0; i < rfid.uid.size; i++) {
      cardID += String(rfid.uid.uidByte[i], HEX);
      cardID.toUpperCase();
      cardID += " ";
    }

    // Check if the detected card is the master card
    if (cardID == mastercard) {
      // Unlock the door
      myservo.write(90);
      successSound();
      delay(4000);
      myservo.write(0);
   
    } else {
      // Check if the detected card is in the list of allowed cards
      bool card_found = false;
      for (int i = 0; i < num_cards; i++) {
        if (cardID == rfid_cards[i]) {
          card_found = true;
          break;
        }
      }

      // If the card is allowed, unlock the door
      if (card_found) {
        myservo.write(90);
        successSound();
        delay(5000);
        myservo.write(0);
 
      } else {
        failSound();
      }
    }

    // Wait for the card to be removed before detecting another card
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  //nhap mat khau
  char keyPress = keypad.getKey();
  if (keyPress)  // neu co phim nhan
  {
    delay(100);

    e = true;
    if (keyPress != '#') {
      //dich phim
      for (int i = 4; i > 0; i--) {
        password_nhap[i] = password_nhap[i - 1];
      }
      password_nhap[0] = keyPress;

      tone(buzzerPin, 1000); // Generate a 1 kHz tone on the buzzer pin
    }


    if (keyPress == '#')  //nhaff OK thi chot mat khau
    {
      tone(buzzerPin, 1000); // Generate a 1 kHz tone on the buzzer pin

      if (e == true) {
        //kiem tra password voi so nhap
        for (int i = 0; i < 5; i++) {
          //password
          if (password_nhap[i] == password[4 - i]) {  //chuc nang nhap mat khau va mo khoa
            input_funct++;
          }
          if (password_nhap[i] == add_rfid[4 - i]) {
            add_funct++;
          }
          if (password_nhap[i] == delete_rfid[4 - i]) {
            delete_funct++;
          }
        }

        //neu nhap dung
        if (input_funct == 5) {
          e = false;
          setLocked(true);
          input_funct = 0;
        } else if (add_funct == 5) {
          e = false;
          addCard();
          add_funct = 0;
        } else if (delete_funct == 5) {
          e = false;
          deleteCard();
          delete_funct = 0;
        }
        //neu nhap sai
        else {
          e = false;
          setLocked(false);
          input_funct = 0;
          add_funct = 0;
          delete_funct = 0;
        }
      }
    }
  }
}

void addCard() {
  // Check if there is space to add a new card
  if (num_cards < 10) {
    // Wait for a card to be detected

    while (!rfid.PICC_IsNewCardPresent()) {
      delay(10);
    }

    // Get the ID of the detected card
    cardID = "";
    if (rfid.PICC_ReadCardSerial()) {
      for (int i = 0; i < rfid.uid.size; i++) {
        cardID += String(rfid.uid.uidByte[i], HEX);
        cardID.toUpperCase();
        cardID += " ";
      }

      // Check if the card is already in the list
      bool card_found = false;
      for (int i = 0; i < num_cards; i++) {
        if (cardID == rfid_cards[i]) {
          card_found = true;
          break;
        }
      }

      // If the card is not already in the list, add it
      if (!card_found) {
        rfid_cards[num_cards] = cardID;
        num_cards++;
        EEPROM.write(num_cards_address, num_cards);
        address = (num_cards - 1) * 20 + 1;
        for (int i = 0; i < 16; i++) {
          EEPROM.write(address + i, cardID.charAt(i));
        }

        successSound();
      } else {

        failSound();
      }

      // Wait for the card to be removed before detecting another card
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    } else {

      failSound();
    }
  } else {

    failSound();
  }
}

void deleteCard() {
  // Wait for a card to be detected

  while (!rfid.PICC_IsNewCardPresent()) {
    delay(10);
  }

  // Get the ID of the detected card
  cardID = "";
  if (rfid.PICC_ReadCardSerial()) {
    for (int i = 0; i < rfid.uid.size; i++) {
      cardID += String(rfid.uid.uidByte[i], HEX);
      cardID.toUpperCase();
      cardID += " ";
    }

    // Check if the card is in the list
    int card_index = -1;
    for (int i = 0; i < num_cards; i++) {
      if (cardID == rfid_cards[i]) {
        card_index = i;
        break;
      }
    }

    // If the card is in the list, remove it
    if (card_index >= 0) {
      for (int i = card_index; i < num_cards - 1; i++) {
        rfid_cards[i] = rfid_cards[i + 1];
      }
      num_cards--;
      EEPROM.write(num_cards_address, num_cards);
      for (int i = card_index; i < num_cards; i++) {
        address = i * 20 + 1;
        for (int j = 0; j < 16; j++) {
          EEPROM.write(address + j, rfid_cards[i].charAt(j));
        }
      }

      successSound();
    } else {
 
      failSound();
    }

    // Wait for the card to be removed before detecting another card
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  } else {

    failSound();
  }
}

void setLocked(int locked) {
  if (locked) {


    myservo.write(90);  //mo cua
    successSound();
    delay(1000);



    for (int i = 5; i > 0; i--) {

  
      delay(1000);
    }
    myservo.write(0);
    //DONG CUA

    
    for (int i = 0; i < 5; i++)  //gan mang led ve 00000
    {
      password_nhap[i] = '0';
    }
  } else {


    for (int i = 0; i < 5; i++)  //gan mang led ve 00000
    {
      password_nhap[i] = '0';
    }
    delay(1000);
    failSound();

  }
}

void failSound() {
  for (int i = 0; i < 3; i++) {                                         // repeat the loop 3 times
    for (frequency = 500; frequency <= 1500; frequency += increment) {  // increase the frequency
      tone(buzzerPin, frequency, 10);                                   // generate the tone
      delay(5);                                                         // wait for a short duration
    }
    for (frequency = 1500; frequency >= 500; frequency -= increment) {  // decrease the frequency
      tone(buzzerPin, frequency, 10);                                   // generate the tone
      delay(5);                                                         // wait for a short duration
    }
  }
  noTone(buzzerPin);  // turn off the buzzer
  delay(1000);        // wait for a longer duration before starting again
}
void successSound() {
  tone(buzzerPin, 1500, 200);  // Generate a 1.5 kHz tone for 200 ms
  delay(200);                  // Wait for 200 ms before generating the next tone
  tone(buzzerPin, 2000, 200);  // Generate a 2 kHz tone for 200 ms
  delay(200);                  // Wait for 200 ms before generating the next tone
  noTone(buzzerPin);           // Turn off the buzzer
  delay(1000);                 // Wait for 1 second before playing the sound again
}
