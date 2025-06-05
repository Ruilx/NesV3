#pragma once

#include <QException>

class OutOfRangeException : public QException{

    QString msg;
public:
    explicit OutOfRangeException(const QString &msg){
        this->msg = msg;
    }

    ~OutOfRangeException(){}
};

