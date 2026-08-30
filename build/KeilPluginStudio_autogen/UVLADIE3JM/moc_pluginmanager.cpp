/****************************************************************************
** Meta object code from reading C++ file 'pluginmanager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/pluginmanager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pluginmanager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSPluginManagerENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSPluginManagerENDCLASS = QtMocHelpers::stringData(
    "PluginManager",
    "projectChanged",
    "",
    "pluginsChanged",
    "logsChanged",
    "statusMessageChanged",
    "lastResultChanged",
    "loadProject",
    "source",
    "reloadProject",
    "installPlugin",
    "pluginId",
    "uninstallPlugin",
    "targetReport",
    "targetIndex",
    "runTool",
    "args",
    "clearLogs",
    "appendLog",
    "level",
    "message",
    "project",
    "KeilProject*",
    "projectLoaded",
    "plugins",
    "logs",
    "statusMessage",
    "lastResult"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSPluginManagerENDCLASS_t {
    uint offsetsAndSizes[56];
    char stringdata0[14];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[15];
    char stringdata4[12];
    char stringdata5[21];
    char stringdata6[18];
    char stringdata7[12];
    char stringdata8[7];
    char stringdata9[14];
    char stringdata10[14];
    char stringdata11[9];
    char stringdata12[16];
    char stringdata13[13];
    char stringdata14[12];
    char stringdata15[8];
    char stringdata16[5];
    char stringdata17[10];
    char stringdata18[10];
    char stringdata19[6];
    char stringdata20[8];
    char stringdata21[8];
    char stringdata22[13];
    char stringdata23[14];
    char stringdata24[8];
    char stringdata25[5];
    char stringdata26[14];
    char stringdata27[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSPluginManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSPluginManagerENDCLASS_t qt_meta_stringdata_CLASSPluginManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13),  // "PluginManager"
        QT_MOC_LITERAL(14, 14),  // "projectChanged"
        QT_MOC_LITERAL(29, 0),  // ""
        QT_MOC_LITERAL(30, 14),  // "pluginsChanged"
        QT_MOC_LITERAL(45, 11),  // "logsChanged"
        QT_MOC_LITERAL(57, 20),  // "statusMessageChanged"
        QT_MOC_LITERAL(78, 17),  // "lastResultChanged"
        QT_MOC_LITERAL(96, 11),  // "loadProject"
        QT_MOC_LITERAL(108, 6),  // "source"
        QT_MOC_LITERAL(115, 13),  // "reloadProject"
        QT_MOC_LITERAL(129, 13),  // "installPlugin"
        QT_MOC_LITERAL(143, 8),  // "pluginId"
        QT_MOC_LITERAL(152, 15),  // "uninstallPlugin"
        QT_MOC_LITERAL(168, 12),  // "targetReport"
        QT_MOC_LITERAL(181, 11),  // "targetIndex"
        QT_MOC_LITERAL(193, 7),  // "runTool"
        QT_MOC_LITERAL(201, 4),  // "args"
        QT_MOC_LITERAL(206, 9),  // "clearLogs"
        QT_MOC_LITERAL(216, 9),  // "appendLog"
        QT_MOC_LITERAL(226, 5),  // "level"
        QT_MOC_LITERAL(232, 7),  // "message"
        QT_MOC_LITERAL(240, 7),  // "project"
        QT_MOC_LITERAL(248, 12),  // "KeilProject*"
        QT_MOC_LITERAL(261, 13),  // "projectLoaded"
        QT_MOC_LITERAL(275, 7),  // "plugins"
        QT_MOC_LITERAL(283, 4),  // "logs"
        QT_MOC_LITERAL(288, 13),  // "statusMessage"
        QT_MOC_LITERAL(302, 10)   // "lastResult"
    },
    "PluginManager",
    "projectChanged",
    "",
    "pluginsChanged",
    "logsChanged",
    "statusMessageChanged",
    "lastResultChanged",
    "loadProject",
    "source",
    "reloadProject",
    "installPlugin",
    "pluginId",
    "uninstallPlugin",
    "targetReport",
    "targetIndex",
    "runTool",
    "args",
    "clearLogs",
    "appendLog",
    "level",
    "message",
    "project",
    "KeilProject*",
    "projectLoaded",
    "plugins",
    "logs",
    "statusMessage",
    "lastResult"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSPluginManagerENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       6,  121, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   92,    2, 0x06,    7 /* Public */,
       3,    0,   93,    2, 0x06,    8 /* Public */,
       4,    0,   94,    2, 0x06,    9 /* Public */,
       5,    0,   95,    2, 0x06,   10 /* Public */,
       6,    0,   96,    2, 0x06,   11 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,   97,    2, 0x02,   12 /* Public */,
       9,    0,  100,    2, 0x02,   14 /* Public */,
      10,    1,  101,    2, 0x02,   15 /* Public */,
      12,    1,  104,    2, 0x02,   17 /* Public */,
      13,    1,  107,    2, 0x102,   19 /* Public | MethodIsConst  */,
      15,    2,  110,    2, 0x02,   21 /* Public */,
      17,    0,  115,    2, 0x02,   24 /* Public */,
      18,    2,  116,    2, 0x02,   25 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool, QMetaType::QVariant,    8,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::QVariantMap, QMetaType::Int,   14,
    QMetaType::QVariantMap, QMetaType::QString, QMetaType::QVariantMap,   11,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   19,   20,

 // properties: name, type, flags
      21, 0x80000000 | 22, 0x00015009, uint(0), 0,
      23, QMetaType::Bool, 0x00015001, uint(0), 0,
      24, QMetaType::QVariantList, 0x00015001, uint(1), 0,
      25, QMetaType::QVariantList, 0x00015001, uint(2), 0,
      26, QMetaType::QString, 0x00015001, uint(3), 0,
      27, QMetaType::QVariantMap, 0x00015001, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject PluginManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSPluginManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSPluginManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSPluginManagerENDCLASS_t,
        // property 'project'
        QtPrivate::TypeAndForceComplete<KeilProject*, std::true_type>,
        // property 'projectLoaded'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'plugins'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'logs'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'statusMessage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lastResult'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PluginManager, std::true_type>,
        // method 'projectChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pluginsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'logsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusMessageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastResultChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadProject'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'reloadProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'installPlugin'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'uninstallPlugin'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'targetReport'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'runTool'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'clearLogs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'appendLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void PluginManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PluginManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->projectChanged(); break;
        case 1: _t->pluginsChanged(); break;
        case 2: _t->logsChanged(); break;
        case 3: _t->statusMessageChanged(); break;
        case 4: _t->lastResultChanged(); break;
        case 5: { bool _r = _t->loadProject((*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->reloadProject(); break;
        case 7: { bool _r = _t->installPlugin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->uninstallPlugin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { QVariantMap _r = _t->targetReport((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 10: { QVariantMap _r = _t->runTool((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->clearLogs(); break;
        case 12: _t->appendLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PluginManager::*)();
            if (_t _q_method = &PluginManager::projectChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PluginManager::*)();
            if (_t _q_method = &PluginManager::pluginsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PluginManager::*)();
            if (_t _q_method = &PluginManager::logsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (PluginManager::*)();
            if (_t _q_method = &PluginManager::statusMessageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (PluginManager::*)();
            if (_t _q_method = &PluginManager::lastResultChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< KeilProject* >(); break;
        }
    }  else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<PluginManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< KeilProject**>(_v) = _t->project(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->projectLoaded(); break;
        case 2: *reinterpret_cast< QVariantList*>(_v) = _t->plugins(); break;
        case 3: *reinterpret_cast< QVariantList*>(_v) = _t->logs(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->statusMessage(); break;
        case 5: *reinterpret_cast< QVariantMap*>(_v) = _t->lastResult(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *PluginManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PluginManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSPluginManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PluginManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void PluginManager::projectChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PluginManager::pluginsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PluginManager::logsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PluginManager::statusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PluginManager::lastResultChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
