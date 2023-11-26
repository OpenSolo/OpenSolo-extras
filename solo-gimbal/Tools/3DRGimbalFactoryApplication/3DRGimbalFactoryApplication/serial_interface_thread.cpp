#include "serial_interface_thread.h"
#include "MAVLink/ardupilotmega/mavlink.h"

#include <QDebug>
#include <QThread>
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <cstring>

SerialInterfaceThread::SerialInterfaceThread(QString serialPortName, int serialPortBaudRate, QObject* parent) :
    QObject(parent),
    m_serialPortName(serialPortName),
    m_serialPortBaudRate(serialPortBaudRate),
    m_interfaceState(INTERFACE_INITIALIZED),
    m_stopRequested(false),
    m_firmwareImageData(NULL),
    m_serialPort(NULL),
    m_periodicRateCmdsTimer(NULL)
{
    // Set up the zero rate command
    mavlink_msg_gimbal_control_pack(0,
                                    0,
                                    &m_zeroRateCmd,
                                    MAVLINK_GIMBAL_SYSTEM_ID,
                                    MAV_COMP_ID_GIMBAL,
                                    0.0,
                                    0.0,
                                    0.0);
}

SerialInterfaceThread::~SerialInterfaceThread()
{
    if (m_firmwareImageData) {
        delete m_firmwareImageData;
    }

    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    if (m_serialPort) {
        delete m_serialPort;
    }

    if (m_periodicRateCmdsTimer) {
        delete m_periodicRateCmdsTimer;
    }
}

void SerialInterfaceThread::run()
{
    m_serialPort = new QSerialPort(this);

    // Create and hook up the periodic rate command timer to the send rate command slot
    m_periodicRateCmdsTimer = new QTimer(this);
    connect(m_periodicRateCmdsTimer, SIGNAL(timeout()), this, SLOT(sendPeriodicRateCmd()));

    // First, attempt to open the serial connection
    m_serialPort->setPortName(m_serialPortName);
    m_serialPort->setBaudRate(m_serialPortBaudRate);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        emit serialPortOpenError(m_serialPort->errorString());
        this->thread()->quit();
    } else {
        connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(handleInput()));
    }
}

void SerialInterfaceThread::handleInput()
{
    mavlink_message_t received_msg;
    mavlink_status_t parse_status;
    char readBuffer[1000];

    int charsToRead = std::min<int>(1000, m_serialPort->bytesAvailable());

    int charsRead = m_serialPort->read(readBuffer, charsToRead);

    if (charsRead < 0) {
        qDebug() << "Read serial port error\n";

    } else if (charsRead > 0) {
        //qDebug("Read %d characters", charsRead);
        //qDebug("Received character 0x%X\n", readBuffer[0]);
        for (int i = 0; i < charsRead; i++) {
            if (mavlink_parse_char(MAVLINK_COMM_0, readBuffer[i], &received_msg, &parse_status)) {
                switch (received_msg.msgid) {
                case MAVLINK_MSG_ID_HEARTBEAT:
                    if (m_interfaceState == INTERFACE_INITIALIZED) {
                        emit receivedHeartbeat();
                        m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                    }
                    break;

                case MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE:
                    if (m_interfaceState == INTERFACE_INITIALIZED) {
                        emit receivedDataTransmissionHandshake();
                        m_interfaceState = INTERFACE_INDICATED_NO_CODE_LOADED;
                    } else if (m_interfaceState == INTERFACE_LOADING_CODE) {
                        handleDataTransmissionHandshake(&received_msg);
                    }
                    break;

                case MAVLINK_MSG_ID_GIMBAL_AXIS_CALIBRATION_PROGRESS:
                    handleGimbalAxisCalibrationProgress(&received_msg);
                    break;

                case MAVLINK_MSG_ID_PARAM_VALUE:
                    handleParamValue(&received_msg);
                    break;

                case MAVLINK_MSG_ID_STATUSTEXT:
                    handleStatusText(&received_msg);
                    break;

                case MAVLINK_MSG_ID_HOME_OFFSET_CALIBRATION_RESULT:
                    handleHomeOffsetCalibrationResult(&received_msg);
                    break;

                case MAVLINK_MSG_ID_FACTORY_PARAMETERS_LOADED:
                    emit factoryParametersLoaded();
                    break;

                case MAVLINK_MSG_ID_REPORT_FACTORY_TESTS_PROGRESS:
                    handleFactoryTestsProgress(&received_msg);
                    break;

                default:
                    //qDebug() << "Unknown message ID received: " << received_msg.msgid << "\n";
                    break;
                }
            }
        }
    }
}

