#pragma once

#include <QException>

class BaseError: QException{
    QString msg;

public:
    explicit BaseError(const QString &msg): QException(){
        this->setMsg(msg);
    }

    BaseError() = default;

    void setMsg(const QString &message){
        this->msg = message;
    }

    [[nodiscard]] QString getMsg() const {
        return this->msg;
    }

    void raise() const override{
        throw *this;
    }

    [[nodiscard]] BaseError *clone() const override{
        return new BaseError(*this);
    }

    [[nodiscard]] const char *what() const noexcept override{
        return this->msg.toStdString().c_str();
    }
};
