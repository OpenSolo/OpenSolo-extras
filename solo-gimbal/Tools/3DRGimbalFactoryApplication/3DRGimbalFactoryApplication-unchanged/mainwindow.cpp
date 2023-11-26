#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "load_firmware_dialog.h"
#include "calibrate_axes_dialog.h"
#include "home_offset_calibration_result_dialog.h"
#include "enter_factory_parameters_dialog.h"
#include "axis_range_test_dialog.h"
#include "version.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    foreach (QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        ui->comPort->addItem(port.portName());
    }

    ui->baudRate->addItem("9600", 9600);
    ui->baudRate->addItem("19200", 19200);
    ui->baudRate->addItem("57600", 57600);
    ui->baudRate->addItem("115200", 115200);
    ui->baudRate->addItem("230400", 230400);

    connect(&m_connectionTimeoutTimer, SIGNAL(timeout()), this, SLOT(connectionTimeout()));

    // Populate the version string
    ui->version_label->setText(QString(GitVersionString) + QString("-") + QString(GitBranch));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    // Initialize and start the serial interface thread
    m_serialThreadObj = new SerialInterfaceThread(ui->comPort->currentText(), ui->baudRate->currentData().toInt());
    m_serialThreadObj->moveToThread(&m_serialThread);
    connect(&m_serialThread, SIGNAL(started()), m_serialThreadObj, SLOT(run()));
    connect(&m_serialThread, SIGNAL(finished()), m_serialThreadObj, SLOT(deleteLater()));
    connect(m_serialThreadObj, SIGNAL(receivedHeartbeat()), this, SLOT(receivedGimbalHeartbeat()));
    connect(m_serialThreadObj, SIGNAL(receivedDataTransmissionHandshake()), this, SLOT(receivedGimbalDataTransmissionHandshake()));
    connect(this, SIGNAL(closeSerialInterface()), m_serialThreadObj, SLOT(requestStop()));
    connect(this, SIGNAL(requestFirmwareDownload(QString)), m_serialThreadObj, SLOT(requestLoadFirmware(QString)));
    //connect(m_serialThreadObj, SIGNAL(firmwareLoadError(QString)), this, SLOT(debugFirmwareLoadError(QString)));
    //connect(m_serialThreadObj, SIGNAL(firmwareLoadProgress(double)), this, SLOT(debugFirmwareLoadProgress(double)));
    connect(m_serialThreadObj, SIGNAL(firmwareLoadError(QString)), this, SIGNAL(firmwareLoadError(QString)));
    connect(m_serialThreadObj, SIGNAL(firmwareLoadProgress(double)), this, SIGNAL(firmwareLoadProgress(double)));
    connect(m_serialThreadObj, SIGNAL(axisCalibrationStarted(int)), this, SIGNAL(axisCalibrationStarted(int)));
    connect(m_serialThreadObj, SIGNAL(axisCalibrationFinished(int,bool)), this, SIGNAL(axisCalibrationFinished(int,bool)));
    connect(this, SIGNAL(requestFirmwareVersion()), m_serialThreadObj, SLOT(requestFirmwareVersion()));
    connect(m_serialThreadObj, SIGNAL(sendFirmwareVersion(QString)), this, SLOT(receiveFirmwareVersion(QString)));
    connect(this, SIGNAL(requestCalibrateAxes()), m_serialThreadObj, SLOT(requestCalibrateAxes()));
    connect(m_serialThreadObj, SIGNAL(serialPortOpenError(QString)), this, SLOT(receiveSerialPortOpenError(QString)));
    connect(this, SIGNAL(retryAxesCalibration()), m_serialThreadObj, SLOT(retryAxesCalibration()));
    connect(this, SIGNAL(requestResetGimbal()), m_serialThreadObj, SLOT(requestResetGimbal()));
    connect(this, SIGNAL(requestHomeOffsetCalibration()), m_serialThreadObj, SLOT(requestCalibrateHomeOffsets()));
    connect(m_serialThreadObj, SIGNAL(homeOffsetCalibrationFinished(bool)), this, SLOT(receiveHomeOffsetCalibrationStatus(bool)));
    connect(m_serialThreadObj, SIGNAL(sendNewHomeOffsets(int,int,int)), this, SIGNAL(sendNewHomeOffsets(int,int,int)));
    connect(this, SIGNAL(requestFactoryParameters()), m_serialThreadObj, SLOT(requestFactoryParameters()));
    connect(m_serialThreadObj, SIGNAL(sendFactoryParameters(QString,QString)), this, SLOT(receiveFactoryParameters(QString,QString)));
    connect(this, SIGNAL(setGimbalFactoryParameters(unsigned short,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,ulong,ulong,ulong)), m_serialThreadObj, SLOT(setGimbalFactoryParameters(unsigned short,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,ulong,ulong,ulong)));
    connect(m_serialThreadObj, SIGNAL(factoryParametersLoaded()), this, SIGNAL(factoryParametersLoaded()));
    connect(this, SIGNAL(requestEraseGimbalFlash()), m_serialThreadObj, SLOT(requestGimbalEraseFlash()));
    connect(this, SIGNAL(requestStartFactoryTests()), m_serialThreadObj, SLOT(requestGimbalFactoryTests()));
    connect(m_serialThreadObj, SIGNAL(factoryTestsStatus(int,int,int,int)), this, SIGNAL(factoryTestsStatus(int,int,int,int)));
    m_serialThread.start();

    // Disable the connect button, enable the disconnect button
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);

    // Change connection status to connecting
    ui->connectionStatus->setText("Connecting...");

    // Start the connection timeout timer
    // (this will timeout if we don't get either a heartbeat or a data transmission handshake from the gimbal)
    m_connectionTimeoutTimer.setSingleShot(true);
    m_connectionTimeoutTimer.start(5000);
}

