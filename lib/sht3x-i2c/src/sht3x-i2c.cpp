/* sht3x-i2c library by Mariano Goluboff
 */

#include "sht3x-i2c.h"

uint8_t crc8(uint8_t *buf, size_t len)
{
    uint8_t crc = 0xFF;

    size_t i, j;
    for (i = 0; i < len; i++)
    {
        crc ^= buf[i];

        for (j = 0; j < 8; j++)
        {
            crc = (crc & 0x80)
                      ? (crc << 1) ^ 0x31
                      : (crc << 1);
        }
    }
    return crc;
}

Sht3xi2c::Sht3xi2c(TwoWire& interface, uint8_t i2c_addr)
{
  _wire = &interface;
  _state = SHT31_STATE_IDLE;
  _i2c_addr = i2c_addr;
}

void Sht3xi2c::begin(uint32_t speed)
{
    if (!_wire->isEnabled())
    {
        _wire->reset();
        _wire->setSpeed(speed);
        _wire->begin();
    }
}

int Sht3xi2c::single_shot(double* temp, double* humid, uint8_t accuracy)
{
    int ret_value = 0;
    uint8_t time_delay;
    if (_state != SHT31_STATE_IDLE)
    {
        if (!stop_periodic()) return SHT31_I2C_ERROR;
    }
    if (!_wire->lock()) return SHT31_I2C_LOCK_ERROR;

    _wire->beginTransmission(_i2c_addr);
    _wire->write(0x24);
    switch (accuracy)
    {
    case SHT31_ACCURACY_HIGH:
        _wire->write(0x00);
        time_delay = 25;
        break;
    case SHT31_ACCURACY_LOW:
        _wire->write(0x16);
        time_delay = 10;
        break;
    default:
        _wire->write(0x0B);
        time_delay = 15;
        break;
    }
    if (_wire->endTransmission() != 0)
    {
        ret_value = SHT31_I2C_ERROR;
        goto end_command;
    }
    delay(time_delay); // Add a delay because when there's a NACK, I2C bus is reset
    ret_value = pr_get_reading(temp, humid);

end_command:
    _wire->unlock();
    return ret_value;
}

int Sht3xi2c::get_reading(double *temp, double *humid)
{
    if (_state != SHT31_STATE_CONTINUOUS) return SHT31_WRONG_MODE;
    int ret_value = 0;
    if (!_wire->lock()) return SHT31_I2C_LOCK_ERROR;

    _wire->beginTransmission(_i2c_addr);
    _wire->write(0xE0);
    _wire->write(0x00);
    if (_wire->endTransmission() != 0)
    {
        ret_value = -1;
        goto end_command;
    }
    ret_value = pr_get_reading(temp, humid);
end_command:
    _wire->unlock();
    return ret_value;
}

int Sht3xi2c::start_periodic(uint8_t accuracy, uint8_t mps)
{
    int ret_value = 0;
    if (_state == SHT31_STATE_CONTINUOUS) return 0;
    if (!_wire->lock()) return SHT31_I2C_LOCK_ERROR;

    _wire->beginTransmission(_i2c_addr);
    if (mps < 1)        // mps 0.5
    {
        _wire->write(0x20);
        switch (accuracy)
        {
        case SHT31_ACCURACY_HIGH:
            _wire->write(0x32);
            break;
        case SHT31_ACCURACY_LOW:
            _wire->write(0x2F);
            break;
        default:
            _wire->write(0x24);
            break;
        }
    } else if (mps == 1)        // mps 1
    {
        _wire->write(0x21);
        switch (accuracy)
        {
        case SHT31_ACCURACY_HIGH:
            _wire->write(0x30);
            break;
        case SHT31_ACCURACY_LOW:
            _wire->write(0x2d);
            break;
        default:
            _wire->write(0x26);
            break;
        }
    } else if (mps < 4)         // mps 2
    {
        _wire->write(0x22);
        switch (accuracy)
        {
        case SHT31_ACCURACY_HIGH:
            _wire->write(0x36);
            break;
        case SHT31_ACCURACY_LOW:
            _wire->write(0x2B);
            break;
        default:
            _wire->write(0x20);
            break;
        }
    } else if (mps < 10)        // mps 4
    {
        _wire->write(0x23);
        switch (accuracy)
        {
        case SHT31_ACCURACY_HIGH:
            _wire->write(0x34);
            break;
        case SHT31_ACCURACY_LOW:
            _wire->write(0x29);
            break;
        default:
            _wire->write(0x22);
            break;
        }
    } else
    {                       // mps 10
        _wire->write(0x27);
        switch (accuracy)
        {
        case SHT31_ACCURACY_HIGH:
            _wire->write(0x37);
            break;
        case SHT31_ACCURACY_LOW:
            _wire->write(0x2A);
            break;
        default:
            _wire->write(0x21);
            break;
        }
    }
    if (_wire->endTransmission() != 0)
    {
        ret_value = SHT31_I2C_ERROR;
    } else {
        _state = SHT31_STATE_CONTINUOUS;
    }
    _wire->unlock();
    return ret_value;
}

bool Sht3xi2c::stop_periodic()
{
    if (!_wire->lock()) return false;
    _wire->beginTransmission(_i2c_addr);
    _wire->write(0x30);
    _wire->write(0x93);
    if (_wire->endTransmission() == 0) _state = SHT31_STATE_IDLE;
    _wire->unlock();
    return (_state == SHT31_STATE_IDLE);
}

int Sht3xi2c::pr_get_reading(double *temp, double *humid)
{
    _wire->requestFrom(_i2c_addr, 6);
    if (_wire->available() != 6)
    {
        return SHT31_I2C_ERROR;
    }
    uint8_t ts[2], rh[2];
    ts[0] = _wire->read();
    ts[1] = _wire->read();
    *temp = -45.0+(double)(175.0*((uint16_t)((ts[0]<<8)|ts[1]))/0xFFFF);
    if (_wire->read() != crc8(ts, 2))
    {
        return SHT31_CRC_ERROR;
    }
    rh[0] = _wire->read();
    rh[1] = _wire->read();
    *humid = 100.0*(double)(uint16_t)((rh[0]<<8)|rh[1])/(double)0xFFFF;
    if (_wire->read() != crc8(rh, 2))
    {
        return SHT31_CRC_ERROR;
    }
    return 0;
}