#include <Arduino.h>
#include <vector>


// Module setup

char moduleName[] = "BPod Panel"; // Name of module for manual override UI and state machine assembler

#define FirmwareVersion 1
#define InputOffset 2
#define OutputOffset 30

#define nInputChannels 20
#define nOutputChannels 24

#define AinOffset 54
#define AoutOffset 66

#define AinChannels 12
#define AoutChannels 2

uint32_t refractoryPeriod = 300; // Minimum amount of time (in microseconds) after a logic transition on a line, before its level is checked again.
                                  // This puts a hard limit on how fast each channel on the board can spam the state machine with events.

// Constants
#define InputChRangeHigh InputOffset+nInputChannels
#define OutputChRangeHigh OutputOffset+nOutputChannels


std::vector<String> eventNames;

int nEventNames = 0; // Number of behavior events this module can generate, 2 for each input channel (high and low)

// Variables
byte opCode = 0;
byte channel = 0;
byte state = 0;
byte thisEvent = 0;
boolean readThisChannel = false; // For implementing refractory period (see variable above)
byte inputChState[nInputChannels] = {0}; // Current state of each input channel
byte lastInputChState[nInputChannels] = {0}; // Last known state of each input channel
byte inputsEnabled[nInputChannels] = {0}; // For each input channel, enabled or disabled
uint32_t inputChSwitchTime[nInputChannels] = {0}; // Time of last detected logic transition
byte events[nInputChannels*2] = {0}; // List of high or low events captured this cycle
byte nEvents = 0; // Number of events captured in the current cycle
uint32_t currentTime = 0; // Current time in microseconds

void setup()
{
  // Serial.begin(115200);
  Serial.begin(1312500);
  SerialUSB.begin(1312500);
  currentTime = micros();
  for (int i = 0; i < nInputChannels; i++) {
    pinMode(i+InputOffset, INPUT_PULLUP);
    inputsEnabled[i] = 1;
    inputChState[i] = 1;
    lastInputChState[i] = 1;
    eventNames.push_back(String(i+InputOffset) + "_Hi");
    eventNames.push_back(String(i+InputOffset) + "_Lo");
    nEventNames += 2;
  }
  for (int i = OutputOffset; i < OutputChRangeHigh; i++) {
    pinMode(i, OUTPUT);
  }
  analogReadResolution(16); // Set analog read resolution to 16 bits
  analogWriteResolution(16); // Set analog write resolution to 16 bits
  analogWrite(DAC0, 0);
  analogWrite(DAC1, 0);
}

void loop()
{
  currentTime = micros();

if (Serial.available()) {
    opCode = Serial.read();
    if (opCode == 255) {
      returnModuleInfo();
    } 
    else if ((opCode >= OutputOffset) && (opCode < OutputChRangeHigh)) {
      state = Serial.read();
      digitalWrite(opCode,state); 
    }
    else if ((opCode >= AinOffset) && (opCode < AinOffset + AinChannels)) {
      returnAnalogRead(opCode);
    } 
    else if ((opCode >= AoutOffset) && (opCode < AoutOffset + AoutChannels)) {
      state = Serial.read();
      int val = (Serial.read() << 8) + Serial.read();
      analogWrite(opCode,val);
    }
    else if (opCode == 'E') {
      channel = Serial.read();
      state = Serial.read();
      if ((channel >= InputOffset) && (channel < InputChRangeHigh)) {
        inputsEnabled[channel-InputOffset] = state;
      }
    }
  }

  if (SerialUSB.available()) {
    opCode = SerialUSB.read();
    if (opCode == 255) {
      returnModuleInfo_USB();
    } 
    else if ((opCode >= OutputOffset) && (opCode < OutputChRangeHigh)) {
      state = SerialUSB.read();
      digitalWrite(opCode,state); 
    }
    else if ((opCode >= AinOffset) && (opCode < AinOffset + AinChannels)) {
      returnAnalogRead_USB(opCode);
    } 
    else if ((opCode >= AoutOffset) && (opCode < AoutOffset + AoutChannels)) {
      int val = (SerialUSB.read() << 8);
      val += SerialUSB.read();
      analogWrite(opCode,val);
    }
    else if (opCode == 'E') {
      channel = SerialUSB.read();
      state = SerialUSB.read();
      if ((channel >= InputOffset) && (channel < InputChRangeHigh)) {
        inputsEnabled[channel-InputOffset] = state;
      }
    }
  }

  thisEvent = 1;
  for (int i = 0; i < nInputChannels; i++) {
    if (inputsEnabled[i] == 1) {
      inputChState[i] = digitalRead(i+InputOffset);
      readThisChannel = false;
      if (currentTime > inputChSwitchTime[i]) {
        if ((currentTime - inputChSwitchTime[i]) > refractoryPeriod) {
          readThisChannel = true;
        }
      } else if ((currentTime + 4294967296-inputChSwitchTime[i]) > refractoryPeriod) {
        readThisChannel = true;
      }
      if (readThisChannel) {
        if ((inputChState[i] == 1) && (lastInputChState[i] == 0)) {
          events[nEvents] = thisEvent; nEvents++;
          inputChSwitchTime[i] = currentTime;
          lastInputChState[i] = inputChState[i];
        }
        if ((inputChState[i] == 0) && (lastInputChState[i] == 1)) {
          events[nEvents] = thisEvent+1; nEvents++;
          inputChSwitchTime[i] = currentTime;
          lastInputChState[i] = inputChState[i];
        }
      }
    }
    thisEvent += 2;
  }
  if (nEvents > 0) {
    Serial.write(events, nEvents);
    SerialUSB.write(events, nEvents);
    nEvents = 0;
  }
}

