/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[65];
    char stringdata0[1274];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 20), // "closeSerialInterface"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 23), // "requestFirmwareDownload"
QT_MOC_LITERAL(4, 57, 16), // "firmwareFileName"
QT_MOC_LITERAL(5, 74, 17), // "firmwareLoadError"
QT_MOC_LITERAL(6, 92, 8), // "errorMsg"
QT_MOC_LITERAL(7, 101, 20), // "firmwareLoadProgress"
QT_MOC_LITERAL(8, 122, 8), // "progress"
QT_MOC_LITERAL(9, 131, 22), // "axisCalibrationStarted"
QT_MOC_LITERAL(10, 154, 4), // "axis"
QT_MOC_LITERAL(11, 159, 23), // "axisCalibrationFinished"
QT_MOC_LITERAL(12, 183, 10), // "successful"
QT_MOC_LITERAL(13, 194, 22), // "requestFirmwareVersion"
QT_MOC_LITERAL(14, 217, 20), // "requestCalibrateAxes"
QT_MOC_LITERAL(15, 238, 20), // "retryAxesCalibration"
QT_MOC_LITERAL(16, 259, 18), // "requestResetGimbal"
QT_MOC_LITERAL(17, 278, 28), // "requestHomeOffsetCalibration"
QT_MOC_LITERAL(18, 307, 18), // "sendNewHomeOffsets"
QT_MOC_LITERAL(19, 326, 9), // "yawOffset"
QT_MOC_LITERAL(20, 336, 11), // "pitchOffset"
QT_MOC_LITERAL(21, 348, 10), // "rollOffset"
QT_MOC_LITERAL(22, 359, 24), // "requestFactoryParameters"
QT_MOC_LITERAL(23, 384, 26), // "setGimbalFactoryParameters"
QT_MOC_LITERAL(24, 411, 8), // "assyYear"
QT_MOC_LITERAL(25, 420, 9), // "assyMonth"
QT_MOC_LITERAL(26, 430, 7), // "assyDay"
QT_MOC_LITERAL(27, 438, 8), // "assyHour"
QT_MOC_LITERAL(28, 447, 10), // "assyMinute"
QT_MOC_LITERAL(29, 458, 10), // "assySecond"
QT_MOC_LITERAL(30, 469, 13), // "serialNumber1"
QT_MOC_LITERAL(31, 483, 13), // "serialNumber2"
QT_MOC_LITERAL(32, 497, 13), // "serialNumber3"
QT_MOC_LITERAL(33, 511, 23), // "factoryParametersLoaded"
QT_MOC_LITERAL(34, 535, 23), // "requestEraseGimbalFlash"
QT_MOC_LITERAL(35, 559, 24), // "requestStartFactoryTests"
QT_MOC_LITERAL(36, 584, 18), // "factoryTestsStatus"
QT_MOC_LITERAL(37, 603, 4), // "test"
QT_MOC_LITERAL(38, 608, 12), // "test_section"
QT_MOC_LITERAL(39, 621, 13), // "test_progress"
QT_MOC_LITERAL(40, 635, 11), // "test_status"
QT_MOC_LITERAL(41, 647, 23), // "receivedGimbalHeartbeat"
QT_MOC_LITERAL(42, 671, 39), // "receivedGimbalDataTransmissio..."
QT_MOC_LITERAL(43, 711, 22), // "debugFirmwareLoadError"
QT_MOC_LITERAL(44, 734, 25), // "debugFirmwareLoadProgress"
QT_MOC_LITERAL(45, 760, 22), // "receiveFirmwareVersion"
QT_MOC_LITERAL(46, 783, 13), // "versionString"
QT_MOC_LITERAL(47, 797, 26), // "receiveSerialPortOpenError"
QT_MOC_LITERAL(48, 824, 34), // "receiveHomeOffsetCalibrationS..."
QT_MOC_LITERAL(49, 859, 24), // "receiveFactoryParameters"
QT_MOC_LITERAL(50, 884, 16), // "assemblyDateTime"
QT_MOC_LITERAL(51, 901, 12), // "serialNumber"
QT_MOC_LITERAL(52, 914, 10), // "closeEvent"
QT_MOC_LITERAL(53, 925, 12), // "QCloseEvent*"
QT_MOC_LITERAL(54, 938, 17), // "connectionTimeout"
QT_MOC_LITERAL(55, 956, 24), // "on_connectButton_clicked"
QT_MOC_LITERAL(56, 981, 27), // "on_disconnectButton_clicked"
QT_MOC_LITERAL(57, 1009, 36), // "on_firmwareImageBrowseButton_..."
QT_MOC_LITERAL(58, 1046, 29), // "on_loadFirmwareButton_clicked"
QT_MOC_LITERAL(59, 1076, 35), // "on_runAxisCalibrationButton_c..."
QT_MOC_LITERAL(60, 1112, 28), // "on_resetGimbalButton_clicked"
QT_MOC_LITERAL(61, 1141, 33), // "on_setHomePositionsButton_cli..."
QT_MOC_LITERAL(62, 1175, 34), // "on_setUnitParametersButton_cl..."
QT_MOC_LITERAL(63, 1210, 33), // "on_eraseGimbalFlashButton_cli..."
QT_MOC_LITERAL(64, 1244, 29) // "on_factoryTestsButton_clicked"

    },
    "MainWindow\0closeSerialInterface\0\0"
    "requestFirmwareDownload\0firmwareFileName\0"
    "firmwareLoadError\0errorMsg\0"
    "firmwareLoadProgress\0progress\0"
    "axisCalibrationStarted\0axis\0"
    "axisCalibrationFinished\0successful\0"
    "requestFirmwareVersion\0requestCalibrateAxes\0"
    "retryAxesCalibration\0requestResetGimbal\0"
    "requestHomeOffsetCalibration\0"
    "sendNewHomeOffsets\0yawOffset\0pitchOffset\0"
    "rollOffset\0requestFactoryParameters\0"
    "setGimbalFactoryParameters\0assyYear\0"
    "assyMonth\0assyDay\0assyHour\0assyMinute\0"
    "assySecond\0serialNumber1\0serialNumber2\0"
    "serialNumber3\0factoryParametersLoaded\0"
    "requestEraseGimbalFlash\0"
    "requestStartFactoryTests\0factoryTestsStatus\0"
    "test\0test_section\0test_progress\0"
    "test_status\0receivedGimbalHeartbeat\0"
    "receivedGimbalDataTransmissionHandshake\0"
    "debugFirmwareLoadError\0debugFirmwareLoadProgress\0"
    "receiveFirmwareVersion\0versionString\0"
    "receiveSerialPortOpenError\0"
    "receiveHomeOffsetCalibrationStatus\0"
    "receiveFactoryParameters\0assemblyDateTime\0"
    "serialNumber\0closeEvent\0QCloseEvent*\0"
    "connectionTimeout\0on_connectButton_clicked\0"
    "on_disconnectButton_clicked\0"
    "on_firmwareImageBrowseButton_clicked\0"
    "on_loadFirmwareButton_clicked\0"
    "on_runAxisCalibrationButton_clicked\0"
    "on_resetGimbalButton_clicked\0"
    "on_setHomePositionsButton_clicked\0"
    "on_setUnitParametersButton_clicked\0"
    "on_eraseGimbalFlashButton_clicked\0"
    "on_factoryTestsButton_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      38,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      18,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  204,    2, 0x06 /* Public */,
       3,    1,  205,    2, 0x06 /* Public */,
       5,    1,  208,    2, 0x06 /* Public */,
       7,    1,  211,    2, 0x06 /* Public */,
       9,    1,  214,    2, 0x06 /* Public */,
      11,    2,  217,    2, 0x06 /* Public */,
      13,    0,  222,    2, 0x06 /* Public */,
      14,    0,  223,    2, 0x06 /* Public */,
      15,    0,  224,    2, 0x06 /* Public */,
      16,    0,  225,    2, 0x06 /* Public */,
      17,    0,  226,    2, 0x06 /* Public */,
      18,    3,  227,    2, 0x06 /* Public */,
      22,    0,  234,    2, 0x06 /* Public */,
      23,    9,  235,    2, 0x06 /* Public */,
      33,    0,  254,    2, 0x06 /* Public */,
      34,    0,  255,    2, 0x06 /* Public */,
      35,    0,  256,    2, 0x06 /* Public */,
      36,    4,  257,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      41,    0,  266,    2, 0x0a /* Public */,
      42,    0,  267,    2, 0x0a /* Public */,
      43,    1,  268,    2, 0x0a /* Public */,
      44,    1,  271,    2, 0x0a /* Public */,
      45,    1,  274,    2, 0x0a /* Public */,
      47,    1,  277,    2, 0x0a /* Public */,
      48,    1,  280,    2, 0x0a /* Public */,
      49,    2,  283,    2, 0x0a /* Public */,
      52,    1,  288,    2, 0x0a /* Public */,
      54,    0,  291,    2, 0x08 /* Private */,
      55,    0,  292,    2, 0x08 /* Private */,
      56,    0,  293,    2, 0x08 /* Private */,
      57,    0,  294,    2, 0x08 /* Private */,
      58,    0,  295,    2, 0x08 /* Private */,
      59,    0,  296,    2, 0x08 /* Private */,
      60,    0,  297,    2, 0x08 /* Private */,
      61,    0,  298,    2, 0x08 /* Private */,
      62,    0,  299,    2, 0x08 /* Private */,
      63,    0,  300,    2, 0x08 /* Private */,
      64,    0,  301,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Double,    8,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,   10,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,   19,   20,   21,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UShort, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::ULong, QMetaType::ULong, QMetaType::ULong,   24,   25,   26,   27,   28,   29,   30,   31,   32,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   37,   38,   39,   40,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Double,    8,
    QMetaType::Void, QMetaType::QString,   46,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   50,   51,
    QMetaType::Void, 0x80000000 | 53,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->closeSerialInterface(); break;
        case 1: _t->requestFirmwareDownload((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->firmwareLoadError((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->firmwareLoadProgress((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 4: _t->axisCalibrationStarted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->axisCalibrationFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: _t->requestFirmwareVersion(); break;
        case 7: _t->requestCalibrateAxes(); break;
        case 8: _t->retryAxesCalibration(); break;
        case 9: _t->requestResetGimbal(); break;
        case 10: _t->requestHomeOffsetCalibration(); break;
        case 11: _t->sendNewHomeOffsets((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 12: _t->requestFactoryParameters(); break;
        case 13: _t->setGimbalFactoryParameters((*reinterpret_cast< unsigned short(*)>(_a[1])),(*reinterpret_cast< unsigned char(*)>(_a[2])),(*reinterpret_cast< unsigned char(*)>(_a[3])),(*reinterpret_cast< unsigned char(*)>(_a[4])),(*reinterpret_cast< unsigned char(*)>(_a[5])),(*reinterpret_cast< unsigned char(*)>(_a[6])),(*reinterpret_cast< ulong(*)>(_a[7])),(*reinterpret_cast< ulong(*)>(_a[8])),(*reinterpret_cast< ulong(*)>(_a[9]))); break;
        case 14: _t->factoryParametersLoaded(); break;
        case 15: _t->requestEraseGimbalFlash(); break;
        case 16: _t->requestStartFactoryTests(); break;
        case 17: _t->factoryTestsStatus((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 18: _t->receivedGimbalHeartbeat(); break;
        case 19: _t->receivedGimbalDataTransmissionHandshake(); break;
        case 20: _t->debugFirmwareLoadError((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 21: _t->debugFirmwareLoadProgress((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 22: _t->receiveFirmwareVersion((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 23: _t->receiveSerialPortOpenError((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 24: _t->receiveHomeOffsetCalibrationStatus((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 25: _t->receiveFactoryParameters((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 26: _t->closeEvent((*reinterpret_cast< QCloseEvent*(*)>(_a[1]))); break;
        case 27: _t->connectionTimeout(); break;
        case 28: _t->on_connectButton_clicked(); break;
        case 29: _t->on_disconnectButton_clicked(); break;
        case 30: _t->on_firmwareImageBrowseButton_clicked(); break;
        case 31: _t->on_loadFirmwareButton_clicked(); break;
        case 32: _t->on_runAxisCalibrationButton_clicked(); break;
        case 33: _t->on_resetGimbalButton_clicked(); break;
        case 34: _t->on_setHomePositionsButton_clicked(); break;
        case 35: _t->on_setUnitParametersButton_clicked(); break;
        case 36: _t->on_eraseGimbalFlashButton_clicked(); break;
        case 37: _t->on_factoryTestsButton_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::closeSerialInterface)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestFirmwareDownload)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::firmwareLoadError)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::firmwareLoadProgress)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::axisCalibrationStarted)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::axisCalibrationFinished)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestFirmwareVersion)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestCalibrateAxes)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::retryAxesCalibration)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestResetGimbal)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestHomeOffsetCalibration)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::sendNewHomeOffsets)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestFactoryParameters)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(unsigned short , unsigned char , unsigned char , unsigned char , unsigned char , unsigned char , unsigned long , unsigned long , unsigned long );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::setGimbalFactoryParameters)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::factoryParametersLoaded)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestEraseGimbalFlash)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestStartFactoryTests)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int , int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::factoryTestsStatus)) {
                *result = 17;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 38)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 38;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 38)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 38;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::closeSerialInterface()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MainWindow::requestFirmwareDownload(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MainWindow::firmwareLoadError(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MainWindow::firmwareLoadProgress(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MainWindow::axisCalibrationStarted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void MainWindow::axisCalibrationFinished(int _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void MainWindow::requestFirmwareVersion()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MainWindow::requestCalibrateAxes()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void MainWindow::retryAxesCalibration()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void MainWindow::requestResetGimbal()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void MainWindow::requestHomeOffsetCalibration()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void MainWindow::sendNewHomeOffsets(int _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void MainWindow::requestFactoryParameters()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void MainWindow::setGimbalFactoryParameters(unsigned short _t1, unsigned char _t2, unsigned char _t3, unsigned char _t4, unsigned char _t5, unsigned char _t6, unsigned long _t7, unsigned long _t8, unsigned long _t9)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)), const_cast<void*>(reinterpret_cast<const void*>(&_t7)), const_cast<void*>(reinterpret_cast<const void*>(&_t8)), const_cast<void*>(reinterpret_cast<const void*>(&_t9)) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void MainWindow::factoryParametersLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void MainWindow::requestEraseGimbalFlash()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void MainWindow::requestStartFactoryTests()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void MainWindow::factoryTestsStatus(int _t1, int _t2, int _t3, int _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
