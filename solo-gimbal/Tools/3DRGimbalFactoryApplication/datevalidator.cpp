#include "datevalidator.h"

#include <QStringList>
#include <QDate>
#include <QDebug>

DateValidator::DateValidator()
{

}

DateValidator::~DateValidator()
{

}

QValidator::State DateValidator::validate(QString& input, int&) const
{
    // Parse the date and determine whether it is valid
    QDate date = QDate::fromString(input, "MM/dd/yyyy");
    // For this application, make sure the year is >= 2010
    if (date.isValid() && (date.year() >= 2010)) {
        return QValidator::Acceptable;
    } else {
        return QValidator::Invalid;
    }
}

