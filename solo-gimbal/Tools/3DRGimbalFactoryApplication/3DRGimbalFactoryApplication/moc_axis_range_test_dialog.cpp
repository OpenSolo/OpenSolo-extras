/****************************************************************************
** Meta object code from reading C++ file 'axis_range_test_dialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "axis_range_test_dialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'axis_range_test_dialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AxisRangeTestDialog_t {
    QByteArrayData data[10];
    char stringdata0[147];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AxisRangeTestDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AxisRangeTestDialog_t qt_meta_stringdata_AxisRangeTestDialog = {
    {
QT_MOC_LITERAL(0, 0, 19), // "AxisRangeTestDialog"
QT_MOC_LITERAL(1, 20, 16), // "requestTestRetry"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 19), // "receiveTestProgress"
QT_MOC_LITERAL(4, 58, 12), // "test_section"
QT_MOC_LITERAL(5, 71, 13), // "test_progress"
QT_MOC_LITERAL(6, 85, 11), // "test_status"
QT_MOC_LITERAL(7, 97, 6), // "reject"
QT_MOC_LITERAL(8, 104, 22), // "on_retryButton_clicked"
QT_MOC_LITERAL(9, 127, 19) // "on_okButton_clicked"

    },
    "AxisRangeTestDialog\0requestTestRetry\0"
    "\0receiveTestProgress\0test_section\0"
    "test_progress\0test_status\0reject\0"
    "on_retryButton_clicked\0on_okButton_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AxisRangeTestDialog[] = {

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
       3,    4,   40,    2, 0x0a /* Public */,
       7,    0,   49,    2, 0x0a /* Public */,
       8,    0,   50,    2, 0x08 /* Private */,
       9,    0,   51,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,    2,    4,    5,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AxisRangeTestDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AxisRangeTestDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->requestTestRetry(); break;
        case 1: _t->receiveTestProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 2: _t->reject(); break;
        case 3: _t->on_retryButton_clicked(); break;
        case 4: _t->on_okButton_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AxisRangeTestDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AxisRangeTestDialog::requestTestRetry)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AxisRangeTestDialog::staticMetaObject = { {
    &QDialog::staticMetaObject,
    qt_meta_stringdata_AxisRangeTestDialog.data,
    qt_meta_data_AxisRangeTestDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AxisRangeTestDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AxisRangeTestDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AxisRangeTestDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int AxisRangeTestDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void AxisRangeTestDialog::requestTestRetry()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
