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
#include "sht3x-i2c.h"

SerialLogHandler logHandler(LOG_LEVEL_INFO);

Sht3xi2c sensor(Wire);

void setup()
{
    sensor.begin(CLOCK_SPEED_400KHZ);
}

void loop()
{
    static uint32_t timer = System.uptime();
    double temp, humid;

    if (System.uptime() - timer > 5)
    {
        timer = System.uptime();
        if (sensor.single_shot(&temp, &humid) == 0)
        {
            Log.info("Temperature: %.2f, Humidity: %.2f", temp, humid);
        }
    }
}