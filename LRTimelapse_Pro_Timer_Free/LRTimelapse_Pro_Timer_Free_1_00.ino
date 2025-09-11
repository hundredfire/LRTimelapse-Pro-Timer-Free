/*
  Pro-Timer Free
  Gunther Wegner
  http://gwegner.de
  http://lrtimelapse.com
  https://github.com/gwegner/LRTimelapse-Pro-Timer-Free
 
  Version 1.14   Bug fixing display delay time > 9:06:07
  Version 1.13   ease in/out Interval ramping implemeted

  Version 1.12   easy entering of Exposure Time 
                 speed up entering of interval and No of shots
                 min autofocus time set to 0.1 will solve problems with several cameras
                 Code optimization
  Version 1.11   Final Version
                 Menory optimization
  Version 1.10   Final Version
                 Some cosmetic improvements in Screens
                 Bug in setup menu fixed
                 CAPTION1 Screen "TLC Edition " added 
                 "Pre Focus Time" remamed in "Autofocus Time"
  Version 1.04B  BETA Version
                 Changed pins for option DPH now Cam 2 shoot = Pin 3 Focus = pin 2
                 some improvements in Bulb Exposure running screen an delay down count screen 
  Version 1.03B  BETA Version
                 Delay Exposure in Event Single Exposure extended to 400 msec
                 Auto Display off time in setup menu adjustable
                 Change order of setup screens. Screen "Start Interval" is now first
                 Max autofocustime extended to 1.5 sec. This value is also used at the Cam wake up function!!
                 dont show remaining Time in event TL runningscreen
  Version 1.02B  BETA Version
                 Sensor control for rising and falling edge implemented 
                 Exposure delay 0 - 400 msec in Sigle exposure mode
                 Sensor triggered Exposure in Single Exposure Mode
                 optimize shoot and focus port control
  Version 1.01B  BETA Version
                 delay time now in sec
                 clean up sketch
                 display sensor status in setup screen for port 2 setup
                 bug fix if sensor goes of during focus
  Version 1.00   BETA Version
                 changed menu structure
  Version 0.93/4 BETA Version
                 HV Sensor control implemented >> Event triggered Timelapse
                 HV left Key in Setup go's to last screen of setup 
  Version 0.93/3 BETA Version
                 HV Adjust No of shots in running screen implemented
  Version 0.93/2 BETA Version
                 HV Bug in interval screen in Setup fixed
                 HV Bug focus handling in stop shooting fixed
                 HV Camera wake up at adjustable interval length implemented, screen in Setup 
  Version 0.93/1 BETA Version
                 HV pinout of camera2 changed: Focus is now SCL, was P2, Shot is now SDA, was P3
                 HV Interval up to 60 min and easy entering of long interval implemented
                 HV Improved Timing handling for shot and focus
                 HV Adjustable autofocus time in setup implemented, Focus Control improved
   Version 0.93  HV second camera port implemented
                 HV external or internal transistor for cam release and focus per #define selectable
                 HV Exposure indicator also on Single Exposure Screen
                 HV Time entering in Bulb Exposure improved, display off after timeout or with each button
                 HV Min Interval setting is limited to min dark time + 0.1 sec >>  this is a bug from version 0.91 and earlier
                 HV Delay Time before TL sequence starts implemented, Display on/off Handling improved
                 HV Auto Display off implemented
                 HV Bug in key Handling in single Mode fixed. Up, down and left key has no function during exposure!!  this is a bug from version 0.91 and earlier
                 HV Bug in Bulb Exposure fixed, some cosmetic improvements on screens
                 HV Decoupling Time adjustable in setup Mode implemented
                 HV Setup Menu implemented, select key only for Display off/on
                 HV Menu Structure changed
  Version 0.92:  HV Timing for Cam release and exposure indicator in timer Interrupt
                 HV Bugg Display dimming not working fixed
                 HV Bugg no exposure in Ramping if interval is <= MIN_DARK_TIME fixed
   Version 0.89: Klaus Heiss: Lcd backlight dimming implemented, Save Params in EEPROM implemented (lib EEPROMConfig) 28.08.17
   Version 0.88: Thanks for Klaus Heiss (KH) for implementing the dynamic key rate
*/


#include <LiquidCrystal.h>
#include "LCD_Keypad_Reader.h"			// credits to: http://www.hellonull.com/?p=282
#include "EEPROMConfig.h"           // Klaus Heiss, www.elite.at

const String CAPTION  =  "Pro-Timer V1.14 ";
const String CAPTION1  = "TLCS  Edition   ";

LCD_Keypad_Reader keypad;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);	//Pin assignments for SainSmart LCD Keypad Shield


#define sensor     // enable event triggerd timelapse
//#define DPH      // change pins for Cam 2 if an 3D printed housing is used and connections are made on the display 

#ifdef sensor
const byte sensor_onL = 2;
const byte sensor_onH = 1;
const byte sensor_off = 0;
byte sensorConf = sensor_off;
byte sensorStat = 0;
byte sensorLastStat = 0;
#endif

// special Character definition

uint8_t ChrRampUp[8] = {
  B00000,
  B01111,
  B00011,
  B00101,
  B01001,
  B10000,
  B00000,
};

uint8_t ChrRampDn[8] = {
  B00000,
  B10000,
  B01001,
  B00101,
  B00011,
  B01111,
  B00000,
};

uint8_t ChrFocus[8] = {
  B00000,
  B00000,
  B00000,
  B00100,
  B00000,
  B00000,
  B00000,
};

uint8_t ChrShoot[8] = {
  B00000,
  B01110,
  B10001,
  B10101,
  B10001,
  B01110,
  B00000,
};
#ifdef sensor
uint8_t ChrHigh[8] = {
  B00000,
  B00111,
  B00100,
  B01110,
  B10101,
  B00100,
  B11100,
};
uint8_t ChrLow[8] = {
  B00000,
  B11100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00111,
};

uint8_t ChrSensor1[8] = {
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
};
#endif

const byte NONE = 0;						// Key constants
const byte SELECT = 1;
const byte LEFT = 2;
const byte UP = 3;
const byte DOWN = 4;
const byte RIGHT = 5;
#ifdef sensor
const byte KEY_SENSOR =6;
#endif

const byte Onboard_LED = 13;
const byte BACK_LIGHT  = 10;

// Camera 1 setings
#define Cam1_ext_HW
//#define Cam1_int_HW
const byte Cam1_shoot = 12;
const byte Cam1_focus = 11;

// Camera 2 setings
#ifdef sensor
//don't change this setting!!!!
#define Cam2_int_HW
#else

// define Cam2 config here if sensor is not defined
//#define Cam2_ext_HW
#define Cam2_int_HW
#endif

const byte on = 1;
const byte off = 0;
#ifdef DPH
const byte Cam2_shoot = 3;       // define pins for Cam2 on DIO 1 and 2 in order to make connections on the display
const byte Cam2_focus = 2;
#else
const byte Cam2_shoot = SDA;
const byte Cam2_focus = SCL;
#endif //DPH

const float RELEASE_TIME_DEFAULT = 0.1;			// default shutter release time for camera

float MIN_DARK_TIME = 0.2;
const float min_MDT = 0.2;
const float max_MDT = 2.0;

const int keyRepeatRate = 100;			// when held, key repeats 1000 / keyRepeatRate times per second

//float decoupleTime   = 0;             // time in 0.1 seconds to wait before doing the first exposure
const float min_DCT  = 0.0;
const float max_DCT  = 4.0;

float AUTO_FOCUS_TIME = 0.1;         // time in 0.1 seconds to pre release focus
const float min_AFT  = 0.1;
const float max_AFT  = 1.5;
byte  focus = 0;

byte CamWakeUptime = 0;
const byte min_WAT = 10;
const byte max_WAT = 30;

int localKey = 0;						// The current pressed key
int lastKeyPressed = -1;				// The last pressed key

unsigned long lastKeyCheckTime = 0;
unsigned long lastKeyPressTime = 0;

float releaseTime = RELEASE_TIME_DEFAULT;			        // Shutter release time for camera
float delayTime              = 0;                     // Dealy time bevor sequnece starts
float delayTimeDC            = 0;                     // Delay Time down counter
const float delayTimeMax = 43200;                     // max delay time = 12:59
const byte delayTimeCursorNO = 0;
const byte delayTimeCursorH  = 1;
const byte delayTimeCursorM  = 2;
const byte delayTimeCursorS  = 3;
byte delayTimeCursor = delayTimeCursorNO;

#ifdef sensor
const int delayMS_Max = 400;
const int delayMS_Min = 0;
int delayMS = delayMS_Min;
int delayMS_CD = delayMS_Min;
byte delayMS_Trigger = 0;
#endif

const byte bulbTimeCursorRdy = 0;
const byte bulbTimeCursorH   = 1;
const byte bulbTimeCursorM   = 2;
const byte bulbTimeCursorS   = 3;
byte bulbTimeCursor = bulbTimeCursorRdy;
const float bulbTimeMax = 28800;       // max bulb time = 7:59:59

unsigned long previousMillis = 0;		   // Timestamp of last shutter release
unsigned long runningTime    = 0;

float interval = 2.0;					         // the current interval
const byte intervalCursorNO = 0;
const byte intervalCursorM  = 1;
const byte intervalCursorS  = 2;
byte intervalCursor = intervalCursorNO;
const float cMinInterval  = 0.2;
const float cMaxInterval = 3659;       // max interval = 60'59"

long maxNoOfShots     = 0;
long currentNoOfShots = 0;
int isRunning         = 0;					  // flag indicates intervalometer is running
unsigned long bulbReleasedAt = 0;

int imageCount = 0;                   // Image count since start of intervalometer

unsigned long rampDuration     = 10;	// ramping duration
const unsigned long rampDurationMax = 300;
float rampTo                   = 0.0;	// ramping interval
unsigned long rampingStartTime = 0;		// ramping start time
unsigned long rampingEndTime   = 0;		// ramping end time
float intervalBeforeRamping    = 0;		// interval before ramping

boolean backLight = HIGH;				      // The current settings for the backlight

// Ease in/out Ramping Definitions

const byte EaseRampingOff =0;
const byte EaseRampingOn  =1;
const byte EaseRampingUp  =2; 
const byte EaseRampingDn  =3;

byte EaseRamping = EaseRampingOff;
int EaseStepsUp;
int EaseStepsDn;

float EaseRampInc;
float Ease_Intvl;
float intervalBeforeEase    = 0;    // interval before Ease ramping



const int SCR_INTERVAL = 0;				    // menu workflow constants
const int SCR_SHOTS    = 1;
const int SCR_MODE     = 9;

/* Screens for Setup */
const int SCR_SU_MDT   = 15;    //Setup  min Dark Time
const int SCR_SU_INTVL = 16;    //Setup Interval
const int SCR_SU_DISP  = 17;    //Setup max display brightness
//const int SCR_SU_DCT   = 18;    //Setup Decoupling time
const int SCR_SU_AFT   = 21;    //Setup AutoFocus Time
const int SCR_SU_WAT   = 22;    //Setup Camera Wakeup Time
const int SCR_SU_ADOT  = 26;    //Setup Auto Display off Time
#ifdef sensor
const int SCR_SU_SENS  = 24;   //Setup Camera2 Port definition Cam2 or Sensor
const int SCR_DELAY_MS = 25;
#endif

const int SCR_EXPOSURE         = 10;
const int SCR_RUNNING          = 2;
const int SCR_CONFIRM_END      = 3;
const int SCR_CONFIRM_END_BULB = 12;
const int SCR_SETTINGS         = 4;
const int SCR_PAUSE            = 5;
const int SCR_RAMP_TIME        = 6;
const int SCR_RAMP_TO          = 7;
const int SCR_NOS_ADJ          = 23;
const int SCR_DONE             = 8;
const int SCR_DELAY_TIME       = 19;
const int SCR_DELAY_COUNT      = 20;
const int SCR_SINGLE           = 11;
const int SCR_EASE_IO          = 30;

const int MODE_M      = 0;
const int MODE_BULB   = 1;
const int MODE_SETUP  = 2;
const int MODE_SINGLE = 3;


int currentMenu = SCR_MODE;		// the currently selected menu
int settingsSel = 1;					// the currently selected settings option
int mode = MODE_M;            // mode: M or Bulb

// K.H. LCD dimming
const int cMinLevel     = 0;  // Min. Background Brightness Levels
const int cMaxLevel     = 5;  // Max. Background Brightness Levels
int  act_BackLightLevel = 4;
char act_BackLightDir   = 'D';

// K.H: EPROM Params
EEPParams EEProm;

// Timer Interrupt Definitions

const byte shooting = 1;
const byte notshooting = 0;
byte cam_Release      = notshooting;
long exposureTime     = 0;           // Timer for exposure in Timer Interrupt (1 msec)
long exposureTimeDisp = 0;           // Timer for exposure Display in Timer Interrupt (1 msec)
byte exposureDisp     = 0;           // set while exposure indicator is on
long displayRS        = 20000;       // Display of in 20 sec (20.000msec)
long displayOff       = displayRS;
const long displayRSmin = 10000;
const long displayRSmax = 60000;
const long displayRSoff = displayRSmax+1000;

long autofocustime     = 0;           // Time for autofocus
int T2RELOAD          = 131;         // Timer reload value for 1000 Hz

int klpTimer = 0;

const int klp1 = 3000;             // same key ist pressed for 3 seconds
const int klp2 = 6000;             // same key ist pressed for 6 seconds

const byte keyspeed1 = 0;
const byte keyspeed2 = 1;
const byte keyspeed3 = 2;

byte keylongpress = keyspeed1;

