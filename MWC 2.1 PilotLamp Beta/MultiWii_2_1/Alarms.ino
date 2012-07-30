 #if defined(BUZZER) || defined(PILOTLAMP)
  static uint8_t beeperOnBox,
                 warn_noGPSfix = 0,
                 warn_failsafe = 0, 
                 warn_runtime = 0; 
 #endif 
 #if defined(BUZZER) 
  static uint8_t buzzerIsOn = 0, buzzerSequenceActive=0, beepDone =0;
  static uint32_t buzzerLastToggleTime;
  uint8_t isBuzzerON() { return buzzerIsOn; } // returns true while buzzer is buzzing; returns 0 for silent periods
 #endif
 
void alarmHandler(uint8_t warn_vbat){
 #if defined(BUZZER) || defined(PILOTLAMP)
  #if defined(RCOPTIONSBEEP)
    static uint8_t i = 0;
    static uint8_t last_rcOptions[CHECKBOXITEMS];
    if (last_rcOptions[i] != rcOptions[i]){toggleBeep = 1;}
    last_rcOptions[i] = rcOptions[i]; 
    i++;
    if(i >= CHECKBOXITEMS)i=0;
  #endif  
  
  //=====================  BeeperOn via rcOptions =====================
  if ( rcOptions[BOXBEEPERON] ){ // unconditional beeper on via AUXn switch 
    beeperOnBox = 1;
  }else{
    beeperOnBox = 0;
  }
  //===================== Beeps for failsafe =====================
  #if defined(FAILSAFE)
    if ( failsafeCnt > (5*FAILSAVE_DELAY) && f.ARMED) {
      warn_failsafe = 1;                                                                   //set failsafe warning level to 1 while landing
      if (failsafeCnt > 5*(FAILSAVE_DELAY+FAILSAVE_OFF_DELAY)) warn_failsafe = 2;          //start "find me" signal after landing   
    }
    if ( failsafeCnt > (5*FAILSAVE_DELAY) && !f.ARMED) warn_failsafe = 2;                  // tx turned off while motors are off: start "find me" signal
    if ( failsafeCnt == 0) warn_failsafe = 0;                                              // turn off alarm if TX is okay
  #endif
  //===================== GPS fix notification handling =====================
  #if GPS
    if ((rcOptions[BOXGPSHOME] || rcOptions[BOXGPSHOLD]) && !f.GPS_FIX){    //if no fix and gps funtion is activated: do warning beeps.
      warn_noGPSfix = 1;    
    }else{
      warn_noGPSfix = 0;
    }
  #endif
  //===================== Runtime Warning =====================
  #if defined(ARMEDTIMEWARNING)
  if (armedTime >= ArmedTimeWarningMicroSeconds){
    warn_runtime = 1;
  }
  #endif
 #endif 
 
 #if defined(BUZZER)
   buzzerHandler(warn_vbat);
 #endif
 #if defined(PILOTLAMP)
   PilotLampHandler();
 #endif
}

#if defined(BUZZER)

void buzzerHandler(uint8_t warn_vbat){
  static uint16_t ontime, offtime, beepcount, repeat, repeatcounter;
  //===================== Priority driven Handling =====================
  // beepcode(length1,length2,length3,pause)
  //D: Double, L: Long, S: Short, N: None
  //if (warn_failsafe == 2)      beep_code('L','N','N','D');                 //failsafe "find me" signal
  if (warn_failsafe == 1) beep_code('S','L','L','M');                 //failsafe landing active         
  else if (warn_noGPSfix == 1) beep_code('S','S','N','M');       
  else if (beeperOnBox == 1)   beep_code('S','S','S','M');                 //beeperon
  else if (warn_runtime == 1 && f.ARMED == 1)beep_code('S','S','M','N'); //Runtime warning      
  else if (warn_vbat == 4)     beep_code('S','L','L','D');       
  else if (warn_vbat == 2)     beep_code('S','S','L','D');       
  else if (warn_vbat == 1)     beep_code('S','L','N','D');          
  else if (toggleBeep == 1)    beep_code('S','N','M','N');       
  else if (toggleBeep == 2)    beep_code('S','S','N','N');       
  else if (toggleBeep > 2)     beep_code('S','S','S','N');  
  //fast confirmation beep
  else if (buzzerSequenceActive == 1) beep_code('N','N','N','N');                //if no signal is needed, finish sequence if not finished yet
  else{                                                                   //reset everything and keep quiet
    buzzerIsOn = 0;
    BUZZERPIN_OFF;
  }  
}

