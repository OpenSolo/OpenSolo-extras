#ifndef LOAD_FIRMWARE_DIALOG_H
#define LOAD_FIRMWARE_DIALOG_H

#include <QDialog>

namespace Ui {
class LoadFirmwareDialog;
}

class LoadFirmwareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadFirmwareDialog(QWidget *parent = 0);
    ~LoadFirmwareDialog();

public slots:
    void updateFirmwareProgress(double progress);
    void firmwareUpdateError(QString errorMsg);
    void reject();

signals:

private:
    Ui::LoadFirmwareDialog *ui;

    int m_progressUpdatesReceived;
};

#endif // LOAD_FIRMWARE_DIALOG_H
