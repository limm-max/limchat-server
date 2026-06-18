#pragma once
#include <memory>
#include <mutex>
#include <iostream>

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;              // 禁拷贝
    Singleton& operator=(const Singleton<T>&) = delete;   // 禁赋值
    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {                    
            _instance = std::shared_ptr<T>(new T);        
        });
        return _instance;                                  
    }
    ~Singleton() {
        std::cout << "singleton destruct\n";               
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;     // 类外定义静态成员