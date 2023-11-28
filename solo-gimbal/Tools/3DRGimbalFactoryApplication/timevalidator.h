#ifndef TIMEVALIDATOR_H
#define TIMEVALIDATOR_H

#include <QValidator>

class TimeValidator : public QValidator
{
public:
    TimeValidator();
    ~TimeValidator();

    State validate(QString& input, int&) const;
};

#endif // TIMEVALIDATOR_H
