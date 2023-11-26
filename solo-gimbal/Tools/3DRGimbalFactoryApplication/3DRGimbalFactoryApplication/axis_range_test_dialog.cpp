#include "axis_range_test_dialog.h"
#include "ui_axis_range_test_dialog.h"

#include "serial_interface_thread.h"

AxisRangeTestDialog::AxisRangeTestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AxisRangeTestDialog)
{
    // Disable all of the title bar buttons (so the user can't close the dialog from the title bar)
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    ui->setupUi(this);
    m_successIcon.load(":/icons/32x32/Images/Green_Tick_32x32.png");
    m_failureIcon.load(":/icons/32x32/Images/Red_X_32x32.png");
    m_inProgressIcon.load(":/icons/32x32/Images/Ellipsis_32x32.png");
}

AxisRangeTestDialog::~AxisRangeTestDialog()
{
    delete ui;
}

void AxisRangeTestDialog::receiveTestProgress(int, int test_section, int test_progress, int test_status)
{
    switch (test_section) {
        case AXIS_RANGE_TEST_SECTION_EL_CHECK_NEG:
            ui->pitchCheckNegativeProgress->setValue(test_progress);
            setStepStatus(ui->pitchCheckNegativeStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_EL_CHECK_POS:
            ui->pitchCheckPositiveProgress->setValue(test_progress);
            setStepStatus(ui->pitchCheckPositiveStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_EL_RETURN_HOME:
            ui->pitchReturnHomeProgress->setValue(test_progress);
            setStepStatus(ui->pitchReturnHomeStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_RL_CHECK_NEG:
            ui->rollCheckNegativeProgress->setValue(test_progress);
            setStepStatus(ui->rollCheckNegativeStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_RL_CHECK_POS:
            ui->rollCheckPositiveProgress->setValue(test_progress);
            setStepStatus(ui->rollCheckPositiveStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_RL_RETURN_HOME:
            ui->rollReturnHomeProgress->setValue(test_progress);
            setStepStatus(ui->rollReturnHomeStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_AZ_CHECK_NEG:
            ui->yawCheckNegativeProgress->setValue(test_progress);
            setStepStatus(ui->yawCheckNegativeStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_AZ_CHECK_POS:
            ui->yawCheckPositiveProgress->setValue(test_progress);
            setStepStatus(ui->yawCheckPositiveStatus, test_status);
            break;

        case AXIS_RANGE_TEST_SECTION_AZ_RETURN_HOME:
            ui->yawReturnHomeProgress->setValue(test_progress);
            setStepStatus(ui->yawReturnHomeStatus, test_status);

            // This is the last step in the sequence, so if it succeeded then the entire test succeeded
            if (test_status == AXIS_RANGE_TEST_STATUS_SUCCEEDED) {
                ui->testStatus->setText("Test Completed Successfully.  Press OK to return to the main screen");
                ui->okButton->setEnabled(true);
            }
            break;
    }

    if ((test_status == AXIS_RANGE_TEST_STATUS_FAILED_NEGATIVE) ||
            (test_status == AXIS_RANGE_TEST_STATUS_FAILED_POSITIVE) ||
            (test_status == AXIS_RANGE_TEST_STATUS_FAILED_HOME)) {
        ui->testStatus->setText("Test failed.  Press Retry to re-run the test, or OK to return to the main screen");
        ui->retryButton->setEnabled(true);
        ui->okButton->setEnabled(true);
    }
}

void AxisRangeTestDialog::setStepStatus(QLabel* statusLabel, int status)
{
    switch (status) {
        case AXIS_RANGE_TEST_STATUS_SUCCEEDED:
            statusLabel->setPixmap(m_successIcon);
            break;

        case AXIS_RANGE_TEST_STATUS_FAILED_NEGATIVE:
        case AXIS_RANGE_TEST_STATUS_FAILED_POSITIVE:
        case AXIS_RANGE_TEST_STATUS_FAILED_HOME:
            statusLabel->setPixmap(m_failureIcon);
            break;
    }
}

void AxisRangeTestDialog::on_okButton_clicked()
{
    accept();
}

void AxisRangeTestDialog::on_retryButton_clicked()
{
    resetTestUI();
    emit requestTestRetry();
}

void AxisRangeTestDialog::resetTestUI()
{
    // Reset the status bars
    ui->pitchCheckNegativeProgress->setValue(0);
    ui->pitchCheckPositiveProgress->setValue(0);
    ui->pitchReturnHomeProgress->setValue(0);
    ui->rollCheckNegativeProgress->setValue(0);
    ui->rollCheckPositiveProgress->setValue(0);
    ui->rollReturnHomeProgress->setValue(0);
    ui->yawCheckNegativeProgress->setValue(0);
    ui->yawCheckPositiveProgress->setValue(0);
    ui->yawReturnHomeProgress->setValue(0);

    // Reset the status indicators
    ui->pitchCheckNegativeStatus->setPixmap(m_inProgressIcon);
    ui->pitchCheckPositiveStatus->setPixmap(m_inProgressIcon);
    ui->pitchReturnHomeStatus->setPixmap(m_inProgressIcon);
    ui->rollCheckNegativeStatus->setPixmap(m_inProgressIcon);
    ui->rollCheckPositiveStatus->setPixmap(m_inProgressIcon);
    ui->rollReturnHomeStatus->setPixmap(m_inProgressIcon);
    ui->yawCheckNegativeStatus->setPixmap(m_inProgressIcon);
    ui->yawCheckPositiveStatus->setPixmap(m_inProgressIcon);
    ui->yawReturnHomeStatus->setPixmap(m_inProgressIcon);

    // Disable the ok and retry buttons
    ui->okButton->setEnabled(false);
    ui->retryButton->setEnabled(false);

    // Reset the status message
    ui->testStatus->setText("Test Running...");
}

void AxisRangeTestDialog::reject()
{
    // Overriding this to prevent default behavior of escape key causing dialog to exit
}