/**
   Initialize everything
*/
void setup() {
  pinMode(Onboard_LED, OUTPUT);
  digitalWrite(Onboard_LED, LOW);   // Turn Onboard LED OFF. it only consumes battery power ;-)

  pinMode(BACK_LIGHT, OUTPUT);
  digitalWrite(BACK_LIGHT, LOW);    // First turn backlight off.

  Serial.begin(9600);

  // init LCD display

  lcd.createChar(0, ChrRampUp);
  lcd.createChar(1, ChrRampDn);
  lcd.createChar(2, ChrShoot);
  lcd.createChar(3, ChrFocus);
#ifdef sensor
  lcd.createChar(4, ChrSensor1);
  lcd.createChar(5, ChrHigh);
  lcd.createChar(6, ChrLow);
#endif

  digitalWrite(BACK_LIGHT, HIGH);    // Turn backlight on.
// inititialize LCD 
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
// print welcome screen))
  lcd.print(F("LRTimelapse.com"));
  lcd.setCursor(0, 1);
  lcd.print( CAPTION );
  delay (1000);
  lcd.setCursor(0, 1);
  lcd.print( CAPTION1 );



  // Load EEPROM params
  EEProm.ParamsRead();
  // check ranges
  EEProm.Params.BackgroundBrightnessLevel = constrain(EEProm.Params.BackgroundBrightnessLevel, cMinLevel,    cMaxLevel);
  EEProm.Params.Interval                  = constrain(EEProm.Params.Interval,                  cMinInterval, cMaxInterval);
  EEProm.Params.MIN_DARK_TIME             = constrain(EEProm.Params.MIN_DARK_TIME,             min_MDT, max_MDT);
//  EEProm.Params.decoupleTime              = constrain(EEProm.Params.decoupleTime,              min_DCT, max_DCT);
  EEProm.Params.AUTO_FOCUS_TIME           = constrain(EEProm.Params.AUTO_FOCUS_TIME,            min_AFT, max_AFT);
  EEProm.Params.CamWakeUptime             = constrain(EEProm.Params.CamWakeUptime,             0, max_WAT);
  EEProm.Params.displayRS                 = constrain(EEProm.Params.displayRS,                 displayRSmin, displayRSoff);

  act_BackLightLevel = EEProm.Params.BackgroundBrightnessLevel;
  interval           = EEProm.Params.Interval;
  MIN_DARK_TIME      = EEProm.Params.MIN_DARK_TIME;
//  decoupleTime       = EEProm.Params.decoupleTime;
  AUTO_FOCUS_TIME    = EEProm.Params.AUTO_FOCUS_TIME;
  CamWakeUptime      = EEProm.Params.CamWakeUptime;
  displayRS          = EEProm.Params.displayRS;

  // wait a moment...  show CAPTION dimming
  /* H.K.: implemented dimming */
  delay(1500);   //1000!!!
  DimLCD(act_BackLightBrightness(), 0, 1);

  pinMode(3, INPUT);            // only relevant if Pro Timer EXT HW is used

// Set up ports for Camera 1 & 2
#ifdef Cam1_ext_HW
  pinMode(Cam1_shoot, OUTPUT);          // initialize output pin for camera release external HW
  pinMode(Cam1_focus, OUTPUT);          // initialize output pin for camera focus external HW
#endif
#ifdef Cam2_ext_HW
  pinMode(Cam2_shoot, OUTPUT);          // initialize output pin for camera release external HW
  pinMode(Cam2_focus, OUTPUT);          // initialize output pin for camera focus external HW
#endif

#ifdef Cam1_int_HW
  pinMode(Cam1_shoot, INPUT_PULLUP);    // initialize output pin for camera release internal HW
  pinMode(Cam1_focus, INPUT_PULLUP);    // initialize output pin for camera focus internal HW

#endif
#ifdef Cam2_int_HW
  pinMode(Cam2_shoot, INPUT_PULLUP);    // initialize output pin for camera release internal HW
  pinMode(Cam2_focus, INPUT_PULLUP);    // initialize output pin for camera focus internal HW
#endif

// init Timer 2 for Interrupt Timing
  noInterrupts();                      // all Interrupts temporary off
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = T2RELOAD;                    // Timer 1msec (1000Hz)
  TCCR2B = 0x5; //Prescaler = 128
  TIMSK2|= (1 << TOIE1);              // activate Timer Overflow Interrupt 

  interrupts();                        // all Interrupts enable

  lcd.setCursor(0, 1);
  mode = MODE_M;

  printModeMenu();
}
// end setup

/**
   The main loop
*/
void loop() {
  if (millis() > lastKeyCheckTime + keySampleRate) 
  {
    lastKeyCheckTime = millis();
    localKey = keypad.getKey();
      if (localKey == 0 ) {
        keylongpress = keyspeed1;            // no key is pressed 
        klpTimer = 0;
      }
 
    if (localKey != lastKeyPressed) {

      processKey();
      keypad.RepeatRate = keyRepeatRateSlow;
    } else {
    
      // key value has not changed, key is being held down, has it been long enough?
      // (but don't process localKey = 0 = no key pressed)

      /* H.K.: implemented function ActRepeateRate instead of constant */
      if (localKey != 0 && millis() > lastKeyPressTime + keypad.ActRepeatRate()) {
        // yes, repeat this key
        if ( (localKey == UP ) || ( localKey == DOWN ) || ( localKey == SELECT ) ) {
          processKey();
        }
      }
    }

#ifdef sensor

    if ( currentMenu == SCR_SU_SENS ) {
      printScreen();  // update screen in case of Cam2 Config in order to display Sensor Status
    }

    if ((delayMS_Trigger == 1)and (delayMS_CD==0))    // Sensor signal has changed -> process key 
    {
      localKey = KEY_SENSOR;
      delayMS_Trigger =0;
      processKey();     
    }
#endif   //sensor

    if ( currentMenu == SCR_RUNNING ) 
    {
     printScreen();	// update running screen in any case
    }
    
    if ( mode == MODE_BULB ) 
    {
     printScreen();  // update running screen in any case
    }

    if ( currentMenu == SCR_SINGLE  ) 
    {
      possiblyEndLongExposure();
    }
    if ( currentMenu == SCR_DELAY_COUNT ) 
    {
      possiblyEndLongDealy();
      printScreen();  // update running screen in any case
    }
  }

  if ((autofocustime == 0) and (focus == 1))    // end of focus time release Camaera
  {
  #ifdef sensor   
   if (((sensorConf == sensor_onL) && (digitalRead(Cam2_focus) == 1))or ((sensorConf == sensor_onH) && (digitalRead(Cam2_focus) == 0))and (currentMenu!=SCR_SINGLE))
   {
    // switch of focus of cam 1 if sensor = low: Happened if sensor goes low during focus time

 Pin_Cam1_focus(off);
    if ( currentMenu == SCR_RUNNING ) {
    lcd.setCursor(7, 1);
    lcd.print(" ");                          // clear Focus symbol
    }
   }
#endif    
    focus = 0;
    printScreen();  // update running screen in any case
    releaseCamera_1();   //release camera after focus
  }

  if (cam_Release == shooting)
  {
    if (exposureTime == 0)       // End of exposure
    {
     cam_Release = notshooting;
     Pin_Cam1_shoot(off);
     Pin_Cam1_focus(off);

      #ifdef sensor
      if (sensorConf == sensor_off)
      {
       Pin_Cam2_shoot(off);
       Pin_Cam2_focus(off);
      }
      #else
       Pin_Cam2_shoot(off);
       Pin_Cam2_focus(off);
      #endif 

    }
  }
  if (( currentMenu == SCR_RUNNING ) or ( currentMenu == SCR_SINGLE )) 
  {

    if ((exposureTimeDisp == 0) and (exposureDisp == 1))      // End of exposure indicator
    {
      exposureDisp = 0;
      lcd.setCursor(7, 1);
      lcd.print(" ");
    }
  }

  if ( isRunning ) // release camera, do Ramping if running
  {	
    running();
  }

  if (displayRS < displayRSoff)
  {
  if (displayOff == 0) // Display timer
  {            
    if (backLight == 1) 
    {
      backLight = 0;

      DimLCD(act_BackLightBrightness(), 0, 1);

      analogWrite(BACK_LIGHT, act_BackLightBrightness()); // Turn PWM backlight on.
      digitalWrite(BACK_LIGHT, LOW); // Turn backlight off.
    }
  }
  }
}   // End of maim loop

/*
  K.H: dimming LCD BAckground light
*/
void DimLCD( byte startval, byte endval, byte stepdelay) 
{
  if (endval < startval) 
  {
    for ( int bl = startval; bl >= endval; bl--) 
    {
      analogWrite(BACK_LIGHT, bl);    // dimming backlight off.
      delay(stepdelay);
      if (localKey != lastKeyPressed) 
      {
        break;
      }
    }
  }
  else 
  {
    for ( int bl = startval; bl <= endval; bl++) 
    {
      analogWrite(BACK_LIGHT, bl);    // dimming backlight off.
      delay(stepdelay);
      if (localKey != lastKeyPressed) 
      {
        break;
      }
    }
  }
}

/**
  K.H: Change Backlight Brightness in Steps
*/
void changeBackLightBrightness( char AMode) {   //U=up, D=douwn, R=rolling

  if ((AMode == 'U') || (AMode == 'D')) 
  {
    act_BackLightDir = AMode;
  }
  if (act_BackLightDir == 'D') 
  {
    act_BackLightLevel --;
  }
  else 
  {
    act_BackLightLevel ++;
  }

  if (act_BackLightLevel < cMinLevel) 
  {
    act_BackLightLevel = cMinLevel;
  }
  if (act_BackLightLevel > cMaxLevel) 
  {
    act_BackLightLevel = cMaxLevel;
  }
  analogWrite(BACK_LIGHT, act_BackLightBrightness()); // Turn PWM backlight on.
}

/**
  K.H. calc PWM-Value for background display brightness
*/
byte act_BackLightBrightness() 
{
  return constrain (act_BackLightLevel * (255 / cMaxLevel), 12, 255);
}

/**
   K.H. save actual level in EPROM
*/
void save_Params() 
{
  String str;
  if (not isRunning) 
  {
    boolean save = false;
    act_BackLightLevel = constrain(act_BackLightLevel, cMinLevel, cMaxLevel);
    if (act_BackLightLevel != EEProm.Params.BackgroundBrightnessLevel) {
      EEProm.Params.BackgroundBrightnessLevel = act_BackLightLevel;
      save = true;
    };
    if (interval != EEProm.Params.Interval) {
      EEProm.Params.Interval                  = interval;
      save = true;
    };
    if (MIN_DARK_TIME != EEProm.Params.MIN_DARK_TIME) {
      EEProm.Params.MIN_DARK_TIME             = MIN_DARK_TIME;
      save = true;
    };
//    if (decoupleTime != EEProm.Params.decoupleTime) {
//      EEProm.Params.decoupleTime              = decoupleTime;
//      save = true;
//    };
    if (AUTO_FOCUS_TIME != EEProm.Params.AUTO_FOCUS_TIME) {
      EEProm.Params.AUTO_FOCUS_TIME            = AUTO_FOCUS_TIME;
      save = true;
    };

    if (CamWakeUptime != EEProm.Params.CamWakeUptime) {
      EEProm.Params.CamWakeUptime             = CamWakeUptime;
      save = true;
    };

    if (displayRS != EEProm.Params.displayRS) {
      EEProm.Params.displayRS                 = displayRS;
      save = true;
    };

    if (save) 
    {

      if (not EEProm.ParamsWrite()) 
      {
        lcd.setCursor(0, 0); lcd.print(F("PROBLEM SAVING  "));
        lcd.setCursor(0, 1); lcd.print(F("PARAMS TO EEPROM"));
        delay(3000);
        lcd.clear();
      }
    }
  }
}

