#pragma once
#include <functional>

class Defer{
public:
    Defer(std::function<void()> func): _func(std::move(func)){}
    ~Defer(){_func();}

    Defer(const Defer&)=delete;
    Defer& operator=(const Defer&)=delete;

private:
    std::function<void()> _func;
};