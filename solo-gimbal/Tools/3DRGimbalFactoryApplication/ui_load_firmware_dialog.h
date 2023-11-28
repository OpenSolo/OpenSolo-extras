/********************************************************************************
** Form generated from reading UI file 'load_firmware_dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOAD_FIRMWARE_DIALOG_H
#define UI_LOAD_FIRMWARE_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>

QT_BEGIN_NAMESPACE

class Ui_LoadFirmwareDialog
{
public:
    QGridLayout *gridLayout;
    QProgressBar *firmwareProgress;
    QLabel *loadingFirmware_label;
    QLabel *erasingFlash_label;

    void setupUi(QDialog *LoadFirmwareDialog)
    {
        if (LoadFirmwareDialog->objectName().isEmpty())
            LoadFirmwareDialog->setObjectName(QString::fromUtf8("LoadFirmwareDialog"));
        LoadFirmwareDialog->resize(366, 88);
        gridLayout = new QGridLayout(LoadFirmwareDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        firmwareProgress = new QProgressBar(LoadFirmwareDialog);
        firmwareProgress->setObjectName(QString::fromUtf8("firmwareProgress"));
        firmwareProgress->setValue(0);
        firmwareProgress->setInvertedAppearance(false);

        gridLayout->addWidget(firmwareProgress, 2, 1, 1, 1);

        loadingFirmware_label = new QLabel(LoadFirmwareDialog);
        loadingFirmware_label->setObjectName(QString::fromUtf8("loadingFirmware_label"));

        gridLayout->addWidget(loadingFirmware_label, 1, 1, 1, 1);

        erasingFlash_label = new QLabel(LoadFirmwareDialog);
        erasingFlash_label->setObjectName(QString::fromUtf8("erasingFlash_label"));

        gridLayout->addWidget(erasingFlash_label, 0, 1, 1, 1);


        retranslateUi(LoadFirmwareDialog);

        QMetaObject::connectSlotsByName(LoadFirmwareDialog);
    } // setupUi

    void retranslateUi(QDialog *LoadFirmwareDialog)
    {
        LoadFirmwareDialog->setWindowTitle(QApplication::translate("LoadFirmwareDialog", "Gimbal Firmware Load Progress", nullptr));
        loadingFirmware_label->setText(QApplication::translate("LoadFirmwareDialog", "Loading Gimbal Firmware...", nullptr));
        erasingFlash_label->setText(QApplication::translate("LoadFirmwareDialog", "Erasing Flash...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoadFirmwareDialog: public Ui_LoadFirmwareDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOAD_FIRMWARE_DIALOG_H
