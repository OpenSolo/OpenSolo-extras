#ifndef CALIBRATE_AXES_DIALOG_H
#define CALIBRATE_AXES_DIALOG_H

#include <QDialog>

#include "MAVLink/ardupilotmega/mavlink.h"

namespace Ui {
class CalibrateAxesDialog;
}

class CalibrateAxesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrateAxesDialog(QWidget *parent = 0);
    ~CalibrateAxesDialog();

public slots:
    void axisCalibrationStarted(int axis);
    void axisCalibrationFinished(int axis, bool successful);
    void reject();

signals:
    void retryAxesCalibration();

private:
    Ui::CalibrateAxesDialog *ui;

    void resetForCalibrationRetry();

private slots:
    void on_cancelButton_clicked();
};

#endif // CALIBRATE_AXES_DIALOG_H
