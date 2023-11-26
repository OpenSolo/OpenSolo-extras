/****************************************************************************
** Meta object code from reading C++ file 'enter_factory_parameters_dialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "enter_factory_parameters_dialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'enter_factory_parameters_dialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EnterFactoryParametersDialog_t {
    QByteArrayData data[18];
    char stringdata0[290];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EnterFactoryParametersDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EnterFactoryParametersDialog_t qt_meta_stringdata_EnterFactoryParametersDialog = {
    {
QT_MOC_LITERAL(0, 0, 28), // "EnterFactoryParametersDialog"
QT_MOC_LITERAL(1, 29, 26), // "setGimbalFactoryParameters"
QT_MOC_LITERAL(2, 56, 0), // ""
QT_MOC_LITERAL(3, 57, 8), // "assyYear"
QT_MOC_LITERAL(4, 66, 9), // "assyMonth"
QT_MOC_LITERAL(5, 76, 7), // "assyDay"
QT_MOC_LITERAL(6, 84, 8), // "assyHour"
QT_MOC_LITERAL(7, 93, 10), // "assyMinute"
QT_MOC_LITERAL(8, 104, 10), // "assySecond"
QT_MOC_LITERAL(9, 115, 13), // "serialNumber1"
QT_MOC_LITERAL(10, 129, 13), // "serialNumber2"
QT_MOC_LITERAL(11, 143, 13), // "serialNumber3"
QT_MOC_LITERAL(12, 157, 23), // "factoryParametersLoaded"
QT_MOC_LITERAL(13, 181, 6), // "accept"
QT_MOC_LITERAL(14, 188, 6), // "reject"
QT_MOC_LITERAL(15, 195, 27), // "on_assemblyDate_textChanged"
QT_MOC_LITERAL(16, 223, 27), // "on_serialNumber_textChanged"
QT_MOC_LITERAL(17, 251, 38) // "on_languageCountry_currentInd..."

    },
    "EnterFactoryParametersDialog\0"
    "setGimbalFactoryParameters\0\0assyYear\0"
    "assyMonth\0assyDay\0assyHour\0assyMinute\0"
    "assySecond\0serialNumber1\0serialNumber2\0"
    "serialNumber3\0factoryParametersLoaded\0"
    "accept\0reject\0on_assemblyDate_textChanged\0"
    "on_serialNumber_textChanged\0"
    "on_languageCountry_currentIndexChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EnterFactoryParametersDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    9,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,   68,    2, 0x0a /* Public */,
      13,    0,   69,    2, 0x0a /* Public */,
      14,    0,   70,    2, 0x0a /* Public */,
      15,    1,   71,    2, 0x08 /* Private */,
      16,    1,   74,    2, 0x08 /* Private */,
      17,    1,   77,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::UShort, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::UChar, QMetaType::ULong, QMetaType::ULong, QMetaType::ULong,    3,    4,    5,    6,    7,    8,    9,   10,   11,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::Int,    2,

       0        // eod
};

void EnterFactoryParametersDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EnterFactoryParametersDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->setGimbalFactoryParameters((*reinterpret_cast< unsigned short(*)>(_a[1])),(*reinterpret_cast< unsigned char(*)>(_a[2])),(*reinterpret_cast< unsigned char(*)>(_a[3])),(*reinterpret_cast< unsigned char(*)>(_a[4])),(*reinterpret_cast< unsigned char(*)>(_a[5])),(*reinterpret_cast< unsigned char(*)>(_a[6])),(*reinterpret_cast< ulong(*)>(_a[7])),(*reinterpret_cast< ulong(*)>(_a[8])),(*reinterpret_cast< ulong(*)>(_a[9]))); break;
        case 1: _t->factoryParametersLoaded(); break;
        case 2: _t->accept(); break;
        case 3: _t->reject(); break;
        case 4: _t->on_assemblyDate_textChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->on_serialNumber_textChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->on_languageCountry_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EnterFactoryParametersDialog::*)(unsigned short , unsigned char , unsigned char , unsigned char , unsigned char , unsigned char , unsigned long , unsigned long , unsigned long );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EnterFactoryParametersDialog::setGimbalFactoryParameters)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EnterFactoryParametersDialog::staticMetaObject = { {
    &QDialog::staticMetaObject,
    qt_meta_stringdata_EnterFactoryParametersDialog.data,
    qt_meta_data_EnterFactoryParametersDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EnterFactoryParametersDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EnterFactoryParametersDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EnterFactoryParametersDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int EnterFactoryParametersDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void EnterFactoryParametersDialog::setGimbalFactoryParameters(unsigned short _t1, unsigned char _t2, unsigned char _t3, unsigned char _t4, unsigned char _t5, unsigned char _t6, unsigned long _t7, unsigned long _t8, unsigned long _t9)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)), const_cast<void*>(reinterpret_cast<const void*>(&_t7)), const_cast<void*>(reinterpret_cast<const void*>(&_t8)), const_cast<void*>(reinterpret_cast<const void*>(&_t9)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
