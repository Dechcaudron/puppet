const int cacheSize = 50;
const int analogInPins = 16;

const byte commandControlByte = 0xff;

byte commandCache[cacheSize];
int lastWrittenPosition = -1;

//Note: Short is int for arduino
const byte shortVarTypeCode = 0x00;
const int shortVarsNumber = 2;
int shortVars[shortVarsNumber];
boolean monitoredShortVars[shortVarsNumber];

boolean monitoredAnalogInputs[analogInPins];

void setup()
{
  Serial.begin(9600);
  
  //Init monitoredAnalogInputs
  resetAnalogInputMonitors();
  
  resetInternalVars();
  resetVarMonitors();
  
  waitForPuppeteerReady();
}

void resetInternalVars()
{
  //Short
  for(int i=0; i<shortVarsNumber; ++i)
  {
    shortVars[i]=0;
  }
}

void resetAnalogInputMonitors()
{
  for(int i=0; i<analogInPins; ++i)
  {
    monitoredAnalogInputs[i] = false;
  }
}

void resetVarMonitors()
{
  //Short
  for(int i=0; i<shortVarsNumber; ++i)
  {
    monitoredShortVars[i]=false;
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
  
  monitorVars();
  
  shortVars[0]++;
  
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
  const byte startMonitoringVarInCommand = 0x05;
  const byte stopMonitoringVarInCommand = 0x06;
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
          
        case startMonitoringVarInCommand:
          if(lastWrittenPosition >= 3)
          {
            if(!startMonitoringVar(commandCache[2], commandCache[3]))
            {
              Serial.write(commandControlByte);
              Serial.write(0xfe);
              //TODO: send error
            }
            
            flushCommandCache(4);
          }
          break;
          
        case stopMonitoringVarInCommand:
          if(lastWrittenPosition >= 3)
          {
            if(!stopMonitoringVar(commandCache[2], commandCache[3]))
            {
              Serial.write(commandControlByte);
              Serial.write(0xfe);
              //TODO: send error
            }
            
            flushCommandCache(4);
          }
          break;
          
        case puppeteerClosedCommand:
          flushCommandCache(2);
          
          //Stop all monitors
          resetAnalogInputMonitors();
          resetVarMonitors();
          
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

boolean startMonitoringVar(byte type, int varPosition)
{
  return changeInternalVarMonitorStatus(type, varPosition, true);
}

boolean stopMonitoringVar(byte type, int varPosition)
{
  return changeInternalVarMonitorStatus(type, varPosition, false);
}

boolean changeInternalVarMonitorStatus(byte type, int varPosition, boolean newStatus)
{
  switch(type)
  {
    case shortVarTypeCode:
      if(varPosition < shortVarsNumber)
      {
        monitoredShortVars[varPosition] = newStatus;
        return true;
      }
      return false;
      
     default:
       return false;
  }
}

void monitorVars()
{
  //Int
  for(int i=0; i<shortVarsNumber; ++i)
  {
    if(monitoredShortVars[i])
    {
      sendShortVar(i, shortVars[i]);
    }
  }
}

void sendAnalogRead(byte pin, int value)
{
  Serial.write(commandControlByte);
  Serial.write(0x1);
  Serial.write(pin);
  Serial.write(highByte(value));
  Serial.write(lowByte(value));
}

const byte varMonitorOutCommand = 0x02;

void sendShortVar(byte index, int value)
{
  const byte shortType = 0x00;
  
  Serial.write(commandControlByte);
  Serial.write(varMonitorOutCommand);
  Serial.write(shortType);
  Serial.write(index);
  Serial.write(highByte(value));
  Serial.write(lowByte(value));
}
