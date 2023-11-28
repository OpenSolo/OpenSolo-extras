/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QGroupBox *connectionGroup;
    QFormLayout *formLayout;
    QLabel *comPort_label;
    QComboBox *comPort;
    QLabel *baudRate_label;
    QComboBox *baudRate;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QGroupBox *calibrationGroup;
    QGridLayout *gridLayout_4;
    QPushButton *runAxisCalibrationButton;
    QPushButton *setHomePositionsButton;
    QPushButton *setUnitParametersButton;
    QPushButton *eraseGimbalFlashButton;
    QPushButton *factoryTestsButton;
    QGroupBox *firmwareGroup;
    QGridLayout *gridLayout_3;
    QLabel *firmwareImage_label;
    QLineEdit *firmwareImage;
    QPushButton *firmwareImageBrowseButton;
    QPushButton *loadFirmwareButton;
    QLabel *serialNumber_label;
    QLabel *assemblyDateTime;
    QSpacerItem *horizontalSpacer;
    QLabel *serialNumber;
    QLabel *firmwareLoaded;
    QLabel *connectionStatus_label;
    QLabel *firmwareLoaded_label;
    QSpacerItem *verticalSpacer;
    QLabel *connectionStatus;
    QLabel *assemblyDateTime_label;
    QLabel *version_label;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(348, 583);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        connectionGroup = new QGroupBox(centralWidget);
        connectionGroup->setObjectName(QString::fromUtf8("connectionGroup"));
        formLayout = new QFormLayout(connectionGroup);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        comPort_label = new QLabel(connectionGroup);
        comPort_label->setObjectName(QString::fromUtf8("comPort_label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, comPort_label);

        comPort = new QComboBox(connectionGroup);
        comPort->setObjectName(QString::fromUtf8("comPort"));

        formLayout->setWidget(0, QFormLayout::FieldRole, comPort);

        baudRate_label = new QLabel(connectionGroup);
        baudRate_label->setObjectName(QString::fromUtf8("baudRate_label"));

        formLayout->setWidget(1, QFormLayout::LabelRole, baudRate_label);

        baudRate = new QComboBox(connectionGroup);
        baudRate->setObjectName(QString::fromUtf8("baudRate"));

        formLayout->setWidget(1, QFormLayout::FieldRole, baudRate);

        connectButton = new QPushButton(connectionGroup);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));

        formLayout->setWidget(2, QFormLayout::FieldRole, connectButton);

        disconnectButton = new QPushButton(connectionGroup);
        disconnectButton->setObjectName(QString::fromUtf8("disconnectButton"));
        disconnectButton->setEnabled(false);

        formLayout->setWidget(3, QFormLayout::FieldRole, disconnectButton);


        gridLayout->addWidget(connectionGroup, 4, 0, 1, 3);

        calibrationGroup = new QGroupBox(centralWidget);
        calibrationGroup->setObjectName(QString::fromUtf8("calibrationGroup"));
        gridLayout_4 = new QGridLayout(calibrationGroup);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        runAxisCalibrationButton = new QPushButton(calibrationGroup);
        runAxisCalibrationButton->setObjectName(QString::fromUtf8("runAxisCalibrationButton"));
        runAxisCalibrationButton->setEnabled(false);

        gridLayout_4->addWidget(runAxisCalibrationButton, 0, 0, 1, 1);

        setHomePositionsButton = new QPushButton(calibrationGroup);
        setHomePositionsButton->setObjectName(QString::fromUtf8("setHomePositionsButton"));
        setHomePositionsButton->setEnabled(false);

        gridLayout_4->addWidget(setHomePositionsButton, 1, 0, 1, 1);

        setUnitParametersButton = new QPushButton(calibrationGroup);
        setUnitParametersButton->setObjectName(QString::fromUtf8("setUnitParametersButton"));
        setUnitParametersButton->setEnabled(false);

        gridLayout_4->addWidget(setUnitParametersButton, 2, 0, 1, 1);

        eraseGimbalFlashButton = new QPushButton(calibrationGroup);
        eraseGimbalFlashButton->setObjectName(QString::fromUtf8("eraseGimbalFlashButton"));
        eraseGimbalFlashButton->setEnabled(false);

        gridLayout_4->addWidget(eraseGimbalFlashButton, 3, 0, 1, 1);

        factoryTestsButton = new QPushButton(calibrationGroup);
        factoryTestsButton->setObjectName(QString::fromUtf8("factoryTestsButton"));
        factoryTestsButton->setEnabled(false);

        gridLayout_4->addWidget(factoryTestsButton, 4, 0, 1, 1);


        gridLayout->addWidget(calibrationGroup, 6, 0, 1, 3);

        firmwareGroup = new QGroupBox(centralWidget);
        firmwareGroup->setObjectName(QString::fromUtf8("firmwareGroup"));
        gridLayout_3 = new QGridLayout(firmwareGroup);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        firmwareImage_label = new QLabel(firmwareGroup);
        firmwareImage_label->setObjectName(QString::fromUtf8("firmwareImage_label"));

        gridLayout_3->addWidget(firmwareImage_label, 0, 0, 1, 1);

        firmwareImage = new QLineEdit(firmwareGroup);
        firmwareImage->setObjectName(QString::fromUtf8("firmwareImage"));
        firmwareImage->setEnabled(false);

        gridLayout_3->addWidget(firmwareImage, 0, 1, 1, 1);

        firmwareImageBrowseButton = new QPushButton(firmwareGroup);
        firmwareImageBrowseButton->setObjectName(QString::fromUtf8("firmwareImageBrowseButton"));
        firmwareImageBrowseButton->setEnabled(false);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(firmwareImageBrowseButton->sizePolicy().hasHeightForWidth());
        firmwareImageBrowseButton->setSizePolicy(sizePolicy);
        firmwareImageBrowseButton->setMaximumSize(QSize(30, 16777215));

        gridLayout_3->addWidget(firmwareImageBrowseButton, 0, 2, 1, 1);

        loadFirmwareButton = new QPushButton(firmwareGroup);
        loadFirmwareButton->setObjectName(QString::fromUtf8("loadFirmwareButton"));
        loadFirmwareButton->setEnabled(false);

        gridLayout_3->addWidget(loadFirmwareButton, 1, 0, 1, 1);


        gridLayout->addWidget(firmwareGroup, 5, 0, 1, 3);

        serialNumber_label = new QLabel(centralWidget);
        serialNumber_label->setObjectName(QString::fromUtf8("serialNumber_label"));

        gridLayout->addWidget(serialNumber_label, 3, 0, 1, 1);

        assemblyDateTime = new QLabel(centralWidget);
        assemblyDateTime->setObjectName(QString::fromUtf8("assemblyDateTime"));

        gridLayout->addWidget(assemblyDateTime, 2, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 2, 4, 1);

        serialNumber = new QLabel(centralWidget);
        serialNumber->setObjectName(QString::fromUtf8("serialNumber"));

        gridLayout->addWidget(serialNumber, 3, 1, 1, 1);

        firmwareLoaded = new QLabel(centralWidget);
        firmwareLoaded->setObjectName(QString::fromUtf8("firmwareLoaded"));

        gridLayout->addWidget(firmwareLoaded, 1, 1, 1, 1);

        connectionStatus_label = new QLabel(centralWidget);
        connectionStatus_label->setObjectName(QString::fromUtf8("connectionStatus_label"));

        gridLayout->addWidget(connectionStatus_label, 0, 0, 1, 1);

        firmwareLoaded_label = new QLabel(centralWidget);
        firmwareLoaded_label->setObjectName(QString::fromUtf8("firmwareLoaded_label"));

        gridLayout->addWidget(firmwareLoaded_label, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 8, 0, 1, 3);

        connectionStatus = new QLabel(centralWidget);
        connectionStatus->setObjectName(QString::fromUtf8("connectionStatus"));

        gridLayout->addWidget(connectionStatus, 0, 1, 1, 1);

        assemblyDateTime_label = new QLabel(centralWidget);
        assemblyDateTime_label->setObjectName(QString::fromUtf8("assemblyDateTime_label"));

        gridLayout->addWidget(assemblyDateTime_label, 2, 0, 1, 1);

        version_label = new QLabel(centralWidget);
        version_label->setObjectName(QString::fromUtf8("version_label"));

        gridLayout->addWidget(version_label, 7, 0, 1, 3);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 348, 21));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "3DR Gimbal Factory Application", nullptr));
        connectionGroup->setTitle(QApplication::translate("MainWindow", "Connection", nullptr));
        comPort_label->setText(QApplication::translate("MainWindow", "COM Port:", nullptr));
        baudRate_label->setText(QApplication::translate("MainWindow", "Baud Rate:", nullptr));
        connectButton->setText(QApplication::translate("MainWindow", "Connect", nullptr));
        disconnectButton->setText(QApplication::translate("MainWindow", "Disconnect", nullptr));
        calibrationGroup->setTitle(QApplication::translate("MainWindow", "Calibration", nullptr));
        runAxisCalibrationButton->setText(QApplication::translate("MainWindow", "Run Axis Calibration", nullptr));
        setHomePositionsButton->setText(QApplication::translate("MainWindow", "Set Home Positions", nullptr));
        setUnitParametersButton->setText(QApplication::translate("MainWindow", "Set Unit Parameters", nullptr));
        eraseGimbalFlashButton->setText(QApplication::translate("MainWindow", "Erase Gimbal Flash", nullptr));
        factoryTestsButton->setText(QApplication::translate("MainWindow", "Run Axis Limits Test", nullptr));
        firmwareGroup->setTitle(QApplication::translate("MainWindow", "Firmware", nullptr));
        firmwareImage_label->setText(QApplication::translate("MainWindow", "Firmware Image:", nullptr));
        firmwareImageBrowseButton->setText(QApplication::translate("MainWindow", "...", nullptr));
        loadFirmwareButton->setText(QApplication::translate("MainWindow", "Load Firmware", nullptr));
        serialNumber_label->setText(QApplication::translate("MainWindow", "Serial Number:", nullptr));
        assemblyDateTime->setText(QApplication::translate("MainWindow", "None", nullptr));
        serialNumber->setText(QApplication::translate("MainWindow", "None", nullptr));
        firmwareLoaded->setText(QApplication::translate("MainWindow", "None", nullptr));
        connectionStatus_label->setText(QApplication::translate("MainWindow", "Connection Status:", nullptr));
        firmwareLoaded_label->setText(QApplication::translate("MainWindow", "Firmware Loaded:", nullptr));
        connectionStatus->setText(QApplication::translate("MainWindow", "Disconnected", nullptr));
        assemblyDateTime_label->setText(QApplication::translate("MainWindow", "Assembly Date/Time:", nullptr));
        version_label->setText(QApplication::translate("MainWindow", "Version Goes Here", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
