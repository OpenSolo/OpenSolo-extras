#ifndef DATEVALIDATOR_H
#define DATEVALIDATOR_H

#include <QValidator>

class DateValidator : public QValidator
{
public:
    DateValidator();
    ~DateValidator();

    State validate(QString& input, int&) const;
};

#endif // DATEVALIDATOR_H
