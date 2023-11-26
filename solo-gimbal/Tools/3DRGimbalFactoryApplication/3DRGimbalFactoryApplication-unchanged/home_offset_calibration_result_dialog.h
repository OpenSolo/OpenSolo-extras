#ifndef HOME_OFFSET_CALIBRATION_RESULT_DIALOG_H
#define HOME_OFFSET_CALIBRATION_RESULT_DIALOG_H

#include <QDialog>

namespace Ui {
class HomeOffsetCalibrationResultDialog;
}

class HomeOffsetCalibrationResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HomeOffsetCalibrationResultDialog(QWidget *parent = 0);
    ~HomeOffsetCalibrationResultDialog();

public slots:
    void receiveNewHomeOffsets(int yawOffset, int pitchOffset, int rollOffset);
    void reject();

private:
    Ui::HomeOffsetCalibrationResultDialog *ui;
};

#endif // HOME_OFFSET_CALIBRATION_RESULT_DIALOG_H