void beep_code(char first, char second, char third, char pause){
  static char patternChar[4];
  uint16_t Duration;
  static uint8_t icnt = 0;
  
  if (buzzerSequenceActive == 0){    //only change sequenceparameters if prior sequence is done
    buzzerSequenceActive = 1;
    patternChar[0] = first; 
    patternChar[1] = second;
    patternChar[2] = third;
    patternChar[3] = pause;
  }
  switch(patternChar[icnt]) {
    case 'L': 
      Duration = 200; 
      break;
    case 'D': 
      Duration = 2000; 
      break;
    case 'N': 
      Duration = 0; 
      break;
    default:
      Duration = 50; 
      break;
  }
  if(icnt <3 && Duration!=0){
    Buzzer_beep(Duration,50);
  }
  if (icnt >=3 && (buzzerLastToggleTime<millis()-Duration) ){
    icnt=0;
    toggleBeep = 0;
    buzzerSequenceActive = 0;                              //sequence is now done, next sequence may begin
  }
  if (beepDone == 1 || Duration == 0){
    if (icnt < 3){icnt++;}    
    buzzerIsOn = 0;
    beepDone =0;
    BUZZERPIN_OFF;
  }  
}

void Buzzer_beep( uint16_t pulse,uint16_t pause){  
  if ( !buzzerIsOn && (millis() >= (buzzerLastToggleTime + pause)) ) {	          // Buzzer is off and pause time is up -> turn it on
    buzzerIsOn = 1;
    BUZZERPIN_ON;
    buzzerLastToggleTime=millis();      // save the time the buzer turned on
  } else if (buzzerIsOn && (millis() >= buzzerLastToggleTime + pulse) ) {         //Buzzer is on and time is up -> turn it off
    buzzerIsOn = 0;
    BUZZERPIN_OFF;
    buzzerLastToggleTime=millis();   
    beepDone =1;
  }
} 
#endif


