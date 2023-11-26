#ifndef AXIS_RANGE_TEST_DIALOG_H
#define AXIS_RANGE_TEST_DIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QLabel>

namespace Ui {
class AxisRangeTestDialog;
}

class AxisRangeTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AxisRangeTestDialog(QWidget *parent = 0);
    ~AxisRangeTestDialog();

public slots:
    void receiveTestProgress(int, int test_section, int test_progress, int test_status);
    void reject();

signals:
    void requestTestRetry();

private:
    Ui::AxisRangeTestDialog *ui;

    QPixmap m_inProgressIcon;
    QPixmap m_successIcon;
    QPixmap m_failureIcon;

    void setStepStatus(QLabel *statusLabel, int status);
    void resetTestUI();

private slots:
    void on_retryButton_clicked();
    void on_okButton_clicked();
};

#endif // AXIS_RANGE_TEST_DIALOG_H
