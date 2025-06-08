#pragma once

#include "BaseError.h"

class OutOfRangeError : public BaseError{
public:
    explicit OutOfRangeError(const QString &msg): BaseError(msg){}

    OutOfRangeError() = default;
};