void SerialInterfaceThread::requestStop()
{
    this->thread()->quit();
    m_stopRequested = true;
}

void SerialInterfaceThread::requestLoadFirmware(QString firmwareImageFileName)
{
    // First, read the firmware image from the data file provided
    QFile firmwareImage(firmwareImageFileName);
    if (!firmwareImage.open(QIODevice::ReadOnly)) {
        emit firmwareLoadError("Error opening supplied firmware image " + firmwareImageFileName);
        return;
    }

    QByteArray tempFirmwareImageData = firmwareImage.readAll();
    firmwareImage.close();
    int firmwareImageNumWords = (tempFirmwareImageData.size() / 2) + 2; // Extra 2 words is for checksum and 0 terminator
    m_firmwareImageDataSize = firmwareImageNumWords - 2;
    m_firmwareImageData = new uint16_t[firmwareImageNumWords];
    std::memset(m_firmwareImageData, 0x0000, firmwareImageNumWords * sizeof(uint16_t));

    // Swap the bytes of the image file and calculate the checksum
    uint16_t firmwareImageChecksum = 0xFFFF;
    for (int i = 0; i < tempFirmwareImageData.size(); i += 2) {
        m_firmwareImageData[i / 2] = (tempFirmwareImageData[i + 1] & 0x00FF) | ((tempFirmwareImageData[i] << 8) & 0xFF00);
        firmwareImageChecksum ^= m_firmwareImageData[i / 2];
    }
    m_firmwareImageData[firmwareImageNumWords - 2] = firmwareImageChecksum;
    m_firmwareImageData[firmwareImageNumWords - 1] = 0x0000;

    // Send the firmware load start command
    // TODO: Sending 0 as number of total packets for now since this isn't used, but should be used per spec
    // so fix this later
    sendLoadFirmwareStart(0);

    m_interfaceState = INTERFACE_LOADING_CODE;
}

void SerialInterfaceThread::handleDataTransmissionHandshake(mavlink_message_t *msg)
{
    mavlink_data_transmission_handshake_t dth_msg;
    mavlink_msg_data_transmission_handshake_decode(msg, &dth_msg);
    sendFirmwareImageBlock(dth_msg.width, dth_msg.payload);
}

void SerialInterfaceThread::sendFirmwareImageBlock(int seqNum, int packetSize)
{
    qDebug("Gimbal requested firmware block %d of packet size %d", seqNum, packetSize);
    if (((seqNum * packetSize) / 2) > m_firmwareImageDataSize) {
        // We've sent the entire firmware image, so send a data transmission handshake to indicate
        // that we're done sending data
        mavlink_message_t dth_msg;
        mavlink_msg_data_transmission_handshake_pack(MAVLINK_GIMBAL_SYSTEM_ID,
                                                     MAV_COMP_ID_GIMBAL,
                                                     &dth_msg,
                                                     MAVLINK_TYPE_UINT16_T,
                                                     (m_firmwareImageDataSize / packetSize) + 1,
                                                     (m_firmwareImageDataSize / packetSize) + 1,
                                                     0,
                                                     (m_firmwareImageDataSize / packetSize) + 1,
                                                     (m_firmwareImageDataSize / packetSize) + 1,
                                                     0);
        sendMavlinkMessage(&dth_msg);
        emit firmwareLoadProgress(1.0);
        m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
    } else {
        // Send the requested data packet
        uint8_t* data = new uint8_t[packetSize];
        std::memcpy(data, reinterpret_cast<uint8_t*>(&(m_firmwareImageData[(seqNum * packetSize) / 2])), packetSize);
        mavlink_message_t data_msg;
        mavlink_msg_encapsulated_data_pack(MAVLINK_GIMBAL_SYSTEM_ID,
                                           MAV_COMP_ID_GIMBAL,
                                           &data_msg,
                                           seqNum,
                                           data);
        sendMavlinkMessage(&data_msg);
        delete data;
        qDebug("Seq num: %d, Packet Size %d, Total Image Size: %d", seqNum, packetSize, m_firmwareImageDataSize);
        emit firmwareLoadProgress(static_cast<double>((seqNum * packetSize) / 2) / static_cast<double>(m_firmwareImageDataSize));
    }
}

