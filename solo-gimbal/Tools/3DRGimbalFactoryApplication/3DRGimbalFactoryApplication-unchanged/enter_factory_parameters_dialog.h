#ifndef ENTER_FACTORY_PARAMETERS_DIALOG_H
#define ENTER_FACTORY_PARAMETERS_DIALOG_H

#include <QDialog>
#include <cstdint>

namespace Ui {
class EnterFactoryParametersDialog;
}

class EnterFactoryParametersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EnterFactoryParametersDialog(QWidget *parent = 0);
    ~EnterFactoryParametersDialog();

public slots:
    void factoryParametersLoaded();
    void accept();
    void reject();

signals:
    void setGimbalFactoryParameters(unsigned short assyYear,
                                    unsigned char assyMonth,
                                    unsigned char assyDay,
                                    unsigned char assyHour,
                                    unsigned char assyMinute,
                                    unsigned char assySecond,
                                    unsigned long serialNumber1,
                                    unsigned long serialNumber2,
                                    unsigned long serialNumber3);

private:
    Ui::EnterFactoryParametersDialog *ui;

    unsigned char m_partCode1;
    unsigned char m_partCode2;
    unsigned char m_design;
    unsigned char m_option;

    void updateGeneratedSerialNumber();

private slots:
    void on_assemblyDate_textChanged(const QString&);
    void on_serialNumber_textChanged(const QString&);
    void on_languageCountry_currentIndexChanged(int);
};

#endif // ENTER_FACTORY_PARAMETERS_DIALOG_H
