/* picopasco now with wio
* -environmental sensor scan of tds, ec, temp, rh, CO2, VOC, soil moisture.
* -automated nutrient mixing based upon user specified recipe and part ratios/concentrations. 
* -relays trigger pump on at differential times to replicate mix recipe. 
* additional libraries required for a successfull build: https://github.com/Seeed-Studio/SGP30_Gas_Sensor https://www.arduinolibraries.info/libraries/sensirion-i2-c-sht4x  *  #include "pins_arduino.h" * mailto://GrayHatGuy@GrayHatGuy.com */
//dev typ common
#include <Arduino.h>
#include <SensirionI2CSht4x.h>
#include <Wire.h>
#include "pins_arduino.h" //custom header for seeed grove pico breakout
#include <SoftwareSerial.h>
#include "sensirion_common.h"
#include "sgp30.h"

#include "LIS3DHTR.h" //for wio acc only

//add to device.h tba
const String DEV_TYPE = "WIO";                           //Select device type: WIO or PICO future add to #define #include device.h

/*                             **user inputs**                       */
//Analog pins PICO
/* //add switch case for defines or .h
#define pHPin A0                                     //pH pin 
#define TdsSensorPin A1                             //TDS sensor pin
#define moisturePin A2                             //moisture sensor pin
*/

//Analog pins WIO
#define pHPin A4                                     //pH 
#define TdsSensorPin A3                             //TDS sensor pin
#define moisturePin A2                             //moisture sensor pin
#define ctl_1 D5                      //control pins
#define ctl_2 D6
#define ctl_3 D7
#define ctl_4 D8

#define VREF 5.0                                  //TDS ADC ref. alt VCC 3.3v
#define SCOUNT 30                                //TDS voltage samples - *currently not used* default 30

const int Mode = 1;  // 0 no mix; 1 - timer; -1 Auto( not used yet)

const int mixIntSP = 10000;                    //while wait auto mix interval [ms] Mode = 1 only

const int n =4;                               //number of pump relays or mixes
int ledPin = LED_BUILTIN;                    //led pin
int t_loop = 97;                            //while wait interval for loop()
int mixprint = 20;                         //number of status lines to print during mix dwell
int looplast = millis();                  //last loop time stamp millis()
int ledState = LOW;                      //init state of led
//need to verify ctl pin output on WIO
//String ctl[n] = {"D5", "D6", "D7", "D8"}; //{12,16,20,21};               //relay GPIO control trigger pins
int trigger = LOW;                     //init state of relay
int tmixSP[n] = {500,1200,200,2000};  //mix time for pump on [ms] 
int mix = 0;                         //0 no mix 1 mix request. Future use for external trigger from UI/downlink
int pumptotal;                      //cumulative pump time since boot
/*                             **user inputs**                                              */

//          ** sensor() globals**
unsigned long previousMillis = millis(); //led do while last time stamp
float mixIntPV = mixIntSP + millis(); //start time for next auto mix
int analogBuffer[SCOUNT], analogBufferTemp[SCOUNT], analogBufferIndex = 0,copyIndex = 0, moistureValue = 0; 
float averageVoltage = 0,tdsValue = 0,temperaturetds = 25, temperature, humidity; 
short pHValue = 0.00;
uint32_t serialNumber; 
uint16_t error; char errorMessage[256]; //error return SHT4x
int errecho = 1; //setup error echo print repeat
int mixcnt = 1;  //mixes since reboot
s16 err = 0;  //error return SPG30
u32 ah = 0; //absolute hydrogen *not used* except in formulae
u16 scaled_ethanol_signal, scaled_h2_signal; //init SGP return
SensirionI2CSht4x sht4x;  //init SHT4x return
LIS3DHTR<TwoWire> lis; //init acc return wio only


