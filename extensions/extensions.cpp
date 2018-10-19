#include <QtCore/qstring.h>
#include "name.h"
#include "metatype_impl.h"

QReadWriteLock N::Extensions::Name_hash::lock {QReadWriteLock::Recursive};
QHash<QString, N::TypeId> N::Extensions::Name_hash::nameToId;

QDebug operator<<(QDebug &dbg, const N::Extensions::ExtensionBase &ex)
{
    dbg.nospace() << "N::Extensions::ExtensionBase(call=" << (void*)ex.call << ", data=" << ex.data << ")";
    return dbg.space();
}

template<> N::TypeId N::qTypeId<N::Extensions::Name_hash, N::Extensions::Name_hash>()
{
    // HACK WARNING
    // Name_hash is used when storing metatype information about all extensions (for example name is used in
    // error messages), it means that to access type id of Name_hash we need to know it's id.
    // To break infinite recursion we pretend here that it is a runtime created type (even if created through
    // static initialization).
    static struct {
        P::TypeIdData typeData;
        N::Extensions::Name_hash::RuntimeData data{QStringLiteral("N::Extensions::Name_hash")};
    } d;
    auto id = &d.typeData;
    id->registerExtension(id, d.data.createExtensionBase(id));
    return id;
}
