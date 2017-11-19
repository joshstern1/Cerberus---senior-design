
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield


// use this line for a breakout or shield with an I2C connection:
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
   #define Serial SerialUSB
#endif

//uint8_t data_SN[16] = { 2, 'D', 4, 8, 7, 6, 'D', 0, 0, 0, 0, 0, 0, 0, 0, 0};
//uint8_t data_ZT[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//uint8_t data_ZN[16] = { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t data_SN[16];
uint8_t data_ZT[16];
uint8_t data_ZN[16];

int iteration;
#define SETUP_MODE 0
#define WRITE_MODE 0

void setup(void) {
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif
  Serial.begin(115200);
  Serial.println("Hello Alarm.com!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  //Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  //Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  //Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Ready to Read from NFC Tag");
  iteration=0;
}


void loop(void) {
  iteration++;
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  // Serial.println(iteration);
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    // Display some basic information about the card
    Serial.println("Found an Alarm.com NFC Tag");
    //Serial.print("  TAG ID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("   TAG ID =  ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength == 4)
    {
      //Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Authenticating Tag With Secret Password");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
      uint8_t newkeya[6] = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB };
    
      if(SETUP_MODE){
          if(iteration == 1){
            success = nfc.Cerberus_Authenticate(uid, uidLength, 4, 0, keya);
          }
          else if(iteration>1){
            success = nfc.Cerberus_Authenticate(uid, uidLength, 4, 0, newkeya);
          }
      
            if(iteration==1){
               uint8_t data_A[16];
               memcpy(data_A, (const uint8_t[]){ 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xFF, 0x07, 0x80, 0x69, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, sizeof data_A);
               success = nfc.Cerberus_Write (7, data_A);
            }
      }
      else{
        success = nfc.Cerberus_Authenticate(uid, uidLength, 4, 0, newkeya);
      }
    
      if (success)
      {
        Serial.println("Ceberus Has Authenticated The Tag ");


      if(WRITE_MODE){
       //memcpy(data_SN, (const uint8_t[]){ 2, 'D', 4, 8, 7, 6, 'D', 0, 0, 0, 0, 0, 0, 0, 0, 0 }, sizeof data_SN);
       success = nfc.Cerberus_Write (4, data_SN);
      }

        // Try to read the contents of block 4
        success = nfc.Cerberus_Read(4, data_SN);

        if(success){
            int i;
            for(i=0;i<7;i++){
              if (data_SN[i]==0){
                success=0;
              }
            }
            for(i=7;i<16;i++){
              if(data_SN[i]>0){
                success=0;
              }
            }
            if(success==0){
              Serial.println("Invalid SN");
            }
          
        }
        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Cerberus Is Reading Serial Number");
          Serial.println("Sensor Serial Number Is:");
          nfc.PrintCerberus(data_SN, 16);
          Serial.println("");


        if(WRITE_MODE){
          //memcpy(data_ZT, (const uint8_t[]){ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, sizeof data_ZT);
          success = nfc.Cerberus_Write (5, data_ZT);
        }

      // Try to read the contents of block 5
        success = nfc.Cerberus_Read(5, data_ZT);
          if(success){
            int i;
              if ((data_ZT[0]>17)||(data_ZT[0]==0)){
                success=0;
              }
            for(i=1;i<16;i++){
              if(data_ZT[i]>0){
                success=0;
              }
            }
            if(success==0){
              Serial.println("Invalid ZT");
            }
          
          }
        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Cerberus Is Reading Zone Type");
          Serial.println("Sensor Zone Type Is:");
          nfc.PrintCerberus(data_ZT, 16);
          Serial.println("");

        if(WRITE_MODE){
          // memcpy(data_ZN, (const uint8_t[]){ 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, sizeof data_ZN);
           success = nfc.Cerberus_Write (6, data_ZN);
        }

        // Try to read the contents of block 6
        success = nfc.Cerberus_Read(6, data_ZN);
          if(success){
            int i;
              if ((data_ZN[0]>88)||(data_ZN[0]==0)){
                success=0;
              }
            for(i=1;i<16;i++){
              if(data_ZN[i]>0){
                success=0;
              }
            }
            if(success==0){
              Serial.println("Invalid ZN");
            }
          
          }
        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Cerberus Is Reading Zone Number");
          Serial.println("Sensor Zone Number Is:");
          nfc.PrintCerberus(data_ZN, 16);
          Serial.println("");
      
          // Wait a bit before reading the card again
         delay(3000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block ZN.  Try again?");
        }
        //dana
      
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block ZT.  Try again?");
        }
           
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block SN.  Try again?");
        }
      
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
        delay(1000);
      }
    }

  }
}