void setup() { 
  // if dev type pin define
  Serial.begin(115200);  
  //Wire.setSDA(PIN_WIRE0_SDA);                 //i2c_0 init ONLY FOR PICO add to dev typ #define .h
  //Wire.setSCL(PIN_WIRE0_SCL); 

  Wire.begin(); 
  //for(int i=0; i < n; i++) {                //set state of relay
    pinMode(ctl_1, OUTPUT);  pinMode(ctl_2, OUTPUT);  pinMode(ctl_3, OUTPUT);  pinMode(ctl_4, OUTPUT);  
    digitalWrite(ctl_1, LOW);  digitalWrite(ctl_2, LOW); digitalWrite(ctl_3, LOW); digitalWrite(ctl_4, LOW); 
  //} 
  sht4x.begin(Wire);                        //SHT4x sensor setup /init
  error = sht4x.serialNumber(serialNumber); 

  while (sgp_probe() != STATUS_OK) {        //SGP30 sensor setup /init
    while (errecho) { 
      Serial.println("SGP30 init failed!"); 
    }
  } 
  err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,&scaled_h2_signal); 
  if (err == STATUS_OK) { 
    Serial.println("requesting ram signal! - Please hold..."); 
  } else { 
    while (errecho) {
      Serial.println("error reading SGP30 signals "); 
    }
  } 
  sgp_set_absolute_humidity(13000);           //SGP30 calibration
  err = sgp_iaq_init();  
  if (error) {                                //error handle SHT4X init
    while (errecho) {
      Serial.println("SHT4x init failed - Verify I2C0 connection with SGP30/SHT4x sensors to GPIO 8(SDA0) GPIO 9(SCL0) VCC and GND. Error trying to execute serialNumber(): "); 
      errorToString(error, errorMessage, 256); Serial.println(errorMessage); 
    }
 } else { 
    Serial.print("Serial Number: "); 
    Serial.println(serialNumber); 
    Serial.println(); 
 } 
 //wio only on board sensors
  lis.begin(Wire1);
    if (!lis) {
      Serial.println("ERROR");
      while(1);
    }
        
  lis.setOutputDataRate(LIS3DHTR_DATARATE_25HZ); //Data output rate
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G); //Scale range set to 2g
  while (!Serial) {
        delay(100);
    }

  pinMode(WIO_MIC, INPUT);
  //wio only 

} 

void loop() { 
  while ((millis()-looplast) > t_loop) {      //loop while wait 
    if ((millis() > mixIntPV) and (Mode == 1)){                 //Set mix flag at interval
      mix = 1; 
      Serial.println(String(millis())+"> MIX: trigger mix = 1 - starting mix now!"); 
    } 
              //toggle led
    if (mix == 1){
      stiritup();             //start mix
    }else {
      sensors();                //call sensor scan
      blink(-1);  
    } 
    looplast = millis();
  }
} 

void blink(long led_int) { 
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  unsigned long currentMillis = millis(); 
  if ((currentMillis - previousMillis >= led_int) and (led_int > 0) ) {   //led_int input in ms -1 toggle rn
    previousMillis = currentMillis; 
    if (ledState == LOW) { 
      ledState = HIGH; 
      } else { 
        ledState = LOW; 
      } 
      digitalWrite(ledPin, ledState); 
    } 
  if (led_int == -1){             //bypass time interval with -1 toggle rn
    if (ledState == LOW) { 
      ledState = HIGH; 
    } else { 
      ledState = LOW; } ; 
    digitalWrite(ledPin, ledState);
  }
}
void sensors() { 
  String printStr = "";  
  error = sht4x.measureHighPrecision(temperature, humidity);        //read sht4x
  printStr = String(millis())+"> SCAN: |t>" + temperature + "|rh>" + humidity; 
  u16 tvoc_ppb, co2_eq_ppm; //read sgp30
  err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);      
  if (err == STATUS_OK) { 
    printStr = printStr + "|voc>" + tvoc_ppb + "|co2>" + co2_eq_ppm; 
  } 
  averageVoltage =  analogRead(TdsSensorPin) * (float)VREF/ 1024.0; float compensationCoefficient=1.0+0.02*(temperaturetds-25.0); //TDS calculations *add loop count for averaging SCOUNT*
  float compensationVoltage=averageVoltage/compensationCoefficient; 
  tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5; //TDS and ec formula
  moistureValue = analogRead(moisturePin); //read moisture sensor 
  pHValue = analogRead(pHPin); //read pH  
  printStr = printStr + "|ec>" + compensationVoltage + "|tds>" + tdsValue + "|moisture>" + moistureValue +"|pH>" +(pHValue/100.00); 

  String prntrelay =""; 
  for (int k=0; k<n;k++){ 
    prntrelay = prntrelay + "|S" + (k+1) + ">" +String(tmixSP[k]) ; 
  }
  //if (DEV_TYPE == "WIO") {  //temp str until update #define .h
  Serial.print(String(printStr+prntrelay));
  wiosensors();
  //} 
  if ((mixIntPV > millis()) and (Mode ==1)){
    Serial.print("|time2mix> "+String(mixIntPV - millis())+String("Mix>"+mix)); 
  } else {
   
  }
  Serial.println();
} 

