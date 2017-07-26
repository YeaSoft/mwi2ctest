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

//#include <thing/pushbutton-GPIO.h>
#include <util/dumper.h>
#include <util/messagespy.h>
#include <util/msgtime.h>
#include <util/timebudget.h>

#include <base/i2cbus.h>
#include <base/mastertime.h>
#include <base/net.h>
#include <thing/GPS_NEO_6M.h>
#include <thing/dcf77.h>
#include <thing/i2cdev-BMP085.h>
#include <thing/i2cdev-LCD_2_4_16_20.h>
#include <thing/i2cdev-LED7_14_SEG.h>
#include <thing/i2cdev-OLED_SSD1306.h>
#include <thing/i2cdev-RTC_DS3231.h>
#include <thing/mqtt.h>
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
// Vin GND Rst En  3V3 GND SCK SD0 CMD SD1 D11 D12 RSV RSV A0   (ESP pins)
//                                         GP9 G10              (GPIOs)
// [ ]Rst                         +----------------------+ =|
// U|                             |                      |  |
// S|                             |    ESP-12F           | =
// B|                             |                      |  |
// [ ]Flsh                        +----------------------+ =|
//         GP1 GP3 G15 G13 G12 G14         GP2 GP0 GP4 Gp5 G16  (GPIOs)
// 3V3 GND Tx  Rx  D8  D7  D6  D5  GND 3V3 D4  D3  D2  D1  D0   (ESP pins)
//                    [TX][RX]            [Sw]   [sda|scl]
//                    SwSerial           Switch   I2C-Bus
//   convention: SCL:yellow cable, SDA:green cable.

// application class
class MyApp : public core::baseapp {
    public:
    MyLed            led1;
    util::messagespy spy;
    util::dumper     dmp;
    // thing::pushbutton_GPIO dbg;

    base::net                 wnet;
    base::i2cbus              i2cb;
    thing::i2cdev_LED7_14_SEG i2cd1;
    String                    oldtime   = "";
    String                    clockType = "";

    bool   redraw = false;
    String temperature;
    String pressure;
    String isoTimeTemp;
    String isoTimePress;
    int    noSat = 0;
    /*
    thing::i2cdev_LCD_2_4_16_20 i2cd2;
    */
    thing::i2cdev_LCD_2_4_16_20 i2cd3;
    /*
        thing::i2cdev_LCD_2_4_16_20 i2cd4;
        thing::i2cdev_LED7_14_SEG   i2cd5;
    */
    thing::i2cdev_OLED_SSD1306 i2coled;
    thing::i2cdev_BMP085       bmp180;
    base::mastertime           mtm;
    thing::GPS_NEO_6M          gps;
    thing::Ntp                 ntpcl;
    thing::mqtt                mqttcl;
    thing::dcf77               dcf;
    thing::i2cdev_RTC_DS3231   hprtc;

    MyApp()
        : core::baseapp( "MyApp" ), led1( "led1", BUILTIN_LED, 500 ), dmp( "dmp" ),
          /* dbg( "dbg", D4, 1000, 5000 ),*/ i2cb( "i2cbus", D2, D1 ),

          i2cd1( "D1", 0x70, 14 ),
          /*
          i2cd2( "D2", 0x25, "2x16" ), */ i2cd3( "D3", 0x26, 4,
                                                 20 ), /* i2cd4( "D4", 0x27, "2x16" ),
i2cd5( "D5", 0x71 )
*/
          bmp180( "bmp180", 0x77 ), i2coled( "D2", 0x3c, 64, 128 ), mtm( "mastertime" ),
          gps( "gps", D6, D7 ), wnet( "net" ), ntpcl( "ntpcl" ), mqttcl( "mqttcl" ), dcf( "dcf77" ),
          // hprtc( "hp-rtc", "DS3231", 0x68 ) {
          hprtc( "hp-rtc", "DS1307", 0x68 ) {
    }

    bool readAppConfig() {
        SPIFFS.begin();
        File f = SPIFFS.open( "/sensorclock.json", "r" );
        if ( !f ) {
            DBG( "SPIFFS needs to contain a sensorclock.json file!" );
            return false;
        } else {
            String jsonstr = "";
            while ( f.available() ) {
                // Lets read line by line from the file
                String lin = f.readStringUntil( '\n' );
                jsonstr    = jsonstr + lin;
            }
            DynamicJsonBuffer jsonBuffer( 200 );
            JsonObject &      root = jsonBuffer.parseObject( jsonstr );
            if ( !root.success() ) {
                DBG( "Invalid JSON received, check SPIFFS file sensorclock.json!" );
                return false;
            } else {
                // SSID             = root["SSID"].as<char *>();
                // JsonArray &servs = root["services"];
                // for ( int i = 0; i < servs.size(); i++ ) {
                //    JsonObject &srv = servs[i];
                //    for ( auto obj : srv ) {
                //        netservices[obj.key] = obj.value.as<char *>();
                //    }
            }
        }

        // for ( auto s : netservices ) {
        //    DBG( "***" + s.first + "->" + s.second );
        //}
        return true;
    }

