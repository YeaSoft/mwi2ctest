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
#include <base/i2cbus.h>

using namespace meisterwerk;

/*
// led class
class MyLed : public core::entity {
    public:
    unsigned long ledLastChange       = 0;
    unsigned long ledBlinkIntervallMs = 500;
    uint8_t       pin                 = BUILTIN_LED;
    bool          state               = false;

    MyLed( String _name, uint8_t _pin ) : core::entity( _name ) {
        pin = _pin;
    }

    virtual void onSetup() {
        pinMode( pin, OUTPUT );
    }

    virtual void onLoop( unsigned long ticker ) {
        unsigned long millis = ( ticker - ledLastChange ) / 1000L;
        if ( millis >= ledBlinkIntervallMs ) {
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
*/
// application class
class MyApp : public core::baseapp {
    public:
    base::i2cbus i2cb;
    MyApp() : core::baseapp( "MyApp" ), i2cb("i2cbus",D1,D2)  {
    }

    virtual void onSetup() {
        // Debug console
        Serial.begin( 115200 );
        DBG("We begin.");
        i2cb.registerEntity();
    }
};

// Application Instantiation
MyApp app;