void returnAnalogRead_USB(int channel){

  SerialUSB.write((byte)channel);
  int value = analogRead(channel);
  SerialUSB.write((byte)(value >> 8));
  SerialUSB.write((byte)(value & 0xFF));
}

void returnModuleInfo_USB() {

  SerialUSB.write(65); // Acknowledge

  SerialUSB.write((byte)((FirmwareVersion >> 24) & 0xFF)); // 4-byte firmware version
  SerialUSB.write((byte)((FirmwareVersion >> 16) & 0xFF));
  SerialUSB.write((byte)((FirmwareVersion >> 8) & 0xFF));
  SerialUSB.write((byte)(FirmwareVersion) & 0xFF);

  SerialUSB.write((byte)(sizeof(moduleName)-1));
  SerialUSB.write((byte*)moduleName, sizeof(moduleName)-1); // Module name

  SerialUSB.write((byte)0x01); // 1 if more info follows, 0 if not

  SerialUSB.write('#'); // Op code for: Number of behavior events this module can generate
  SerialUSB.write(nInputChannels*2); // 2 states for each input channel

  SerialUSB.write((byte)0x01); // 1 if more info follows, 0 if not

  SerialUSB.write('E'); // Op code for: Behavior event names
  SerialUSB.write((byte)nEventNames);

  for (int i = 0; i < nEventNames; i++) { // Once for each event name
    SerialUSB.write(eventNames[i].length()); // Send event name length
    SerialUSB.write((byte*)eventNames[i].c_str(), eventNames[i].length()); // Send event name
  }

  SerialUSB.write((byte)0x00); // 1 if more info follows, 0 if not

}

void returnAnalogRead(int channel){

  Serial.write((byte)channel);
  int value = analogRead(channel);
  Serial.write((byte)(value >> 8));
  Serial.write((byte)(value & 0xFF));
}

void returnModuleInfo() {

  Serial.write(65); // Acknowledge

  Serial.write((byte)((FirmwareVersion >> 24) & 0xFF)); // 4-byte firmware version
  Serial.write((byte)((FirmwareVersion >> 16) & 0xFF));
  Serial.write((byte)((FirmwareVersion >> 8) & 0xFF));
  Serial.write((byte)(FirmwareVersion) & 0xFF);

  Serial.write((byte)(sizeof(moduleName)-1));
  Serial.write((byte*)moduleName, sizeof(moduleName)-1); // Module name

  Serial.write((byte)0x01); // 1 if more info follows, 0 if not

  Serial.write('#'); // Op code for: Number of behavior events this module can generate
  Serial.write(nInputChannels*2); // 2 states for each input channel

  Serial.write((byte)0x01); // 1 if more info follows, 0 if not

  Serial.write('E'); // Op code for: Behavior event names
  Serial.write((byte)nEventNames);

  for (int i = 0; i < nEventNames; i++) { // Once for each event name
    Serial.write(eventNames[i].length()); // Send event name length
    Serial.write((byte*)eventNames[i].c_str(), eventNames[i].length()); // Send event name
  }

  Serial.write((byte)0x00); // 1 if more info follows, 0 if not

}