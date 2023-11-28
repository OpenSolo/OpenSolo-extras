#include "load_firmware_dialog.h"
#include "ui_load_firmware_dialog.h"

#include <QMessageBox>

LoadFirmwareDialog::LoadFirmwareDialog(QWidget *parent) :
    QDialog(parent),
    m_progressUpdatesReceived(0),
    ui(new Ui::LoadFirmwareDialog)
{
    ui->setupUi(this);

    // Disable all of the title bar buttons (so the user can't close the dialog from the title bar)
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
}

LoadFirmwareDialog::~LoadFirmwareDialog()
{
    delete ui;
}

void LoadFirmwareDialog::updateFirmwareProgress(double progress)
{
    // The gimbal erases flash after requesting the first block,
    // so we have to wait for the 2nd progress update to know the flash
    // erase is complete
    if (++m_progressUpdatesReceived == 2) {
        ui->erasingFlash_label->setText("Erasing Flash...done");
    }

    ui->firmwareProgress->setValue(static_cast<int>(progress * 100.0));

    // If we're done loading the firmware, inform the main window and close ourselves
    if (progress == 1.0) {
        this->accept();
    }
}

void LoadFirmwareDialog::firmwareUpdateError(QString errorMsg)
{
    QMessageBox msg;
    msg.setText("Firmware Load Error");
    msg.setInformativeText("The following error was encountered while trying to load firmware: " + errorMsg + ".  Please correct the issue and try again");
    msg.setIcon(QMessageBox::Warning);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
    QDialog::reject();
}

void LoadFirmwareDialog::reject()
{
    // Overriding this to prevent default behavior of escape key causing dialog to exit
}
