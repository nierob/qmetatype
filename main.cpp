#include <QtCore>
#include "metatype.h"

int main(int argc, char** argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    qDebug() << "----------------Int--------------------------";
    auto intId = N::qTypeId<int>();
    qDebug() << "Succesfull geting type id of int:" << intId;
    qDebug() << "sizeof(int):" << sizeof(int) << "while metatype says that the size is:" << N::Extensions::Allocation::sizeOf(intId);
    qDebug() << "alignof(int):" << alignof(int) << "while metatype says that the align is:" << N::Extensions::Allocation::alignOf(intId);
    int intCopy = 6;
    int *i = static_cast<int*>(N::Extensions::Allocation::create(intId, &intCopy));
    qDebug() << "create(int):" << *i;
    qDebug() << "try to call qdebug stream (int), should result in error";
    N::Extensions::QDebugStream::qDebugStream(intId, qDebug() << "   Testing qdebug stream output, should result in error", i);
    qDebug() << "let's try to re-register the type with new a extension";
    N::qTypeId<int, N::Extensions::QDebugStream>();
    qDebug() << "try to call qdebug stream (int):";
    N::Extensions::QDebugStream::qDebugStream(intId, qDebug() << "   Testing qdebug stream output after registration of QDebug extension value:", i);
    qDebug() << "destroy(int)...";
    N::Extensions::Allocation::destroy(intId, i);

    qDebug() << "----------------QString--------------------------";
    auto qstringId = N::qTypeId<QString>();
    qDebug() << "Succesfull geting type id of QString:" << qstringId << "as named type" << N::Extensions::Name_dlsym::name(qstringId);
    qDebug() << "sizeof(QString):" << sizeof(QString) << "while metatype says that the size is:" << N::Extensions::Allocation::sizeOf(qstringId);
    qDebug() << "alignof(QString):" << alignof(QString) << "while metatype says that the align is:" << N::Extensions::Allocation::alignOf(qstringId);
    QString stringCopy = QLatin1String("String");
    QString *string = static_cast<QString*>(N::Extensions::Allocation::create(qstringId, &stringCopy));
    qDebug() << "create(QString):" << *string;
    qDebug() << "destroy(QString)...";
    N::Extensions::Allocation::destroy(qstringId, string);

    qDebug() << "----------------Unsigned int--------------------------";
    auto preRegistrationIntId = N::Extensions::Name_dlsym::fromName(QStringLiteral("unsigned"));
    qDebug() << "Succesfull geting type id of unsigned before using the qTypeId:" << preRegistrationIntId;
    auto uintId = N::qTypeId<unsigned, N::Extensions::Name_dlsym>();
    qDebug() << "All ids are in sync:" << (uintId == preRegistrationIntId);
    qDebug() << "Unsigned int by default is known as:" << N::Extensions::Name_dlsym::name(uintId);


    qDebug() << "----------------Char--------------------------";
    auto charId = N::qTypeId<char, N::Extensions::Name_hash>();
    qDebug() << "Succesfull geting type id of char:" << charId;
    qDebug() << "Lookup type id by name:" << N::Extensions::Name_hash::fromName("char");
    qDebug() << "Lookup type id by name returned correct id:" << (N::Extensions::Name_hash::fromName("char") == charId);
    qDebug() << "Char by default is known as:" << N::Extensions::Name_hash::name(charId);
    return 0;
}