void MainWindow::receiveSerialPortOpenError(QString errorMsg)
{
    // Disable the disconnect button, enable the connect button
    ui->disconnectButton->setEnabled(false);
    ui->connectButton->setEnabled(true);

    // Change connection status to disconnected
    ui->connectionStatus->setText("Disconnected");

    // If there's a pending connection timeout, cancel it
    m_connectionTimeoutTimer.stop();

    QMessageBox msg;
    msg.setText("Error Opening Serial Port");
    msg.setInformativeText("Encountered error \"" + errorMsg + "\" while trying to open the serial port.  Please check your settings and try again");
    msg.setIcon(QMessageBox::Warning);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
}

void MainWindow::connectionTimeout()
{
    QMessageBox msg;
    msg.setText("Gimbal Connection Timeout");
    msg.setInformativeText("Didn't receive a message from the gimbal in 5 seconds.  Please check gimbal power and connection and try again");
    msg.setIcon(QMessageBox::Warning);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();

    // Ask the serial interface thread to stop
    emit closeSerialInterface();

    // Disable the disconnect button, enable the connect button
    ui->disconnectButton->setEnabled(false);
    ui->connectButton->setEnabled(true);

    // Change connection status to disconnected
    ui->connectionStatus->setText("Disconnected");
}

void MainWindow::receivedGimbalHeartbeat()
{
    // If we receive a gimbal heartbeat when we connect, it means there is firmware loaded on the gimbal

    // Cancel the pending connection timeout
    m_connectionTimeoutTimer.stop();

    // Set connection status to connected
    ui->connectionStatus->setText("Connected");

    // Enable the load firmware and calibration controls
    ui->loadFirmwareButton->setEnabled(true);
    ui->firmwareImage->setEnabled(true);
    ui->firmwareImageBrowseButton->setEnabled(true);
    ui->runAxisCalibrationButton->setEnabled(true);
    ui->setHomePositionsButton->setEnabled(true);
    ui->setUnitParametersButton->setEnabled(true);
    ui->eraseGimbalFlashButton->setEnabled(true);
    ui->factoryTestsButton->setEnabled(true);

    // Request gimbal firmware version
    emit requestFirmwareVersion();

    // Request the factory parameters (assembly date, serial number)
    emit requestFactoryParameters();
}

void MainWindow::receivedGimbalDataTransmissionHandshake()
{
    // If we receive a data transmission handshake when we connect, it means there is no firmware loaded on the gimbal

    // Cancel the pending connection timeout
    m_connectionTimeoutTimer.stop();

    // Set connection status to connected
    ui->connectionStatus->setText("Connected");

    // Enable only the load firmware controls
    ui->loadFirmwareButton->setEnabled(true);
    ui->firmwareImage->setEnabled(true);
    ui->firmwareImageBrowseButton->setEnabled(true);

    // Show the user a message indicating that they need to load firmware
    QMessageBox msg;
    msg.setText("No firmware loaded on gimbal");
    msg.setInformativeText("Before calibrating the gimbal, firmware must be loaded on it");
    msg.setIcon(QMessageBox::Information);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
}

void MainWindow::on_disconnectButton_clicked()
{
    // Ask the serial interface thread to stop
    emit closeSerialInterface();

    // Disable the disconnect button, enable the connect button
    ui->disconnectButton->setEnabled(false);
    ui->connectButton->setEnabled(true);

    // Disable the load firmware and calibration controls
    ui->loadFirmwareButton->setEnabled(false);
    ui->firmwareImage->setEnabled(false);
    ui->firmwareImageBrowseButton->setEnabled(false);
    ui->runAxisCalibrationButton->setEnabled(false);
    ui->setHomePositionsButton->setEnabled(false);
    ui->setUnitParametersButton->setEnabled(false);
    ui->eraseGimbalFlashButton->setEnabled(false);
    ui->factoryTestsButton->setEnabled(false);

    // Change connection status to disconnected
    ui->connectionStatus->setText("Disconnected");

    // Clear out firmware version
    ui->firmwareLoaded->setText("None");

    // Clear out assembly date/time and serial number
    ui->assemblyDateTime->setText("None");
    ui->serialNumber->setText("None");

    // If there's a pending connection timeout, cancel it
    m_connectionTimeoutTimer.stop();
}

