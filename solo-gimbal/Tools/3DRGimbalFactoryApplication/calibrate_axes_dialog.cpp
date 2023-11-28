#include "calibrate_axes_dialog.h"
#include "ui_calibrate_axes_dialog.h"

#include <QMessageBox>

CalibrateAxesDialog::CalibrateAxesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrateAxesDialog)
{
    // Disable all of the title bar buttons (so the user can't close the dialog from the title bar)
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    ui->setupUi(this);

    ui->pitchProgress->setMaximum(1);
    ui->pitchProgress->setValue(0);
    ui->rollProgress->setMaximum(1);
    ui->rollProgress->setValue(0);
    ui->yawProgress->setMaximum(1);
    ui->yawProgress->setValue(0);
}

CalibrateAxesDialog::~CalibrateAxesDialog()
{
    delete ui;
}

void CalibrateAxesDialog::axisCalibrationStarted(int axis)
{
    switch (axis) {
        case GIMBAL_AXIS_PITCH:
            ui->pitchProgress->setMaximum(0);
            ui->pitchProgress->setValue(0);
            ui->pitchStatus->setText("Calibrating");
            break;

        case GIMBAL_AXIS_ROLL:
            ui->rollProgress->setMaximum(0);
            ui->rollProgress->setValue(0);
            ui->rollStatus->setText("Calibrating");
            break;

        case GIMBAL_AXIS_YAW:
            ui->yawProgress->setMaximum(0);
            ui->yawProgress->setValue(0);
            ui->yawStatus->setText("Calibrating");
            break;
    }
}

void CalibrateAxesDialog::axisCalibrationFinished(int axis, bool successful)
{
    // Pre-create the message dialogs
    QMessageBox successMsg;
    successMsg.setText("Calibration Successful");
    successMsg.setInformativeText("The axis calibration was successful!  Press OK to continue");
    successMsg.setIcon(QMessageBox::Information);
    successMsg.setStandardButtons(QMessageBox::Ok);
    successMsg.setDefaultButton(QMessageBox::Ok);

    QMessageBox failureMsg;
    failureMsg.setText("Calibration Failed");
    failureMsg.setInformativeText("The axis calibration failed.  Press RETRY to re-attempt the axis calibration, or CANCEL to return to the main screen");
    failureMsg.setIcon(QMessageBox::Warning);
    failureMsg.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
    failureMsg.setDefaultButton(QMessageBox::Retry);

    switch (axis) {
        case GIMBAL_AXIS_PITCH:
            ui->pitchProgress->setMaximum(1);
            if (successful) {
                ui->pitchProgress->setValue(1);
                ui->pitchStatus->setText("Completed");
            } else {
                ui->pitchProgress->setValue(0);
                ui->pitchStatus->setText("Failed");
                if (failureMsg.exec() == QMessageBox::Retry) {
                    resetForCalibrationRetry();
                } else {
                    QDialog::reject();
                }
            }
            break;

        case GIMBAL_AXIS_ROLL:
            ui->rollProgress->setMaximum(1);
            if (successful) {
                ui->rollProgress->setValue(1);
                ui->rollStatus->setText("Completed");
            } else {
                ui->rollProgress->setValue(0);
                ui->rollStatus->setText("Failed");
                if (failureMsg.exec() == QMessageBox::Retry) {
                    resetForCalibrationRetry();
                } else {
                    QDialog::reject();
                }
            }
            break;

        case GIMBAL_AXIS_YAW:
            ui->yawProgress->setMaximum(1);
            if (successful) {
                ui->yawProgress->setValue(1);
                ui->yawStatus->setText("Completed");
                successMsg.exec();
                this->accept();
            } else {
                ui->yawProgress->setValue(0);
                ui->yawStatus->setText("Failed");
                if (failureMsg.exec() == QMessageBox::Retry) {
                    resetForCalibrationRetry();
                } else {
                    QDialog::reject();
                }
            }
            break;
    }
}

void CalibrateAxesDialog::resetForCalibrationRetry()
{
    ui->pitchProgress->setMaximum(1);
    ui->pitchProgress->setValue(0);
    ui->pitchStatus->setText("Waiting for calibration");
    ui->rollProgress->setMaximum(1);
    ui->rollProgress->setValue(0);
    ui->rollStatus->setText("Waiting for calibration");
    ui->yawProgress->setMaximum(1);
    ui->yawProgress->setValue(0);
    ui->yawStatus->setText("Waiting for calibration");
    emit retryAxesCalibration();
}

void CalibrateAxesDialog::reject()
{
    // Overriding this to prevent default behavior of escape key causing dialog to exit
}

void CalibrateAxesDialog::on_cancelButton_clicked()
{
    // Close ourselves with the reject return code
    QDialog::reject();
}
