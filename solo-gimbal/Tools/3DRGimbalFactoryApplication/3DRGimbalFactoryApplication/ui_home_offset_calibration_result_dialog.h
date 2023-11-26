/********************************************************************************
** Form generated from reading UI file 'home_offset_calibration_result_dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HOME_OFFSET_CALIBRATION_RESULT_DIALOG_H
#define UI_HOME_OFFSET_CALIBRATION_RESULT_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_HomeOffsetCalibrationResultDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *pitchHomeOffset_label;
    QLabel *yawHomeOffset_label;
    QLabel *rollHomeOffset_label;
    QLabel *yawHomeOffset;
    QLabel *pitchHomeOffset;
    QLabel *rollHomeOffset;
    QLabel *calibrationStatus_label;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *HomeOffsetCalibrationResultDialog)
    {
        if (HomeOffsetCalibrationResultDialog->objectName().isEmpty())
            HomeOffsetCalibrationResultDialog->setObjectName(QString::fromUtf8("HomeOffsetCalibrationResultDialog"));
        HomeOffsetCalibrationResultDialog->resize(291, 138);
        gridLayout = new QGridLayout(HomeOffsetCalibrationResultDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        pitchHomeOffset_label = new QLabel(HomeOffsetCalibrationResultDialog);
        pitchHomeOffset_label->setObjectName(QString::fromUtf8("pitchHomeOffset_label"));

        gridLayout->addWidget(pitchHomeOffset_label, 2, 0, 1, 1);

        yawHomeOffset_label = new QLabel(HomeOffsetCalibrationResultDialog);
        yawHomeOffset_label->setObjectName(QString::fromUtf8("yawHomeOffset_label"));

        gridLayout->addWidget(yawHomeOffset_label, 1, 0, 1, 1);

        rollHomeOffset_label = new QLabel(HomeOffsetCalibrationResultDialog);
        rollHomeOffset_label->setObjectName(QString::fromUtf8("rollHomeOffset_label"));

        gridLayout->addWidget(rollHomeOffset_label, 3, 0, 1, 1);

        yawHomeOffset = new QLabel(HomeOffsetCalibrationResultDialog);
        yawHomeOffset->setObjectName(QString::fromUtf8("yawHomeOffset"));

        gridLayout->addWidget(yawHomeOffset, 1, 1, 1, 1);

        pitchHomeOffset = new QLabel(HomeOffsetCalibrationResultDialog);
        pitchHomeOffset->setObjectName(QString::fromUtf8("pitchHomeOffset"));

        gridLayout->addWidget(pitchHomeOffset, 2, 1, 1, 1);

        rollHomeOffset = new QLabel(HomeOffsetCalibrationResultDialog);
        rollHomeOffset->setObjectName(QString::fromUtf8("rollHomeOffset"));

        gridLayout->addWidget(rollHomeOffset, 3, 1, 1, 1);

        calibrationStatus_label = new QLabel(HomeOffsetCalibrationResultDialog);
        calibrationStatus_label->setObjectName(QString::fromUtf8("calibrationStatus_label"));

        gridLayout->addWidget(calibrationStatus_label, 0, 0, 1, 2);

        buttonBox = new QDialogButtonBox(HomeOffsetCalibrationResultDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setEnabled(false);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 4, 0, 1, 2);


        retranslateUi(HomeOffsetCalibrationResultDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), HomeOffsetCalibrationResultDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), HomeOffsetCalibrationResultDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(HomeOffsetCalibrationResultDialog);
    } // setupUi

    void retranslateUi(QDialog *HomeOffsetCalibrationResultDialog)
    {
        HomeOffsetCalibrationResultDialog->setWindowTitle(QApplication::translate("HomeOffsetCalibrationResultDialog", "Home Offset Calibration Result", nullptr));
        pitchHomeOffset_label->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "New Pitch Home Offset:", nullptr));
        yawHomeOffset_label->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "New Yaw Home Offset:", nullptr));
        rollHomeOffset_label->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "New Roll Home Offset:", nullptr));
        yawHomeOffset->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "Waiting...", nullptr));
        pitchHomeOffset->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "Waiting...", nullptr));
        rollHomeOffset->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "Waiting...", nullptr));
        calibrationStatus_label->setText(QApplication::translate("HomeOffsetCalibrationResultDialog", "Calibrating Home Offsets...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HomeOffsetCalibrationResultDialog: public Ui_HomeOffsetCalibrationResultDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HOME_OFFSET_CALIBRATION_RESULT_DIALOG_H
