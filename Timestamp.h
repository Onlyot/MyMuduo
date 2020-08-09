#pragma once

#include <iostream>
#include <string>

class Timestamp{

public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpochArg);
    static Timestamp now();

    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};