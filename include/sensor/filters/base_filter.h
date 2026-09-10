#if !defined(BASE_FILTER_H)
#define BASE_FILTER_H

class BaseFilter
{
public:
    virtual ~BaseFilter() = default;
    virtual float filter(float &raw) = 0; // Pass by reference to modify in-place
};

#endif // BASE_FILTER_H
