#pragma once

#include "BaseError.h"

class RuntimeError : public BaseError {
public:
    explicit RuntimeError(const QString &msg) : BaseError(msg) {}

    RuntimeError() = default;
};