#if defined (PILOTLAMP) 
  void PilotLampHandler(){
    //==================LED Sequence ===========================
    if (beeperOnBox){
      PilotLampSequence();
    }else{
      //==================GREEN LED===========================
      if (f.ARMED && f.ACC_MODE) usePilotLamp('G',1000,1000);
      else if (f.ARMED) usePilotLamp('G',500,500);
      else usePilotLamp('G',0,0);    //switch off  --> muss noch programmiert werden
      //==================BLUE LED===========================
      #if GPS
        if (!f.GPS_FIX) usePilotLamp('B',100,100);
        else if ((rcOptions[BOXGPSHOME] || rcOptions[BOXGPSHOLD]) && f.GPS_FIX) usePilotLamp('B',1000,1000);
        else usePilotLamp('B',100,1000);
      #endif   
      //==================RED LED===========================
      if (warn_failsafe==1)usePilotLamp('R',100,100);
      else if (warn_failsafe==2)usePilotLamp('R',1000,2000);
      else usePilotLamp('R',0,0);
     }
   }
  
  int usePilotLamp(char resource, uint16_t pulse, uint16_t pause){  
    static uint8_t channel =0, channelIsOn[4] = {0,0,0,0};
    static uint32_t channelLastToggleTime[4] ={0,0,0,0};
    
     switch(resource) {
        case 'B': 
          channel = 0;
          break;
        case 'R': 
          channel = 1;
          break;
        case 'G': 
          channel = 2;
          break;
        default:
          channel = 3;
          break;
      }
    if ( !channelIsOn[channel] && (millis() >= (channelLastToggleTime[channel] + pause))&& pulse != 0 ) {	         
      channelIsOn[channel] = 1;
          
      switch(channel) {
        case 0: 
          PilotLamp(PL_BLU_ON);
          break;
        case 1: 
          PilotLamp(PL_RED_ON);
          break;
        case 2: 
          PilotLamp(PL_GRN_ON);
          break;
        default:
          PilotLamp(PL_BZR_ON); 
          break;
      }
      channelLastToggleTime[channel]=millis();      
      return 0;
    } else if (channelIsOn[channel] && (millis() >= channelLastToggleTime[channel] + pulse)|| pulse==0 ) {        
      channelIsOn[channel] = 0;
      switch(channel) {
        case 0: 
          PilotLamp(PL_BLU_OFF);
          break;
        case 1: 
          PilotLamp(PL_RED_OFF);
          break;
        case 2: 
          PilotLamp(PL_GRN_OFF);
          break;
        default:
          PilotLamp(PL_BZR_OFF); 
          break;
      }
      channelLastToggleTime[channel]=millis();    
      return 1;
    }
  } 
  
  union pl_reg {
      struct {
          unsigned grn: 1;
          unsigned blu: 1;
          unsigned red: 1;
          unsigned bzr: 1;
      } ctrl;
      uint8_t pl_ctrl_all;
  };
  
  // *********************************************************************************************************************
  // PilotLamp() is called with the PL_XX parameter (see below). In order to prevent unneccessary
  // increases in IMU cycle time, the bit-bang freq control waveform is only generated if the device (LED or buzzer) 
  // state update is different from the previous state.
  // Available device parameters are:
  //    PL_INIT { must be called once in main sketch setup() }
  //    PL_GRN_ON, PL_GRN_OFF { Green LED control }
  //    PL_BLU_ON, PL_BLU_OFF { Blue  LED control }
  //    PL_RED_ON, PL_RED_OFF { Red   LED control }
  //    PL_BZR_ON, PL_BZR_OFF { Buzzer control }
  //
  void PilotLamp(uint16_t device)
  {
      uint8_t i;
      static union pl_reg mode;
      
      if(device == PL_INIT) {                            // Initialize the Pilot Lamp.
          mode.pl_ctrl_all = 0x00;                       // Reset all LED and Buzzer status states.
          for (i=0;i<4;i++) {                            // Create the required freq waveforms to control the various modes.
              PL_PIN_ON;
              delayMicroseconds(PL_GRN_OFF);
              PL_PIN_OFF;
              delayMicroseconds(PL_GRN_OFF);
  
              PL_PIN_ON;
              delayMicroseconds(PL_BLU_OFF);
              PL_PIN_OFF;
              delayMicroseconds(PL_BLU_OFF);
  
              PL_PIN_ON;
              delayMicroseconds(PL_RED_OFF);
              PL_PIN_OFF;
              delayMicroseconds(PL_RED_OFF);
                
              PL_PIN_ON;
              delayMicroseconds(PL_BZR_OFF);
              PL_PIN_OFF;
              delayMicroseconds(PL_BZR_OFF);              
          }
      }
      else {                                                  // Check to see if the new state request is different than the current state.
          if(device==PL_GRN_OFF && mode.ctrl.grn==1) {
              mode.ctrl.grn = 0;
          }
          else if(device==PL_GRN_ON && mode.ctrl.grn==0) {
              mode.ctrl.grn = 1;
          }
          else if(device==PL_BLU_OFF && mode.ctrl.blu==1) {
              mode.ctrl.blu = 0;
          }
          else if(device==PL_BLU_ON && mode.ctrl.blu==0) {
              mode.ctrl.blu = 1;
          }
          else if(device==PL_RED_OFF && mode.ctrl.red==1) {
              mode.ctrl.red = 0;
          }
          else if(device==PL_RED_ON && mode.ctrl.red==0) {
              mode.ctrl.red = 1;
          }
          else if(device==PL_BZR_OFF && mode.ctrl.bzr==1) {
              mode.ctrl.bzr = 0;
          }
          else if(device==PL_BZR_ON && mode.ctrl.bzr==0) {
              mode.ctrl.bzr = 1;
          }
          else {                                             // No state changes
              PL_PIN_OFF;
              return;                                       // Skip signal generation.
          }
          
          for (i=0;i<4;i++) {                                // Create the freq signal to activate Pilot Lamp.
              PL_PIN_ON;
              delayMicroseconds(device);
              PL_PIN_OFF;
              delayMicroseconds(device);
          }
      }
      
      return;
  }
      
  
  // *****************************************************************************************************************
  // PilotLampTest() can be used to troubleshoot the LED/Buzzer module. Place it in the main loop and you 
  // should see the three LED's sequence in order with a periodic short beep.
  void PilotLampTest(void)
  {
       static uint8_t cam1 = 0;
       static uint8_t cam2 = 0;
       static uint8_t state = 0;
       
       if(cam1++ > 40) {                                // Update rate is perhaps 2Hz, but will vary.
           cam1 = 0;
           if(cam2++ > 20 && state==1) {                // Periodically beep the buzzer (every few seconds).
               cam2 = 0;
               PilotLamp(PL_BZR_ON);
               delay(200);                              // Brute force delay is needed because maim loop will immediately turn off buzzer.
           }
           else {
               switch(state) {                          // Light-chase Cycle the LED's.
                   case 0:
                       state++;
                       PilotLamp(PL_BZR_OFF);
                       break;
                   case 1:
                       state++;
                       PilotLamp(PL_GRN_ON);
                       break;
                   case 2:
                       state++;
                       PilotLamp(PL_GRN_OFF);
                       PilotLamp(PL_BLU_ON);
                       break;
                   case 3:
                       state++;
                       PilotLamp(PL_BLU_OFF);
                       PilotLamp(PL_RED_ON);
                       break;
                   case 4:
                       state = 0;
                       PilotLamp(PL_RED_OFF);
                       break;
               }
          }             
       }
    
      return;
  }
  
   void PilotLampSequence(void)
  {
       static uint32_t lastswitch = 0;
       static uint8_t state = 0;
       
       if(millis() >= (lastswitch + 500)) {                                // Update rate is perhaps 2Hz, but will vary.
           lastswitch = millis();
           state++;
       }
           switch(state) {                          // Light-chase Cycle the LED's.
             case 0:
                       
                       PilotLamp(PL_GRN_ON);
                       break;
                   case 1:
                       
                       PilotLamp(PL_GRN_OFF);
                       PilotLamp(PL_BLU_ON);
                       break;
                   case 2:
                       
                       PilotLamp(PL_BLU_OFF);
                       PilotLamp(PL_RED_ON);
                       break;
                   case 3:
                       state = 0;
                       PilotLamp(PL_RED_OFF);
                       break;
               
          }             
       
     
      return;
  }
