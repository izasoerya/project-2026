#if !defined(PROFILE_H)
#define PROFILE_H

#include <Arduino.h>

class Profile
{
    float _bottomSet;
    float _topSet;

public:
    Profile(
        float bottom = 0.0f, float top = 0.0f)
        : _bottomSet(bottom), _topSet(top) {}
    ~Profile() {}

    float getBottomSet() { return _bottomSet; }
    float getTopSet() { return _topSet; }
};

#endif // PROFILE_H
