/********************************************************************************
** Form generated from reading UI file 'enter_factory_parameters_dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ENTER_FACTORY_PARAMETERS_DIALOG_H
#define UI_ENTER_FACTORY_PARAMETERS_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

QT_BEGIN_NAMESPACE

class Ui_EnterFactoryParametersDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *factoryParametersInstructions_label;
    QLabel *assemblyDate_label;
    QLineEdit *assemblyDate;
    QDialogButtonBox *buttonBox;
    QLineEdit *assemblyTime;
    QLabel *assemblyTime_label;
    QLabel *settingFactoryParameters_label;
    QGroupBox *serialNumberGroup;
    QGridLayout *gridLayout_2;
    QLabel *serialNumber_label;
    QComboBox *languageCountry;
    QLabel *languageCountry_label;
    QLabel *generatedSerialNumber_label;
    QLabel *generatedSerialNumber;
    QLineEdit *serialNumber;

    void setupUi(QDialog *EnterFactoryParametersDialog)
    {
        if (EnterFactoryParametersDialog->objectName().isEmpty())
            EnterFactoryParametersDialog->setObjectName(QString::fromUtf8("EnterFactoryParametersDialog"));
        EnterFactoryParametersDialog->resize(360, 277);
        gridLayout = new QGridLayout(EnterFactoryParametersDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        factoryParametersInstructions_label = new QLabel(EnterFactoryParametersDialog);
        factoryParametersInstructions_label->setObjectName(QString::fromUtf8("factoryParametersInstructions_label"));
        factoryParametersInstructions_label->setWordWrap(true);

        gridLayout->addWidget(factoryParametersInstructions_label, 0, 1, 1, 2);

        assemblyDate_label = new QLabel(EnterFactoryParametersDialog);
        assemblyDate_label->setObjectName(QString::fromUtf8("assemblyDate_label"));

        gridLayout->addWidget(assemblyDate_label, 1, 1, 1, 1);

        assemblyDate = new QLineEdit(EnterFactoryParametersDialog);
        assemblyDate->setObjectName(QString::fromUtf8("assemblyDate"));

        gridLayout->addWidget(assemblyDate, 1, 2, 1, 1);

        buttonBox = new QDialogButtonBox(EnterFactoryParametersDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 6, 1, 1, 2);

        assemblyTime = new QLineEdit(EnterFactoryParametersDialog);
        assemblyTime->setObjectName(QString::fromUtf8("assemblyTime"));

        gridLayout->addWidget(assemblyTime, 2, 2, 1, 1);

        assemblyTime_label = new QLabel(EnterFactoryParametersDialog);
        assemblyTime_label->setObjectName(QString::fromUtf8("assemblyTime_label"));

        gridLayout->addWidget(assemblyTime_label, 2, 1, 1, 1);

        settingFactoryParameters_label = new QLabel(EnterFactoryParametersDialog);
        settingFactoryParameters_label->setObjectName(QString::fromUtf8("settingFactoryParameters_label"));

        gridLayout->addWidget(settingFactoryParameters_label, 5, 1, 1, 2);

        serialNumberGroup = new QGroupBox(EnterFactoryParametersDialog);
        serialNumberGroup->setObjectName(QString::fromUtf8("serialNumberGroup"));
        gridLayout_2 = new QGridLayout(serialNumberGroup);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        serialNumber_label = new QLabel(serialNumberGroup);
        serialNumber_label->setObjectName(QString::fromUtf8("serialNumber_label"));

        gridLayout_2->addWidget(serialNumber_label, 2, 0, 1, 1);

        languageCountry = new QComboBox(serialNumberGroup);
        languageCountry->setObjectName(QString::fromUtf8("languageCountry"));

        gridLayout_2->addWidget(languageCountry, 1, 1, 1, 1);

        languageCountry_label = new QLabel(serialNumberGroup);
        languageCountry_label->setObjectName(QString::fromUtf8("languageCountry_label"));

        gridLayout_2->addWidget(languageCountry_label, 1, 0, 1, 1);

        generatedSerialNumber_label = new QLabel(serialNumberGroup);
        generatedSerialNumber_label->setObjectName(QString::fromUtf8("generatedSerialNumber_label"));

        gridLayout_2->addWidget(generatedSerialNumber_label, 3, 0, 1, 1);

        generatedSerialNumber = new QLabel(serialNumberGroup);
        generatedSerialNumber->setObjectName(QString::fromUtf8("generatedSerialNumber"));

        gridLayout_2->addWidget(generatedSerialNumber, 3, 1, 1, 1);

        serialNumber = new QLineEdit(serialNumberGroup);
        serialNumber->setObjectName(QString::fromUtf8("serialNumber"));

        gridLayout_2->addWidget(serialNumber, 2, 1, 1, 1);


        gridLayout->addWidget(serialNumberGroup, 4, 1, 1, 2);

        QWidget::setTabOrder(assemblyDate, assemblyTime);
        QWidget::setTabOrder(assemblyTime, languageCountry);
        QWidget::setTabOrder(languageCountry, serialNumber);

        retranslateUi(EnterFactoryParametersDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), EnterFactoryParametersDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), EnterFactoryParametersDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(EnterFactoryParametersDialog);
    } // setupUi

    void retranslateUi(QDialog *EnterFactoryParametersDialog)
    {
        EnterFactoryParametersDialog->setWindowTitle(QApplication::translate("EnterFactoryParametersDialog", "Set Factory Parameters", nullptr));
        factoryParametersInstructions_label->setText(QApplication::translate("EnterFactoryParametersDialog", "<html><head/><body><p>Please enter the assembly date and time, and serial number of this gimbal, then press OK to commit these values to flash</p></body></html>", nullptr));
        assemblyDate_label->setText(QApplication::translate("EnterFactoryParametersDialog", "Assembly Date (MM/DD/YYYY):", nullptr));
        assemblyDate->setInputMask(QApplication::translate("EnterFactoryParametersDialog", "09/09/9999;_", nullptr));
        assemblyTime->setInputMask(QApplication::translate("EnterFactoryParametersDialog", "09:09:09;_", nullptr));
        assemblyTime_label->setText(QApplication::translate("EnterFactoryParametersDialog", "Assembly Time: (HH/MM/SS)", nullptr));
        settingFactoryParameters_label->setText(QApplication::translate("EnterFactoryParametersDialog", "Setting Parameters...", nullptr));
        serialNumberGroup->setTitle(QApplication::translate("EnterFactoryParametersDialog", "Serial Number", nullptr));
        serialNumber_label->setText(QApplication::translate("EnterFactoryParametersDialog", "Serial Number (5 Digits):", nullptr));
        languageCountry_label->setText(QApplication::translate("EnterFactoryParametersDialog", "Language/Country:", nullptr));
        generatedSerialNumber_label->setText(QApplication::translate("EnterFactoryParametersDialog", "Generated Serial Number:", nullptr));
        generatedSerialNumber->setText(QApplication::translate("EnterFactoryParametersDialog", "None", nullptr));
        serialNumber->setInputMask(QApplication::translate("EnterFactoryParametersDialog", "99999;_", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EnterFactoryParametersDialog: public Ui_EnterFactoryParametersDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ENTER_FACTORY_PARAMETERS_DIALOG_H