void SerialInterfaceThread::handleGimbalAxisCalibrationProgress(mavlink_message_t *msg)
{
    mavlink_gimbal_axis_calibration_progress_t decoded_msg;
    mavlink_msg_gimbal_axis_calibration_progress_decode(msg, &decoded_msg);

    switch (decoded_msg.calibration_axis) {
        case GIMBAL_AXIS_PITCH:
            if (decoded_msg.calibration_status == GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED) {
                m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                emit axisCalibrationFinished(GIMBAL_AXIS_PITCH, true);
            } else if (decoded_msg.calibration_status == GIMBAL_AXIS_CALIBRATION_STATUS_FAILED) {
                m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                emit axisCalibrationFinished(GIMBAL_AXIS_PITCH, false);
            } else if (m_interfaceState != INTERFACE_CALIBRATING_GIMBAL_EL) {
                m_interfaceState = INTERFACE_CALIBRATING_GIMBAL_EL;
                emit axisCalibrationStarted(GIMBAL_AXIS_PITCH);
            }
            break;

        case GIMBAL_AXIS_ROLL:
            if (decoded_msg.calibration_status == GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED) {
                m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                emit axisCalibrationFinished(GIMBAL_AXIS_ROLL, true);
            } else if (decoded_msg.calibration_status == GIMBAL_AXIS_CALIBRATION_STATUS_FAILED) {
                m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                emit axisCalibrationFinished(GIMBAL_AXIS_ROLL, false);
            } else if (m_interfaceState != INTERFACE_CALIBRATING_GIMBAL_RL) {
                m_interfaceState = INTERFACE_CALIBRATING_GIMBAL_RL;
                emit axisCalibrationStarted(GIMBAL_AXIS_ROLL);
            }
            break;

        case GIMBAL_AXIS_YAW:
            if (decoded_msg.calibration_status == GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED) {
                m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                emit axisCalibrationFinished(GIMBAL_AXIS_YAW, true);
            } else if (decoded_msg.calibration_status == GIMBAL_AXIS_CALIBRATION_STATUS_FAILED) {
                m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
                emit axisCalibrationFinished(GIMBAL_AXIS_YAW, false);
            } else if (m_interfaceState != INTERFACE_CALIBRATING_GIMBAL_AZ) {
                m_interfaceState = INTERFACE_CALIBRATING_GIMBAL_AZ;
                emit axisCalibrationStarted(GIMBAL_AXIS_YAW);
            }
            break;
    }
}

void SerialInterfaceThread::requestCalibrateAxes()
{
    // Set all commutation calibration parameters to 0
    mavlinkSetParam("CC_YAW_SLOPE", 0.0);
    mavlinkSetParam("CC_YAW_ICEPT", 0.0);
    mavlinkSetParam("CC_ROLL_SLOPE", 0.0);
    mavlinkSetParam("CC_ROLL_ICEPT", 0.0);
    mavlinkSetParam("CC_PITCH_SLOPE", 0.0);
    mavlinkSetParam("CC_PITCH_ICEPT", 0.0);

    // Commit the zeroed out calibration parameters to flash
    mavlinkSetParam("COMMIT_FLASH", 69.0);

    // Reset the gimbal
    resetGimbal();
}

void SerialInterfaceThread::requestResetGimbal()
{
    resetGimbal();
}

