// app.cpp - Sample Application 1
//
// This small sample application built with
// the MeisterWerk Framework flashes the
// internal LED

// platform includes
#include <ESP8266WiFi.h>

// Let MeisterWerk output debugs on Serial
#define DEBUG 1

// framework includes
#include <MeisterWerk.h>

#include <thing/pushbutton-GPIO.h>
#include <util/dumper.h>
#include <util/messagespy.h>
#include <util/timebudget.h>

#include <base/i2cbus.h>
#include <thing/i2cdev-BMP085.h>

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

// application class
class MyApp : public core::baseapp {
    public:
    MyLed                  led1;
    util::messagespy       spy;
    util::dumper           dmp;
    thing::pushbutton_GPIO dbg;

    base::i2cbus         i2cb;
    thing::i2cdev_BMP085 i2cd;
    MyApp()
        : core::baseapp( "MyApp" ), led1( "led1", BUILTIN_LED, 500 ), dmp( "dmp" ),
          dbg( "dbg", D4, 1000, 5000 ), i2cb( "i2cbus", D1, D2 ), i2cd( "i2cSensor1" ) {
    }

    virtual void onSetup() {
        // Debug console
        Serial.begin( 115200 );
        // register myself
        registerEntity();

        spy.registerEntity();
        dmp.registerEntity();
        dbg.registerEntity();
        led1.registerEntity( 50000 );

        i2cb.registerEntity();
        i2cd.registerEntity();
    }
    void onRegister() override {
        subscribe( "dbg/short" );
        subscribe( "dbg/long" );
        subscribe( "dbg/extralong" );
    }

    void onLoop( unsigned long timer ) override {
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
