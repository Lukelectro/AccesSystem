// https://wiki.makerspaceleiden.nl/mediawiki/index.php/Powernode_1.1
//
#define AART_LED        (GPIO_NUM_16)
#define RELAY           (GPIO_NUM_5)
#define TRIAC           (GPIO_NUM_4)
//#define CURRENT_COIL    (GPIO_NUM_36) // New IO
#define CURRENT_COIL    (GPIO_NUM_15) // the one that collides with the card reader
#define SW1             (GPIO_NUM_2)
#define SW2             (GPIO_NUM_39)
#define OPTO1           (GPIO_NUM_34)
#define OPTO2           (GPIO_NUM_35)

const int MAINSFREQ = 50;
const float WINDOW = 20.0, INTERCEPT = -0.8 , SLOPE = 1; // Intercept and slope should be calibrated, see //https://www.instructables.com/id/Simplified-Arduino-AC-Current-Measurement-Using-AC/ 

#include <RunningStatistics.h> ; //https://www.instructables.com/id/Simplified-Arduino-AC-Current-Measurement-Using-AC/ but slightly improved uppon by using const instead of variables for things that should be const
// Even when not using Filters.h it fails to compile because there is no analog.Write on ESP32, even though analog.Write is not actualy used but it is in the code of FilterTwoPole::test();


void setup() {

  Serial.begin(115200);
  delay(250);
  Serial.print("\n\n\n\nBooting ");
  Serial.println(__FILE__);
  Serial.println(__DATE__ " " __TIME__);

  pinMode(AART_LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
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
    RunningStatistics inputStats;                 // create statistics to look at the raw test signal
    inputStats.setWindowSecs( WINDOW / MAINSFREQ );
    
    unsigned int x = analogRead(CURRENT_COIL);
    inputStats.input(x);  // log to Stats function
  
    static double avg = x, savg = 0, savg2 = 0;
    avg = (5000 * avg + x) / 5001;
    savg = (savg * 299 + (avg - x)) / 300;
    savg2 = (savg2 * 299 + savg * savg) / 300;

    static unsigned long lastCurrentMeasure = 0;
    float Sigma = inputStats.sigma();
    if (millis() - lastCurrentMeasure > 1000) {
      Serial.printf("Current %f -> %f\n", avg / 1024., (savg2 - savg * savg) / 1024.);
      // convert signal sigma value to current in amps: current_amps = intercept + slope * inputStats.sigma();
      Serial.printf("I with Filters lib: sigma %f, Amps: %f\n", Sigma, INTERCEPT+SLOPE*Sigma );
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