void SerialInterfaceThread::resetGimbal()
{
    m_interfaceState = INTERFACE_INITIALIZED;

    mavlink_message_t reset_msg;
    mavlink_msg_reset_gimbal_pack(0,
                                  0,
                                  &reset_msg,
                                  MAVLINK_GIMBAL_SYSTEM_ID,
                                  MAV_COMP_ID_GIMBAL);
    sendMavlinkMessage(&reset_msg);
}

void SerialInterfaceThread::mavlinkSetParam(const char *param_name, float param_value)
{
    mavlink_message_t param_set_msg;
    mavlink_msg_param_set_pack(0,
                               0,
                               &param_set_msg,
                               MAVLINK_GIMBAL_SYSTEM_ID,
                               MAV_COMP_ID_GIMBAL,
                               param_name,
                               param_value,
                               MAV_PARAM_TYPE_REAL32);
    sendMavlinkMessage(&param_set_msg);
}

void SerialInterfaceThread::requestFirmwareVersion()
{
    // Request the firmware version parameter
    sendParamRequest("SYSID_SWVER");
}

void SerialInterfaceThread::handleParamValue(mavlink_message_t *msg)
{
    mavlink_param_value_t decoded_msg;
    mavlink_msg_param_value_decode(msg, &decoded_msg);
    if (QString(decoded_msg.param_id) == QString("SYSID_SWVER")) {
        IntOrFloat float_converter;
        float_converter.float_value = decoded_msg.param_value;
        uint32_t version = float_converter.uint32_value;
        uint8_t versionMajor = (version >> 24) & 0x000000FF;
        uint8_t versionMinor = (version >> 16) & 0x000000FF;
        uint8_t versionRevision = (version >> 8) & 0x000000FF;
        uint8_t versionCommit = (version & 0x0000007F);
        bool isDirty = version & 0x00000080;

        QString versionString = "v" +
                QString::number(versionMajor) +
                "." +
                QString::number(versionMinor) +
                "." +
                QString::number(versionRevision) +
                "." +
                QString::number(versionCommit) +
                (isDirty ? "-dirty" : "");

        emit sendFirmwareVersion(versionString);
    } else if (QString(decoded_msg.param_id) == QString("CC_YAW_HOME")) {
        m_lastYawHomeOffset = static_cast<int>(decoded_msg.param_value);
        sendParamRequest("CC_PITCH_HOME");
    } else if (QString(decoded_msg.param_id) == QString("CC_PITCH_HOME")) {
        m_lastPitchHomeOffset = static_cast<int>(decoded_msg.param_value);
        sendParamRequest("CC_ROLL_HOME");
    } else if (QString(decoded_msg.param_id) == QString("CC_ROLL_HOME")) {
        m_lastRollHomeOffset = static_cast<int>(decoded_msg.param_value);
        emit sendNewHomeOffsets(m_lastYawHomeOffset, m_lastPitchHomeOffset, m_lastRollHomeOffset);
    } else if (QString(decoded_msg.param_id) == QString("SYSID_ASSY_DATE")) {
        IntOrFloat converter;
        converter.float_value = decoded_msg.param_value;
        m_lastAssyDate = converter.uint32_value;
        sendParamRequest("SYSID_ASSY_TIME");
    } else if (QString(decoded_msg.param_id) == QString("SYSID_ASSY_TIME")) {
        IntOrFloat converter;
        converter.float_value = decoded_msg.param_value;
        m_lastAssyTime = converter.uint32_value;
        sendParamRequest("SYSID_SER_NUM_1");
    } else if (QString(decoded_msg.param_id) == QString("SYSID_SER_NUM_1")) {
        IntOrFloat converter;
        converter.float_value = decoded_msg.param_value;
        m_lastSerialNumber1 = converter.uint32_value;
        sendParamRequest("SYSID_SER_NUM_2");
    } else if (QString(decoded_msg.param_id) == QString("SYSID_SER_NUM_2")) {
        IntOrFloat converter;
        converter.float_value = decoded_msg.param_value;
        m_lastSerialNumber2 = converter.uint32_value;
        sendParamRequest("SYSID_SER_NUM_3");
    } else if (QString(decoded_msg.param_id) == QString("SYSID_SER_NUM_3")) {
        IntOrFloat converter;
        converter.float_value = decoded_msg.param_value;
        m_lastSerialNumber3 = converter.uint32_value;

        // Decode the various factory parameters and emit them
        uint16_t assy_year = ((m_lastAssyDate >> 16) & 0x0000FFFF);
        uint8_t assy_month = ((m_lastAssyDate >> 8) & 0x000000FF);
        uint8_t assy_day = (m_lastAssyDate & 0x000000FF);
        uint8_t assy_hour = ((m_lastAssyTime >> 24) & 0x000000FF);
        uint8_t assy_minute = ((m_lastAssyTime >> 16) & 0x000000FF);
        uint8_t assy_second = ((m_lastAssyTime >> 8) & 0x000000FF);
        QString dateTimeString = QDateTime(QDate(assy_year, assy_month, assy_day), QTime(assy_hour, assy_minute, assy_second)).toString("MM/dd/yyyy hh:mm:ss");

        uint8_t part_code_1 = (m_lastSerialNumber1 >> 24) & 0x000000FF;
        uint8_t part_code_2 = (m_lastSerialNumber1 >> 16) & 0x000000FF;
        uint8_t design = (m_lastSerialNumber1 >> 8) & 0x000000FF;
        uint8_t languageCountry = (m_lastSerialNumber1 & 0x000000FF);
        uint8_t option = (m_lastSerialNumber2 >> 24) & 0x000000FF;
        uint16_t year = (m_lastSerialNumber2 >> 8) & 0x0000FFFF;
        uint8_t month = (m_lastSerialNumber2 & 0x000000FF);
        QString serialNumber = QString(part_code_1) +
                QString(part_code_2) +
                QString::number(design) +
                QString::number(languageCountry) +
                QString(option) +
                QString::number(year) +
                QString::number(month, 16) +
                QString::number(m_lastSerialNumber3);
        emit sendFactoryParameters(dateTimeString, serialNumber);
    } else {
        // If we don't have special handling installed for a particular parameter,
        // just emit the value over the debug stream
        qDebug("Received parameter %s, value %f", decoded_msg.param_id, decoded_msg.param_value);
    }
}

