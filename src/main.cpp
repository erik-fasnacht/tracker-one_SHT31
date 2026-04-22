/*
 * Project: Tracker One Temperature & Humidity Monitor
 * Author: Erik Fasnacht
 * Date: 4/14/2026
 * Description: Built on Tracker Edge v21, this application extends the base
 *              firmware by integrating an SHT31 temperature and humidity sensor
 *              on the M8 connector. Threshold configuration uses Particle's
 *              environment variables feature, a newer alternative to defining
 *              a custom configuration schema, allowing TEMP_HIGH and TIME_ZONE
 *              to be set directly from the Particle Console without reflashing
 *              firmware.
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/tracker-edge/tracker-edge-firmware/
 */

 // inlcude headers for Particle device and firmware features
#include "Particle.h"

// includes for various libraries
#include "tracker_config.h"
#include "tracker.h"
#include "sht3x-i2c.h"

// defines the firmware version
#define FIRWARE_VERSION 21

// SYSTEM_THREAD is on by default in Device OS 6.2.0+
#ifndef SYSTEM_VERSION_v620
    SYSTEM_THREAD(ENABLED);
#endif

// SEMI_AUTOMATIC delays cloud connection until Particle.connect() is called in setup()
SYSTEM_MODE(SEMI_AUTOMATIC);

// PRODUCT_ID is set automatically in Device OS 4.0.0+
#ifndef SYSTEM_VERSION_v400ALPHA1
    PRODUCT_ID(TRACKER_PRODUCT_ID);
#endif

// firmware version is defined above
PRODUCT_VERSION(FIRWARE_VERSION);

// Serial logger: TRACE level for app code, INFO for noisy GPS/modem subsystems
SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});

// SHT31 sensor instance on I2C bus 3 (M8 connector), address 0x44
Sht3xi2c sensor(Wire3, 0x44);  // SHT31 on I2C bus 3 (M8 connector), address 0x44

// Reusable CloudEvent instance for publishes outside of location
CloudEvent event; 

// Threshold globals — initialized to safe defaults, overridden by Console env vars:
//   TEMP_HIGH – high temperature alert threshold (°F)
//   TIME_ZONE – UTC offset in hours for local time in location publishes
double        limit_high  = 86.0;           // °F (≈ 30 °C)
double        time_zone   = -7.0;           // UTC offset, defaults to PDT

// Forward declarations
void myLocationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context);
void update_env_vars();

void setup()
{
    // initialize the Tracker Edge firmware and device
    Tracker::instance().init();

    // Append SHT31 readings and local time to every location publish payload
    Tracker::instance().location.regLocGenCallback(myLocationGenerationCallback);

    // Enable 5V on the M8 connector to power the SHT31
    pinMode(CAN_PWR, OUTPUT);
    digitalWrite(CAN_PWR, HIGH);
    delay(500);

    // Start SHT31 in periodic measurement mode at 400 kHz
    sensor.begin(CLOCK_SPEED_400KHZ);
    sensor.start_periodic();

    // Connect to the Particle Cloud
    Particle.connect();

    // Load cached env vars on boot so thresholds are correct before the first check
    update_env_vars();
}

void loop()
{
    Tracker::instance().loop();
}

// Appends sh31_temp (°F), sh31_humid, and local_time to the location publish JSON payload.
// Called automatically by the Tracker Edge framework on each location fix.
void myLocationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context)
{
    double temp, humid;

    int err = sensor.get_reading(&temp, &humid);
    if (err == 0)
    {
        double temp_f = (temp * 9.0 / 5.0) + 32.0;
        writer.name("sh31_temp").value(temp_f);
        writer.name("sh31_humid").value(humid);
        if (Time.isValid()) {
            writer.name("local_time").value(Time.format(Time.now(), "%I:%M:%S %p"));
        }
        Log.info("temp=%.2lf°F hum=%.2lf", temp_f, humid);

        if (temp_f > limit_high) {
            Log.warn("Temperature %.2lf°F is above the high threshold of %.2lf°F", temp_f, limit_high);

            particle::Variant obj;
            obj.set("temperature", temp_f);

            event.name("high temp event");
            event.data(obj);
            Particle.publish(event);
            Log.info("publishing %s", obj.toJSON().c_str());

            waitForNot(event.isSending, 60000);

            if (event.isSent()) {
                Log.info("publish succeeded");
            } else if (!event.isOk()) {
                Log.warn("publish failed error=%d", event.error());
            }
            event.clear();
        }
    }
    else {
        Log.info("no sensor err=%d", err);
    }
}

// Reads TEMP_HIGH and TIME_ZONE from Particle Console environment variables.
// If a variable is not set, the existing global value (default or previously set) is kept.
void update_env_vars() {
    String env_val;

    if (System.getEnv("TEMP_HIGH", env_val)) {
        limit_high = env_val.toFloat();
        Log.info("TEMP_HIGH: %.2lf°F", limit_high);
    }

    if (System.getEnv("TIME_ZONE", env_val)) {
        time_zone = env_val.toFloat();
        Log.info("TIME_ZONE: %.1lf", time_zone);
    }
    Time.zone(time_zone);   // always apply; uses default or Console-supplied value

}
