/********************************************************************************
** Form generated from reading UI file 'calibrate_axes_dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CALIBRATE_AXES_DIALOG_H
#define UI_CALIBRATE_AXES_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_CalibrateAxesDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *pitchStatus_label;
    QLabel *rollStatus;
    QProgressBar *rollProgress;
    QLabel *rollStatus_label;
    QLabel *yawStatus;
    QProgressBar *yawProgress;
    QLabel *yawStatus_label;
    QProgressBar *pitchProgress;
    QLabel *pitchStatus;
    QFrame *cancelButtonFrame;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *cancelButton;

    void setupUi(QDialog *CalibrateAxesDialog)
    {
        if (CalibrateAxesDialog->objectName().isEmpty())
            CalibrateAxesDialog->setObjectName(QString::fromUtf8("CalibrateAxesDialog"));
        CalibrateAxesDialog->resize(400, 131);
        gridLayout = new QGridLayout(CalibrateAxesDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        pitchStatus_label = new QLabel(CalibrateAxesDialog);
        pitchStatus_label->setObjectName(QString::fromUtf8("pitchStatus_label"));

        gridLayout->addWidget(pitchStatus_label, 0, 0, 1, 1);

        rollStatus = new QLabel(CalibrateAxesDialog);
        rollStatus->setObjectName(QString::fromUtf8("rollStatus"));

        gridLayout->addWidget(rollStatus, 1, 1, 1, 1);

        rollProgress = new QProgressBar(CalibrateAxesDialog);
        rollProgress->setObjectName(QString::fromUtf8("rollProgress"));
        rollProgress->setMaximum(0);
        rollProgress->setValue(0);
        rollProgress->setTextVisible(false);

        gridLayout->addWidget(rollProgress, 1, 2, 1, 1);

        rollStatus_label = new QLabel(CalibrateAxesDialog);
        rollStatus_label->setObjectName(QString::fromUtf8("rollStatus_label"));

        gridLayout->addWidget(rollStatus_label, 1, 0, 1, 1);

        yawStatus = new QLabel(CalibrateAxesDialog);
        yawStatus->setObjectName(QString::fromUtf8("yawStatus"));

        gridLayout->addWidget(yawStatus, 2, 1, 1, 1);

        yawProgress = new QProgressBar(CalibrateAxesDialog);
        yawProgress->setObjectName(QString::fromUtf8("yawProgress"));
        yawProgress->setMaximum(0);
        yawProgress->setValue(0);
        yawProgress->setTextVisible(false);

        gridLayout->addWidget(yawProgress, 2, 2, 1, 1);

        yawStatus_label = new QLabel(CalibrateAxesDialog);
        yawStatus_label->setObjectName(QString::fromUtf8("yawStatus_label"));

        gridLayout->addWidget(yawStatus_label, 2, 0, 1, 1);

        pitchProgress = new QProgressBar(CalibrateAxesDialog);
        pitchProgress->setObjectName(QString::fromUtf8("pitchProgress"));
        pitchProgress->setMaximum(0);
        pitchProgress->setValue(0);
        pitchProgress->setTextVisible(false);

        gridLayout->addWidget(pitchProgress, 0, 2, 1, 1);

        pitchStatus = new QLabel(CalibrateAxesDialog);
        pitchStatus->setObjectName(QString::fromUtf8("pitchStatus"));

        gridLayout->addWidget(pitchStatus, 0, 1, 1, 1);

        cancelButtonFrame = new QFrame(CalibrateAxesDialog);
        cancelButtonFrame->setObjectName(QString::fromUtf8("cancelButtonFrame"));
        cancelButtonFrame->setFrameShape(QFrame::StyledPanel);
        cancelButtonFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(cancelButtonFrame);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        cancelButton = new QPushButton(cancelButtonFrame);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        horizontalLayout->addWidget(cancelButton);


        gridLayout->addWidget(cancelButtonFrame, 3, 0, 1, 3);


        retranslateUi(CalibrateAxesDialog);

        QMetaObject::connectSlotsByName(CalibrateAxesDialog);
    } // setupUi

    void retranslateUi(QDialog *CalibrateAxesDialog)
    {
        CalibrateAxesDialog->setWindowTitle(QApplication::translate("CalibrateAxesDialog", "Gimbal Axis Calibration Progress", nullptr));
        pitchStatus_label->setText(QApplication::translate("CalibrateAxesDialog", "Pitch Status:", nullptr));
        rollStatus->setText(QApplication::translate("CalibrateAxesDialog", "Waiting to Calibrate", nullptr));
        rollStatus_label->setText(QApplication::translate("CalibrateAxesDialog", "Roll Status:", nullptr));
        yawStatus->setText(QApplication::translate("CalibrateAxesDialog", "Waiting to Calibrate", nullptr));
        yawStatus_label->setText(QApplication::translate("CalibrateAxesDialog", "Yaw Status:", nullptr));
        pitchStatus->setText(QApplication::translate("CalibrateAxesDialog", "Waiting to Calibrate", nullptr));
        cancelButton->setText(QApplication::translate("CalibrateAxesDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CalibrateAxesDialog: public Ui_CalibrateAxesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CALIBRATE_AXES_DIALOG_H