void SerialInterfaceThread::handleStatusText(mavlink_message_t *msg)
{
    mavlink_statustext_t decoded_msg;
    mavlink_msg_statustext_decode(msg, &decoded_msg);

    qDebug("Received status text message: %s", decoded_msg.text);
}

void SerialInterfaceThread::handleFactoryTestsProgress(mavlink_message_t* msg)
{
    mavlink_report_factory_tests_progress_t decoded_msg;
    mavlink_msg_report_factory_tests_progress_decode(msg, &decoded_msg);

    emit factoryTestsStatus(decoded_msg.test, decoded_msg.test_section, decoded_msg.test_section_progress, decoded_msg.test_status);

    /*
    char* section;
    char* status;
    char* test;

    switch (decoded_msg.test) {
        case FACTORY_TEST_AXIS_RANGE_LIMITS:
            test = "Axis range";

            switch (decoded_msg.test_section) {
                case AXIS_RANGE_TEST_SECTION_EL_CHECK_NEG:
                    section = "El check neg";
                    break;

                case AXIS_RANGE_TEST_SECTION_EL_CHECK_POS:
                    section = "El check pos";
                    break;

                case AXIS_RANGE_TEST_SECTION_EL_RETURN_HOME:
                    section = "El return home";
                    break;

                case AXIS_RANGE_TEST_SECTION_AZ_CHECK_NEG:
                    section = "Az check neg";
                    break;

                case AXIS_RANGE_TEST_SECTION_AZ_CHECK_POS:
                    section = "Az check pos";
                    break;

                case AXIS_RANGE_TEST_SECTION_AZ_RETURN_HOME:
                    section = "Az return home";
                    break;

                case AXIS_RANGE_TEST_SECTION_RL_CHECK_NEG:
                    section = "Rl check neg";
                    break;

                case AXIS_RANGE_TEST_SECTION_RL_CHECK_POS:
                    section = "Rl check pos";
                    break;

                case AXIS_RANGE_TEST_SECTION_RL_RETURN_HOME:
                    section = "Rl return home";
                    break;
            }

            switch (decoded_msg.test_status) {
                case AXIS_RANGE_TEST_STATUS_IN_PROGRESS:
                    status = "In progress";
                    break;

                case AXIS_RANGE_TEST_STATUS_SUCCEEDED:
                    status = "Succeeded";
                    break;

                case AXIS_RANGE_TEST_STATUS_FAILED_NEGATIVE:
                    status = "Failed negative";
                    break;

                case AXIS_RANGE_TEST_STATUS_FAILED_POSITIVE:
                    status = "Failed positive";
                    break;
            }

            break;
    }

    qDebug("Test: %s, Section: %s, Progress: %d, Status: %s", test, section, decoded_msg.test_section_progress, status);
    */
}

