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
#include <thing/i2cdev-BMP085.h>

using namespace meisterwerk;

// application class
class MyApp : public core::baseapp {
    public:
    base::i2cbus         i2cb;
    thing::i2cdev_BMP085 i2cd;
    MyApp() : core::baseapp( "MyApp" ), i2cb( "i2cbus", D1, D2 ), i2cd( "i2cSensor1" ) {
    }

    virtual void onSetup() {
        // Debug console
        Serial.begin( 115200 );
        DBG( "We begin." );
        i2cb.registerEntity();
        i2cd.registerEntity();
    }
};

// Application Instantiation
MyApp app;