void MainWindow::on_firmwareImageBrowseButton_clicked()
{
    QString firmwareFileName = QFileDialog::getOpenFileName(this, "Select Firmware Image", QString(), "Firmware Image Files (*.hex);;All Files (*.*)");
    ui->firmwareImage->setText(firmwareFileName);
}

void MainWindow::on_loadFirmwareButton_clicked()
{
    // Make sure the file path in the firmware image textbox exists
    if (!QFile(ui->firmwareImage->text()).exists()) {
        QMessageBox msg;
        msg.setText("Invalid Firmware File");
        msg.setInformativeText("The file \"" + ui->firmwareImage->text() + "\" does not appear to exist.  Please pick a valid file and try again");
        msg.setIcon(QMessageBox::Warning);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setDefaultButton(QMessageBox::Ok);
        msg.exec();
    } else {
        LoadFirmwareDialog firmwareDialog;
        connect(this, SIGNAL(firmwareLoadError(QString)), &firmwareDialog, SLOT(firmwareUpdateError(QString)));
        connect(this, SIGNAL(firmwareLoadProgress(double)), &firmwareDialog, SLOT(updateFirmwareProgress(double)));
        emit requestFirmwareDownload(ui->firmwareImage->text());
        if (firmwareDialog.exec() == QDialog::Accepted) {
            // Once the firmware load has completed, bring up the calibrate axes dialog
            // (in case the newly loaded gimbal needs to run the axis calibration)
            CalibrateAxesDialog calibrateAxesDialog;
            connect(this, SIGNAL(axisCalibrationStarted(int)), &calibrateAxesDialog, SLOT(axisCalibrationStarted(int)));
            connect(this, SIGNAL(axisCalibrationFinished(int,bool)), &calibrateAxesDialog, SLOT(axisCalibrationFinished(int,bool)));
            connect(&calibrateAxesDialog, SIGNAL(retryAxesCalibration()), this, SIGNAL(retryAxesCalibration()));
            if (calibrateAxesDialog.exec() == QDialog::Accepted) {
                // Once the axis calibration is complete, but only if it completed successfully,
                // bring up the factory parameters dialog to set assembly date/time and serial number
                EnterFactoryParametersDialog factoryParmsDialog;
                connect(&factoryParmsDialog, SIGNAL(setGimbalFactoryParameters(unsigned short,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,ulong,ulong,ulong)), this, SIGNAL(setGimbalFactoryParameters(unsigned short,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,ulong,ulong,ulong)));
                connect(this, SIGNAL(factoryParametersLoaded()), &factoryParmsDialog, SLOT(factoryParametersLoaded()));
                factoryParmsDialog.exec();

                // Once the factory parameters dialog has been disposed of, enable the rest of the UI, since there's now firmware loaded
                ui->runAxisCalibrationButton->setEnabled(true);
                ui->setHomePositionsButton->setEnabled(true);
                ui->setUnitParametersButton->setEnabled(true);
                ui->eraseGimbalFlashButton->setEnabled(true);
                ui->factoryTestsButton->setEnabled(true);

                // Reload the firmware version, since it has probably changed after the update
                emit requestFirmwareVersion();
                // Also reload the factory parameters, since the user has entered new ones
                emit requestFactoryParameters();
            }
        }
    }
}

void MainWindow::receiveFirmwareVersion(QString versionString)
{
    ui->firmwareLoaded->setText(versionString);
}

void MainWindow::on_runAxisCalibrationButton_clicked()
{
    // First, make sure the user really wants to run the axis calibration
    QMessageBox msg;
    msg.setText("Confirm Axis Calibration");
    msg.setInformativeText("The axis calibration takes ~5 minutes, and can't be canceled once started.  Are you sure you want to proceed?");
    msg.setIcon(QMessageBox::Information);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    if (msg.exec() == QMessageBox::Yes) {
        emit requestCalibrateAxes();
        CalibrateAxesDialog calibrateAxesDialog;
        connect(this, SIGNAL(axisCalibrationStarted(int)), &calibrateAxesDialog, SLOT(axisCalibrationStarted(int)));
        connect(this, SIGNAL(axisCalibrationFinished(int,bool)), &calibrateAxesDialog, SLOT(axisCalibrationFinished(int,bool)));
        connect(&calibrateAxesDialog, SIGNAL(retryAxesCalibration()), this, SIGNAL(retryAxesCalibration()));
        if (calibrateAxesDialog.exec() == QDialog::Rejected) {
            // If the user canceled the calibration, reset the gimbal
            emit requestResetGimbal();
        }
    }
}