void SerialInterfaceThread::retryAxesCalibration()
{
    m_interfaceState = INTERFACE_INDICATED_CODE_LOADED;
    resetGimbal();
}

void SerialInterfaceThread::sendMavlinkMessage(mavlink_message_t *msg)
{
    int message_len = mavlink_msg_to_send_buffer((uint8_t*)(&(m_messageBuffer[0])), msg);

    m_serialPort->write(&(m_messageBuffer[0]), message_len);
}

void SerialInterfaceThread::sendLoadFirmwareStart(int numPackets)
{
    mavlink_message_t loadFWMsg;
    mavlink_msg_data_transmission_handshake_pack(MAVLINK_GIMBAL_SYSTEM_ID,
                                                 MAV_COMP_ID_GIMBAL,
                                                 &loadFWMsg,
                                                 MAVLINK_TYPE_UINT16_T,
                                                 numPackets,
                                                 numPackets,
                                                 0,
                                                 0,
                                                 0,
                                                 0);
    sendMavlinkMessage(&loadFWMsg);
}

void SerialInterfaceThread::requestCalibrateHomeOffsets()
{
    sendCalibrateHomeOffsets();
}

void SerialInterfaceThread::sendCalibrateHomeOffsets()
{
    mavlink_message_t calHomeOffsetsMsg;
    mavlink_msg_set_home_offsets_pack(0,
                                      0,
                                      &calHomeOffsetsMsg,
                                      MAVLINK_GIMBAL_SYSTEM_ID,
                                      MAV_COMP_ID_GIMBAL);
    sendMavlinkMessage(&calHomeOffsetsMsg);
}

void SerialInterfaceThread::handleHomeOffsetCalibrationResult(mavlink_message_t* msg)
{
    mavlink_home_offset_calibration_result_t decoded_msg;
    mavlink_msg_home_offset_calibration_result_decode(msg, &decoded_msg);
    if (decoded_msg.calibration_result == GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED) {
        emit homeOffsetCalibrationFinished(true);

        // Ask for the new yaw home offset
        sendParamRequest("CC_YAW_HOME");
    } else {
        emit homeOffsetCalibrationFinished(false);
    }
}

void SerialInterfaceThread::sendParamRequest(QString paramName)
{
    mavlink_message_t param_request_msg;
    mavlink_msg_param_request_read_pack(0,
                                        0,
                                        &param_request_msg,
                                        MAVLINK_GIMBAL_SYSTEM_ID,
                                        MAV_COMP_ID_GIMBAL,
                                        paramName.toLocal8Bit(),
                                        -1);
    sendMavlinkMessage(&param_request_msg);
}