/**
   Process the key presses - do the Menu Navigation
*/
void processKey() {

  // select key will switch backlight on at any time
  if ( localKey == SELECT ) {
    if (lastKeyPressed == SELECT) {
    }
    else 
    {
      backLight = !backLight;
      if (backLight) {
        analogWrite(BACK_LIGHT, act_BackLightBrightness()); // Turn PWM backlight on.
        //        digitalWrite(BACK_LIGHT, HIGH); // Turn backlight on.
        displayOff = displayRS;  //reload display off timer
      }
      else 
      {
        digitalWrite(BACK_LIGHT, LOW); // Turn backlight off.
        displayOff = 500;
        //        lastDispTurnedOffTime = millis();
      }
      lastKeyPressed = localKey;
      lastKeyPressTime = millis();
    }
  }
  else {
    lastKeyPressed = localKey;
    lastKeyPressTime = millis();
    if (backLight == 1); {

    }
    if (displayOff == 0) {
      backLight = 1;
      analogWrite(BACK_LIGHT, act_BackLightBrightness()); // Turn PWM backlight on.
    }

    if (lastKeyPressed |= NONE) {
      displayOff = displayRS;     // reload display off timer if any key is pressed and display is on
    }
  }

  // do the menu navigation
  switch ( currentMenu ) {

    case SCR_SINGLE:

      if (cam_Release == notshooting) {   // up / down and left key only if not shooting
        if ( localKey == UP ) {
int bTinc = 0;
          switch (bulbTimeCursor) {

            case bulbTimeCursorRdy:
              bulbTimeCursor = bulbTimeCursorS;
              if (releaseTime < 1) {
                releaseTime = (float)((int)(releaseTime + 1));
              }
              break;

            case bulbTimeCursorS:
              bTinc = 1;
              break;

            case bulbTimeCursorM:
              bTinc = 60;
              break;

            case bulbTimeCursorH:
              bTinc = 3600;
              break;
          }
              releaseTime = (float)((int)(releaseTime + bTinc));
              if (releaseTime > bulbTimeMax) {
                releaseTime = releaseTime - bTinc;
              }

        }

        if ( localKey == DOWN ) {
int bTdec = 0;

          switch (bulbTimeCursor) {

            case bulbTimeCursorRdy:
              bulbTimeCursor = bulbTimeCursorS;
              releaseTime = (float)((int)(releaseTime - 1));
              if (releaseTime > bulbTimeMax) {
                releaseTime = bulbTimeMax;
              }
              break;

            case bulbTimeCursorS:
              bTdec = 1;
//              releaseTime = (float)((int)(releaseTime - 1));
              break;

            case bulbTimeCursorM:
              bTdec = 60;
//              releaseTime = (float)((int)(releaseTime - 60));
//              if (releaseTime < 0) {
//                releaseTime = releaseTime + 60;
//              }

              break;

            case bulbTimeCursorH:
              bTdec = 3600;
//              releaseTime = (float)((int)(releaseTime - 3600));
//              if (releaseTime < 0) {
//                releaseTime = releaseTime + 3600;
//              }
              break;
          }
              releaseTime = (float)((int)(releaseTime - bTdec));
              if (releaseTime < 0) {
                releaseTime = releaseTime + bTdec;
              }
          

          if ( releaseTime < RELEASE_TIME_DEFAULT ) { // if it's too short after decrementing, set to the default release time.
            releaseTime = RELEASE_TIME_DEFAULT;
            lcd.clear();
            if (releaseTime < 1) {
              bulbTimeCursor = bulbTimeCursorRdy;
            }
          }
        }
      }

      if ( localKey == LEFT ) 
#ifdef sensor
      {
        if (releaseTime < 1) 
        {
         if (sensorConf > sensor_off)
         { 
          currentMenu = SCR_DELAY_MS;
 
         }
         else
         {
         currentMenu = SCR_DELAY_TIME;
         delayTimeCursor = delayTimeCursorNO;
         }
        } 
        else
        {
          switch (bulbTimeCursor) 
          {
            case bulbTimeCursorS:
              bulbTimeCursor = bulbTimeCursorM;
              break;
  
            case bulbTimeCursorM:
              bulbTimeCursor = bulbTimeCursorH;
              break;
  
            case bulbTimeCursorH:
             if (sensorConf > sensor_off)
             { 
              currentMenu = SCR_DELAY_MS;
     
             }
             else
             {
              currentMenu = SCR_DELAY_TIME;
              delayTimeCursor = delayTimeCursorNO;
             } 
               break;
  
            case bulbTimeCursorRdy:
              if (( bulbReleasedAt == 0 )and (focus == 0)) // if not running, go to previous screen
              { 
                bulbTimeCursor = bulbTimeCursorS;
              } 
              else // if running, go to confirm screen
              {  
               currentMenu = SCR_CONFIRM_END_BULB;
              }
              break;
          }
       } 
      }

#else // sensor

      {
        if (releaseTime < 1) {
         currentMenu = SCR_DELAY_TIME;
         delayTimeCursor = delayTimeCursorNO;
        }
        else
        {
          switch (bulbTimeCursor) 
          {
            case bulbTimeCursorS:
              bulbTimeCursor = bulbTimeCursorM;
              break;
  
            case bulbTimeCursorM:
              bulbTimeCursor = bulbTimeCursorH;
              break;
  
            case bulbTimeCursorH:
              currentMenu = SCR_DELAY_TIME;
              delayTimeCursor = delayTimeCursorNO;
               break;
  
            case bulbTimeCursorRdy:
              if (( bulbReleasedAt == 0 )and (focus == 0)) // if not running, go to previous screen
              { 
                bulbTimeCursor = bulbTimeCursorS;
              } 
              else // if running, go to confirm screen
              {  
               currentMenu = SCR_CONFIRM_END_BULB;
              }
              break;
          }
       } 
      }
#endif      // sensor
 
      if ( localKey == RIGHT ) 
      {
        switch (bulbTimeCursor) 
        {
          case bulbTimeCursorS:
            bulbTimeCursor = bulbTimeCursorRdy;
            break;

          case bulbTimeCursorM:
            bulbTimeCursor = bulbTimeCursorS;
            break;

          case bulbTimeCursorH:
            bulbTimeCursor = bulbTimeCursorM;
            break;

          case bulbTimeCursorRdy:
            if ( bulbReleasedAt == 0 ) // if not running, go to main screen
            { 
#ifdef sensor
            if (sensorConf == sensor_off)
            { 
#endif //sensor              
               delayTimeDC = delayTime * 1000;
               if (delayTimeDC > 0) 
               {
                currentMenu = SCR_DELAY_COUNT;
                previousMillis = millis();                            
               }
               else 
               {
//                if (decoupleTime > 0) 
//                {
//                  lcd.clear();
//                  lcd.setCursor(0, 0);
//                  lcd.print(F("Decoupling..."));
//                  delay( decoupleTime * 1000 );
//                }
               releaseCamera();
               } 
#ifdef sensor
            }
            else
            {
            releaseCamera();  // in case of event single exposure releas cam without delay or decoupling
            }
#endif  /sensor            
           }
        }
      }
#ifdef sensor
      if ( localKey == KEY_SENSOR) 
      {
       if ( bulbTimeCursor == bulbTimeCursorRdy)
       {  
        if (  cam_Release != shooting)
        {
              releaseCamera(); 
        }      
       }             
      }
#endif   // sensor
   
      break;

#ifdef sensor
    case SCR_DELAY_MS:
      if ( localKey == RIGHT ) 
      {
        currentMenu = SCR_SINGLE;
       lcd.setCursor(7, 1);
       lcd.print("    ");
        
      }
      else if ( localKey == LEFT ) 
      {
        currentMenu = SCR_MODE;
      }
      else if ( localKey == DOWN )  
      {
        if (keylongpress > keyspeed1)
        {
        delayMS -= 10;
        }
        else  
        {
        delayMS -= 1;
        }
        if (delayMS < delayMS_Min) 
        {
          delayMS = delayMS_Min;
        }
      }
      else if ( localKey == UP )  
      {
        if (keylongpress > keyspeed1)
        {
        delayMS += 10;
        }
        else  
        {
        delayMS += 1;
        }
        if (delayMS > delayMS_Max) 
        {
          delayMS = delayMS_Max;
        }
      }

      break;
#endif  //sensor


    case SCR_DELAY_TIME:

      if ( localKey == UP ) {
int dTinc = 0;
        switch (delayTimeCursor) {

          case delayTimeCursorNO:
            delayTimeCursor = delayTimeCursorS;
//       if (delayTime == 0){
//            delayTime = 1; //(float)((float)(delayTime + 1));
//       }     

//            delayTime = (float)((float)(delayTime + 1));
//            if (delayTime > delayTimeMax) {
//              delayTime = delayTime - 1;
//            }
            break;

          case delayTimeCursorH:
            dTinc = 3600;
//            delayTime = (float)((float)(delayTime + 3600));  //+ 1h
//            if (delayTime > delayTimeMax) {
//              delayTime = delayTime - 3600;
//            }
            break;

          case delayTimeCursorM:
            dTinc = 60;

//            delayTime = (float)((float)(delayTime + 60));
//            if (delayTime > delayTimeMax) {
//              delayTime = delayTime - 60;
//            }
            break;
            
         case delayTimeCursorS:
            dTinc = 1;

//            delayTime = (float)((float)(delayTime + 1));
//            if (delayTime > delayTimeMax) {
//              delayTime = delayTime - 1;
//            }
            break;

        }
          delayTime = (float)((float)(delayTime + dTinc));
          if (delayTime > delayTimeMax) {
            delayTime = delayTime - dTinc;
          }
         
      }
      if ( localKey == DOWN ) {
int dTdec = 0;

        switch (delayTimeCursor) {

          case delayTimeCursorNO:
            delayTimeCursor = delayTimeCursorS;
//            delayTime = (float)((float)(delayTime - 1));
//            if (delayTime < 0) {
//              delayTime = delayTime + 1;
//            }
            if (delayTime == 0) {
              delayTimeCursor = delayTimeCursorNO;
            }

            break;

          case delayTimeCursorH:
             dTdec = 3600;
//            delayTime = (float)((float)(delayTime - 3600));  //+ 1h
//            if (delayTime < 0) {
//              delayTime = delayTime + 3600;
//            }
//            if (delayTime == 0) {
//              delayTimeCursor = delayTimeCursorNO;
//            }
            break;

          case delayTimeCursorM:
             dTdec = 60;
//            delayTime = (float)((float)(delayTime - 60));
//            if (delayTime < 0) {
//              delayTime = delayTime + 60;
//            }
//            if (delayTime == 0) {
//              delayTimeCursor = delayTimeCursorNO;
//            }
            break;
         case delayTimeCursorS:
             dTdec = 1;
//            delayTime = (float)((float)(delayTime - 1));
//            if (delayTime < 0) {
//              delayTime = delayTime + 1;
//            }
//            if (delayTime == 0) {
//              delayTimeCursor = delayTimeCursorNO;
//            }
            break;
        }
            delayTime = (float)((float)(delayTime - dTdec));
            if (delayTime < 0) {
              delayTime = delayTime + dTdec;
            }
            if (delayTime == 0) {
              delayTimeCursor = delayTimeCursorNO;
            }
        
      }

      if ( localKey == RIGHT ) {

        switch (delayTimeCursor) {

          case delayTimeCursorNO:
           if (mode == MODE_SINGLE)
           {
            currentMenu = SCR_SINGLE;
            lcd.clear();
             if (releaseTime > 1){
             bulbTimeCursor = bulbTimeCursorS;
             }
           }
           else
           {
            currentMenu = SCR_INTERVAL;
           }  
            delayTimeDC = delayTime * 1000;
            intervalCursor = intervalCursorNO;
          break;

          case delayTimeCursorH:
           delayTimeCursor = delayTimeCursorM;
          break;

          case delayTimeCursorM:
            delayTimeCursor = delayTimeCursorS;
          break;
          
         case delayTimeCursorS:
          if (mode == MODE_SINGLE)
          {
           currentMenu = SCR_SINGLE;
           lcd.clear();
            if (releaseTime > 1){
            bulbTimeCursor = bulbTimeCursorS;
            }
         }
         else
         {
          currentMenu = SCR_INTERVAL;
         }  
          delayTimeDC = delayTime * 1000;
          intervalCursor = intervalCursorNO;
         break;
        }
      }

      if ( localKey == LEFT ) 
      {

        switch (delayTimeCursor) {

          case delayTimeCursorNO:
            currentMenu = SCR_MODE;
            lcd.clear();
          break;

          case delayTimeCursorH:
           currentMenu = SCR_MODE;
            if (mode == MODE_M) {
             releaseTime = RELEASE_TIME_DEFAULT;   // when finaly switching to M-Mode, set the shortest shutter release time.
            }
          break;

          case delayTimeCursorM:
            delayTimeCursor = delayTimeCursorH;
          break;
          
         case delayTimeCursorS:
           delayTimeCursor = delayTimeCursorM;
         break;
        }
      }  
     break;

    case SCR_DELAY_COUNT:

      if ( localKey == LEFT ) {
        currentMenu = SCR_MODE;
      }
      break;

    case SCR_INTERVAL:

      if ( localKey == UP ) {

        switch (intervalCursor) {

          case intervalCursorNO:
            if ( interval >= 10 )
            {
              intervalCursor = intervalCursorS;
            }
            else
            {
            if (keylongpress > keyspeed1)
            {
              interval = (float)((int)(interval * 10) + 10) / 10; // round to 1 decimal place
            }
            else
            {
              interval = (float)((int)(interval * 10) + 1) / 10; // round to 1 decimal place
            }  
            }
            if ( interval >= 10 ) {
              intervalCursor = intervalCursorS;
            }
            break;

          case intervalCursorS:
            interval = (float)((int)interval + 1); // round to 1 decimal place
            if ( interval > cMaxInterval ) {
              interval = interval - 1;
            }
            break;

          case intervalCursorM:
            interval = (float)((int)interval + 60);
            if ( interval > cMaxInterval ) {
              interval = interval - 60;
            }
            break;
        }
      }

      if ( localKey == DOWN ) {

        switch (intervalCursor) {

          case intervalCursorNO:
            if (interval > 10)
            {
              intervalCursor =  intervalCursorS;
            }
            else
            {
            if (keylongpress > keyspeed1)
            {
              interval = (float)((int)(interval * 10) - 10) / 10; // round to 1 decimal place
            }
            else
            {
              interval = (float)((int)(interval * 10) - 1) / 10; // round to 1 decimal place
            }
            }
            break;

          case intervalCursorS:
            interval = (float)((int)interval - 1); // round to 1 decimal place
            if ( interval < 10 ) {
              interval = 9.9;
              intervalCursor = intervalCursorNO;
            }
            break;

          case intervalCursorM:
            interval = (float)((int)interval - 60);
            if ( interval < 0 )
            {
              interval = interval + 60;
            }
            if ( interval < 10 )
            {
              intervalCursor = intervalCursorNO;
            }

            break;
        }

        // limit min Interval depending on MIN_DARK_TIME or AUTO_FOCUS_TIME

        if (AUTO_FOCUS_TIME > MIN_DARK_TIME)
        {
          if ( interval < (AUTO_FOCUS_TIME + 0.1) )
          {
            interval = (AUTO_FOCUS_TIME + 0.1);                 // in MODE_MC & MODE_BULB min interval = AUTO_FOCUS_TIME + 0.2!!
          }
        }
        else
        {
          if ( interval < (MIN_DARK_TIME + 0.1) )
          {
            interval = (MIN_DARK_TIME + 0.1);                 // in MODE_MC & MODE_BULB min interval = MIN_DARK_TIME + 0.2!!
          }
        }

      }
      if ( localKey == RIGHT ) {

        switch (intervalCursor)
        {
          case intervalCursorNO:
            lcd.clear();
            rampTo = interval;      // set rampTo default to the current interval
            currentMenu = SCR_SHOTS;
            break;

          case intervalCursorS:
            lcd.clear();
            rampTo = interval;      // set rampTo default to the current interval
            currentMenu = SCR_SHOTS;
            break;

          case intervalCursorM:
            intervalCursor = intervalCursorS;
            break;
        }
      }

      if ( localKey == LEFT )
      {
        switch (intervalCursor)
        {
          case intervalCursorNO:
            lcd.clear();
            rampTo = interval;      // set rampTo default to the current interval
            currentMenu = SCR_DELAY_TIME;
            delayTimeCursor = delayTimeCursorNO;
            break;

          case intervalCursorM:
            lcd.clear();
            rampTo = interval;      // set rampTo default to the current interval
            currentMenu = SCR_DELAY_TIME;
            delayTimeCursor = delayTimeCursorNO;
            break;

          case intervalCursorS:
            intervalCursor = intervalCursorM;
            break;
        }
      }

      break;

    case SCR_MODE:

      if ( localKey == RIGHT ) 
      {
        if (mode == MODE_SETUP) 
        {
          currentMenu = SCR_SU_INTVL;
        }
        else
        {
#ifdef sensor
        if ((sensorConf > sensor_off)and(mode == MODE_SINGLE)) 
        {
          currentMenu = SCR_DELAY_MS;
        }
        else
        {
#endif  // sensor        

          currentMenu = SCR_DELAY_TIME;
          delayTimeCursor = delayTimeCursorNO;
          releaseTime = RELEASE_TIME_DEFAULT;
          imageCount = 0;    // clear image count
#ifdef sensor
        }
#endif        
        if (mode == MODE_SINGLE) 
        {
         bulbTimeCursor=bulbTimeCursorRdy;
        }

       }

       if (mode == MODE_BULB) 
       {
          if ( interval < (MIN_DARK_TIME + 0.1) ) 
          {
            interval = (MIN_DARK_TIME + 0.1);                 // in MODE_MC min interval = MIN_DARK_TIME + 0.1!!
          }
       }
      }
      else if ( localKey == LEFT ) 
      {
        if (mode == MODE_SETUP) {
        #ifdef sensor  
          currentMenu = SCR_SU_SENS;
        #else
        currentMenu = SCR_SU_ADOT;
        #endif  
        }
      }
      else if ( localKey == UP )  
      {
        switch (mode) 
        {
          case MODE_M:
            mode = MODE_BULB;
            break;

          case MODE_BULB:
            mode = MODE_SINGLE;
            break;

          case MODE_SINGLE:
            mode = MODE_SETUP;
            break;

          case MODE_SETUP:
            mode = MODE_M;
            break;
        }
      }

      else if ( localKey == DOWN )  
      {
        switch (mode) 
        {
          case MODE_M:
            mode = MODE_SETUP;
            break;

          case MODE_BULB:
            mode = MODE_M;
            break;

          case MODE_SETUP:
            mode = MODE_SINGLE;
            break;

          case MODE_SINGLE:
            mode = MODE_BULB;
            break;
        }
      }
      break;

    case SCR_SU_MDT:
      if ( localKey == RIGHT ) 
      {
        currentMenu = SCR_SU_AFT;
        save_Params();
      }

      else if ( localKey == LEFT ) 
      {
        currentMenu = SCR_SU_INTVL;
        save_Params();
      }
      else if ( localKey == DOWN )  
      {
        MIN_DARK_TIME -= 0.1;
        if (MIN_DARK_TIME < min_MDT) 
        {
         MIN_DARK_TIME = min_MDT;
        }
      }
      else if ( localKey == UP )  
      {
        MIN_DARK_TIME += 0.1;
        if (MIN_DARK_TIME > max_MDT) 
        {
          MIN_DARK_TIME = max_MDT;
        }
      }

      break;

//    case SCR_SU_DCT:
//      if ( localKey == RIGHT ) 
//      {
//        currentMenu = SCR_SU_AFT;
//        save_Params();
//      }
//      else if ( localKey == LEFT ) 
//      {
//        currentMenu = SCR_SU_MDT;
//        save_Params();
//      }
//      else if ( localKey == DOWN )  
//      {
//        decoupleTime -= 0.1;
//        if (decoupleTime < min_DCT) {
//          decoupleTime = min_DCT;
//        }
//      }
//      else if ( localKey == UP )  
//      {
//        decoupleTime += 0.1;
//        if (decoupleTime > max_DCT) 
//        {
//          decoupleTime = max_DCT;
//        }
//      }
//      break;

    case SCR_SU_AFT:
      if ( localKey == RIGHT ) 
      {
        currentMenu = SCR_SU_WAT;
        save_Params();
      }
      else if ( localKey == LEFT ) 
      {
        currentMenu = SCR_SU_MDT;
        save_Params();
      }
      else if ( localKey == DOWN )  
      {
        AUTO_FOCUS_TIME -= 0.1;
        if (AUTO_FOCUS_TIME < min_AFT) 
        {
          AUTO_FOCUS_TIME = min_AFT;
        }
      }
      else if ( localKey == UP )  
      {
        AUTO_FOCUS_TIME += 0.1;
        if (AUTO_FOCUS_TIME > max_AFT) 
        {
          AUTO_FOCUS_TIME = max_AFT;
        }
      }

      break;

    case SCR_SU_WAT:
      if ( localKey == RIGHT ) {
//        currentMenu = SCR_SU_INTVL;
        currentMenu = SCR_SU_DISP;
        save_Params();
        intervalCursor = intervalCursorNO;

        if ( interval < (MIN_DARK_TIME + 0.1) ) {
          interval = (MIN_DARK_TIME + 0.1);
        }
      }
      else if ( localKey == LEFT ) {
        currentMenu = SCR_SU_AFT;
        save_Params();
      }
      else if ( localKey == DOWN )  {
        if (CamWakeUptime >= min_WAT)
        {
          CamWakeUptime -= 1;
        }
        if (CamWakeUptime < min_WAT)
        {
          CamWakeUptime = 0;
        }
      }
      else if ( localKey == UP )  {
        CamWakeUptime += 1;
        if (CamWakeUptime < min_WAT)
        {
          CamWakeUptime = min_WAT;
        }

        if (CamWakeUptime > max_WAT)
        {
          CamWakeUptime = max_WAT;
        }
      }
      break;

    case SCR_SU_INTVL:

      if ( localKey == RIGHT ) {

        switch (intervalCursor)
        {
          case intervalCursorNO:
            lcd.clear();
//            currentMenu = SCR_SU_DISP;
            currentMenu = SCR_SU_MDT;
            save_Params();
            break;

          case intervalCursorS:
            lcd.clear();
            currentMenu = SCR_SU_DISP;
            save_Params();
            break;

          case intervalCursorM:
            intervalCursor = intervalCursorS;
            break;
        }
      }

      if ( localKey == LEFT )
      {
        switch (intervalCursor)
        {
          case intervalCursorNO:
            lcd.clear();
//            currentMenu = SCR_SU_WAT;
            currentMenu = SCR_MODE;
            save_Params();
            break;

          case intervalCursorM:
            lcd.clear();
//            currentMenu = SCR_SU_WAT;
            currentMenu = SCR_MODE;
            save_Params();
            break;

          case intervalCursorS:
            intervalCursor = intervalCursorM;
            break;
        }
      }

      if ( localKey == UP ) {

        switch (intervalCursor) {

          case intervalCursorNO:

            if ( interval >= 10 )
            {
              intervalCursor = intervalCursorS;
            }
            else
            {
              interval = (float)((int)(interval * 10) + 1) / 10; // round to 1 decimal place
            }
            if ( interval >= 10 ) {
              intervalCursor = intervalCursorS;
            }

            break;

          case intervalCursorS:
            interval = (float)((int)interval + 1); // round to 1 decimal place
            if ( interval > cMaxInterval ) {
              interval = interval - 1;
            }
            break;

          case intervalCursorM:
            interval = (float)((int)interval + 60);
            if ( interval > cMaxInterval ) {
              interval = interval - 60;
            }
            break;
        }
      }

      if ( localKey == DOWN ) {

        switch (intervalCursor) {

          case intervalCursorNO:
            if ( interval >= 10 ) {
              intervalCursor = intervalCursorS;
            }
            else
            {
              interval = (float)((int)(interval * 10) - 1) / 10; // round to 1 decimal place
            }
            break;

          case intervalCursorS:
            interval = (float)((int)interval - 1); // round to 1 decimal place
            if ( interval < 10 ) {
              interval = 9.9;
              intervalCursor = intervalCursorNO;
            }
            break;

          case intervalCursorM:
            interval = (float)((int)interval - 60);
            if ( interval < 0 )
            {
              interval = interval + 60;
            }
            if ( interval < 10 )
            {
              intervalCursor = intervalCursorNO;
            }

            break;
        }
        // limit min Interval depending on MIN_DARK_TIME or AUTO_FOCUS_TIME

        if (AUTO_FOCUS_TIME > MIN_DARK_TIME)
        {
          if ( interval < (AUTO_FOCUS_TIME + 0.1) )
          {
            interval = (AUTO_FOCUS_TIME + 0.1);                 // in MODE_MC & MODE_BULB min interval = AUTO_FOCUS_TIME + 0.2!!
          }
        }
        else
        {
          if ( interval < (MIN_DARK_TIME + 0.1) )
          {
            interval = (MIN_DARK_TIME + 0.1);                 // in MODE_MC & MODE_BULB min interval = MIN_DARK_TIME + 0.2!!
          }
        }
      }
      break;

    case SCR_SU_DISP:
      if ( localKey == RIGHT ) {
        
        currentMenu = SCR_SU_ADOT;
        save_Params();
      }
      else if ( localKey == LEFT ) {
        currentMenu = SCR_SU_WAT;
        save_Params();
        intervalCursor = intervalCursorNO;
      }
      else if ( localKey == DOWN )  {
        act_BackLightLevel -= 1;
        if (act_BackLightLevel < cMinLevel) {
          act_BackLightLevel = cMinLevel;
        }
        analogWrite(BACK_LIGHT, act_BackLightBrightness()); // Turn PWM backlight on.
      }
      else if ( localKey == UP )  {
        act_BackLightLevel += 1;
        if (act_BackLightLevel > cMaxLevel) {
          act_BackLightLevel = cMaxLevel;
        }
        analogWrite(BACK_LIGHT, act_BackLightBrightness()); // Turn PWM backlight on.
      }
 
      break;

    case SCR_SU_ADOT:
      if ( localKey == RIGHT ) {
#ifdef sensor
        currentMenu = SCR_SU_SENS;
        save_Params();
#else        
        currentMenu = SCR_MODE;
        save_Params();
#endif        
      }
      else if ( localKey == LEFT ) {
        currentMenu = SCR_SU_DISP;
        save_Params();
      }

     else if ( localKey == DOWN )  {
        if (displayRS == displayRSoff)
        {
          displayRS = displayRSmax;
          displayOff = displayRS;   
        }
        else
        {
          displayRS -= 1000;
        }
        if (displayRS < displayRSmin) 
        {
          displayRS = displayRSmin;
        }
      }
      
      else if ( localKey == UP )  {
        if (displayRS >= displayRSmin)
        {
         displayRS += 1000;
        }
        if (displayRS > displayRSmax) 
        {
          displayRS = displayRSoff;
        }
      }
      break;

#ifdef sensor
   case SCR_SU_SENS:
      if ( localKey == RIGHT ) {
        currentMenu = SCR_MODE;
      }

      else if ( localKey == LEFT ) {
        currentMenu = SCR_SU_ADOT;
      }

      else if ( localKey == DOWN )  {
      if (sensorConf == sensor_onL)
      { 
      sensorConf = sensor_onH; 
      }
      else
      {
      sensorConf = sensor_off; 
      pinMode(Cam2_shoot, INPUT_PULLUP);          // initialize output pin for camera release internal HW
      }
       
      }
      else if ( localKey == UP )  {
      if (sensorConf == sensor_off)
      { 
      sensorConf = sensor_onH; 
      }
      else
      {
      sensorConf = sensor_onL; 
      }
      pinMode(Cam2_shoot, OUTPUT);          // initialize output pin for power  on sensor
      digitalWrite(Cam2_shoot, HIGH);
      }
      break;
#endif  //sensor

    case SCR_SHOTS:

      if ( localKey == UP ) {

      switch(keylongpress){
        case keyspeed1:
        maxNoOfShots ++;
        break;
        
        case keyspeed2:
        maxNoOfShots += 10;
        break;
        
        case keyspeed3:
        maxNoOfShots += 100;
        break;
      }

//      if ( maxNoOfShots >= 2500 ) {
//          maxNoOfShots += 100;
//        } else if ( maxNoOfShots >= 1000 ) {
//          maxNoOfShots += 50;
//        } else if ( maxNoOfShots >= 100 ) {
//          maxNoOfShots += 25;
//        } else if ( maxNoOfShots >= 10 ) {
//          maxNoOfShots += 10;
//        } else {
//          maxNoOfShots ++;
//        }
        if ( maxNoOfShots >= 9999 ) { // prevents screwing the ui
          maxNoOfShots = 9999;
        }
      }

      if ( localKey == DOWN ) {

      switch(keylongpress){
        case keyspeed1:
        maxNoOfShots --;
        break;
        
        case keyspeed2:
        maxNoOfShots -= 10;
        break;
        
        case keyspeed3:
        if (maxNoOfShots > 1000)
        {
        maxNoOfShots -= 100;
        }
        else
        {
         maxNoOfShots -= 25;
        }
        break;
      }
      if (maxNoOfShots < 0)
      {
        maxNoOfShots = 0;
      }  
    }
        
        
//        if ( maxNoOfShots > 2500 ) {
//          maxNoOfShots -= 100;
//        } else if ( maxNoOfShots > 1000 ) {
//          maxNoOfShots -= 50;
//        } else if ( maxNoOfShots > 100 ) {
//          maxNoOfShots -= 25;
//        } else if ( maxNoOfShots > 10 ) {
//          maxNoOfShots -= 10;
//        } else if ( maxNoOfShots > 0) {
//          maxNoOfShots -= 1;
//        } else {
//          maxNoOfShots = 0;
//        }
//        }

      if ( localKey == LEFT ) {
        //        currentMenu = SCR_MODE;
        currentMenu = SCR_INTERVAL;
        intervalCursor = intervalCursorNO;
        lcd.clear();
      }

      if ( localKey == RIGHT ) {
        switch (mode) {
          case MODE_M:
          if (maxNoOfShots > 0){
            currentMenu = SCR_EASE_IO;
          }
          else 
          { 
           if (delayTimeDC > 0) {
              currentMenu = SCR_DELAY_COUNT;
              previousMillis = millis();
            }
            else {
              currentMenu = SCR_RUNNING;   // Start shooting
              //             currentMenu = SCR_RAMP_TIME;   // Ramping
              //             rampDuration =0;

              firstShutter();
            }
          }
            break;
            
          case MODE_BULB:
            currentMenu = SCR_EXPOSURE; // in Bulb mode ask for exposure
            break;
            /* HV for S-Motor Controll */
        }
      }
      break;

    // Exposure Time
    case SCR_EXPOSURE:
      if ( localKey == UP ) {
        if(releaseTime < 10)
        {
        if (keylongpress > keyspeed1)
        {
        releaseTime = (float)((int)(releaseTime * 10) + 10) / 10; // round to 1 decimal place
        }
        else
        {
        releaseTime = (float)((int)(releaseTime * 10) + 1) / 10; // round to 1 decimal place
        }
          if (releaseTime > 9.9)
          {
           bulbTimeCursor = bulbTimeCursorS;
          }              

        }
        else
        { 
          switch (bulbTimeCursor) {

            case bulbTimeCursorRdy:
              bulbTimeCursor = bulbTimeCursorS;
              if (releaseTime < 1) {
                releaseTime = (float)((int)(releaseTime + 1));
              }
              break;

            case bulbTimeCursorS:
              releaseTime = (float)((int)(releaseTime + 1));
              if (releaseTime > bulbTimeMax) {
                releaseTime = releaseTime - 1;
              }
              break;

            case bulbTimeCursorM:
              releaseTime = (float)((int)(releaseTime + 60));
              if (releaseTime > bulbTimeMax) {
                releaseTime = releaseTime - 60;
              }
              break;

            case bulbTimeCursorH:
              releaseTime = (float)((int)(releaseTime + 3600));
              if (releaseTime > bulbTimeMax) {
                releaseTime = releaseTime - 3600;
              }
              break;
          } 
        }
        if ( releaseTime > interval - MIN_DARK_TIME ) { // no release times longer then (interval-Min_Dark_Time)
          releaseTime = interval - MIN_DARK_TIME;
        }

        
      }

      if ( localKey == DOWN ) {
        if (( releaseTime > RELEASE_TIME_DEFAULT )and ( releaseTime <10)) {
        if (keylongpress > keyspeed1)
        {
          releaseTime = (float)((int)(releaseTime * 10) - 10) / 10; // round to 1 decimal place
        }
        else
        {
          releaseTime = (float)((int)(releaseTime * 10) - 1) / 10; // round to 1 decimal place
        }
        }
        else
        {
          switch (bulbTimeCursor) {

            case bulbTimeCursorRdy:
              bulbTimeCursor = bulbTimeCursorS;
              releaseTime = (float)((int)(releaseTime - 1));
              if (releaseTime > bulbTimeMax) {
                releaseTime = bulbTimeMax;
              }
              break;

            case bulbTimeCursorS:
              releaseTime = (float)((int)(releaseTime - 1));
               if ( releaseTime <10) {
               bulbTimeCursor = bulbTimeCursorRdy;
               releaseTime = (9.9);
               }
              break;

            case bulbTimeCursorM:
              releaseTime = (float)((int)(releaseTime - 60));
              if (releaseTime < 0) {
                releaseTime = releaseTime + 60;
              }

              break;

//            case bulbTimeCursorH:
//              releaseTime = (float)((int)(releaseTime - 3600));
//              if (releaseTime < 0) {
//                releaseTime = releaseTime + 3600;
//              }
//              break;
          }
        if ( releaseTime <10) {
         bulbTimeCursor = bulbTimeCursorRdy;
        }
          
        }

          if ( releaseTime < RELEASE_TIME_DEFAULT ) { // if it's too short after decrementing, set to the default release time.
            releaseTime = RELEASE_TIME_DEFAULT;
            lcd.clear();
            if (releaseTime < 1) {
              bulbTimeCursor = bulbTimeCursorRdy;
            }
          }        
      }

      if ( localKey == LEFT ) {

          switch (bulbTimeCursor) 
          {
            case bulbTimeCursorS:
              bulbTimeCursor = bulbTimeCursorM;
              break;
  
            case bulbTimeCursorM:
              currentMenu = SCR_SHOTS;
              break;
  
            case bulbTimeCursorRdy:
                bulbTimeCursor = bulbTimeCursorS;
              break;
          }        
      }

      if ( localKey == RIGHT ) {

        switch (bulbTimeCursor) 
        {
          case bulbTimeCursorS:
            bulbTimeCursor = bulbTimeCursorRdy;
            break;

          case bulbTimeCursorM:
            bulbTimeCursor = bulbTimeCursorS;
            break;

          case bulbTimeCursorRdy:
             
               delayTimeDC = delayTime * 1000;
               if (delayTimeDC > 0) 
               {
                currentMenu = SCR_DELAY_COUNT;
                previousMillis = millis();                            
               }
               else 
               {
//                if (decoupleTime > 0) 
//                {
//                  lcd.clear();
//                  lcd.setCursor(0, 0);
//                  lcd.print(F("Decoupling..."));
//                  delay( decoupleTime * 1000 );
//                }
//               releaseCamera();
                 currentMenu = SCR_RUNNING;   // Start shooting
                 firstShutter();

               } 
           }
        }

//        if (delayTimeDC > 0) {
//          currentMenu = SCR_DELAY_COUNT;
//          previousMillis = millis();
//        }
//        else {
//          currentMenu = SCR_RUNNING;   // Start shooting
//
//          firstShutter();
//        }
//      }
      break;

    case SCR_EASE_IO:



      if ( localKey == UP ) {
        if (EaseStepsUp < maxNoOfShots / 3 ) {
          EaseStepsUp += 5;
        }
      }


      if ( localKey == DOWN ) {
        EaseStepsUp -= 5;

        if ( EaseStepsUp < 0 ) {
          EaseStepsUp = 0;
          EaseRamping = EaseRampingOff;

        }
      }

      if ( localKey == LEFT ) {
        currentMenu = SCR_SHOTS;
      }

      if ( localKey == RIGHT ) {

        //            currentMenu = SCR_RUNNING;   // Start shooting

            if (EaseStepsUp > 0) {
              EaseRamping = EaseRampingUp;                 // do ease ramping Up
              EaseStepsDn = EaseStepsUp;

              EaseRampInc = ((((float)(interval - MIN_DARK_TIME )) / (float)EaseStepsUp));
//Serial.println(EaseRampInc);
              intervalBeforeEase = interval;
              interval = MIN_DARK_TIME;                                       // set first interval for Ramping
            }
        
        if (delayTimeDC > 0) {
          currentMenu = SCR_DELAY_COUNT;
          previousMillis = millis();
        }

        else {
          currentMenu = SCR_RUNNING;   // Ramping

          if (isRunning == 0) {            //
            
            lcd.clear();
            isRunning = 1;
            firstShutter();
          }
        }
      }
break;      

    case SCR_RUNNING:

      if ( localKey == LEFT ) { // LEFT from Running Screen aborts

        if ( rampingEndTime == 0 ) { 	// if ramping not active, stop the whole shot, other otherwise only the ramping
          if ( isRunning ) { 		// if is still runing, show confirm dialog
            currentMenu = SCR_CONFIRM_END;
          } else {				// if finished, go to start screen
            //            currentMenu = SCR_INTERVAL;
            currentMenu = SCR_MODE;
          }
          lcd.clear();
        } else { // stop ramping
          rampingStartTime = 0;
          rampingEndTime = 0;
        }
      }

      if ( localKey == RIGHT ) {
        currentMenu = SCR_SETTINGS;
        lcd.clear();
      }

      if ( localKey == UP ) {
        changeBackLightBrightness('U');
      }
      if ( localKey == DOWN ) {
        changeBackLightBrightness('D');
      }
      break;

    case SCR_CONFIRM_END:
      if ( localKey == LEFT ) { // Really abort
        //       currentMenu = SCR_INTERVAL;
        currentMenu = SCR_MODE;

        if ( bulbReleasedAt > 0 ) { // if we are shooting in bulb mode, instantly stop the exposure
          bulbReleasedAt = 0;
        }
        stopShooting();
        lcd.clear();
      }
      if ( localKey == RIGHT ) { // resume
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }
      break;

    case SCR_SETTINGS:

      if ( localKey == DOWN ) {
        settingsSel += 1;

       if ( (settingsSel >2) && (maxNoOfShots == 0 )) {
         settingsSel = 2;
       }
        else {
        if (settingsSel > 3) {
        settingsSel = 3 ;
        }
       }
      }

      if ( localKey == UP ) {
        settingsSel -= 1;

       if ( settingsSel < 1 ) {
         settingsSel = 1;
       }
      }
      if ( localKey == LEFT ) {
        settingsSel = 1;
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }

      if ( localKey == RIGHT && settingsSel == 1 ) {
        isRunning = 0;
        currentMenu = SCR_PAUSE;
        lcd.clear();
      }

      if ( localKey == RIGHT && settingsSel == 2 ) {
        currentMenu = SCR_RAMP_TIME;
        lcd.clear();
      }
      
      if ( localKey == RIGHT && settingsSel == 3 ) {
        currentMenu = SCR_NOS_ADJ;
        currentNoOfShots = maxNoOfShots;
        
        lcd.clear();
      }

      break;

    case SCR_PAUSE:
      if ( localKey == RIGHT ) {
        currentMenu = SCR_RUNNING;
        isRunning = 1;
        previousMillis = millis() - (imageCount * 1000); // prevent counting the paused time as running time;
        lcd.clear();
      }
      break;

    case SCR_RAMP_TIME:
      if ( localKey == RIGHT ) {
        if (rampDuration == 0) {
          currentMenu = SCR_RUNNING;

        }
        else {
          currentMenu = SCR_RAMP_TO;
          lcd.clear();
        }
      }

      if ( localKey == LEFT ) {
       currentMenu = SCR_RUNNING;
        lcd.clear();
      }

      if ( localKey == UP ) {
        if ( rampDuration >= 10) {
          rampDuration += 10;
        } else {
          rampDuration += 1;
        }
        if (rampDuration > rampDurationMax)
        {
          rampDuration = rampDurationMax;
        }
      }
      if ( localKey == DOWN ) {
        if ( rampDuration > 10 ) {
          rampDuration -= 10;
        } else {
          rampDuration -= 1;
        }
        if ( rampDuration <= 1 ) {
          rampDuration = 1;
        }
      }

      break;


    case SCR_RAMP_TO:
      if ( localKey == LEFT ) {
        currentMenu = SCR_RAMP_TIME;
        lcd.clear();
      }

      if ( localKey == UP ) {
        if ( rampTo < 20 ) {
          rampTo = (float)((int)(rampTo * 10) + 1) / 10; // round to 1 decimal place
        } else {
          rampTo = (float)((int)rampTo + 1); // round to 1 decimal place
        }
        if ( rampTo > cMaxInterval ) {
          rampTo = cMaxInterval;
        }
      }

      if ( localKey == DOWN ) {
        if ( rampTo > cMinInterval) {
          if ( rampTo < 20 ) {
            rampTo = (float)((int)(rampTo * 10) - 1) / 10; // round to 1 decimal place
          } else {
            rampTo = (float)((int)rampTo - 1);
          }
        }
      }

      if ( localKey == RIGHT ) { // start Interval ramping
        if ( rampTo != interval ) { // only if a different Ramping To interval has been set!
          intervalBeforeRamping = interval;
          rampingStartTime = millis();
          rampingEndTime = rampingStartTime + rampDuration * 60 * 1000;
        }

        // go back to main screen
        currentMenu = SCR_RUNNING;
        lcd.clear();

      }
      break;

    case SCR_NOS_ADJ:

      if ( localKey == RIGHT ) {
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }

      if ( localKey == LEFT ) {
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }

      if (EaseRamping != EaseRampingDn){    // dont change NOS while Ease ramping down
        if ( localKey == UP ) {
        if ( maxNoOfShots >= 2500 ) {
            maxNoOfShots += 100;
          } else if ( maxNoOfShots >= 1000 ) {
            maxNoOfShots += 50;
          } else if ( maxNoOfShots >= 100 ) {
            maxNoOfShots += 25;
          } else if ( maxNoOfShots >= 10 ) {
            maxNoOfShots += 10;
          } else {
            maxNoOfShots ++;
          }
          if ( maxNoOfShots >= 9999 ) { // prevents screwing the ui
            maxNoOfShots = 9999;
          }
  
         }
      }

      if (EaseRamping != EaseRampingDn){    // dont change NOS while Ease ramping down
        if ( localKey == DOWN ) {
          if ( maxNoOfShots > 2500 ) {
            maxNoOfShots -= 100;
          } else if ( maxNoOfShots > 1000 ) {
            maxNoOfShots -= 50;
          } else if ( maxNoOfShots > 100 ) {
            maxNoOfShots -= 25;
          } else if ( maxNoOfShots > 10 ) {
            maxNoOfShots -= 10;
          } else if ( maxNoOfShots > 0) {
            maxNoOfShots -= 1;
          } 
         if (maxNoOfShots < currentNoOfShots) {
         maxNoOfShots = currentNoOfShots;   
         }
        }
      }
      break;


    case SCR_DONE:

      if ( localKey == LEFT || localKey == RIGHT ) {
        //        currentMenu = SCR_INTERVAL;
        currentMenu = SCR_MODE;

        stopShooting();
        lcd.clear();
      }
      break;



    case SCR_CONFIRM_END_BULB:
      if ( localKey == LEFT ) 
      { // Really abort
        stopShooting();
        currentMenu = SCR_SINGLE;
        lcd.clear();
      }
      if ( localKey == RIGHT ) 
      { // resume
        currentMenu = SCR_SINGLE;
        lcd.clear();
      }
      break;
  }
  printScreen();
}

