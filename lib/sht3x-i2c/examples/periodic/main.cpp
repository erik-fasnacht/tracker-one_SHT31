/*
 * Copyright (c) 2020 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Particle.h"

#include "tracker_config.h"
#include "tracker.h"
#include "sht3x-i2c.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

PRODUCT_ID(TRACKER_PRODUCT_ID);
PRODUCT_VERSION(TRACKER_PRODUCT_VERSION);

SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});

Sht3xi2c sensor(Wire3);

void loc_gen_cb(JSONWriter &writer, LocationPoint &point, const void *context)
{
    double temp, humid;

    if (sensor.get_reading(&temp, &humid) == 0)
    {
        writer.name("i2c_temp").value(temp);
        writer.name("i2c_humid").value(humid);
    }
}

void setup()
{
    Tracker::instance().init();
    Tracker::instance().location.regLocGenCallback(loc_gen_cb);
    
    pinMode(CAN_PWR, OUTPUT);       // Turn on 5V output on M8 connector
    digitalWrite(CAN_PWR, HIGH);    // Turn on 5V output on M8 connector
    delay(500);

    sensor.begin(CLOCK_SPEED_400KHZ);
    sensor.start_periodic();

    Particle.connect();
}

void loop()
{
    Tracker::instance().loop();
}