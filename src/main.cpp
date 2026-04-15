/*
 * Project: Tracker One Temperature & Humidity Monitor
 * Author: Erik Fasnacht
 * Date: 4/14/2026
 * Description: Built on Tracker Edge v21, this application extends the base
 *              firmware by integrating an SHT31 temperature and humidity sensor
 *              on the M8 connector. Threshold configuration uses Particle's
 *              environment variables feature, a newer alternative to defining
 *              a custom configuration schema, allowing TEMP_HIGH, TEMP_LOW,
 *              and TEMP_PERIOD to be set directly from the Particle Console
 *              without reflashing firmware.
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/tracker-edge/tracker-edge-firmware/
 */

#include "Particle.h"

#include "tracker_config.h"
#include "tracker.h"
#include "sht3x-i2c.h"

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

// Product version is defined in tracker_config.h and used for OTA targeting
PRODUCT_VERSION(TRACKER_PRODUCT_VERSION);

// Serial logger: TRACE level for app code, INFO for noisy GPS/modem subsystems
SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});

Sht3xi2c sensor(Wire3, 0x44);  // SHT31 on I2C bus 3 (M8 connector), address 0x44
CloudEvent event;               // reusable event object for alert publishes

// Alert state — persists across check_temp() calls to detect transitions
enum State {
    STATE_NORMAL = 0,
    STATE_HIGH   = 1,
    STATE_LOW    = 2
};

unsigned long lastPublish = 0;      // Timestamp of the last temperature check publish, used to enforce TEMP_PERIOD
State state = STATE_NORMAL;         // Initial state is normal until a reading exceeds a threshold

// Threshold and period globals — initialized to safe defaults, overridden by Console env vars:
//   TEMP_HIGH   – high temperature alert threshold (°C)
//   TEMP_LOW    – low temperature alert threshold (°C)
//   TEMP_PERIOD – seconds between temperature checks
double limit_high = 30.0;                       // default high threshold in °C, can be overridden by TEMP_HIGH environment variable in Particle Console
double limit_low  = 15.0;                       // default low threshold in °C, can be overridden by TEMP_LOW environment variable in Particle Console
std::chrono::milliseconds temp_period = 60s;    // default period between temperature checks, can be overridden by TEMP_PERIOD environment variable in Particle Console

// Forward declarations
void myLocationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context);
void update_env_vars();
void check_temp();


void setup()
{
    Tracker::instance().init();

    // Append SHT31 readings to every location publish payload
    Tracker::instance().location.regLocGenCallback(myLocationGenerationCallback);

    // Enable 5V on the M8 connector to power the SHT31
    pinMode(CAN_PWR, OUTPUT);
    digitalWrite(CAN_PWR, HIGH);
    delay(500);

    // Start SHT31 in periodic measurement mode at 400 kHz
    sensor.begin(CLOCK_SPEED_400KHZ);
    sensor.start_periodic();

    Particle.connect();

    // Load cached env vars on boot so thresholds are correct before the first check
    update_env_vars();
}

void loop()
{
    Tracker::instance().loop();

    // Run the temperature check on the configured period
    if ((lastPublish == 0) || (millis() - lastPublish >= temp_period.count())) {
        lastPublish = millis();
        update_env_vars();  // refresh thresholds and period from Console before each check
        check_temp();
    }
}

// Appends sh31_temp and sh31_humid to the location publish JSON payload.
// Called automatically by the Tracker Edge framework on each location fix.
void myLocationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context)
{
    double temp, humid;

    int err = sensor.get_reading(&temp, &humid);
    if (err == 0)
    {
        writer.name("sh31_temp").value(temp);
        writer.name("sh31_humid").value(humid);
        Log.info("temp=%.2lf hum=%.2lf", temp, humid);
    }
    else {
        Log.info("no sensor err=%d", err);
    }
}

// Reads TEMP_HIGH, TEMP_LOW, and TEMP_PERIOD from Particle Console environment variables.
// If a variable is not set, the existing global value (default or previously set) is kept.
void update_env_vars() {
    String env_val;

    if (System.getEnv("TEMP_HIGH", env_val)) {
        limit_high = env_val.toFloat();
        Log.info("TEMP_HIGH: %.2lf", limit_high);
    }
    if (System.getEnv("TEMP_LOW", env_val)) {
        limit_low = env_val.toFloat();
        Log.info("TEMP_LOW: %.2lf", limit_low);
    }
    if (System.getEnv("TEMP_PERIOD", env_val)) {
        int period_sec = env_val.toInt();
        if (period_sec > 0) {
            temp_period = std::chrono::milliseconds((long long)period_sec * 1000);
            Log.info("TEMP_PERIOD: %lld ms", temp_period.count());
        }
    }
}

// Reads the SHT31 sensor, updates alert state, and publishes a cloud event
// ("high temp event" or "low temp event") when a threshold is exceeded.
// No event is published when temperature is within the normal range.
void check_temp() {

    double temp, humid;
    int current_reading = sensor.get_reading(&temp, &humid);

    if (current_reading == 0)
    {
        if (temp > limit_high) {
            Log.warn("Temperature %.2lf is above the high threshold of %.2lf", temp, limit_high);
            state = STATE_HIGH;
        }
        else if (temp < limit_low) {
            Log.warn("Temperature %.2lf is below the low threshold of %.2lf", temp, limit_low);
            state = STATE_LOW;
        }
        else {
            Log.info("temp=%.2lf hum=%.2lf", temp, humid);
            state = STATE_NORMAL;
        }
    }
    else {
        Log.info("no sensor err=%d", current_reading);
    }

    // Only publish on an alert — normal readings are covered by the location publish
    if (state == STATE_HIGH || state == STATE_LOW) {
        particle::Variant obj;
        obj.set("temperature", temp);

        event.name(state == STATE_HIGH ? "high temp event" : "low temp event");
        event.data(obj);
        Particle.publish(event);
        Log.info("publishing %s", obj.toJSON().c_str());

        // Block until the publish completes or times out (60 s)
        waitForNot(event.isSending, 60000);

        if (event.isSent()) {
            Log.info("publish succeeded");
            event.clear();
        } else if (!event.isOk()) {
            Log.info("publish failed error=%d", event.error());
            event.clear();
        }
    }
}
