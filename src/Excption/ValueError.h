#pragma once

#include <QException>

class ValueError : public QException{

    QString msg;
public:
    explicit ValueError(const QString &msg){
        this->msg = msg;
    }

    ~ValueError(){}
};

