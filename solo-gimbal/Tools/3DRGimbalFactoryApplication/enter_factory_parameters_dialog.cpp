#include "enter_factory_parameters_dialog.h"
#include "ui_enter_factory_parameters_dialog.h"
#include "datevalidator.h"
#include "timevalidator.h"

#include <QDate>
#include <QTime>
#include <QRegExp>
#include <QRegExpValidator>
#include <QMessageBox>

EnterFactoryParametersDialog::EnterFactoryParametersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EnterFactoryParametersDialog),
    m_partCode1('G'),
    m_partCode2('B'),
    m_design(1),
    m_option('A')
{
    ui->setupUi(this);

    // Disable all of the title bar buttons (so the user can't close the dialog from the title bar)
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Hide the setting parameters indicator
    ui->settingFactoryParameters_label->setVisible(false);

    // Populate the assembly date and time field with the current date
    // and time by default
    QDate currentDate = QDate::currentDate();
    ui->assemblyDate->setText(currentDate.toString("MM/dd/yyyy"));
    QTime currentTime = QTime::currentTime();
    ui->assemblyTime->setText(currentTime.toString("hh/mm/ss"));

    // Set up range validators for date and time entry fields
    ui->assemblyDate->setValidator(new DateValidator()); // For this application, date validator doesn't allow pre-2010 years
    ui->assemblyTime->setValidator(new TimeValidator());

    // Set up contents of language/country combobox
    ui->languageCountry->addItem("English/US", 1);

    // Generate the initial value of the generated serial number
    updateGeneratedSerialNumber();
}

EnterFactoryParametersDialog::~EnterFactoryParametersDialog()
{
    delete ui;
}

void EnterFactoryParametersDialog::accept()
{
    //TODO: Do any additional validation we need to do here
    if (ui->serialNumber->text().length() != 5) {
        QMessageBox msg;
        msg.setText("Invalid Serial Number");
        msg.setInformativeText("The serial number must contain 5 numeric digits.  Please enter a valid serial number and try again");
        msg.setIcon(QMessageBox::Warning);
        msg.setStandardButtons(QMessageBox::Ok);
        msg.setDefaultButton(QMessageBox::Ok);
        msg.exec();
    } else {
        QDate assemblyDate = QDate::fromString(ui->assemblyDate->text(), "MM/dd/yyyy");
        QTime assemblyTime = QTime::fromString(ui->assemblyTime->text(), "hh:mm:ss");

        // Assemble the different serial numbers
        uint32_t ser_1 = 0;
        uint32_t ser_2 = 0;
        uint32_t ser_3 = 0;
        ser_1 |= (((uint32_t)m_partCode1 << 24) & 0xFF000000);
        ser_1 |= (((uint32_t)m_partCode2 << 16) & 0x00FF0000);
        ser_1 |= (((uint32_t)m_design << 8) & 0x0000FF00);
        ser_1 |= ((uint32_t)ui->languageCountry->currentData().toInt() & 0x000000FF);
        ser_2 |= (((uint32_t)m_option << 24) & 0xFF000000);
        ser_2 |= (((uint32_t)(assemblyDate.year() - 2010) << 8) & 0x00FFFF00);
        ser_2 |= ((uint32_t)assemblyDate.month() & 0x000000FF);
        ser_3 = ui->serialNumber->text().toInt();

        emit setGimbalFactoryParameters(assemblyDate.year(),
                                        assemblyDate.month(),
                                        assemblyDate.day(),
                                        assemblyTime.hour(),
                                        assemblyTime.minute(),
                                        assemblyTime.second(),
                                        ser_1,
                                        ser_2,
                                        ser_3);

        // Disable the input fields and show the loading message
        ui->assemblyDate->setEnabled(false);
        ui->assemblyTime->setEnabled(false);
        ui->serialNumber->setEnabled(false);
        ui->settingFactoryParameters_label->setVisible(true);
    }
}

void EnterFactoryParametersDialog::reject()
{
    // Overriding this to prevent default behavior of escape key causing dialog to exit
}

void EnterFactoryParametersDialog::factoryParametersLoaded()
{
    ui->settingFactoryParameters_label->setText("Setting parameters...done");

    // Inform the user that the load was successful
    QMessageBox msg;
    msg.setText("Parameters Loaded");
    msg.setInformativeText("The Factory Parameters were Sucessfully Loaded!");
    msg.setIcon(QMessageBox::Information);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();

    // Close the dialog
    QDialog::accept();
}

void EnterFactoryParametersDialog::updateGeneratedSerialNumber()
{
    QDate assemblyDate = QDate::fromString(ui->assemblyDate->text(), "MM/dd/yyyy");

    QString generatedSerialNumber = QString(m_partCode1) +
            QString(m_partCode2) +
            QString::number(m_design) +
            QString::number(ui->languageCountry->currentData().toInt()) +
            QString(m_option) +
            QString::number(assemblyDate.year() - 2010) +
            QString::number(assemblyDate.month(), 16) +
            ui->serialNumber->text();
    ui->generatedSerialNumber->setText(generatedSerialNumber);
}

void EnterFactoryParametersDialog::on_assemblyDate_textChanged(const QString&)
{
    updateGeneratedSerialNumber();
}

void EnterFactoryParametersDialog::on_serialNumber_textChanged(const QString&)
{
    updateGeneratedSerialNumber();
}

void EnterFactoryParametersDialog::on_languageCountry_currentIndexChanged(int)
{
    updateGeneratedSerialNumber();
}
