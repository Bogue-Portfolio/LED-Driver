#include <Encoder.h>
#include <EEPROM.h>
Encoder knob(3,5);

const int redPin = 10;
const int grnPin = 9;
const int bluPin = 11;

const int switch1 = 7;
const int switch2 = 6;
const int switch3 = 5;

const int OFF = 0;
const int BREATHE = 1;
const int H_CYCLE = 2;
const int MORSE = 3;
const int WHITE = 4;
const int CONST = 5;

const int SENSOR_MAX = 1000;
const int SENSOR_MIN = 512;
const int NUM_MODES = 6;

float rgb[3];

float H = 0;
float dH = .1/360;
float cycleHOffset = 0;
float S = 1;
float dS = .00001;

float V = 1;
float dV= .00001;

String message = "feel the bern";
String morseKey[] = {".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."};
float timeStep = 1.0/2000;
float morse_H = 0;
int *compMorse = NULL;
int compMorseLength = 0;

int mode = 0;
int newMode = 0;
int metaMode = 0;

float Vmod = 1;
float metaData[4];

int last = 0;

boolean s[5]; //Arrays start at one. Totally.

const int numReadings = 10;
float pastReadings[numReadings];
int readingIdx = 0;


const int switchClk  = 7;
const int switchData = 2;
const int switchHold = 6;

int count;
int saveCount;
void setup(){
  pinMode(redPin, OUTPUT);
  pinMode(grnPin, OUTPUT);
  pinMode(bluPin, OUTPUT);
 // pinMode(switch1, INPUT);
  //pinMode(switch2, INPUT);
  pinMode(5, INPUT);
  //pinMode(switch3, INPUT);
  pinMode(3, INPUT);
  pinMode(5,INPUT);
  //setMode(MORSE, 240);  
  pinMode(switchClk,OUTPUT);
  pinMode(switchData,INPUT);
  pinMode(switchHold,OUTPUT);
  setMode(CONST,70);
  Serial.begin(9600);
  EEPROM.get(0,H);
  digitalWrite(13,OUTPUT);
  updateMode();
}

void loop(){

  count++;
  if(count %50 == 0){
    digitalWrite(13,LOW);
    updateMode();
    
    count = 0;
    saveCount++;
  }
  if(saveCount > 250 && mode != H_CYCLE){
    float tmp;
    EEPROM.get(0,tmp);
    if(tmp != H){
      digitalWrite(13,HIGH);
      EEPROM.put(0,H);
    }
    saveCount = 0;
  }
  
  int reading = -knob.read();
  if(reading%4 == 0){
    if(last != reading/4){
      setHue(H + 10*(reading/4-last));
      last = reading/4;
    }
  }
  updateVmod();
  tick();
  if(mode == H_CYCLE)
    getRGB(normalizeHue(H + cycleHOffset),S,V*Vmod,rgb);
  else
    getRGB(H,S,V*Vmod,rgb);  
  setLED(rgb);
}

void updateMode(){
  digitalWrite(switchHold,HIGH);
    byte data = 0;
    for(int i = 0;i<8;i++){
      data = (data >> 1) | digitalRead(switchData) << 7;
      digitalWrite(switchClk, LOW);
      digitalWrite(switchClk, HIGH);
    }
    digitalWrite(switchHold,LOW);
    newMode = data & 0x0F;
    if(newMode != mode){
      setMode(newMode,H);
      //Serial.println(newMode);
    }
    s[1] = (data >> 7) & 0x1;
    s[2] = (data >> 6) & 0x1;
    s[3] = (data >> 5) & 0x1;
    s[4] = (data >> 4) & 0x1;
}

void updateVmod(){
  if(s[1]){
    pastReadings[readingIdx] = (.95/numReadings)*getExtBrightness();
    
    float tmp = .05;
    for(int i = 0;i< numReadings;i++)
      tmp += pastReadings[i];
    Vmod = tmp;
    readingIdx++;
    readingIdx %= numReadings;
  }
  else
    Vmod = Vmod*.5 + .5*(.95*getExtBrightness()+.05);
  if(s[2])
    Vmod = 1;
}

