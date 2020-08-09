#pragma once
/*
noncopyable被继承使得派生类对象无法拷贝构造和赋值操作
*/
class noncopyable{
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

