#include <QtCore>

#include "metatype_impl.h"
#include "extensions/name.h"

QDebug operator<<(QDebug &dbg, N::TypeId id)
{
    if (!id) {
        return dbg << "TypeId(Unknown)";
    }
    // TODO allow warning less name access
    auto typeName = N::Extensions::Name_hash::name(id);
    dbg.nospace() << "TypeId(";
    if (!typeName.isEmpty())
        dbg << typeName << ',' << ' ';
    dbg << (void*)id << ")";
    return dbg.space();
}