float getExtBrightness(){
  return constrain(1-(analogRead(5)-SENSOR_MIN)/float(SENSOR_MAX-SENSOR_MIN),0,1);
}
void setHue(float Hue){
  H = Hue - 360 * floor(Hue/360);
}
float normalizeHue(float Hue){
  return Hue - 360 * floor(Hue/360);
}
void tick(){
  switch(mode){
  case BREATHE:
    switch(metaMode){
    case 0: 
      metaData[0] +=1;
      if(metaData[0] >= 30000){
        metaMode = 1;
        metaData[0] = 0;
      }
      break;
    case 1:
      V -= metaData[1];
      if(V<.4){
        V = .4;
        metaMode = 2;
      }
      break;
    case 2: 
      metaData[0] +=1;
      if(metaData[0] >= 10000){
        metaMode = 3;
        metaData[0] = 0;
      }
      break;
    case 3:
      V += metaData[1];
      if(V > 1){
        V = 1;
        metaMode = 0;
      }
      break;

    }
    break;
    case H_CYCLE:
      cycleHOffset += dH;
      cycleHOffset = normalizeHue(cycleHOffset);
    break;
    case MORSE:
      // m0 = Compiled morse step
      // m1 = Time Unit goal
      // m2 = current time unit
      if(metaData[2] <= metaData[1]){
        metaData[2] +=timeStep;
        break;
      }
      metaData[2] = 0;
      metaData[0]++;
      
      if(metaData[0] >= compMorseLength)
        metaData[0] = 0;
        
      metaData[1] = abs(compMorse[(int)metaData[0]]);
      
      if(compMorse[(int)metaData[0]] > 0)
        V = 1;
      else 
        V = 0;
    break;      
  }
}
void setMode(int m, float p){
  mode = m;
  metaMode = 0;
  switch(m){
  case BREATHE:
    setHue(p);
    S = 1;
    V = 1;
    metaData[0] = 0;
    metaData[1] = .00005;
    break;
  case H_CYCLE:
    H = p;
    S = 1;
    V = 1;
    break;
  case MORSE:{
      H = p;
      S = 1;
      V = 1;
      metaData[0] = -1;
      metaData[1] = 0;
      metaData[2] = 10;
      
      int length = 1+message.length();
      for(int i = 0;i<message.length();i++){
        if(message.charAt(i) != ' ')
          length+=morseKey[message.charAt(i)-'a'].length()*2;
      }
      
      if(compMorse == NULL)
        delete [] compMorse;
      compMorse = new int[length];
      compMorseLength = length;
      int compI = 0;
      
      for(int i = 0;i<message.length();i++){
        if(message.charAt(i) == ' '){
          compMorse[compI] = -4;
          compI++;
        } else{
          for(int i2 = 0;i2< morseKey[message.charAt(i)-'a'].length();i2++){
            char c = morseKey[message.charAt(i)-'a'].charAt(i2);
            if(c == '.')
              compMorse[compI] = 1;
            else
              compMorse[compI] = 3;
            compMorse[compI+1] = -1;
            compI+=2;
          }
          compMorse[compI] = -2;
          compI++;
        }
      }
      compMorse[length-1] = -8;
      break;
    }
  case WHITE:
    S = 0;
    V =1;
    break;
  case CONST:
    S = 1;
    setHue(p);
    V = 1;
    break;
  default:
    S=V=0;
  }
}


void setLED(float rgb[]){
  analogWrite(redPin,int(rgb[0]*255));
  analogWrite(grnPin,int(rgb[1]*255));
  analogWrite(bluPin,int(rgb[2]*255));
}

void getRGB(float H, float S, float V, float rgb[]) {
  //H -= 360 * int(H/360);
  //if(H<0) H+=360;
  float Hp = H/60;
  float C = V*S;
  float X = C*(1.0-abs((Hp-2*int(Hp/2))-1));
  float m = V-C;
  C += m;
  X += m;
  float *r = &rgb[0];
  float *g = &rgb[1];
  float *b = &rgb[2];
  switch (int(Hp)){
  case 0:
    *r = C;
    *g = X;
    *b = m;
    break;
  case 1:
    *r = X;
    *g = C;
    *b = m;
    break;
  case 2:
    *r = m;
    *g = C;
    *b = X;
    break;
  case 3:
    *r = m;
    *g = X;
    *b = C;
    break;
  case 4:
    *r = X;
    *g = m;
    *b = C;
    break;
  case 5:
    *r = C;
    *g = m;
    *b = X;
    break;
  default:
    *r = 0;
    *g = 0;
    *b = 0;
    break;
  }
}


