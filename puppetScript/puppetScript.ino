const int cacheSize = 50;
const int analogInPins = 16;

const byte monitorInCommand = 0x01;

byte commandCache[cacheSize];
int lastWrittenPosition = -1;

boolean monitoredAnalogInputs[analogInPins];

void setup()
{
  Serial.begin(9600);
  
  //Init monitoredAnalogInputs
  for(int i=0; i<analogInPins; ++i)
  {
    monitoredAnalogInputs[i] = false;
  }
}

void loop()
{
  readSerialInput();
  
  //flushBuffer();
  
  interpretCommandCache();
  
  monitorAnalogInputs();
  
  delay(500);
}

void readSerialInput()
{ 
  while(Serial.available())
  {
    commandCache[++lastWrittenPosition] = byte(Serial.read());
  }
}

void flushBuffer()
{ 
  for(int i=0; i<=lastWrittenPosition; ++i)
  {
    Serial.write(commandCache[i]);
  }
}

void interpretCommandCache()
{
  Serial.write(0xfe);
  
  if(lastWrittenPosition != -1)
  {
    switch(commandCache[0])
    {
      case monitorInCommand:
        if(lastWrittenPosition > 0)
        {
          int pin = commandCache[1];
          monitoredAnalogInputs[pin] = true;
          flushCommandCache(2);
        }else{
          return;
        }
        break;
        
      case 0x03:
        
        break;
    }
  }
}

void flushCommandCache(int units)
{
  byte auxCache[cacheSize];
  
  for(int i=units; i<cacheSize; ++i)
  {
    auxCache[i-units] = commandCache[i];
  }
  
  for(int i=0; i<cacheSize; ++i)
  {
    commandCache[i] = auxCache[i];
  }
  
  lastWrittenPosition -= units;
}

void monitorAnalogInputs()
{
  for(int i=0; i<analogInPins; ++i)
  {
    if(monitoredAnalogInputs[i])
    {
      sendAnalogRead(i, analogRead(i));
    }
  }
}

void sendAnalogRead(int pin, int value)
{
  Serial.write(0xff);
  Serial.write(0x1);
  Serial.write(byte(pin));
  Serial.write(highByte(value));
  Serial.write(lowByte(value));
}