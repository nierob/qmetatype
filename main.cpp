#include <QtCore>
#include "metatype.h"

/******************************************/
/** This is QMetaType for Qt6 prototype. **/
/******************************************/


/* During Qt5 we learned many thing about type information that we need and that we collect.
The current system is quite good, but it has some deficiances.
- The whole registry is keept behind a mutex and it is very central, the mutex usage actually
  shows on profilers
- It is impossible for a user to extend collected data. That is example of QML in which
  a parallel structure has to be created. Similar problem is shared with data stream operators
  registration, and the code that tries to handle that
- The current implementation is quite fragile from BC perspective, we store data that may not
  match the reality after a while.
- Q_DECLARE_METATYPE is confusing for users, it got better as now we strongly recomand to place it
  after the class declaration, but still...
- It is impossible to register types that are not constructible, the orginal requirement comes from
  QVariant and has no sense in QMetaType context
- Distinction between builtins and custom types is confusing and is responsible for growing binary size.
- Unloading plugins is not supported by the QMetaType, we agreed that Qt is not supporting that case
  but people still do it and the current implementation leaks memory. Same happens for dynamically
  created types, there is no way to "unregister" a type, even if such funciton exists.
- More... TODO
*/


int main(int argc, char** argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    qDebug() << "----------------Int--------------------------";
    auto intId = N::qRegisterType<int>();
    qDebug() << "Succesfull registration of int:" << intId;
    qDebug() << "sizeof(int):" << sizeof(int) << "while metatype says that the size is:" << N::Extensions::Allocation::sizeOf(intId);
    qDebug() << "alignof(int):" << alignof(int) << "while metatype says that the align is:" << N::Extensions::Allocation::alignOf(intId);
    int intCopy = 6;
    int *i = static_cast<int*>(N::Extensions::Allocation::create(intId, &intCopy));
    qDebug() << "create(int):" << *i;
    qDebug() << "try to call qdebug stream (int), should result in error";
    N::Extensions::QDebugStream::qDebugStream(intId, qDebug() << "   Testing qdebug stream output, should result in error", i);
    qDebug() << "let's try to re-register the type with new a extension";
    N::qRegisterType<int, N::Extensions::QDebugStream>();
    qDebug() << "try to call qdebug stream (int):";
    N::Extensions::QDebugStream::qDebugStream(intId, qDebug() << "   Testing qdebug stream output after registration of QDebug extension value:", i);
    qDebug() << "destroy(int)...";
    N::Extensions::Allocation::destroy(intId, i);

    qDebug() << "----------------QString--------------------------";
    auto qstringId = N::qRegisterType<QString>();
    qDebug() << "Succesfull registration of QString:" << qstringId << "as named type" << N::Extensions::Name::name(qstringId);
    qDebug() << "sizeof(QString):" << sizeof(QString) << "while metatype says that the size is:" << N::Extensions::Allocation::sizeOf(qstringId);
    qDebug() << "alignof(QString):" << alignof(QString) << "while metatype says that the align is:" << N::Extensions::Allocation::alignOf(qstringId);
    QString stringCopy = QLatin1String("String");
    QString *string = static_cast<QString*>(N::Extensions::Allocation::create(qstringId, &stringCopy));
    qDebug() << "create(QString):" << *string;
    qDebug() << "destroy(QString)...";
    N::Extensions::Allocation::destroy(qstringId, string);

    qDebug() << "----------------Unsigned int--------------------------";
    auto preRegistrationIntId = N::Extensions::Name::fromName(QStringLiteral("unsigned"));
    qDebug() << "Succesfull pre-registration of unsigned:" << preRegistrationIntId;
    auto uintId = N::qRegisterType<unsigned>();
    qDebug() << "Pre-registration and registartion returned the same value:" << (uintId == preRegistrationIntId);
    qDebug() << "Unsigned int by default is known as:" << N::Extensions::Name::name(uintId);

    return 0;
}