#if !defined(KALMAN_FILTER)
#define KALMAN_FILTER

#include "base_filter.h"

class KalmanFilter : public BaseFilter
{
private:
    const float _q; // Process noise
    const float _r; // Measurement noise
    float _x;       // State estimate
    float _p;       // Error covariance

public:
    KalmanFilter(float q, float r)
        : _q(q), _r(r), _x(0.0f), _p(1.0f) {}

    ~KalmanFilter() override = default;

    float filter(float &raw) override
    {
        // Predict
        _p = _p + _q;

        // Update
        float k = _p / (_p + _r); // Kalman gain
        _x = _x + k * (raw - _x);
        _p = (1.0f - k) * _p;

        // Return filtered value
        raw = _x;
        return raw;
    }
};

#endif // KALMAN_FILTER
