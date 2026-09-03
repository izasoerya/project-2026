#if !defined(BASE_SENSOR_H)
#define BASE_SENSOR_H

class BaseSensor
{
protected:
    unsigned char _id;
    const char *_name;

public:
    BaseSensor(unsigned char id, const char *name) : _id(id), _name(name) {}
    virtual ~BaseSensor() = default;

    virtual float read() = 0;

    unsigned char getId() const { return _id; }
    const char *getName() const { return _name; }
};

#endif // BASE_SENSOR_H
