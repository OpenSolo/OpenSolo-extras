/****************************************************************************
** Meta object code from reading C++ file 'calibrate_axes_dialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "calibrate_axes_dialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'calibrate_axes_dialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CalibrateAxesDialog_t {
    QByteArrayData data[9];
    char stringdata0[136];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CalibrateAxesDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CalibrateAxesDialog_t qt_meta_stringdata_CalibrateAxesDialog = {
    {
QT_MOC_LITERAL(0, 0, 19), // "CalibrateAxesDialog"
QT_MOC_LITERAL(1, 20, 20), // "retryAxesCalibration"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 22), // "axisCalibrationStarted"
QT_MOC_LITERAL(4, 65, 4), // "axis"
QT_MOC_LITERAL(5, 70, 23), // "axisCalibrationFinished"
QT_MOC_LITERAL(6, 94, 10), // "successful"
QT_MOC_LITERAL(7, 105, 6), // "reject"
QT_MOC_LITERAL(8, 112, 23) // "on_cancelButton_clicked"

    },
    "CalibrateAxesDialog\0retryAxesCalibration\0"
    "\0axisCalibrationStarted\0axis\0"
    "axisCalibrationFinished\0successful\0"
    "reject\0on_cancelButton_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CalibrateAxesDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   40,    2, 0x0a /* Public */,
       5,    2,   43,    2, 0x0a /* Public */,
       7,    0,   48,    2, 0x0a /* Public */,
       8,    0,   49,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,    4,    6,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CalibrateAxesDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CalibrateAxesDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->retryAxesCalibration(); break;
        case 1: _t->axisCalibrationStarted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->axisCalibrationFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: _t->reject(); break;
        case 4: _t->on_cancelButton_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CalibrateAxesDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CalibrateAxesDialog::retryAxesCalibration)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CalibrateAxesDialog::staticMetaObject = { {
    &QDialog::staticMetaObject,
    qt_meta_stringdata_CalibrateAxesDialog.data,
    qt_meta_data_CalibrateAxesDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CalibrateAxesDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CalibrateAxesDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CalibrateAxesDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CalibrateAxesDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void CalibrateAxesDialog::retryAxesCalibration()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