void stopShooting() {
  isRunning = 0;
  imageCount = 0;
  runningTime = 0;
  bulbReleasedAt = 0;
  cam_Release = notshooting;
  focus=0;
  // switch off shoot and focus
 Pin_Cam1_shoot(off);
 Pin_Cam1_focus(off);
 
#ifdef sensor
if (sensorConf == sensor_off)
{
 Pin_Cam2_shoot(off);
 Pin_Cam2_focus(off);
}
#else
 Pin_Cam2_shoot(off);
 Pin_Cam2_focus(off);
#endif
}

void firstShutter() 
{
  if (delayTime == 0)
  {
//  if (decoupleTime > 0) 
//  {
//    lcd.clear();
//    lcd.setCursor(0, 0);
//    lcd.print(F("Decoupling..."));
//    delay( decoupleTime * 1000 );
//  }
  }
  previousMillis = millis();
    runningTime = 0;
  isRunning = 1;

  lcd.clear();
  printRunningScreen();

  // do the first release instantly, the subsequent ones will happen in the loop
  releaseCamera();
//  imageCount++;   
}

void printScreen() {
  lcd.setCursor(0, 0);    // code optimisation

  switch ( currentMenu ) {

    case SCR_INTERVAL:
      printIntervalMenu();
      break;

    case SCR_MODE:
      printModeMenu();
      break;

    case SCR_SHOTS:
      printNoOfShotsMenu();
      break;

    case SCR_EXPOSURE:
      printExposureMenu();
      break;

    case SCR_RUNNING:
      printRunningScreen();
      break;

    case SCR_CONFIRM_END:
      printConfirmEndScreen();
      break;

    case SCR_CONFIRM_END_BULB:
      printConfirmEndScreenBulb();
      break;

    case SCR_SETTINGS:
      printSettingsMenu();
      break;

    case SCR_PAUSE:
      printPauseMenu();
      break;

    case SCR_RAMP_TIME:
      printRampDurationMenu();
      break;

    case SCR_RAMP_TO:
      printRampToMenu();
      break;

    case SCR_NOS_ADJ:
      printChgNofShotsMenu();
      break;
      
    case SCR_EASE_IO:
      printEaseRampMenu();
      break;
      
    case SCR_DONE:
      printDoneScreen();
      break;

    case SCR_SINGLE:
      printSingleScreen();
      break;

    case SCR_SU_MDT:
      print_SU_MDT_Screen();
      break;

//    case SCR_SU_DCT:
//      print_SU_DCT_Screen();
//      break;

    case SCR_SU_AFT:
      print_SU_AFT_Screen();
      break;

    case SCR_SU_WAT:
      print_SU_WAT_Screen();
      break;

    case SCR_SU_INTVL:
      printIntervalMenu();
      //      print_SU_INTVL_Screen();
      break;

    case SCR_SU_DISP:
      print_SU_DISP_Screen();
      break;

    case SCR_SU_ADOT:
      print_SU_ADOT_Screen();
      break;

#ifdef sensor
    case SCR_SU_SENS:
      print_SU_SENSOR_Screen();
      break;

    case SCR_DELAY_MS:
      print_DELAY_MS_Screen();
      break;
#endif  //sensor

    case SCR_DELAY_TIME:
      print_DELAY_TIME_Screen();
      break;

    case SCR_DELAY_COUNT:
      print_DELAY_COUNT_Screen();
      break;
  }
}

