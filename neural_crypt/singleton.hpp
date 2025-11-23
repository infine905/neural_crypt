#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include "neural_crypt/neural_crypt.hpp"

template<typename T>
class singleton
{
protected:
    singleton() {}
    ~singleton() {}

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;

    singleton(singleton&&) = delete;
    singleton& operator=(singleton&&) = delete;

public:
    C_FORCEINLINE static T& get()
    {
        static auto inst = CRYPT_PTR(new T{});
        return *inst;
    }
};

#endif