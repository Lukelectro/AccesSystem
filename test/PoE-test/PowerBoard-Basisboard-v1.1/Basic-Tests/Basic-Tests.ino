// https://wiki.makerspaceleiden.nl/mediawiki/index.php/Powernode_1.1
//
#define AART_LED        (GPIO_NUM_16)
//#define RELAY           (GPIO_NUM_5) // commented out because that ticking noise is... noisy
#define TRIAC           (GPIO_NUM_4)
//#define CURRENT_COIL    (GPIO_NUM_36) // New IO
#define CURRENT_COIL    (GPIO_NUM_15) // the one that collides with the card reader
#define SW1             (GPIO_NUM_2)
#define SW2             (GPIO_NUM_39)
#define OPTO1           (GPIO_NUM_34)
#define OPTO2           (GPIO_NUM_35)

const int MAINSFREQ = 50;
const int MAINSPERIOD = 1000/MAINSFREQ; // in miliseconds, not seconds, hence the 1000
const int NCYCLES = 100;                 // number of cycles to calculate Irms over.
const int TRIM = -208; // Check DC offset in ADC counts and correct it here so raw - DC_OFFSET is zero
const int ADCMAX = 1<<12; // 12 bit ADC on ESP32
const float VCC = 3.317, RLOAD=49.9;
const int CURTRANS = 1000; // current transformer transfer ratio
const int DC_OFFSET = 0.5*ADCMAX+TRIM;


void setup() {

  Serial.begin(115200);
  delay(250);
  Serial.print("\n\n\n\nBooting ");
  Serial.println(__FILE__);
  Serial.println(__DATE__ " " __TIME__);

  pinMode(AART_LED, OUTPUT);
  #ifdef RELAY{
    pinMode(RELAY, OUTPUT);
  }
  #endif
 
  pinMode(TRIAC, OUTPUT);
  pinMode(CURRENT_COIL, ANALOG); // analog

#ifdef SW1
  pinMode(SW1, INPUT);
#endif
#ifdef SW2
  pinMode(SW2, INPUT);
#endif
}

void loop() {
#ifdef AART_LED
  {
    static unsigned long aartLedLastChange = 0;
    static int aartLedState = 0;
    if (millis() - aartLedLastChange > 100) {
      aartLedState = (aartLedState + 1) & 7;
      digitalWrite(AART_LED, aartLedState ? HIGH : LOW);
      aartLedLastChange = millis();
    };
  }
#endif

#ifdef RELAY
  {
    static unsigned long relayLastChange = 0;
    if (millis() - relayLastChange > 1000) {
      digitalWrite(RELAY, !digitalRead(RELAY));
      relayLastChange = millis();
    };
  }
#endif


#ifdef TRIAC
  {
    static unsigned long TriacLastChange = 0;
    if (millis() - TriacLastChange > 1010) {
      digitalWrite(TRIAC, !digitalRead(TRIAC));
      TriacLastChange = millis();
    };
  }
#endif

#ifdef CURRENT_COIL
  {  
    static unsigned long lastCurrentMeasure = 0, window;
    int Raw = analogRead(CURRENT_COIL); // making this unsigned causes problems with AC_i not going negative where it should.
    static unsigned int numsamples = 0;
    double AC_i = ((Raw - DC_OFFSET)*VCC*CURTRANS)/(ADCMAX*RLOAD); // instantaneous value / momentane waarde
    /* 
     Irms = sqrt( (AC_i*AC_i)_1 + (AC_i*AC_i)_2 + (AC_i*AC_i)_3 + ... + (AC_i*AC_i)_n / n )    
     Irms = sqrt(AC_i*AC_i)_1 + sqrt(AC_i*AC_i)_2 + sqrt(AC_i*AC_i)_3 + ... + sqrt(AC_i*AC_i)_n / sqrt(n);
     Irms = ( abs(AC_i_1) + abs(AC_i_2) + ... + abs(AC_i_n) ) / sqrt(n);
     Irms = abs(AC_i_1)/ sqrt(n); + abs(AC_i_2)/ sqrt(n); + .../ sqrt(n); + abs(AC_i_n) / sqrt(n);

     Maar, n (numsamples) moet daarbij een geheel veelvoud zijn van samplerate/mainsfreq, omdat het alleen opgaat over een hele cyclus van het harmonische signaal en niet over een deel van de curve ervan
    */
    numsamples++;
    static double Iabs_sum = 0 , Irms= 0;
    Iabs_sum += abs(AC_i);
    
    if (millis() - window > (MAINSPERIOD*NCYCLES) ) { // hope milis() does not jitter too much...
      //Irms = Iabs_sum / sqrt(numsamples); // now it's a whole number of full cycles, calculate Irms
      Irms = Iabs_sum / numsamples; // let's try this, because that sqrt does not make sense, despite the definitions of rms.
      Iabs_sum = 0; // reset
      numsamples = 0; // reset
      window = millis();
    };
   
    if (millis() - lastCurrentMeasure > 1000 ) {
     // Serial.printf("Current: Irms = %f A, Iabs_sum = %f , Iinst = %f, raw = %u, numsamples = %u\n", Irms, Iabs_sum, AC_i, Raw, numsamples);
       Serial.printf("Current: Irms (or Iavg?) = %f A, raw = %u\n", Irms, Raw);
      lastCurrentMeasure = millis();
    };
  }
#endif

#ifdef SW2
  {
    static unsigned long last = 0;
    if (digitalRead(SW2) != last) {
      Serial.printf("Current state SW2: %d\n", digitalRead(SW2));
      last = digitalRead(SW2);
    };
  }
#endif
#ifdef SW1
  {
    static unsigned long last = 0;
    if (digitalRead(SW1) != last) {
      Serial.printf("Current state SW1: %d\n", digitalRead(SW1));
      last = digitalRead(SW1);
    };
  }
#endif

#ifdef OPTO1
  {
    static unsigned last = 0;
    if (millis() - last > 1000) {
      last = millis();
      Serial.printf("OPTO 1: %d\n", analogRead(OPTO1));
    }
  }
#endif
#ifdef OPTO2
  {
    static unsigned last = 0;
    if (millis() - last > 1000) {
      last = millis();
      Serial.printf("OPTO 2: %d\n", analogRead(OPTO2));
    }
  }
#endif


  static unsigned long lastReport = 0, lastCntr = 0, Cntr = 0;
  Cntr++;
  if (millis() - lastReport > 10000) {
    float rate =  1000. * (Cntr - lastCntr) / (millis() - lastReport) + 0.05;
    if (rate > 10)
      Serial.printf("Loop rate: %.1f #/second\n", rate);
    else
      Serial.printf("Warning: LOW Loop rate: %.1f #/second\n", rate);
    lastReport = millis();
    lastCntr = Cntr;
  }

}


