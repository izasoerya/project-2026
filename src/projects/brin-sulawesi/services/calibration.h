#if !defined(CALIBRATION_H)
#define CALIBRATION_H

#include <projects/brin-sulawesi/models/task_context.h>
#include "application.h"

class CalibrationKernel
{
private:
    SensorContext *_sensorCtx;
    NetworkingContext *_netCtx;

public:
    CalibrationKernel(SensorContext *sensorCtx, NetworkingContext *netCtx)
        : _sensorCtx(sensorCtx), _netCtx(netCtx) {}

    void calibrateTurbidity()
    {
        bool isDoneCalibrating = false;
        while (!isDoneCalibrating)
        {
            SensorObject sensor = Application::samplingTask(&_sensorCtx);
            _netCtx->mqtt.publish(GlobalConfig::SENSOR_TOPIC, sensor.toCSV());
            delay(3000);
        }
    }
};

#endif // CALIBRATION_H
