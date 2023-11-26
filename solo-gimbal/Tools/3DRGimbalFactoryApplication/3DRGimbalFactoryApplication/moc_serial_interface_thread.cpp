/****************************************************************************
** Meta object code from reading C++ file 'serial_interface_thread.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "serial_interface_thread.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'serial_interface_thread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SerialInterfaceThread_t {
    QByteArrayData data[54];
    char stringdata0[899];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SerialInterfaceThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SerialInterfaceThread_t qt_meta_stringdata_SerialInterfaceThread = {
    {
QT_MOC_LITERAL(0, 0, 21), // "SerialInterfaceThread"
QT_MOC_LITERAL(1, 22, 17), // "receivedHeartbeat"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 33), // "receivedDataTransmissionHands..."
QT_MOC_LITERAL(4, 75, 17), // "firmwareLoadError"
QT_MOC_LITERAL(5, 93, 8), // "errorMsg"
QT_MOC_LITERAL(6, 102, 20), // "firmwareLoadProgress"
QT_MOC_LITERAL(7, 123, 8), // "progress"
QT_MOC_LITERAL(8, 132, 22), // "axisCalibrationStarted"
QT_MOC_LITERAL(9, 155, 4), // "axis"
QT_MOC_LITERAL(10, 160, 23), // "axisCalibrationFinished"
QT_MOC_LITERAL(11, 184, 10), // "successful"
QT_MOC_LITERAL(12, 195, 19), // "sendFirmwareVersion"
QT_MOC_LITERAL(13, 215, 13), // "versionString"
QT_MOC_LITERAL(14, 229, 19), // "serialPortOpenError"
QT_MOC_LITERAL(15, 249, 29), // "homeOffsetCalibrationFinished"
QT_MOC_LITERAL(16, 279, 18), // "sendNewHomeOffsets"
QT_MOC_LITERAL(17, 298, 9), // "yawOffset"
QT_MOC_LITERAL(18, 308, 11), // "pitchOffset"
QT_MOC_LITERAL(19, 320, 10), // "rollOffset"
QT_MOC_LITERAL(20, 331, 21), // "sendFactoryParameters"
QT_MOC_LITERAL(21, 353, 12), // "assyDateTime"
QT_MOC_LITERAL(22, 366, 12), // "serialNumber"
QT_MOC_LITERAL(23, 379, 23), // "factoryParametersLoaded"
QT_MOC_LITERAL(24, 403, 18), // "factoryTestsStatus"
QT_MOC_LITERAL(25, 422, 4), // "test"
QT_MOC_LITERAL(26, 427, 12), // "test_section"
QT_MOC_LITERAL(27, 440, 13), // "test_progress"
QT_MOC_LITERAL(28, 454, 11), // "test_status"
QT_MOC_LITERAL(29, 466, 3), // "run"
QT_MOC_LITERAL(30, 470, 11), // "handleInput"
QT_MOC_LITERAL(31, 482, 11), // "requestStop"
QT_MOC_LITERAL(32, 494, 19), // "requestLoadFirmware"
QT_MOC_LITERAL(33, 514, 21), // "firmwareImageFileName"
QT_MOC_LITERAL(34, 536, 20), // "requestCalibrateAxes"
QT_MOC_LITERAL(35, 557, 18), // "requestResetGimbal"
QT_MOC_LITERAL(36, 576, 22), // "requestFirmwareVersion"
QT_MOC_LITERAL(37, 599, 20), // "retryAxesCalibration"
QT_MOC_LITERAL(38, 620, 27), // "requestCalibrateHomeOffsets"
QT_MOC_LITERAL(39, 648, 26), // "setGimbalFactoryParameters"
QT_MOC_LITERAL(40, 675, 8), // "assyYear"
QT_MOC_LITERAL(41, 684, 9), // "assyMonth"
QT_MOC_LITERAL(42, 694, 7), // "assyDay"
QT_MOC_LITERAL(43, 702, 8), // "assyHour"
QT_MOC_LITERAL(44, 711, 10), // "assyMinute"
QT_MOC_LITERAL(45, 722, 10), // "assySecond"
QT_MOC_LITERAL(46, 733, 13), // "serialNumber1"
QT_MOC_LITERAL(47, 747, 13), // "serialNumber2"
QT_MOC_LITERAL(48, 761, 13), // "serialNumber3"
QT_MOC_LITERAL(49, 775, 24), // "requestFactoryParameters"
QT_MOC_LITERAL(50, 800, 23), // "requestGimbalEraseFlash"
QT_MOC_LITERAL(51, 824, 25), // "requestGimbalFactoryTests"
QT_MOC_LITERAL(52, 850, 28), // "requestCalibrationParameters"
QT_MOC_LITERAL(53, 879, 19) // "sendPeriodicRateCmd"

    },
    "SerialInterfaceThread\0receivedHeartbeat\0"
    "\0receivedDataTransmissionHandshake\0"
    "firmwareLoadError\0errorMsg\0"
    "firmwareLoadProgress\0progress\0"
    "axisCalibrationStarted\0axis\0"
    "axisCalibrationFinished\0successful\0"
    "sendFirmwareVersion\0versionString\0"
    "serialPortOpenError\0homeOffsetCalibrationFinished\0"
    "sendNewHomeOffsets\0yawOffset\0pitchOffset\0"
    "rollOffset\0sendFactoryParameters\0"
    "assyDateTime\0serialNumber\0"
    "factoryParametersLoaded\0factoryTestsStatus\0"
    "test\0test_section\0test_progress\0"
    "test_status\0run\0handleInput\0requestStop\0"
    "requestLoadFirmware\0firmwareImageFileName\0"
    "requestCalibrateAxes\0requestResetGimbal\0"
    "requestFirmwareVersion\0retryAxesCalibration\0"
    "requestCalibrateHomeOffsets\0"
    "setGimbalFactoryParameters\0assyYear\0"
    "assyMonth\0assyDay\0assyHour\0assyMinute\0"
    "assySecond\0serialNumber1\0serialNumber2\0"
    "serialNumber3\0requestFactoryParameters\0"
    "requestGimbalEraseFlash\0"
    "requestGimbalFactoryTests\0"
    "requestCalibrationParameters\0"
    "sendPeriodicRateCmd"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SerialInterfaceThread[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  154,    2, 0x06 /* Public */,
       3,    0,  155,    2, 0x06 /* Public */,
       4,    1,  156,    2, 0x06 /* Public */,
       6,    1,  159,    2, 0x06 /* Public */,
       8,    1,  162,    2, 0x06 /* Public */,
      10,    2,  165,    2, 0x06 /* Public */,
      12,    1,  170,    2, 0x06 /* Public */,
      14,    1,  173,    2, 0x06 /* Public */,
      15,    1,  176,    2, 0x06 /* Public */,
      16,    3,  179,    2, 0x06 /* Public */,
      20,    2,  186,    2, 0x06 /* Public */,
      23,    0,  191,    2, 0x06 /* Public */,
      24,    4,  192,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      29,    0,  201,    2, 0x0a /* Public */,
      30,    0,  202,    2, 0x0a /* Public */,
      31,    0,  203,    2, 0x0a /* Public */,
      32,    1,  204,    2, 0x0a /* Public */,
      34,    0,  207,    2, 0x0a /* Public */,
      35,    0,  208,    2, 0x0a /* Public */,
      36,    0,  209,    2, 0x0a /* Public */,
      37,    0,  210,    2, 0x0a /* Public */,
      38,    0,  211,    2, 0x0a /* Public */,
      39,    9,  212,    2, 0x0a /* Public */,
      49,    0,  231,    2, 0x0a /* Public */,
      50,    0,  232,    2, 0x0a /* Public */,
      51,    0,  233,    2, 0x0a /* Public */,
      52,    0,  234,    2, 0x0a /* Public */,
      53,    0,  235,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Double,    7,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,    9,   11,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,   17,   18,   19,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   21,   22,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   25,   26,   27,   28,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   33,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UShort, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::ULong, QMetaType::ULong, QMetaType::ULong,   40,   41,   42,   43,   44,   45,   46,   47,   48,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SerialInterfaceThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SerialInterfaceThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->receivedHeartbeat(); break;
        case 1: _t->receivedDataTransmissionHandshake(); break;
        case 2: _t->firmwareLoadError((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->firmwareLoadProgress((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 4: _t->axisCalibrationStarted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->axisCalibrationFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: _t->sendFirmwareVersion((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->serialPortOpenError((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->homeOffsetCalibrationFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->sendNewHomeOffsets((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 10: _t->sendFactoryParameters((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 11: _t->factoryParametersLoaded(); break;
        case 12: _t->factoryTestsStatus((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 13: _t->run(); break;
        case 14: _t->handleInput(); break;
        case 15: _t->requestStop(); break;
        case 16: _t->requestLoadFirmware((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 17: _t->requestCalibrateAxes(); break;
        case 18: _t->requestResetGimbal(); break;
        case 19: _t->requestFirmwareVersion(); break;
        case 20: _t->retryAxesCalibration(); break;
        case 21: _t->requestCalibrateHomeOffsets(); break;
        case 22: _t->setGimbalFactoryParameters((*reinterpret_cast< unsigned short(*)>(_a[1])),(*reinterpret_cast< unsigned char(*)>(_a[2])),(*reinterpret_cast< unsigned char(*)>(_a[3])),(*reinterpret_cast< unsigned char(*)>(_a[4])),(*reinterpret_cast< unsigned char(*)>(_a[5])),(*reinterpret_cast< unsigned char(*)>(_a[6])),(*reinterpret_cast< ulong(*)>(_a[7])),(*reinterpret_cast< ulong(*)>(_a[8])),(*reinterpret_cast< ulong(*)>(_a[9]))); break;
        case 23: _t->requestFactoryParameters(); break;
        case 24: _t->requestGimbalEraseFlash(); break;
        case 25: _t->requestGimbalFactoryTests(); break;
        case 26: _t->requestCalibrationParameters(); break;
        case 27: _t->sendPeriodicRateCmd(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SerialInterfaceThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::receivedHeartbeat)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::receivedDataTransmissionHandshake)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::firmwareLoadError)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::firmwareLoadProgress)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::axisCalibrationStarted)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(int , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::axisCalibrationFinished)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::sendFirmwareVersion)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::serialPortOpenError)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::homeOffsetCalibrationFinished)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::sendNewHomeOffsets)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::sendFactoryParameters)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::factoryParametersLoaded)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (SerialInterfaceThread::*)(int , int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialInterfaceThread::factoryTestsStatus)) {
                *result = 12;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SerialInterfaceThread::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_SerialInterfaceThread.data,
    qt_meta_data_SerialInterfaceThread,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SerialInterfaceThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SerialInterfaceThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SerialInterfaceThread.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SerialInterfaceThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 28;
    }
    return _id;
}

// SIGNAL 0
void SerialInterfaceThread::receivedHeartbeat()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SerialInterfaceThread::receivedDataTransmissionHandshake()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SerialInterfaceThread::firmwareLoadError(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SerialInterfaceThread::firmwareLoadProgress(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SerialInterfaceThread::axisCalibrationStarted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void SerialInterfaceThread::axisCalibrationFinished(int _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void SerialInterfaceThread::sendFirmwareVersion(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void SerialInterfaceThread::serialPortOpenError(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void SerialInterfaceThread::homeOffsetCalibrationFinished(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void SerialInterfaceThread::sendNewHomeOffsets(int _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void SerialInterfaceThread::sendFactoryParameters(QString _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void SerialInterfaceThread::factoryParametersLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void SerialInterfaceThread::factoryTestsStatus(int _t1, int _t2, int _t3, int _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
