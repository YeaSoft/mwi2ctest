// app.cpp - Sample Application 1
//
// This small sample application built with
// the MeisterWerk Framework flashes the
// internal LED

// platform include

#include <ESP8266WiFi.h>

// Let MeisterWerk output debugs on Serial
#define DEBUG 1

// framework includes
#include <MeisterWerk.h>

#include <thing/pushbutton-GPIO.h>
#include <util/dumper.h>
#include <util/messagespy.h>
#include <util/msgtime.h>
#include <util/timebudget.h>

#include <base/i2cbus.h>
#include <base/mastertime.h>
#include <base/net.h>
#include <thing/GPS_NEO_6M.h>
#include <thing/i2cdev-LCD_2_4_16_20.h>
#include <thing/i2cdev-LED7_14_SEG.h>
#include <thing/i2cdev-OLED_SSD1306.h>
#include <thing/ntp.h>

using namespace meisterwerk;

// led class
class MyLed : public core::entity {
    public:
    unsigned long ledLastChange       = 0;
    unsigned long ledBlinkIntervallUs = 500000L;
    uint8_t       pin                 = BUILTIN_LED;
    bool          state               = false;

    MyLed( String name, uint8_t pin, unsigned long ledBlinkIntervallMs )
        : core::entity( name ), pin{pin} {
        ledBlinkIntervallUs = ledBlinkIntervallMs * 1000;
    }

    virtual void onRegister() {
        pinMode( pin, OUTPUT );
    }

    virtual void onLoop( unsigned long ticker ) {
        unsigned long micros = util::timebudget::delta( ledLastChange, ticker );
        if ( micros >= ledBlinkIntervallUs ) {
            ledLastChange = ticker;
            if ( state ) {
                state = false;
                digitalWrite( pin, HIGH );
            } else {
                state = true;
                digitalWrite( pin, LOW );
            }
        }
    }
};

//[Lcd+]
// Vin GND Rst En  3V3 GND SCK SD0 CMD SD1 SD2 SD3 RSV RSV A0
// [ ]Rst                         +----------------------+ =|
// U|                             |                      |  |
// S|                             |    ESP-12F           | =
// B|                             |                      |  |
// [ ]Flsh                        +----------------------+ =|
// 3V3 GND Tx  Rx  D8  D7  D6  D5  GND 3V3 D4  D3  D2  D1  D0
//                    [TX][RX]            [Sw]   [sda|scl]
//                    SwSerial           Switch   I2C-Bus
//   convention: SCL:yellow cable, SDA:green cable.

// application class
class MyApp : public core::baseapp {
    public:
    MyLed                  led1;
    util::messagespy       spy;
    util::dumper           dmp;
    thing::pushbutton_GPIO dbg;

    base::net                 wnet;
    base::i2cbus              i2cb;
    thing::i2cdev_LED7_14_SEG i2cd1;
    String                    oldtime = "";
    /*
    thing::i2cdev_LCD_2_4_16_20 i2cd2;
    thing::i2cdev_LCD_2_4_16_20 i2cd3;
    thing::i2cdev_LCD_2_4_16_20 i2cd4;
    thing::i2cdev_LED7_14_SEG   i2cd5;
*/
    thing::i2cdev_OLED_SSD1306 i2coled;

    base::mastertime  mtm;
    thing::GPS_NEO_6M gps;
    thing::Ntp        ntpcl;

    MyApp()
        : core::baseapp( "MyApp" ), led1( "led1", BUILTIN_LED, 500 ), dmp( "dmp" ),
          dbg( "dbg", D4, 1000, 5000 ), i2cb( "i2cbus", D2, D1 ),

          i2cd1( "D1", 0x70, 14 ),
          /*
          i2cd2( "D2", 0x25, "2x16" ), i2cd3( "D3", 0x26, "4x20" ), i2cd4( "D4", 0x27, "2x16" ),
          i2cd5( "D5", 0x71 )
          */
          i2coled( "D2", 0x3c, "128x64" ), mtm( "mastertime" ), gps( "gps" ), wnet( "net" ),
          ntpcl( "ntpcl" ) {
    }

    virtual void onSetup() {
        // Debug console
        Serial.begin( 115200 );

        // register myself
        registerEntity( 100000 );

        spy.registerEntity();
        dmp.registerEntity();
        dbg.registerEntity();
        led1.registerEntity( 50000 );

        i2cb.registerEntity();
        i2cd1.registerEntity();
        /*
        i2cd2.registerEntity();
        i2cd3.registerEntity();
        i2cd4.registerEntity();
        i2cd5.registerEntity();
        */
        wnet.registerEntity();

        i2coled.registerEntity();

        mtm.registerEntity();
        gps.registerEntity();
        ntpcl.registerEntity();
    }
    void onRegister() override {
        // subscribe( "dbg/short" );
        // subscribe( "dbg/long" );
        // subscribe( "dbg/extralong" );
    }

    int  l = 0;
    void onLoop( unsigned long timer ) override {
        if ( l == 0 ) {
            l = 1;
            publish( "D1/display", "{\"text\":\"Boot\"}" );
            publish( "D2/display" );
        } else {
            char         s[5];
            TimeElements tt;
            strcpy( s, "0000" );
            time_t localt = util::msgtime::time_t2local( now() );
            breakTime( localt, tt );
            int hr         = tt.Hour;
            int mr         = tt.Minute;
            s[0]           = '0' + hr / 10;
            s[1]           = '0' + hr % 10;
            s[2]           = '0' + mr / 10;
            s[3]           = '0' + mr % 10;
            String newtime = String( s );
            if ( oldtime != newtime ) {
                oldtime = newtime;
                publish( "D1/display", "{\"text\":\"" + newtime + "\"}" );
            }
        }
    }

    virtual void onReceive( String origin, String topic, String msg ) {
        if ( topic == "dbg/short" ) {
            // publish( "relais1/toggle" );
        } else if ( topic == "dbg/long" ) {
            // publish( "relais1/on", "{\"duration\":5}" );
        }
    }
};
// Application Instantiation
MyApp app;
