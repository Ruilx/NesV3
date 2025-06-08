#pragma once

#include "BaseError.h"

class IOError : public BaseError{
public:
    explicit IOError(const QString &msg): BaseError(msg){}

    IOError() = default;
};

