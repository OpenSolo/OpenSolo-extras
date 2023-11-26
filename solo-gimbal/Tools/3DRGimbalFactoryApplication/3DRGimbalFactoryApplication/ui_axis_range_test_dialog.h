/********************************************************************************
** Form generated from reading UI file 'axis_range_test_dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AXIS_RANGE_TEST_DIALOG_H
#define UI_AXIS_RANGE_TEST_DIALOG_H

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

class Ui_AxisRangeTestDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *pitchCheckPositiveStatus;
    QFrame *buttonsFrame;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *okButton;
    QPushButton *retryButton;
    QLabel *testStatus;
    QLabel *yawReturnHomeStatus;
    QLabel *rollReturnHomeStatus;
    QLabel *yawCheckNegativeStatus;
    QLabel *pitchReturnHomeStatus;
    QLabel *pitchCheckNegativeStatus;
    QLabel *rollCheckNegativeStatus;
    QProgressBar *rollCheckNegativeProgress;
    QProgressBar *yawCheckNegativeProgress;
    QLabel *rollCheckPositiveStatus;
    QProgressBar *rollReturnHomeProgress;
    QProgressBar *rollCheckPositiveProgress;
    QLabel *yawCheckPositiveStatus;
    QLabel *pitchCheckPositive_label;
    QLabel *rollCheckNegative_label;
    QProgressBar *pitchCheckPositiveProgress;
    QLabel *pitchCheckNegative_label;
    QLabel *pitchReturnHome_label;
    QLabel *rollCheckPositive_label;
    QLabel *rollReturnHome_label;
    QLabel *yawCheckNegative_label;
    QLabel *yawCheckPositive_label;
    QLabel *yawReturnHome_label;
    QProgressBar *pitchCheckNegativeProgress;
    QProgressBar *pitchReturnHomeProgress;
    QProgressBar *yawCheckPositiveProgress;
    QProgressBar *yawReturnHomeProgress;
    QLabel *testStatus_label;

    void setupUi(QDialog *AxisRangeTestDialog)
    {
        if (AxisRangeTestDialog->objectName().isEmpty())
            AxisRangeTestDialog->setObjectName(QString::fromUtf8("AxisRangeTestDialog"));
        AxisRangeTestDialog->resize(395, 434);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(AxisRangeTestDialog->sizePolicy().hasHeightForWidth());
        AxisRangeTestDialog->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(AxisRangeTestDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        pitchCheckPositiveStatus = new QLabel(AxisRangeTestDialog);
        pitchCheckPositiveStatus->setObjectName(QString::fromUtf8("pitchCheckPositiveStatus"));
        pitchCheckPositiveStatus->setMinimumSize(QSize(32, 32));
        pitchCheckPositiveStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(pitchCheckPositiveStatus, 1, 2, 1, 1);

        buttonsFrame = new QFrame(AxisRangeTestDialog);
        buttonsFrame->setObjectName(QString::fromUtf8("buttonsFrame"));
        buttonsFrame->setFrameShape(QFrame::StyledPanel);
        buttonsFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(buttonsFrame);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        okButton = new QPushButton(buttonsFrame);
        okButton->setObjectName(QString::fromUtf8("okButton"));
        okButton->setEnabled(false);

        horizontalLayout->addWidget(okButton);

        retryButton = new QPushButton(buttonsFrame);
        retryButton->setObjectName(QString::fromUtf8("retryButton"));
        retryButton->setEnabled(false);

        horizontalLayout->addWidget(retryButton);


        gridLayout->addWidget(buttonsFrame, 10, 0, 1, 3);

        testStatus = new QLabel(AxisRangeTestDialog);
        testStatus->setObjectName(QString::fromUtf8("testStatus"));
        testStatus->setWordWrap(true);

        gridLayout->addWidget(testStatus, 9, 1, 1, 2);

        yawReturnHomeStatus = new QLabel(AxisRangeTestDialog);
        yawReturnHomeStatus->setObjectName(QString::fromUtf8("yawReturnHomeStatus"));
        yawReturnHomeStatus->setMinimumSize(QSize(32, 32));
        yawReturnHomeStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(yawReturnHomeStatus, 8, 2, 1, 1);

        rollReturnHomeStatus = new QLabel(AxisRangeTestDialog);
        rollReturnHomeStatus->setObjectName(QString::fromUtf8("rollReturnHomeStatus"));
        rollReturnHomeStatus->setMinimumSize(QSize(32, 32));
        rollReturnHomeStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(rollReturnHomeStatus, 5, 2, 1, 1);

        yawCheckNegativeStatus = new QLabel(AxisRangeTestDialog);
        yawCheckNegativeStatus->setObjectName(QString::fromUtf8("yawCheckNegativeStatus"));
        yawCheckNegativeStatus->setMinimumSize(QSize(32, 32));
        yawCheckNegativeStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(yawCheckNegativeStatus, 6, 2, 1, 1);

        pitchReturnHomeStatus = new QLabel(AxisRangeTestDialog);
        pitchReturnHomeStatus->setObjectName(QString::fromUtf8("pitchReturnHomeStatus"));
        pitchReturnHomeStatus->setMinimumSize(QSize(32, 32));
        pitchReturnHomeStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(pitchReturnHomeStatus, 2, 2, 1, 1);

        pitchCheckNegativeStatus = new QLabel(AxisRangeTestDialog);
        pitchCheckNegativeStatus->setObjectName(QString::fromUtf8("pitchCheckNegativeStatus"));
        pitchCheckNegativeStatus->setMinimumSize(QSize(32, 32));
        pitchCheckNegativeStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(pitchCheckNegativeStatus, 0, 2, 1, 1);

        rollCheckNegativeStatus = new QLabel(AxisRangeTestDialog);
        rollCheckNegativeStatus->setObjectName(QString::fromUtf8("rollCheckNegativeStatus"));
        rollCheckNegativeStatus->setMinimumSize(QSize(32, 32));
        rollCheckNegativeStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(rollCheckNegativeStatus, 3, 2, 1, 1);

        rollCheckNegativeProgress = new QProgressBar(AxisRangeTestDialog);
        rollCheckNegativeProgress->setObjectName(QString::fromUtf8("rollCheckNegativeProgress"));
        rollCheckNegativeProgress->setValue(0);

        gridLayout->addWidget(rollCheckNegativeProgress, 3, 1, 1, 1);

        yawCheckNegativeProgress = new QProgressBar(AxisRangeTestDialog);
        yawCheckNegativeProgress->setObjectName(QString::fromUtf8("yawCheckNegativeProgress"));
        yawCheckNegativeProgress->setValue(0);

        gridLayout->addWidget(yawCheckNegativeProgress, 6, 1, 1, 1);

        rollCheckPositiveStatus = new QLabel(AxisRangeTestDialog);
        rollCheckPositiveStatus->setObjectName(QString::fromUtf8("rollCheckPositiveStatus"));
        rollCheckPositiveStatus->setMinimumSize(QSize(32, 32));
        rollCheckPositiveStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(rollCheckPositiveStatus, 4, 2, 1, 1);

        rollReturnHomeProgress = new QProgressBar(AxisRangeTestDialog);
        rollReturnHomeProgress->setObjectName(QString::fromUtf8("rollReturnHomeProgress"));
        rollReturnHomeProgress->setValue(0);

        gridLayout->addWidget(rollReturnHomeProgress, 5, 1, 1, 1);

        rollCheckPositiveProgress = new QProgressBar(AxisRangeTestDialog);
        rollCheckPositiveProgress->setObjectName(QString::fromUtf8("rollCheckPositiveProgress"));
        rollCheckPositiveProgress->setValue(0);

        gridLayout->addWidget(rollCheckPositiveProgress, 4, 1, 1, 1);

        yawCheckPositiveStatus = new QLabel(AxisRangeTestDialog);
        yawCheckPositiveStatus->setObjectName(QString::fromUtf8("yawCheckPositiveStatus"));
        yawCheckPositiveStatus->setMinimumSize(QSize(32, 32));
        yawCheckPositiveStatus->setPixmap(QPixmap(QString::fromUtf8(":/icons/32x32/Images/Ellipsis_32x32.png")));

        gridLayout->addWidget(yawCheckPositiveStatus, 7, 2, 1, 1);

        pitchCheckPositive_label = new QLabel(AxisRangeTestDialog);
        pitchCheckPositive_label->setObjectName(QString::fromUtf8("pitchCheckPositive_label"));

        gridLayout->addWidget(pitchCheckPositive_label, 1, 0, 1, 1);

        rollCheckNegative_label = new QLabel(AxisRangeTestDialog);
        rollCheckNegative_label->setObjectName(QString::fromUtf8("rollCheckNegative_label"));

        gridLayout->addWidget(rollCheckNegative_label, 3, 0, 1, 1);

        pitchCheckPositiveProgress = new QProgressBar(AxisRangeTestDialog);
        pitchCheckPositiveProgress->setObjectName(QString::fromUtf8("pitchCheckPositiveProgress"));
        pitchCheckPositiveProgress->setValue(0);

        gridLayout->addWidget(pitchCheckPositiveProgress, 1, 1, 1, 1);

        pitchCheckNegative_label = new QLabel(AxisRangeTestDialog);
        pitchCheckNegative_label->setObjectName(QString::fromUtf8("pitchCheckNegative_label"));

        gridLayout->addWidget(pitchCheckNegative_label, 0, 0, 1, 1);

        pitchReturnHome_label = new QLabel(AxisRangeTestDialog);
        pitchReturnHome_label->setObjectName(QString::fromUtf8("pitchReturnHome_label"));

        gridLayout->addWidget(pitchReturnHome_label, 2, 0, 1, 1);

        rollCheckPositive_label = new QLabel(AxisRangeTestDialog);
        rollCheckPositive_label->setObjectName(QString::fromUtf8("rollCheckPositive_label"));

        gridLayout->addWidget(rollCheckPositive_label, 4, 0, 1, 1);

        rollReturnHome_label = new QLabel(AxisRangeTestDialog);
        rollReturnHome_label->setObjectName(QString::fromUtf8("rollReturnHome_label"));

        gridLayout->addWidget(rollReturnHome_label, 5, 0, 1, 1);

        yawCheckNegative_label = new QLabel(AxisRangeTestDialog);
        yawCheckNegative_label->setObjectName(QString::fromUtf8("yawCheckNegative_label"));

        gridLayout->addWidget(yawCheckNegative_label, 6, 0, 1, 1);

        yawCheckPositive_label = new QLabel(AxisRangeTestDialog);
        yawCheckPositive_label->setObjectName(QString::fromUtf8("yawCheckPositive_label"));

        gridLayout->addWidget(yawCheckPositive_label, 7, 0, 1, 1);

        yawReturnHome_label = new QLabel(AxisRangeTestDialog);
        yawReturnHome_label->setObjectName(QString::fromUtf8("yawReturnHome_label"));

        gridLayout->addWidget(yawReturnHome_label, 8, 0, 1, 1);

        pitchCheckNegativeProgress = new QProgressBar(AxisRangeTestDialog);
        pitchCheckNegativeProgress->setObjectName(QString::fromUtf8("pitchCheckNegativeProgress"));
        pitchCheckNegativeProgress->setValue(0);

        gridLayout->addWidget(pitchCheckNegativeProgress, 0, 1, 1, 1);

        pitchReturnHomeProgress = new QProgressBar(AxisRangeTestDialog);
        pitchReturnHomeProgress->setObjectName(QString::fromUtf8("pitchReturnHomeProgress"));
        pitchReturnHomeProgress->setValue(0);

        gridLayout->addWidget(pitchReturnHomeProgress, 2, 1, 1, 1);

        yawCheckPositiveProgress = new QProgressBar(AxisRangeTestDialog);
        yawCheckPositiveProgress->setObjectName(QString::fromUtf8("yawCheckPositiveProgress"));
        yawCheckPositiveProgress->setValue(0);

        gridLayout->addWidget(yawCheckPositiveProgress, 7, 1, 1, 1);

        yawReturnHomeProgress = new QProgressBar(AxisRangeTestDialog);
        yawReturnHomeProgress->setObjectName(QString::fromUtf8("yawReturnHomeProgress"));
        yawReturnHomeProgress->setValue(0);

        gridLayout->addWidget(yawReturnHomeProgress, 8, 1, 1, 1);

        testStatus_label = new QLabel(AxisRangeTestDialog);
        testStatus_label->setObjectName(QString::fromUtf8("testStatus_label"));

        gridLayout->addWidget(testStatus_label, 9, 0, 1, 1);


        retranslateUi(AxisRangeTestDialog);

        QMetaObject::connectSlotsByName(AxisRangeTestDialog);
    } // setupUi

    void retranslateUi(QDialog *AxisRangeTestDialog)
    {
        AxisRangeTestDialog->setWindowTitle(QApplication::translate("AxisRangeTestDialog", "Axis Range Test", nullptr));
        pitchCheckPositiveStatus->setText(QString());
        okButton->setText(QApplication::translate("AxisRangeTestDialog", "OK", nullptr));
        retryButton->setText(QApplication::translate("AxisRangeTestDialog", "Retry", nullptr));
        testStatus->setText(QApplication::translate("AxisRangeTestDialog", "Test Running...", nullptr));
        yawReturnHomeStatus->setText(QString());
        rollReturnHomeStatus->setText(QString());
        yawCheckNegativeStatus->setText(QString());
        pitchReturnHomeStatus->setText(QString());
        pitchCheckNegativeStatus->setText(QString());
        rollCheckNegativeStatus->setText(QString());
        rollCheckPositiveStatus->setText(QString());
        yawCheckPositiveStatus->setText(QString());
        pitchCheckPositive_label->setText(QApplication::translate("AxisRangeTestDialog", "Pitch Check Positive Range:", nullptr));
        rollCheckNegative_label->setText(QApplication::translate("AxisRangeTestDialog", "Roll Check Negative Range:", nullptr));
        pitchCheckNegative_label->setText(QApplication::translate("AxisRangeTestDialog", "Pitch Check Negative Range:", nullptr));
        pitchReturnHome_label->setText(QApplication::translate("AxisRangeTestDialog", "Pitch Return Home:", nullptr));
        rollCheckPositive_label->setText(QApplication::translate("AxisRangeTestDialog", "Roll Check Positive Range:", nullptr));
        rollReturnHome_label->setText(QApplication::translate("AxisRangeTestDialog", "Roll Return Home:", nullptr));
        yawCheckNegative_label->setText(QApplication::translate("AxisRangeTestDialog", "Yaw Check Negative Range:", nullptr));
        yawCheckPositive_label->setText(QApplication::translate("AxisRangeTestDialog", "Yaw Check Positive Range:", nullptr));
        yawReturnHome_label->setText(QApplication::translate("AxisRangeTestDialog", "Yaw Return Home:", nullptr));
        testStatus_label->setText(QApplication::translate("AxisRangeTestDialog", "Test Status:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AxisRangeTestDialog: public Ui_AxisRangeTestDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AXIS_RANGE_TEST_DIALOG_H
