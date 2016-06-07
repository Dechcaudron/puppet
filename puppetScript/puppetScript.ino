const int cacheSize = 50;
const int analogInPins = 16;

const byte commandControlByte = 0xff;

byte commandCache[cacheSize];
int lastWrittenPosition = -1;

boolean monitoredAnalogInputs[analogInPins];

void setup()
{
  Serial.begin(9600);
  
  //Init monitoredAnalogInputs
  resetAnalogInputMonitors();
  
  waitForPuppeteerReady();
}

void resetAnalogInputMonitors()
{
  for(int i=0; i<analogInPins; ++i)
  {
    monitoredAnalogInputs[i] = false;
  }
}

void waitForPuppeteerReady()
{
  const byte puppeteerReadyInCommand = 0x00;
   
  do
  {
    
    readSerialInput();
    if(lastWrittenPosition >= 2)
    {
      if(commandCache[0] == commandControlByte && commandCache[1] == puppeteerReadyInCommand && commandCache[2] == puppeteerReadyInCommand)
      {
        sendPuppetReadyCommand();
        flushCommandCache(3);
        break;
      }else
      {
        flushCommandCache(1);
      }
    }
    
    delay(500);  
  }while(true);
  
}

void sendPuppetReadyCommand()
{
  const byte puppetReadyCommand = 0x00;
  
  Serial.write(commandControlByte);
  Serial.write(puppetReadyCommand);
  Serial.write(puppetReadyCommand);
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
  const byte startMonitoringInCommand = 0x01;
  const byte stopMonitoringInCommand = 0x02;
  const byte setPWMInCommand = 0x04;
  const byte puppeteerClosedCommand = 0x99;
  
  if(lastWrittenPosition > 0)
  {
    //Check for control byte
    if(commandCache[0] == commandControlByte)
    {
      switch(commandCache[1])
      {
        case startMonitoringInCommand:
          if(lastWrittenPosition >= 2)
          {
            int pin = commandCache[2];
            monitoredAnalogInputs[pin] = true;
            flushCommandCache(3);
          }else{
            return;
          }
          break;
          
        case stopMonitoringInCommand:
          if(lastWrittenPosition >= 2)
          {
            int pin = commandCache[2];
            monitoredAnalogInputs[pin] = false;
            flushCommandCache(3);
          }
          break;
          
        case setPWMInCommand:
          if(lastWrittenPosition >= 3)
          {
            int pin = commandCache[2];
            int value = commandCache[3];
            
            analogWrite(pin, value);
            
            flushCommandCache(4);
          }
          break;
          
        case puppeteerClosedCommand:
          flushCommandCache(2);
          
          //Stop all monitors
          resetAnalogInputMonitors();
          
          //Wait until explicit puppeteer connection again
          waitForPuppeteerReady();
        
      }
    }
    else
    {
      //Flush first byte of queue if it does not match control byte
      flushCommandCache(1);
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
