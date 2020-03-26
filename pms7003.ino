/* 
 * Connecting to PMS7003 sensor via UART
 * 
 * PMS7003 sends 32 byte frames that consist of:
 * 2 start characters - 0x42 and 0x4d
 * 2-byte frame length (should always be 28)
 * 13 2-byte data numbers
 * 2-byte checksum (sum of all previous bytes
 * including start characters)
 * 
 * high byte sent first
 * values with std are standardized reading
 * ones without it are "under atmospheric environment"
 * manual suggest using standardized values
 */

#include <SoftwareSerial.h>

#define PMS7003_RX_PIN 2
#define PMS7003_TX_PIN 0

SoftwareSerial pmsSerial(PMS7003_RX_PIN, PMS7003_TX_PIN); // (rx, tx)
bool inFrame = false;
byte byteIndex = 0;
byte buff;

bool newData = false;
uint16_t calculatedCheckSum = 0;

struct pms7003DataStruct {
  uint16_t frameLength;
  uint16_t pm1Std;
  uint16_t pm25Std;
  uint16_t pm10Std;
  uint16_t pm1;
  uint16_t pm25;
  uint16_t pm10;
  uint16_t particles03um;
  uint16_t particles05um;
  uint16_t particles1um;
  uint16_t particles25um;
  uint16_t particles5um;
  uint16_t particles10um;
  uint16_t checkSum;
} receivedPacket;

void setup() {
  Serial.begin(115200);
  while(!Serial) { }

  pmsSerial.begin(9600);

  Serial.print("Initialized\n");
}

void loop() {
  // if more than one packet is available clear serial buffer
  if (pmsSerial.available() > 32){
    clearPmsSerial();
  }

    while ( pmsSerial.available() > 0  && newData == false){
      int incomingByte = pmsSerial.read();
      
      if (!inFrame){
        if (incomingByte == 0x42 && byteIndex == 0) {
          // first start byte
          processByte(incomingByte);
        
      } else if (incomingByte == 0x4d && byteIndex == 1){
        // second start byte
        inFrame = true;
        processByte(incomingByte);
        }
      } else {
        processByte(incomingByte);
        if (byteIndex == 32){
          newData = true;
          inFrame = false;
        }
      }
    } 
    
      if (newData){
        unsigned int receivedCheckSum = receivedPacket.checkSum;
        
        if (calculatedCheckSum == receivedCheckSum){
          printResult();
        }
        
        newData = false;
        byteIndex = 0;
        calculatedCheckSum = 0;
      }    
  }

void clearPmsSerial(){
      int drain = pmsSerial.available();
      for (int i = drain; i > 0; i--) {
        pmsSerial.read();
      }
}

void processByte(int incomingByte){
  calculatedCheckSum += incomingByte;

  if (byteIndex % 2 == 0) {
    // got  the first byte of 2-byte data
    // store it in buffer
    buff = incomingByte;
  } else {
    // got the second byte, we can assemble data
    uint16_t data = (buff << 8) + (incomingByte & 0xff);
    processData(data);
  }

  byteIndex++;
}

void processData(uint16_t data){
    // at index 0 and 1 we get start characters
    // they're always the same and get checked elsewhere
    // data at index 28 and 29 is called 'reserved' in manual
    // no clue what's it for or why it exists
    switch (byteIndex) {
      case 3:
        receivedPacket.frameLength = data;
        break;
      case 5:
        receivedPacket.pm1Std = data;
        break;
      case 7:
        receivedPacket.pm25Std = data;
        break;
      case 9:
        receivedPacket.pm10Std = data;
        break;
      case 11:
        receivedPacket.pm1 = data;
        break;
      case 13:
        receivedPacket.pm25 = data;
        break;
      case 15:
        receivedPacket.pm10 = data;
        break;
      case 17:
        receivedPacket.particles03um = data;
        break;
      case 19:
        receivedPacket.particles05um = data;
        break;
      case 21:
        receivedPacket.particles1um = data;
        break;
      case 23:
        receivedPacket.particles25um = data;
        break;
      case 25:
        receivedPacket.particles5um = data;
        break;
      case 27:
        receivedPacket.particles10um = data;
        break;
      case 31:
        receivedPacket.checkSum = data;
        // substracting received checksum bytes from calculated checksum
        // this is easier than making another if to not add them in the first place
        calculatedCheckSum -= (data >> 8) + (data & 0xff);
        break;
    }
}

void printResult(){
 // Serial.print("Odebrano dane z czujnika\n");
          char line1[22];
          char line2[22];

          sprintf(line1, "PM25S %d PM25 %d", receivedPacket.pm25Std, receivedPacket.pm25);
          sprintf(line2, "PM10S %d PM10 %d", receivedPacket.pm10Std, receivedPacket.pm10);

          Serial.print(line1);
          Serial.print("\n");
          Serial.print(line2);
          Serial.print("\n");

          Serial.print("PM 1: ");
          Serial.print(receivedPacket.pm1Std);
          Serial.print("\n");

          char particlesLine[100];
          sprintf(particlesLine, "0.3um: %d, 0.5um: %d, 1um: %d, 2.5um: %d, 5um: %d, 10um: %d\n",
          receivedPacket.particles03um, receivedPacket.particles05um, receivedPacket.particles1um,
          receivedPacket.particles25um, receivedPacket.particles5um, receivedPacket.particles10um);

          Serial.print(particlesLine);
          Serial.print("----------------------------------------------------\n");
}
