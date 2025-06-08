#pragma once

#include "IOError.h"

class QFile;
class FileError: public IOError {
public:
    explicit FileError(const QFile &file, const QString &msg = QString());

    FileError() = default;
};

