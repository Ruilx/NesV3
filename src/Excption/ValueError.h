#pragma once

#include "BaseError.h"

class ValueError : public BaseError{
public:
    explicit ValueError(const QString &msg): BaseError(msg){}

    ValueError() = default;
};