void MainWindow::debugFirmwareLoadError(QString errorMsg)
{
    qDebug("Firmware Load Error: %s", errorMsg);
}

void MainWindow::debugFirmwareLoadProgress(double progress)
{
    qDebug("Firmware Load Progress: %f%%", progress);
}

void MainWindow::on_resetGimbalButton_clicked()
{
    emit requestResetGimbal();
    CalibrateAxesDialog calibrateAxesDialog;
    connect(this, SIGNAL(axisCalibrationStarted(int)), &calibrateAxesDialog, SLOT(axisCalibrationStarted(int)));
    connect(this, SIGNAL(axisCalibrationFinished(int,bool)), &calibrateAxesDialog, SLOT(axisCalibrationFinished(int,bool)));
    connect(&calibrateAxesDialog, SIGNAL(retryAxesCalibration()), this, SIGNAL(retryAxesCalibration()));
    calibrateAxesDialog.exec();
}

void MainWindow::on_setHomePositionsButton_clicked()
{
    QMessageBox msg;
    msg.setText("Confirm Home Offset Calibration");
    msg.setInformativeText("This will set the gimbal's current position to its new \"home\" position.  Make sure that the gimbal is immobilized in an appropriate fixture before proceding.  Do you want to proceed?");
    msg.setIcon(QMessageBox::Information);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    if (msg.exec() == QMessageBox::Yes) {
        HomeOffsetCalibrationResultDialog dialog;
        connect(this, SIGNAL(sendNewHomeOffsets(int,int,int)), &dialog, SLOT(receiveNewHomeOffsets(int,int,int)));
        emit requestHomeOffsetCalibration();
        dialog.exec();
    }
}

void MainWindow::receiveHomeOffsetCalibrationStatus(bool successful)
{
    qDebug("Home offset calibration finished with result: %d", successful);
}

void MainWindow::receiveFactoryParameters(QString assemblyDateTime, QString serialNumber)
{
    ui->assemblyDateTime->setText(assemblyDateTime);
    ui->serialNumber->setText(serialNumber);
}

void MainWindow::on_setUnitParametersButton_clicked()
{
    EnterFactoryParametersDialog dialog;
    connect(&dialog, SIGNAL(setGimbalFactoryParameters(unsigned short,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,ulong,ulong,ulong)), this, SIGNAL(setGimbalFactoryParameters(unsigned short,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,ulong,ulong,ulong)));
    connect(this, SIGNAL(factoryParametersLoaded()), &dialog, SLOT(factoryParametersLoaded()));
    dialog.exec();
    // After the user sets the new factory parameters, request the new parameters from the gimbal to update the UI
    emit requestFactoryParameters();
}

void MainWindow::on_eraseGimbalFlashButton_clicked()
{
    QMessageBox msg;
    msg.setText("Confirm flash erase");
    msg.setInformativeText("WARNING: This will erase gimbal flash, including firmware and calibration values.  This will render the gimbal inoperable until new firmware is loaded, and will also require the gimbal to re-calibrate itself after new firmware is loaded.  Are you sure you want to proceed?");
    msg.setIcon(QMessageBox::Warning);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    if (msg.exec() == QMessageBox::Yes) {
        emit requestEraseGimbalFlash();

        // Disable the load firmware and calibration controls
        ui->loadFirmwareButton->setEnabled(false);
        ui->firmwareImage->setEnabled(false);
        ui->firmwareImageBrowseButton->setEnabled(false);
        ui->runAxisCalibrationButton->setEnabled(false);
        ui->setHomePositionsButton->setEnabled(false);
        ui->setUnitParametersButton->setEnabled(false);
        ui->eraseGimbalFlashButton->setEnabled(false);
        ui->factoryTestsButton->setEnabled(false);

        // Change connection status to disconnected
        ui->connectionStatus->setText("Erasing flash...");

        // Clear out firmware version
        ui->firmwareLoaded->setText("None");

        // Clear out assembly date/time and serial number
        ui->assemblyDateTime->setText("None");
        ui->serialNumber->setText("None");
    }
}

void MainWindow::on_factoryTestsButton_clicked()
{
    AxisRangeTestDialog dialog;
    connect(this, SIGNAL(factoryTestsStatus(int,int,int,int)), &dialog, SLOT(receiveTestProgress(int,int,int,int)));
    connect(&dialog, SIGNAL(requestTestRetry()), this, SIGNAL(requestStartFactoryTests()));
    emit requestStartFactoryTests();
    dialog.exec();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    // Ask the serial interface thread to stop
    emit closeSerialInterface();
}
