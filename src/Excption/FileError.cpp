#include "FileError.h"

#include <QFile>
FileError::FileError(const QFile &file, const QString &msg) : IOError(msg){
    if(msg.isEmpty()){
        this->setMsg("FileError: " + file.fileName() + " " + msg)
    }
}