/**
   Running, releasing Camera
*/
void running() {

  // do this every interval only
  if ( ( millis() - previousMillis ) >=  ( ( interval * 1000 )) ) {

    if ( ( maxNoOfShots != 0 ) && ( imageCount >= maxNoOfShots ) ) { // sequence is finished
      // stop shooting
      isRunning = 0;
      currentMenu = SCR_DONE;
      lcd.clear();
      printDoneScreen(); // invoke manually
      stopShooting();
    }

    else { // is running

      runningTime += (millis() - previousMillis );
      previousMillis = millis();

     releaseCamera();
     EaseRamp();
    }
  }

  // do this always (multiple times per interval)
  possiblyRampInterval();
}

/**
   If ramping was enabled do the ramping
*/
void possiblyRampInterval() {

  if ( ( millis() < rampingEndTime ) && ( millis() >= rampingStartTime ) ) {
    interval = intervalBeforeRamping + ( (float)( millis() - rampingStartTime ) / (float)( rampingEndTime - rampingStartTime ) * ( rampTo - intervalBeforeRamping ) );

    if ( releaseTime > interval - MIN_DARK_TIME ) { // if ramping makes the interval too short for the exposure time in bulb mode, adjust the exposure time
      releaseTime =  interval - MIN_DARK_TIME;
    }
    if (releaseTime < 0.1) { // HV correct releaseTime at short interval!!
      releaseTime = 0.1;
    }
  } else {
    rampingStartTime = 0;
    rampingEndTime = 0;
  }
}

