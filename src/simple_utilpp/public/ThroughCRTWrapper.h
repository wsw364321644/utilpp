#include <algorithm>
template<class value_type>
class ThroughCRTWrapper {
public:
    typedef void(*DeconstructFn_t)(value_type*);
    ThroughCRTWrapper() {}
    ThroughCRTWrapper(ThroughCRTWrapper&) = delete;
    ThroughCRTWrapper(ThroughCRTWrapper&& other) {
        *this = std::forward<ThroughCRTWrapper&&>(other);
    }
    template <class... _Types>
    ThroughCRTWrapper(_Types&&... _Args) {
        SetValue(std::forward<_Types>(_Args)...);
    }
    ~ThroughCRTWrapper() {
        Reset();
    }
    ThroughCRTWrapper& operator =(ThroughCRTWrapper&& other) {
        std::swap(freeFunc, other.freeFunc);
        std::swap(Value, other.Value);
        return *this;
    }
    template <class... _Types>
    void SetValue(_Types&&... _Args) {
        Reset();
        Value = new value_type(std::forward<_Types>(_Args)...);
        freeFunc = [](value_type* value) {
            if (value) {
                delete value;
            }
            };
    }

    void Reset() {
        if (freeFunc) {
            freeFunc(Value);
            freeFunc = nullptr;
            Value = nullptr;
        }
    }
    const value_type& GetValue() const {
        return *Value;
    }
private:
    DeconstructFn_t freeFunc{ nullptr };
    value_type* Value{ nullptr };
};