    virtual void onSetup() {
        // Debug console
        Serial.begin( 115200 );

        // register myself
        registerEntity( 50000 );

        // spy.registerEntity();
        dmp.registerEntity();
        // dbg.registerEntity();
        led1.registerEntity( 50000 );

        i2cb.registerEntity();
        i2cd1.registerEntity();
        /*
        i2cd2.registerEntity();
        */
        i2cd3.registerEntity();
        /*
        i2cd4.registerEntity();
        i2cd5.registerEntity();
        */
        bmp180.registerEntity();
        wnet.registerEntity();

        i2coled.registerEntity();

        mtm.registerEntity();
        gps.registerEntity();
        ntpcl.registerEntity();
        mqttcl.registerEntity();
        // dcf.registerEntity();
        hprtc.registerEntity();
        // }
        // void onRegister() override {
        subscribe( "*/temperature" );
        subscribe( "*/pressure" );
        subscribe( "mastertime/time/set" );
        subscribe( "*/gps" );
        dPrint( "D1", "0000" );
        dPrint( "D2", "", 0, 0, 1 );
        dPrint( "D3", "", 0, 0, 1 );
        // subscribe( "dbg/short" );
        // subscribe( "dbg/long" );
        // subscribe( "dbg/extralong" );
    }

    void dPrint( String display, String text, int y = 0, int x = 0, int clear = 0, int textsize = 1,
                 int blinkmode = 0 ) {
        String topic = display + "/display/set";
        String msg   = "{\"x\":" + String( x ) + ",\"y\":" + String( y ) +
                     ",\"textsize\":" + String( textsize ) + ",\"blink\":" + String( blinkmode ) +
                     ",\"clear\":" + String( clear ) + ",\"text\":\"" + text + "\"}";
        publish( topic, msg );
    }

    String oldiso       = "";
    String oldClockType = "";
    String oldSnoSat    = "";
    void onLoop( unsigned long timer ) override {
        char         s[5];
        TimeElements tt;
        String       snoSat;
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
        if ( oldtime != newtime || redraw ) {
            redraw  = false;
            oldtime = newtime;
            dPrint( "D1", newtime, 0, 0, 0, 0, 1 ); // 1: blink colon 500m
        }
        String iso = util::msgtime::time_t2ISO( localt );
        iso[10]    = ' ';
        iso[19]    = ' ';
        if ( oldiso != iso ) {
            oldiso = iso;
            dPrint( "D2",
                    iso + "\n\n" + isoTimeTemp + "\n" + temperature + "\n" + isoTimePress + "\n" +
                        pressure,
                    0, 0, 1 );
            dPrint( "D3", iso, 0 );
            if ( clockType != oldClockType ) {
                oldClockType = clockType;
                dPrint( "D3", "Clock type: " + clockType + "      ", 1 );
            }
            snoSat = String( noSat );
            if ( snoSat != oldSnoSat ) {
                oldSnoSat = snoSat;
                dPrint( "D3", "Satellites: " + snoSat + "    ", 2 );
            }
        }
    }

    virtual void onReceive( const char *origin, const char *ctopic, const char *msg ) override {
        String            topic( ctopic );
        int               p  = topic.indexOf( "/" );
        String            t1 = topic.substring( p + 1 );
        DynamicJsonBuffer jsonBuffer( 200 );
        JsonObject &      root = jsonBuffer.parseObject( msg );
        if ( !root.success() ) {
            DBG( "AppTemp: Invalid JSON received: " + String( msg ) );
            return;
        }
        if ( t1 == "temperature" ) {
            isoTimeTemp = root["time"].as<char *>();
            temperature = root["temperature"].as<char *>();
        }
        if ( t1 == "pressure" ) {
            isoTimePress = root["time"].as<char *>();
            pressure     = root["pressure"].as<char *>();
        }
        if ( t1 == "gps" ) {
            noSat = root["satellites"];
        }
        if ( topic == "mastertime/time/set" ) {
            clockType = root["timesource"].as<char *>();
        }
    }
};
// Application Instantiation
MyApp app;