void wiosensors() {
//wio only
  String ws = "";
  int light = analogRead(WIO_LIGHT), val = analogRead(WIO_MIC);               //light get
  float x_values = lis.getAccelerationX(), y_values= lis.getAccelerationY(), z_values = lis.getAccelerationZ(); 
  ws = String("|Lt>"+String(light)+"|xyz>"+String(x_values)+","+String(y_values)+","+String(z_values)+"|snd>"+String(val)+"|"); 
  Serial.print(ws);
}
  
void stiritup(){ 
  for (int m =0; m<n; m++){ 
    sensors();
    int pumpon = millis() + tmixSP[m];            //capture pump end time

    //digitalWrite(ctl[m], HIGH);                   //trigger relay high
    switch (m) {
      case 0:
        digitalWrite(ctl_1, HIGH);                   //trigger relay high
        break;
      case 1:
        digitalWrite(ctl_2, HIGH);                   //trigger relay high
        break;
      case 2:
        digitalWrite(ctl_3, HIGH);                   //trigger relay high
        break;
      case 3:
        digitalWrite(ctl_4, HIGH);                   //trigger relay high
        break;
      default:
        // statements
        break;
    }

    Serial.println (String(millis())+"> MIXING: Pump #"+String(m+1)+" ON! for "+tmixSP[m]+" at "+millis()+" ms ends at "+pumpon+ " ms");
    int cnt = 0;
    while (pumpon >= millis()) {                  //while pumping print countdown to end at fixed interval
      blink(-1);                                  //blink toggle on demand (-1) else (n>=0 is blink interva)
      Serial.println(String(millis())+"> MIXING: Pump #"+String(m+1)+" "+(pumpon - millis())+" ms of pump time remaining " + (mixprint-cnt) + "/" + mixprint ); cnt = cnt +1;
      delay(tmixSP[m]/mixprint);                  //report status of the remaining pump time setting print interval at 1/19 of the dwell time.
    } 
    //digitalWrite(ctl[m], LOW);
      switch (m) {
      case 0:
        digitalWrite(ctl_1, LOW);                   //trigger relay high
        break;
      case 1:
        digitalWrite(ctl_2, LOW);                   //trigger relay high
        break;
      case 2:
        digitalWrite(ctl_3, LOW);                   //trigger relay high
        break;
      case 3:
        digitalWrite(ctl_4, LOW);                   //trigger relay high
        break;
      default:
        // statements
        break;
    }
    pumptotal = pumptotal + tmixSP[m];
    Serial.println(String(millis())+"> MIXING: Pump #" + String(m+1) + " done! - " + millis() +" ms "); 
  } 
  mix = 0; //reset mix for return to scan loop
  mixIntPV =  mixIntSP + millis(); 
  Serial.println(String (millis())+"> MIXING: Completed "+String(mixcnt )+ " mixes with a total pump on time of " + pumptotal+" ms since boot.. reset mix=" + String(mix) + " next mix at " + String(mixIntPV)+" ms"); mixcnt = mixcnt +1;
}