/**
   Actually release the camera
*/
void releaseCamera()
{

#ifdef sensor   
   if (((sensorConf == sensor_onH) && (digitalRead(Cam2_focus) == 1)or (sensorConf == sensor_onL) && (digitalRead(Cam2_focus) == 0)or (sensorConf == sensor_off)or (currentMenu == SCR_SINGLE)))
   
   {
   //  release cam if cam 2 is defined as sensor andsensor input = high in single exposure mode
  
imageCount++;     
  // switch on focus pin Cam 1

 Pin_Cam1_focus(on);

  // switch on focus pin Cam 2

  if (sensorConf == sensor_off)   // switch focus of cam port 2 only if set as cam 2
   {
 Pin_Cam2_focus(on);
   }
    if (( currentMenu == SCR_RUNNING ) or ( currentMenu == SCR_SINGLE )) 
    { // display focus indicator on running screen only
      lcd.setCursor(7, 1);
      lcd.write(byte(3));
    }
  
    //  check and set Autofocus time
    if ((CamWakeUptime > 0) and (interval > CamWakeUptime))
    {
      autofocustime = max_AFT * 1000;      // focus time = 1.5 sec !! wake up camera!!
    }
    else
    {
      autofocustime = AUTO_FOCUS_TIME * 1000;
    }
    focus = 1;   
   }
#else   // sensor not defined
  
imageCount++;     
  // switch on focus pin Cam 1

 Pin_Cam1_focus(on);
 Pin_Cam2_focus(on);

  if (( currentMenu == SCR_RUNNING ) or ( currentMenu == SCR_SINGLE )) { // display focus indicator on running screen only
    lcd.setCursor(7, 1);
    lcd.write(byte(3));
  }

  //  check and set autofocus time
  if ((CamWakeUptime > 0) and (interval > CamWakeUptime))
  {
    autofocustime = 1000;      // focus time = 1 sec !! wake up camera!!
  }
  else
  {
    autofocustime = AUTO_FOCUS_TIME * 1000;
  }
  focus = 1;
#endif //sensor
}

void releaseCamera_1()
{
#ifdef sensor
   if (((sensorConf == sensor_onH) && (digitalRead(Cam2_focus) == 1))or ((sensorConf == sensor_onL) && (digitalRead(Cam2_focus) == 0))or (sensorConf == sensor_off)or (currentMenu == SCR_SINGLE))
   {
   // cam release only if sensor is defined and sensor input = high or sensor is not defined or in single exposure mode 
  
  if (releaseTime < 0.2) {
    exposureTimeDisp = releaseTime * 2000;     // for better viewability
  }
  else {
    exposureTimeDisp = releaseTime * 1000;
  }
  exposureTime = releaseTime * 1000;
  cam_Release = shooting;
  if (( currentMenu == SCR_RUNNING ) or ( currentMenu == SCR_SINGLE )) { // display exposure indicator on running screen only

    // display exposure indicator
    lcd.setCursor(7, 1);
    lcd.write(byte(2));
    exposureDisp = 1;
  }
  // switch on shooting pin cam 1

  Pin_Cam1_shoot(on);
  
 if (sensorConf == sensor_off)   // switch on shooting pin cam 2 if set as cam 2
  {
  Pin_Cam2_shoot(on);
  }     

  cam_Release = shooting;

  // long trigger in Bulb-Mode for longer exposures
  if ( releaseTime > 1 )
  {
    if ( bulbReleasedAt == 0 )
    {
      bulbReleasedAt = millis();
    }
  }
 }
#else // sensor
  if (releaseTime < 0.2) {
    exposureTimeDisp = releaseTime * 2000;     // for better viewability
  }
  else {
    exposureTimeDisp = releaseTime * 1000;
  }
  exposureTime = releaseTime * 1000;
  cam_Release = shooting;
  if (( currentMenu == SCR_RUNNING ) or ( currentMenu == SCR_SINGLE )) { // display exposure indicator on running screen only

    // display exposure indicator
    lcd.setCursor(7, 1);
    lcd.write(byte(2));
    exposureDisp = 1;
  }
  // switch on shooting pin cam 1 and 2
  Pin_Cam1_shoot(on);
  Pin_Cam2_shoot(on);
  
      
  cam_Release = shooting;

  // long trigger in Bulb-Mode for longer exposures
  if ( releaseTime > 1 )
  {
    if ( bulbReleasedAt == 0 )
    {
      bulbReleasedAt = millis();
    }
  }
#endif  // sensor  
}
void EaseRamp() {
//  int Dist_Motor;

  if (EaseRamping == EaseRampingOn) {                               // Ease Ramping activ but no ramping
    interval = intervalBeforeEase;
  }
  if (EaseRamping > EaseRampingOff) {                               // Ease Ramping activ

    if (EaseRamping == EaseRampingUp) {                             // Ease Ramping up activ
      interval +=  EaseRampInc;
      EaseStepsUp -= 1;
      if (EaseStepsUp == 0) {
        EaseRamping = EaseRampingOn;                                // prepare for ramping down
      }
    }

    if (EaseRamping != EaseRampingDn)
    {
      if ((maxNoOfShots - imageCount + 1) == EaseStepsDn) {
        EaseRamping = EaseRampingDn;           // start ramping down!!
        interval = intervalBeforeEase;         // at current interval
        EaseRamping = EaseRampingDn;
      }
    }
    
    if (EaseRamping == EaseRampingDn) {                               // Ease Ramping activ
      interval -= EaseRampInc;
      EaseStepsDn -= 1;
      if (EaseStepsDn == 0) {
        EaseRamping = EaseRampingOff;   // prepare end
        interval = intervalBeforeEase;
      }
    }
//Serial.println(interval);    
  }
 } 



/**
  Will be called by the loop and check if a bulb exposure has to end. If so, it will stop the exposure.
*/
void possiblyEndLongExposure() {
  if ( ( bulbReleasedAt != 0 ) && ( millis() >= ( bulbReleasedAt + releaseTime * 1000 ) ) ) {
    bulbReleasedAt = 0;
  }

  if ( currentMenu == SCR_SINGLE ) {
    printSingleScreen();
  }
}

/**
  Will be called by the loop and check if a Delay has to end. If so, it will start sequence.
*/
void possiblyEndLongDealy() {
  if (delayTimeDC > 0) {
    delayTimeDC -= (millis() - previousMillis );
    previousMillis = millis();
  }
  if (delayTimeDC < 0) {
    delayTimeDC = 0;
  }

  if (delayTimeDC == 0) {
    if (mode == MODE_SINGLE)
    {
      currentMenu = SCR_SINGLE;
      lcd.clear();
      releaseCamera();
    }
    else
    {
    currentMenu = SCR_RUNNING;   // Start shooting
    firstShutter();
    }
  }
}

// ---------------------- SCREENS AND MENUS -------------------------------

/**
   Pause Mode
*/
void printPauseMenu() {

  lcd.setCursor(0, 0);
  lcd.print(F("PAUSE...        "));
  lcd.setCursor(0, 1);
  lcd.print(F("Continue      > "));
}

/**
   Configure Interval setting (main screen)
*/
void printIntervalMenu() {

//  lcd.setCursor(0, 0);

  if ( currentMenu == SCR_INTERVAL)
  {
    lcd.print(F("Interval        "));
  }
  else
  {
    lcd.print(F("Start Interval  "));
  }

  lcd.setCursor(0, 1);
  if ( interval < 10 ) {
    lcd.print( printFloat( interval, 5, 1 ) );
    lcd.print(F( " sec         " ));

  } else {

    int minutes = ( (int)interval / 60 ) % 60;
    int secs = ( (int)interval ) % 60;
    String sMinutes = fillZero( minutes );
    String sSecs = fillZero( secs );

    if (interval > 3599)
    {
      lcd.print(F("60"));
    }
    else
    {
      lcd.print( sMinutes );
    }
    lcd.print( "'");
    lcd.print( sSecs );
    lcd.print((char)34);
    lcd.print(" ");
  }

  switch (intervalCursor) {

    case intervalCursorM:
      lcd.print(F("MIN  sec>"));
      break;

    case intervalCursorS:
      lcd.print(F("SEC  <min"));
      break;

    case intervalCursorNO:
      if (interval < 10)
      {
        lcd.print(F(" sec      "));
      }
      else
      {
        lcd.print(F("          "));
      }
      break;
  }
}
/**
   Configure Exposure setting in Bulb mode
*/
void printExposureMenu() {

//  lcd.setCursor(0, 0);
  lcd.print(F("Exposure        "));
  lcd.setCursor(0, 1);

if (releaseTime < 10)
{
  lcd.print(releaseTime);
  lcd.print(F( " sec        " ));
}
else
{
  int minutes = ( (int)releaseTime / 60 ) % 60;
  int secs = ( (int)releaseTime ) % 60;
  String sMinutes = fillZero( minutes );
  String sSecs = fillZero( secs );
      lcd.print( sMinutes );
      lcd.print( "'");
      lcd.print( sSecs );
      lcd.print((char)34);
      lcd.print( " " );

    switch (bulbTimeCursor) {

      case bulbTimeCursorRdy:
    
        lcd.print(F( "   Start>"));
        break;

      case bulbTimeCursorS:
        lcd.print(F( "SEC  <min"));
        break;

      case bulbTimeCursorM:
        lcd.print(F( "MIN     <"));
        break;
     }
}
}

