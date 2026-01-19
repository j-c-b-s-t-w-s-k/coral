/****************************************************************************
** Meta object code from reading C++ file 'statusbadge.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.18)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "qt/widgets/statusbadge.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'statusbadge.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.18. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StatusBadge_t {
    QByteArrayData data[15];
    char stringdata0[124];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StatusBadge_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StatusBadge_t qt_meta_stringdata_StatusBadge = {
    {
QT_MOC_LITERAL(0, 0, 11), // "StatusBadge"
QT_MOC_LITERAL(1, 12, 13), // "statusChanged"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 6), // "Status"
QT_MOC_LITERAL(4, 34, 9), // "newStatus"
QT_MOC_LITERAL(5, 44, 7), // "clicked"
QT_MOC_LITERAL(6, 52, 6), // "status"
QT_MOC_LITERAL(7, 59, 7), // "Pending"
QT_MOC_LITERAL(8, 67, 6), // "Active"
QT_MOC_LITERAL(9, 74, 6), // "Funded"
QT_MOC_LITERAL(10, 81, 9), // "Completed"
QT_MOC_LITERAL(11, 91, 8), // "Disputed"
QT_MOC_LITERAL(12, 100, 9), // "Cancelled"
QT_MOC_LITERAL(13, 110, 7), // "Expired"
QT_MOC_LITERAL(14, 118, 5) // "Error"

    },
    "StatusBadge\0statusChanged\0\0Status\0"
    "newStatus\0clicked\0status\0Pending\0"
    "Active\0Funded\0Completed\0Disputed\0"
    "Cancelled\0Expired\0Error"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StatusBadge[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       1,   28, // properties
       1,   32, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,
       5,    0,   27,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,

 // properties: name, type, flags
       6, 0x80000000 | 3, 0x0049510b,

 // properties: notify_signal_id
       0,

 // enums: name, alias, flags, count, data
       3,    3, 0x0,    8,   37,

 // enum data: key, value
       7, uint(StatusBadge::Pending),
       8, uint(StatusBadge::Active),
       9, uint(StatusBadge::Funded),
      10, uint(StatusBadge::Completed),
      11, uint(StatusBadge::Disputed),
      12, uint(StatusBadge::Cancelled),
      13, uint(StatusBadge::Expired),
      14, uint(StatusBadge::Error),

       0        // eod
};

void StatusBadge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StatusBadge *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->statusChanged((*reinterpret_cast< Status(*)>(_a[1]))); break;
        case 1: _t->clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StatusBadge::*)(Status );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StatusBadge::statusChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StatusBadge::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StatusBadge::clicked)) {
                *result = 1;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<StatusBadge *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< Status*>(_v) = _t->status(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<StatusBadge *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setStatus(*reinterpret_cast< Status*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject StatusBadge::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_StatusBadge.data,
    qt_meta_data_StatusBadge,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StatusBadge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StatusBadge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StatusBadge.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int StatusBadge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void StatusBadge::statusChanged(Status _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StatusBadge::clicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
