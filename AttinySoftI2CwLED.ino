#define FAC 1
#define I2C_CPUFREQ (F_CPU/FAC)
#define I2C_NOINTERRUPT 0
#define I2C_PULLUP 1
#define I2C_TIMEOUT 100

/* Adjust to your own liking */
#ifdef __AVR_ATmega328P__
/* PORTC Corresponds to A4/A5 - the hardware I2C pins on Arduinos */
#define SDA_PORT PORTD
#define SDA_PIN 3
#define SCL_PORT PORTD
#define SCL_PIN 5
#define I2C_HARDWARE 0
#else
#define SDA_PORT PORTB
#define SDA_PIN 0
#define SCL_PORT PORTB
#define SCL_PIN 1
#endif
// #define I2C_SLOWMODE 1
#define I2C_FASTMODE 0


#include <RF24.h>
#include <SPI.h>
#include <SoftI2CMaster.h>
#include <SoftWire.h>  // softi2cmaster
#include <avr/io.h>
#include <nRF24L01.h>
RF24 radio(PA2, PA3, 4000000);  // default is 10MHz
SoftWire Wire = SoftWire();
#include "SSD1306Ascii.h"
#include "SSD1306AsciiSoftWire.h"  // need to use the modified one
SSD1306AsciiWire oled;

const byte address[6] = "00001";
const int this_id = 803;

struct package
{
  float temp ;
  float humi ;
  float lux ;
  int id;
};

//------------------------------------------------------------------------------
void CPUSlowDown(int fac) {
  // slow down processor by a fac
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS1) | _BV(CLKPS0);
}
  
//float b = 0;
//float h = 0;
//float t = 0;
int v1 = false;
int v2 = false;
int v3 = false;
int v4 = false;
int v5 = false;
int v6 = false;
int v7 = false;
uint32_t humi = 0;
uint32_t lite = 0;
uint32_t temp = 0; 

// typedef struct package Package;
package data {0.0, 0.0, 0.0, this_id};

void read_send() {
//    data.humi = h;
//    data.temp = t;
//    data.lux = b;
//    data.id = this_id;
    if (radio.isChipConnected () && (data.temp > -40)) 
    {
      oled.print(radio.write(&data, sizeof(data)));
      oled.println(" +");
    } else {
      oled.println("^");}
  }

void setup(void) {
#if FAC != 1
  CPUSlowDown(FAC);
#endif

  Wire.begin();
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(Adafruit5x7);
  oled.clear();
  delay(1000/FAC);
  v1 = i2c_start(0x70);
  v2 = i2c_write(0xA8);
  v3 = i2c_write(0x00);
  v4 = i2c_write(0x00); i2c_stop();
  v5 = i2c_start(0x70);
  i2c_write(0xE1);
  v6 = i2c_write(0x08);
  v7 = i2c_write(0x00); i2c_stop();

  Wire.beginTransmission(0x23);
  Wire.write(0x41);
  Wire.endTransmission(true);
  Wire.beginTransmission(0x23);
  Wire.write(0x7f);
  Wire.endTransmission(true);
  Wire.beginTransmission(0x23);
  Wire.write(0x10);
  Wire.endTransmission(true);
  delay(2000/FAC);

  if (!radio.begin()) {
//    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  }
  else{
      delay(1000);                       // wait for a second
//      Serial.println(F("radio hardware is good!!"));
    }
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS); 
  radio.setPALevel(RF24_PA_MAX);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();          //This sets the module as transmitter
  radio.openWritingPipe(address); //Setting the address where we will send the data
}

void loop(void)
{
  oled.clear();
  v1 = i2c_start(0x70);
  v2 = i2c_write(0xBA);  // reset 
  v1 = i2c_start(0x70);
  v2 = i2c_write(0xAC);
  v3 = i2c_write(0x33);
  v4 = i2c_write(0x00); i2c_stop();
  delay(200);
  i2c_start(0x71);
  v1 = i2c_read(false);
  v2 = i2c_read(false);
  v3 = i2c_read(false);
  v4 = i2c_read(false);
  v5 = i2c_read(false);
  v6 = i2c_read(true); i2c_stop();
  temp   = v4 & 0x0F;
  temp <<= 8;
  temp  |= v5;
  temp <<= 8;
  temp  |= v6;

  humi = (uint32_t)v2 ;
  humi <<= 8;
  humi  |= v3;
  humi <<= 4;
  humi  |= v4 >> 4;
  if (humi > 0x100000) {humi = 0x100000;}
  data.humi = ((float)humi / 0x100000) * 100;
  data.temp = ((float)temp / 0x100000) * 200 - 50;

//  i2c_start(0x46); i2c_write(0x07); i2c_stop();
//  delay(200);
  i2c_start(0x47);
  lite = (uint32_t) i2c_read(false);
  v7 = i2c_read(true); i2c_stop();

  oled.print("0b"); oled.println(v1, BIN);
  oled.print("0x"); oled.print(v2, HEX);
  oled.print("  0x"); oled.print(v3, HEX);
  oled.print("  0x"); oled.println(v4, HEX);
  oled.print("0x"); oled.print(v5, HEX);
  oled.print("  0x"); oled.println(v6, HEX);
  oled.print("--0x"); oled.print(lite, HEX);
  oled.print("  0x"); oled.println(v7, HEX);

  lite <<= 8;
  lite |= v7;
  data.lux = float(lite) / 2.5875;
  oled.print(data.humi, 2); oled.print("%  ");
  oled.print(data.temp, 2); oled.println("C  "); 
  oled.println(data.lux, 2);
  read_send();
  delay(10000/FAC);
}
