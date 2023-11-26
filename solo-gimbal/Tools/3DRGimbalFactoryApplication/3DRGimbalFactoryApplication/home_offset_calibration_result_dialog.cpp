#include "home_offset_calibration_result_dialog.h"
#include "ui_home_offset_calibration_result_dialog.h"

HomeOffsetCalibrationResultDialog::HomeOffsetCalibrationResultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HomeOffsetCalibrationResultDialog)
{
    ui->setupUi(this);

    // Disable all of the title bar buttons (so the user can't close the dialog from the title bar)
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
}

HomeOffsetCalibrationResultDialog::~HomeOffsetCalibrationResultDialog()
{
    delete ui;
}


void HomeOffsetCalibrationResultDialog::receiveNewHomeOffsets(int yawOffset, int pitchOffset, int rollOffset)
{
    ui->calibrationStatus_label->setText("Calibrating...Done");
    ui->yawHomeOffset->setText(QString::number(yawOffset));
    ui->pitchHomeOffset->setText(QString::number(pitchOffset));
    ui->rollHomeOffset->setText(QString::number(rollOffset));

    ui->buttonBox->setEnabled(true);
}

void HomeOffsetCalibrationResultDialog::reject()
{
    // Overriding this to prevent default behavior of escape key causing dialog to exit
}