#endif

//=========================LED RING===============================================
#if defined(LED_RING)

#define LED_RING_ADDRESS 0x6D //7 bits

void i2CLedRingState() {
  uint8_t b[10];
  static uint8_t state;
  
  if (state == 0) {
    b[0]='z'; 
    b[1]= (180-heading)/2; // 1 unit = 2 degrees;
    i2c_rep_start(LED_RING_ADDRESS<<1);
    for(uint8_t i=0;i<2;i++)
      i2c_write(b[i]);
    i2c_stop();
    state = 1;
  } else if (state == 1) {
    b[0]='y'; 
    b[1]= constrain(angle[ROLL]/10+90,0,180);
    b[2]= constrain(angle[PITCH]/10+90,0,180);
    i2c_rep_start(LED_RING_ADDRESS<<1);
    for(uint8_t i=0;i<3;i++)
      i2c_write(b[i]);
    i2c_stop();
    state = 2;
  } else if (state == 2) {
    b[0]='d'; // all unicolor GREEN 
    b[1]= 1;
    if (f.ARMED) b[2]= 1; else b[2]= 0;
    i2c_rep_start(LED_RING_ADDRESS<<1);
    for(uint8_t i=0;i<3;i++)
      i2c_write(b[i]);
    i2c_stop();
    state = 0;
  }
}