/**
   Configure Mode setting
*/

#ifdef sensor
void printModeMenu() {

//  lcd.setCursor(0, 0);
  lcd.print(F("Mode            "));
  lcd.setCursor(0, 1);
  if (sensorConf > sensor_off)
  {
  switch (mode) {
    case MODE_M:
      lcd.print(F( "Event TL (M)    " ));
      break;
    case MODE_BULB:
      lcd.print(F("Event TL Bulb   " ));
      break;
    case MODE_SINGLE:
      lcd.print(F( "Event Singl.Exp." ));
      break;
case MODE_SETUP:
      lcd.print(F( "Setup           " ));
      break;
  }
 }
 else
 {
  switch (mode) {
    case MODE_M:
      lcd.print(F( "Timelapse (M)   " ));
      break;
    case MODE_BULB:
      lcd.print(F( "TL Bulb (Astro) " ));
      break;
    case MODE_SINGLE:
      lcd.print(F( "Single Exposure " ));
      break;
    case MODE_SETUP:
      lcd.print(F( "Setup           " ));
      break;
  }  
 }
} 
#else
void printModeMenu() {

//  lcd.setCursor(0, 0);
  lcd.print(F("Mode            "));
  lcd.setCursor(0, 1);

  switch (mode) {
    case MODE_M:
      lcd.print(F( "Timelapse (M)   " ));
      break;
    case MODE_BULB:
      lcd.print(F( "TL Bulb (Astro) " ));
      break;
    case MODE_SINGLE:
      lcd.print(F( "Single Exposure " ));
      break;
case MODE_SETUP:
      lcd.print(F( "Setup           " ));
      break;
  }
}
#endif 

/**
   Configure no of shots - 0 means infinity
*/
void printNoOfShotsMenu() {

//  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("No of shots     "));
  lcd.setCursor(0, 1);
  if ( maxNoOfShots > 0 ) {
    lcd.print( printInt( maxNoOfShots, 4 ) );
  lcd.print(F( "       ")); // clear rest of display
  } else {
    lcd.print(F("unlimited       " ));
  }
}

/**
   Print running screen
*/
void printRunningScreen() {
//  lcd.setCursor(0, 0);
  lcd.print( printInt( imageCount, 4 ) );

  if ( maxNoOfShots > 0 ) {
    lcd.print( "R:" );
    lcd.print( printInt( maxNoOfShots - imageCount, 4 ) );
    //     lcd.print( " " );
    lcd.setCursor(0, 1);
    // print remaining time
    unsigned long remainingSecs = (maxNoOfShots - imageCount) * interval;

#ifdef sensor 
    if (sensorConf == sensor_off)
    {
    lcd.print( "T-");
    lcd.setCursor(2, 1);
    if ((remainingSecs/60/60)<100)
    {
    lcd.print( fillZero( remainingSecs / 60 / 60 ) );
    }
    else
    {
    lcd.print( ">>");
    }
    lcd.setCursor(4, 1);
    lcd.print( ":" );
    lcd.print( fillZero( ( remainingSecs / 60 ) % 60 ) );
    }
#else
    lcd.print( "T-");
    lcd.setCursor(2, 1);
    if ((remainingSecs/60/60)<100)
    {
    lcd.print( fillZero( remainingSecs / 60 / 60 ) );
    }
    else
    {
    lcd.print( ">>");
    }
    lcd.setCursor(4, 1);
    lcd.print( ":" );
    lcd.print( fillZero( ( remainingSecs / 60 ) % 60 ) );
#endif
 
  }



  updateTime();

  lcd.setCursor(10, 0);
#ifdef sensor
   if ((sensorConf == sensor_onH)or (sensorConf == sensor_onL))
   {
     if (digitalRead(Cam2_focus) == 1)
     { 
      lcd.write(byte(4));
     }
     else
     {
      lcd.print ("_");    
     }
   }
   else
   { 
#endif //sensor

    if (EaseRamping > EaseRampingOff) {         // Ease RampingUp activ
      if (EaseRamping == EaseRampingUp) {
        lcd.write(byte(0));
      }

      if (EaseRamping == EaseRampingDn) {         // Ease RampingDn activ
        lcd.write(byte(1));
      }
      if (EaseRamping == EaseRampingOn) {         // Ease Ramping activ
        lcd.print( " " );
      }
    }


    if ( millis() < rampingEndTime ) {
     if (rampTo > interval) {
      lcd.write(byte(0));
    }
    else if (rampTo < interval) {
      lcd.write(byte(1));
    }
   } else {
    lcd.print( " " );
   }
#ifdef sensor
  }
#endif

  if ( interval < 10 ) { // prevent the interval display from being cut
    lcd.setCursor(12,0);
//    lcd.print (" ");
    lcd.print( printFloat( interval, 4, 1 ) );
  } else {
    //    lcd.print( printFloat( interval, 4, 0 ) );
    lcd.setCursor(11,0);

    int minutes = ( (int)interval / 60 ) % 60;
    int secs = ( (int)interval ) % 60;
    String sMinutes = fillZero( minutes );
    String sSecs = fillZero( secs );

    if (interval > 3599)
    {
      lcd.print("60");
    }
    else
    {
      lcd.print( sMinutes );
    }
    lcd.print( "'");
    lcd.print( sSecs );
    lcd.print((char)34);
  }
}

void printDoneScreen() {
  if (imageCount > 0)   //dont refresh done screen if it is called again !!
  {
  // print elapsed image count))
  //  lcd.setCursor(0, 0);
  lcd.print("Done ");
  lcd.print( imageCount );
  lcd.print( " shots.");

  // print elapsed time when done
  lcd.setCursor(0, 1);
  lcd.print( "t=");
  lcd.print( fillZero( runningTime / 1000 / 60 / 60 ) );
  lcd.print( ":" );
  lcd.print( fillZero( ( runningTime / 1000 / 60 ) % 60 ) );
  lcd.print("    ;-)");
  }
}

void printConfirmEndScreen() {
//  lcd.setCursor(0, 0);
  lcd.print( "Stop shooting?  ");
  lcd.setCursor(0, 1);
  lcd.print( "< Stop    Cont.>");
}

void printConfirmEndScreenBulb() {
//  lcd.setCursor(0, 0);
  lcd.print( "Stop exposure?  ");
  lcd.setCursor(0, 1);
  lcd.print( "< Stop    Cont.>");
}

void printSingleScreen() {
  lcd.setCursor(0, 0);

#ifdef sensor
  if ( releaseTime < 1 ) 
  {
  if (sensorConf > sensor_off)
  {
    lcd.print(F( "Event Singl.Exp."));
  }
  else
  { 
    lcd.print(F( "Single Exposure "));
  }
    lcd.setCursor(0, 1);
    lcd.print(releaseTime); // under one second
    lcd.print( "   ");
    lcd.setCursor(11, 1);
      if (sensorConf > sensor_off)
      {
        lcd.print( "Wait");
       if (digitalRead(Cam2_focus) == 1)
        {
        lcd.write(byte(4));
        }
        else
        {
        lcd.print("_");  
        }       
      }
      else
      {
        lcd.print(F( "Fire>"));
      }
  }    
#else
  if ( releaseTime < 1 ) {

    lcd.print(F( "Single Exposure "));
    lcd.setCursor(0, 1);
    lcd.print(releaseTime); // under one second
    lcd.print( "   ");
    lcd.setCursor(11, 1);
    lcd.print( "Fire>");
   }
#endif  //sensor   
   
   else 
   {
#ifdef sensor
      if (sensorConf > sensor_off)
      {
      lcd.print(F( "Event Bulb Exp. "));
      }
      else
      {
      lcd.print(F( "Bulb Exposure   "));
      }
#else
 
    lcd.print(F( "Bulb Exposure   "));
#endif
    lcd.setCursor(0, 1);

    //if (cam_Release == notshooting) {
      // display exposure time setting

//prepPrintTime(releaseTime);

//      int hours = (int)releaseTime / 60 / 60;
//      int minutes = ( (int)releaseTime / 60 ) % 60;
//      int secs = ( (int)releaseTime ) % 60;
//      String sHours = fillZero( hours );
//      String sMinutes = fillZero( minutes );
//      String sSecs = fillZero( secs );      
    if (( bulbReleasedAt == 0 )and (focus == 0)) { // if not shooting
     
//      int hours = (int)releaseTime / 60 / 60;
//      int minutes = ( (int)releaseTime / 60 ) % 60;
//      int secs = ( (int)releaseTime ) % 60;
//      String sHours = fillZero( hours );
//      String sMinutes = fillZero( minutes );
//      String sSecs = fillZero( secs );

prepPrintTime(releaseTime);

//      lcd.print( pHours );
//      lcd.print(":");
//      lcd.print( pMinutes );
//      lcd.print( "'");
//      lcd.print( pSecs );
//      lcd.print((char)34);
//      lcd.print( " " );
    }

  if ( bulbReleasedAt == 0 ) { // currently not running
    // if (cam_Release == notshooting) {

   if (focus == 1)
   {
      
//      int hours = (int)releaseTime / 60 / 60;
//      int minutes = ( (int)releaseTime / 60 ) % 60;
//      int secs = ( (int)releaseTime ) % 60;
//      String bHours = fillZero( hours );
//      String bMinutes = fillZero( minutes );
//      String bSecs = fillZero( secs );

    lcd.setCursor(0, 1);
    lcd.print("< Stop  "); 

prepPrintTime(releaseTime);
    lcd.setCursor(7, 1);
    lcd.write(byte(3));           // Focus symbol

//      lcd.print( pHours );
//      lcd.print(":");
//      lcd.print( pMinutes );
//      lcd.print( "'");
//      lcd.print( pSecs );
//      lcd.print((char)34);
//      lcd.print( " " );


  
//    lcd.setCursor(8, 1);
//    lcd.print( bHours );
//    lcd.setCursor(10, 1);
//    lcd.print(":");
//    lcd.setCursor(11, 1);
//    lcd.print( bMinutes );
//    lcd.setCursor(13, 1);
//    lcd.print(":");
//    lcd.setCursor(14, 1);
//    lcd.print( bSecs );
//    lcd.setCursor(7, 1);
//    lcd.write(byte(3));           // Focus symbol

   }
   else
   {
    lcd.setCursor(10, 1);

    switch (bulbTimeCursor) {

      case bulbTimeCursorRdy:
#ifdef sensor
      if (sensorConf > sensor_off)
      {
        lcd.print( " Wait");
       if (digitalRead(Cam2_focus) == 1)
        {
        lcd.write(byte(4));
        }
        else
        {
        lcd.print("_");  
        }       
      }
      else
      {
        lcd.print(F( " Fire>"));
      }        
      break;       
#else        
        lcd.print(F( " Fire>"));
        break;
#endif
      case bulbTimeCursorS:
        lcd.print(F( "S <min"));
        break;

      case bulbTimeCursorM:
        lcd.print(F( "M <hrs"));
        break;

      case bulbTimeCursorH:
        lcd.print(F( "H    <"));
        break;
    }
   }
  } 
  else { // running

    //    bulbReleasedAt = 0;
    unsigned long runningTime = ( bulbReleasedAt + releaseTime * 1000 ) - millis();
    //    unsigned long runningTime = (releaseTime * 1000 );
 
     unsigned long finerRunningTime = runningTime+1000;

     int hours = finerRunningTime / 1000 / 60 / 60;
     int minutes = (finerRunningTime / 1000 / 60) % 60;
     int secs = (finerRunningTime / 1000 ) % 60;

      
//      int hours = runningTime / 1000 / 60 / 60;
//      int minutes = ( runningTime / 1000 / 60 ) % 60;
//      int secs = ( runningTime / 1000 ) % 60;
      String sHours = fillZero( hours );
      String sMinutes = fillZero( minutes );
      String sSecs = fillZero( secs );


    lcd.setCursor(0, 1);
    lcd.print(F("< Stop ")); 

    lcd.setCursor(8, 1);
    lcd.print( sHours );
//    lcd.setCursor(10, 1);
    lcd.print(":");
//    lcd.setCursor(11, 1);
    lcd.print( sMinutes );
//    lcd.setCursor(13, 1);
    lcd.print("'");
//    lcd.setCursor(14, 1);
    lcd.print( sSecs );
    lcd.setCursor(7, 1);
    lcd.write(byte(2));           // Shooting symbol
 
   }
  }
}

#ifdef sensor
void print_DELAY_MS_Screen(){
  lcd.setCursor(0, 0);
  lcd.print(F( "Delay Exposure  "));
  lcd.setCursor(0, 1);
//  lcd.print (" ");
//  lcd.print (delayMS);
  lcd.print( printInt( delayMS, 4 ) );

  lcd.print (F(" msec         "));

}
#endif
void print_DELAY_TIME_Screen() {
  lcd.setCursor(0, 0);
  lcd.print(F( "Delay Time      "));
  lcd.setCursor(0, 1);

//prepPrintTime(delayTime);
  // display delay time setting
  int hours = (long)delayTime / 60 / 60;
  int minutes = ( (long)delayTime / 60 ) % 60;
  int secs = ( (long)delayTime ) % 60;
  String sHours = fillZero( hours );
  String sMinutes = fillZero( minutes );
  String sSecs = fillZero( secs );

  lcd.print( sHours );
  lcd.print(":");
  lcd.print( sMinutes );
  lcd.print( "'");
  lcd.print(sSecs);
  lcd.print((char)34);

  switch (delayTimeCursor) {

    case delayTimeCursorH:
      lcd.print(F("H min>"));
      break;

    case delayTimeCursorM:
      lcd.print("M <hrs");
      break;

    case delayTimeCursorS:
      lcd.print(F("S <min"));
      break;


    case delayTimeCursorNO:
      lcd.print(F("          "));
      break;
  }
}

