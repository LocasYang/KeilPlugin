/****************************************************************************
** Meta object code from reading C++ file 'keilproject.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/keilproject.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'keilproject.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.6.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSKeilProjectENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSKeilProjectENDCLASS = QtMocHelpers::stringData(
    "KeilProject",
    "dataChanged",
    "",
    "sourcePath",
    "kind",
    "valid",
    "errorString",
    "targets"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSKeilProjectENDCLASS_t {
    uint offsetsAndSizes[16];
    char stringdata0[12];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[5];
    char stringdata5[6];
    char stringdata6[12];
    char stringdata7[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSKeilProjectENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSKeilProjectENDCLASS_t qt_meta_stringdata_CLASSKeilProjectENDCLASS = {
    {
        QT_MOC_LITERAL(0, 11),  // "KeilProject"
        QT_MOC_LITERAL(12, 11),  // "dataChanged"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 10),  // "sourcePath"
        QT_MOC_LITERAL(36, 4),  // "kind"
        QT_MOC_LITERAL(41, 5),  // "valid"
        QT_MOC_LITERAL(47, 11),  // "errorString"
        QT_MOC_LITERAL(59, 7)   // "targets"
    },
    "KeilProject",
    "dataChanged",
    "",
    "sourcePath",
    "kind",
    "valid",
    "errorString",
    "targets"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSKeilProjectENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       5,   21, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x06,    6 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // properties: name, type, flags
       3, QMetaType::QString, 0x00015001, uint(0), 0,
       4, QMetaType::QString, 0x00015001, uint(0), 0,
       5, QMetaType::Bool, 0x00015001, uint(0), 0,
       6, QMetaType::QString, 0x00015001, uint(0), 0,
       7, QMetaType::QVariantList, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject KeilProject::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSKeilProjectENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSKeilProjectENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSKeilProjectENDCLASS_t,
        // property 'sourcePath'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'kind'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'valid'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'errorString'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'targets'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<KeilProject, std::true_type>,
        // method 'dataChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void KeilProject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<KeilProject *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->dataChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (KeilProject::*)();
            if (_t _q_method = &KeilProject::dataChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<KeilProject *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->sourcePath(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->kind(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->valid(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->errorString(); break;
        case 4: *reinterpret_cast< QVariantList*>(_v) = _t->targets(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
    (void)_a;
}

const QMetaObject *KeilProject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KeilProject::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSKeilProjectENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int KeilProject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void KeilProject::dataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