void blinkLedRing() {
  uint8_t b[3];
  b[0]='k';
  b[1]= 10;
  b[2]= 10;
  i2c_rep_start(LED_RING_ADDRESS<<1);
  for(uint8_t i=0;i<3;i++)
    i2c_write(b[i]);
  i2c_stop();

}
#endif

//=========================LED FLASHER===============================================
#if defined(LED_FLASHER)
static uint8_t led_flasher_sequence = 0;
/* if we load a specific sequence and do not want it change, set this flag */
static enum {
	LED_FLASHER_AUTO,
	LED_FLASHER_CUSTOM
} led_flasher_control = LED_FLASHER_AUTO;

void init_led_flasher() {
  LED_FLASHER_DDR |= (1<<LED_FLASHER_BIT);
  LED_FLASHER_PORT &= ~(1<<LED_FLASHER_BIT);
}

void led_flasher_set_sequence(uint8_t s) {
  led_flasher_sequence = s;
}

void inline switch_led_flasher(uint8_t on) {
  if (on) {
    LED_FLASHER_PORT |= (1<<LED_FLASHER_BIT);
  } else {
    LED_FLASHER_PORT &= ~(1<<LED_FLASHER_BIT);
  }
}

void auto_switch_led_flasher() {
  uint8_t seg = (currentTime/1000/125)%8;
  if (led_flasher_sequence & 1<<seg) {
    switch_led_flasher(1);
  } else {
    switch_led_flasher(0);
  }
}

/* auto-select a fitting sequence according to the
 * copter situation
 */
void led_flasher_autoselect_sequence() {
  if (led_flasher_control != LED_FLASHER_AUTO) return;
  #if defined(LED_FLASHER_SEQUENCE_MAX)
  /* do we want the complete illumination no questions asked? */
  if (rcOptions[BOXLEDMAX]) {
  #else
  if (0) {
  #endif
    led_flasher_set_sequence(LED_FLASHER_SEQUENCE_MAX);
  } else {
    /* do we have a special sequence for armed copters? */
    #if defined(LED_FLASHER_SEQUENCE_ARMED)
    led_flasher_set_sequence(f.ARMED ? LED_FLASHER_SEQUENCE_ARMED : LED_FLASHER_SEQUENCE);
    #else
    /* Let's load the plain old boring sequence */
    led_flasher_set_sequence(LED_FLASHER_SEQUENCE);
    #endif
  }
}

#endif

#if defined(LANDING_LIGHTS_DDR)
void init_landing_lights(void) {
  LANDING_LIGHTS_DDR |= 1<<LANDING_LIGHTS_BIT;
}

void inline switch_landing_lights(uint8_t on) {
  if (on) {
    LANDING_LIGHTS_PORT |= 1<<LANDING_LIGHTS_BIT;
  } else {
    LANDING_LIGHTS_PORT &= ~(1<<LANDING_LIGHTS_BIT);
  }
}

void auto_switch_landing_lights(void) {
  if (rcOptions[BOXLLIGHTS]
  #if defined(LANDING_LIGHTS_AUTO_ALTITUDE) & SONAR
	|| (sonarAlt >= 0 && sonarAlt <= LANDING_LIGHTS_AUTO_ALTITUDE && f.ARMED)
  #endif
  ) {
    switch_landing_lights(1);
  } else {
    switch_landing_lights(0);
  }
}
#endif

