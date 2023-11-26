#include "timevalidator.h"

#include <QTime>
#include <QDebug>

TimeValidator::TimeValidator()
{

}

TimeValidator::~TimeValidator()
{

}

QValidator::State TimeValidator::validate(QString& input, int&) const
{
    // Parse the date and determine whether it is valid
    QTime time = QTime::fromString(input, "hh:mm:ss");
    if (time.isValid()) {
        return QValidator::Acceptable;
    } else {
        return QValidator::Invalid;
    }
}