void SerialInterfaceThread::setGimbalFactoryParameters(unsigned short assyYear,
                                                        unsigned char assyMonth,
                                                        unsigned char assyDay,
                                                        unsigned char assyHour,
                                                        unsigned char assyMinute,
                                                        unsigned char assySecond,
                                                        unsigned long serialNumber1,
                                                        unsigned long serialNumber2,
                                                        unsigned long serialNumber3)
{
    // Compose the assembly date and time into the internal 32-bit format to compute the checksum "magic numbers"
    uint32_t assyDate = 0;
    uint32_t assyTime = 0;

    assyDate |= ((uint32_t)assyYear << 16);
    assyDate |= ((uint32_t)assyMonth << 8);
    assyDate |= ((uint32_t)assyDay);

    assyTime |= ((uint32_t)assyHour << 24);
    assyTime |= ((uint32_t)assyMinute << 16);
    assyTime |= ((uint32_t)assySecond << 8);

    // Compute the checksum "magic numbers"
    uint32_t magic1 = ((uint32_t)FACTORY_PARAM_CHECK_MAGIC_1) - assyDate;
    uint32_t magic2 = ((uint32_t)FACTORY_PARAM_CHECK_MAGIC_2) - assyTime;
    uint32_t magic3 = ((uint32_t)FACTORY_PARAM_CHECK_MAGIC_3) - serialNumber1;

    // Compose the factory param set mavlink message
    mavlink_message_t factory_param_set_msg;
    mavlink_msg_set_factory_parameters_pack(0,
                                            0,
                                            &factory_param_set_msg,
                                            MAVLINK_GIMBAL_SYSTEM_ID,
                                            MAV_COMP_ID_GIMBAL,
                                            magic1,
                                            magic2,
                                            magic3,
                                            assyYear,
                                            assyMonth,
                                            assyDay,
                                            assyHour,
                                            assyMinute,
                                            assySecond,
                                            serialNumber1,
                                            serialNumber2,
                                            serialNumber3);
    sendMavlinkMessage(&factory_param_set_msg);
}

void SerialInterfaceThread::requestFactoryParameters()
{
    // We need to request the assembly date, the assembly time, and the serial number parameter
    // We start by requesting the assembly date parameter, the other requests will be sequenced
    // by the replys coming in
    sendParamRequest("SYSID_ASSY_DATE");
}

void SerialInterfaceThread::requestGimbalEraseFlash()
{
    mavlink_message_t erase_flash_msg;
    mavlink_msg_erase_gimbal_firmware_and_config_pack(0,
                                                      0,
                                                      &erase_flash_msg,
                                                      MAVLINK_GIMBAL_SYSTEM_ID,
                                                      MAV_COMP_ID_GIMBAL,
                                                      GIMBAL_FIRMWARE_ERASE_KNOCK);
    sendMavlinkMessage(&erase_flash_msg);
    // Reset interface state to initialized so that we treat incoming data transmission handshakes
    // as coming from an unprogrammed gimbal
    m_interfaceState = INTERFACE_INITIALIZED;
}

void SerialInterfaceThread::sendPeriodicRateCmd()
{
    sendMavlinkMessage(&m_zeroRateCmd);
}

void SerialInterfaceThread::requestGimbalFactoryTests()
{
    // Start the rate commands timer to start sending rate commands to the gimbal
    // The gimbal will not drive motors without periodic rate commands
    //m_periodicRateCmdsTimer->start(10);

    // Command the gimbal to start running the factory tests
    mavlink_message_t start_factory_tests_msg;
    mavlink_msg_perform_factory_tests_pack(0,
                                           0,
                                           &start_factory_tests_msg,
                                           MAVLINK_GIMBAL_SYSTEM_ID,
                                           MAV_COMP_ID_GIMBAL);
    sendMavlinkMessage(&start_factory_tests_msg);
}

void SerialInterfaceThread::requestCalibrationParameters()
{
    sendParamRequest("CC_YAW_SLOPE");
    sendParamRequest("CC_YAW_ICEPT");
    sendParamRequest("CC_PITCH_SLOPE");
    sendParamRequest("CC_PITCH_ICEPT");
    sendParamRequest("CC_ROLL_SLOPE");
    sendParamRequest("CC_ROLL_ICEPT");
    sendParamRequest("PID_YAW_P");
}