void print_DELAY_COUNT_Screen() {
//  lcd.setCursor(0, 0);

  if (mode == MODE_SINGLE)
  { 
  lcd.print(F( "Exposure in...  "));
  }
  else
  {  
  lcd.print(F( "TL starts in... "));
  }
  unsigned long finerDelayTime = delayTimeDC+1000;

  int hours = finerDelayTime / 1000 / 60 / 60;
  int minutes = (finerDelayTime / 1000 / 60) % 60;
  int secs = (finerDelayTime / 1000 ) % 60;

  String sHours = fillZero( hours );
  String sMinutes = fillZero( minutes );
  String sSecs = fillZero( secs );
  lcd.setCursor (0, 1);
  lcd.print("< Stop  ");
  lcd.setCursor(8, 1);
  lcd.print( sHours );
  //  lcd.setCursor(9, 1);
  lcd.print(":");
//  lcd.setCursor(10, 1);
  lcd.print( sMinutes );
  //  lcd.setCursor(12, 1);
  lcd.print( "'");
//  lcd.setCursor(13, 1);
  lcd.print( sSecs );
//  lcd.print((char)34);
}

void print_SU_MDT_Screen() {
//  lcd.setCursor(0, 0);
  lcd.print(F( "min Dark Time   "));
  lcd.setCursor(0, 1);
  lcd.print( printFloat( MIN_DARK_TIME, 4, 1 ) );
  lcd.print( " sec        " );
}

//void print_SU_DCT_Screen() {
////  lcd.setCursor(0, 0);
//  lcd.print(F( "Decoupling Time "));
//  lcd.setCursor(0, 1);
//  lcd.print( printFloat( decoupleTime, 4, 1 ) );
//  lcd.print( " sec        " );
//}

void print_SU_AFT_Screen() {
//  lcd.setCursor(0, 0);
  lcd.print(F( "Autofocus Time  "));
  lcd.setCursor(0, 1);
  lcd.print( printFloat( AUTO_FOCUS_TIME, 4, 1 ) );
  lcd.print( " sec        " );
}

void print_SU_WAT_Screen() {
//  lcd.setCursor(0, 0);
  if (CamWakeUptime < min_WAT)
  {
    lcd.print(F( "Cam wake up     "));
  }
  else
  {
    lcd.print(F( "Cam wake up at  "));
  }
  lcd.setCursor(0, 1);
  if (CamWakeUptime < min_WAT)
  {
    lcd.print(F( "off             "));
  }
  else
  {
    lcd.print(F( "Intvl.> "));
    lcd.setCursor(8, 1);
    lcd.print( printFloat( CamWakeUptime, 2, 0 ) );
    lcd.print( " sec " );
  }
}

void print_SU_INTVL_Screen() {
//  lcd.setCursor(0, 0);
  lcd.print(F("Start Interval  "));
  lcd.setCursor(0, 1);
  if ( interval < 20 ) {
    lcd.print( printFloat( interval, 5, 1 ) );
  } else {
    lcd.print( printFloat( interval, 3, 0 ) );
  }
  lcd.print(F( " sec       " ));
  lcd.setCursor(15, 1);
  lcd.print( " " );
}

void print_SU_DISP_Screen() {
//  lcd.setCursor(0, 0);
  lcd.print(F("Std Disp Bright."));
  lcd.setCursor(0, 1);
  lcd.print(F( "Level " ));
  lcd.print(act_BackLightLevel + 1);
  lcd.print(F( "         " ));
}

void print_SU_ADOT_Screen() {
//  lcd.setCursor(0, 0);
  lcd.print(F("Auto Display off"));
  lcd.setCursor(0, 1);
  if (displayRS == displayRSoff)
  {
    lcd.print( "always on       ");
  }
  else
  {
   lcd.print(F( "after " ));
   lcd.print(displayRS / 1000);
   lcd.print( " sec      " );
  }
}


#ifdef sensor
void print_SU_SENSOR_Screen() {
//  lcd.setCursor(0, 0);
  lcd.print(F("Port 2 Setup     "));
  lcd.setCursor(0, 1);
  if ( sensorConf > sensor_off)
  {
  lcd.print(F("Sensor Input    " ));
  lcd.setCursor(13, 1);
  if (sensorConf == sensor_onH)
  {
  lcd.write(byte(5)); 
  }
  else
  {
  lcd.write(byte(6)); 
  }
   lcd.setCursor(15, 1);
    if (digitalRead(Cam2_focus) == 1)
    {
    lcd.write(byte(4));
    }
    else
    {
    lcd.print("_");  
    }
  }
  else
  {
   lcd.print(F("Camera 2        " ));
  }
}
#endif

/**
   Update the time display in the main screen
*/
void updateTime() {


    unsigned long finerRunningTime = runningTime + (millis() - previousMillis);   

  if ( isRunning ) {

    int hours = finerRunningTime / 1000 / 60 / 60;
    int minutes = (finerRunningTime / 1000 / 60) % 60;
    int secs = (finerRunningTime / 1000 ) % 60;

    String sHours = fillZero( hours );
    String sMinutes = fillZero( minutes );
    String sSecs = fillZero( secs );

    lcd.setCursor(8, 1);

    if (hours > 99)
    {
      lcd.print(">>");
    }
    else
    {
      lcd.print( sHours );
    }
    lcd.setCursor(10, 1);
    lcd.print(":");
    lcd.setCursor(11, 1);
    lcd.print( sMinutes );
    lcd.setCursor(13, 1);
    lcd.print(":");
    lcd.setCursor(14, 1);
    lcd.print( sSecs );

  } else {
    lcd.setCursor(8, 1);
    lcd.print("   Done!");
  }
}

/**
   Print Settings Menu
*/
void printSettingsMenu() {

  lcd.setCursor(0, 0);
  if (maxNoOfShots > 0) { 
    if (settingsSel == 1) { 
    lcd.print(">Pause          ");
    lcd.setCursor(0, 1);
    lcd.print(" Ramp Interval  ");
    }
    if (settingsSel == 2) { 
    lcd.print(">Ramp Interval  ");
    lcd.setCursor(0, 1);
    lcd.print(" No of shots inc");
    }
    if (settingsSel == 3) { 
    lcd.print(" Ramp Interval  ");
    lcd.setCursor(0, 1);
    lcd.print(">No of shots inc");
    }
  }
  else {
  lcd.setCursor(1, 0);
  lcd.print("Pause          ");
  lcd.setCursor(1, 1);
  lcd.print("Ramp Interval  ");
  lcd.setCursor(0, settingsSel - 1);
  lcd.print(">");
  lcd.setCursor(0, 1 - (settingsSel - 1));
  lcd.print(" ");
  }
}

/**
   Print Ramping Duration Menu
*/
void printRampDurationMenu() {

  lcd.setCursor(0, 0);
  lcd.print("Ramp Time (min) ");

  lcd.setCursor(0, 1);
  lcd.print( printInt( rampDuration, 3 ) );

// lcd.print( rampDuration );

  lcd.print( " min         " );
}

/**
   Print Ramping To Menu
*/
void printRampToMenu() {

  lcd.setCursor(0, 0);
  lcd.print("Ramp to (Intvl.)");

  lcd.setCursor(0, 1);
  if ( rampTo < 20 ) {
    lcd.print( printFloat( rampTo, 5, 1 ) );
  } else {
    lcd.print( printFloat( rampTo, 3, 0 ) );
  }
  lcd.print( "             " );
}
void printEaseRampMenu() {

  //  lcd.setCursor(0, 0);
  lcd.print(F  ("Ease in/out     "));

  lcd.setCursor(0, 1);
  lcd.print( EaseStepsUp);

  if (EaseStepsUp == 0) {
    lcd.print(F("   no Ease i/o>" ));
  }

  else {
    lcd.print(F("  Shots       " ));
  }

}


void printChgNofShotsMenu() {

//  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("No of shots inc.");
  lcd.setCursor(0, 1);
  lcd.print( printInt( maxNoOfShots, 4 ) );
  lcd.print( "           "); // clear rest of display
}


ISR(TIMER2_OVF_vect)
{
  TCNT2 = T2RELOAD;             // Reload Timer Counter
  TIFR2 = 0x00;


#ifdef sensor

    if (( currentMenu == SCR_SINGLE ) and ( bulbTimeCursor == bulbTimeCursorRdy))
    {
      if(delayMS_Trigger == 0)
      { 
      sensorStat = digitalRead(Cam2_focus);
        if (sensorStat != sensorLastStat)
        {
       sensorLastStat = sensorStat; 
    
          if ((sensorConf == sensor_onH) and (sensorStat == 1)) 
          {
           delayMS_CD = delayMS;
           delayMS_Trigger = 1;
          }
          if ((sensorConf == sensor_onL) and (sensorStat == 0)) 
          { 
           delayMS_CD = delayMS;
           delayMS_Trigger = 1;
          } 
        }
      }
      if (delayMS_Trigger == 1)
      {
        if (delayMS_CD > 0)
        {
        delayMS_CD -=1;    
        }
        else
        {
          if ((sensorConf == sensor_onH) and (sensorStat == 1)) 
          { 
            Pin_Cam1_shoot(on);
            Pin_Cam1_focus(on);
          }
          if ((sensorConf == sensor_onL) and (sensorStat == 0)) 
          { 
            Pin_Cam1_shoot(on);
            Pin_Cam1_focus(on);
         }
       }
      }
    }     
#endif //sensor

  if (exposureTime > 0) {
    exposureTime --;
  }
  if (exposureTimeDisp > 0) {
    exposureTimeDisp --;
  }

  if (autofocustime > 0) {
    autofocustime --;
  }

  if (displayOff > 0) {
    displayOff --;
  }
  if (klpTimer < klp2)
  {
  klpTimer ++;
  }
  if (klpTimer == klp1)
  {
     keylongpress = keyspeed2;           
  }
  if (klpTimer == klp2)
  {
     keylongpress = keyspeed3;           
  }
}

void Pin_Cam1_shoot (byte state)
{
  if (state == on)
  {
  // switch on cam1 shoot pin
    #ifdef Cam1_ext_HW
     digitalWrite(Cam1_shoot, HIGH);
    #endif
    #ifdef Cam1_int_HW
     pinMode(Cam1_shoot, OUTPUT);            // Set port to output
     digitalWrite(Cam1_shoot, LOW);          // Set port to low > relase cam
    #endif
  }
  else
  { 
  // switch off cam1 shoot pin
   #ifdef Cam1_ext_HW
     digitalWrite(Cam1_shoot, LOW);    // end of exposure cam 1
   #endif

   #ifdef Cam1_int_HW
    pinMode(Cam1_shoot, INPUT_PULLUP);          // Set port to input = high
   #endif    
  }
}

void Pin_Cam2_shoot (byte state)
{
  if (state == on)
  {
  // switch on cam2 shoot pin
    #ifdef Cam2_ext_HW
     digitalWrite(Cam2_shoot, HIGH);
    #endif
    #ifdef Cam2_int_HW
     pinMode(Cam2_shoot, OUTPUT);            // Set port to output
     digitalWrite(Cam2_shoot, LOW);          // Set port to low > relase cam
    #endif
  }
  else
  { 
  // switch off cam2 shoot pin
   #ifdef Cam2_ext_HW
     digitalWrite(Cam2_shoot, LOW);    // end of exposure cam 1
   #endif

   #ifdef Cam2_int_HW
    pinMode(Cam2_shoot, INPUT_PULLUP);          // Set port to input = high
   #endif    
  }
}

void Pin_Cam1_focus (byte state)
{
  if (state == on)
  {
  // switch on cam1 focus pin
   #ifdef Cam1_ext_HW
    digitalWrite(Cam1_focus, HIGH);
   #endif

   #ifdef Cam1_int_HW
    pinMode(Cam1_focus, OUTPUT);            // Set port to output
    digitalWrite(Cam1_focus, LOW);          // Set port to low > relase cam
   #endif
  }
  else
  { 
  // switch off cam1 focus pin
  #ifdef Cam1_ext_HW
    digitalWrite(Cam1_focus, LOW);
  #endif  
  #ifdef Cam1_int_HW
    pinMode(Cam1_focus, INPUT_PULLUP);          // Set port to input = high
  #endif   
  }
}

void Pin_Cam2_focus (byte state)
{
  if (state == on)
  {
  // switch on cam2 focus pin
   #ifdef Cam2_ext_HW
    digitalWrite(Cam2_focus, HIGH);
   #endif

   #ifdef Cam2_int_HW
    pinMode(Cam2_focus, OUTPUT);            // Set port to output
    digitalWrite(Cam2_focus, LOW);          // Set port to low > relase cam
   #endif
  }
  else
  { 
  // switch off cam2 focus pin
  #ifdef Cam2_ext_HW
      digitalWrite(Cam2_focus, LOW);
  #endif  
  #ifdef Cam2_int_HW
        pinMode(Cam2_focus, INPUT_PULLUP);          // Set port to input = high
  #endif   
  }
}





// ----------- HELPER METHODS -------------------------------------

/**
   Fill in leading zero to numbers in order to always have 2 digits
*/
String fillZero( int input ) {

  String sInput = String( input );
  if ( sInput.length() < 2 ) {
    sInput = "0";
    sInput.concat( String( input ));
  }
  return sInput;
}

String printFloat(float f, int total, int dec) {

  static char dtostrfbuffer[8];
  String s = dtostrf(f, total, dec, dtostrfbuffer);
  return s;
}

String printInt( int i, int total) {
  float f = i;
  static char dtostrfbuffer[8];
  String s = dtostrf(f, total, 0, dtostrfbuffer);
  return s;
}

void prepPrintTime(float pTime)
{
      int phours = (int)pTime / 60 / 60;
      int pminutes = ( (int)pTime / 60 ) % 60;
      int psecs = ( (int)pTime ) % 60;
      String pHours = fillZero( phours );
      String pMinutes = fillZero( pminutes );
      String pSecs = fillZero( psecs );
      lcd.print( pHours );
      lcd.print(":");
      lcd.print( pMinutes );
      lcd.print( "'");
      lcd.print( pSecs );
      lcd.print((char)34);
      lcd.print( " " );

} 
