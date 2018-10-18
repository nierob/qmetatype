#include <QtCore/qstring.h>
#include "name.h"

QReadWriteLock N::Extensions::Name_hash::lock;
QHash<QString, N::TypeId> N::Extensions::Name_hash::nameToId;

QDebug operator<<(QDebug &dbg, const N::Extensions::ExtensionBase &ex)
{
    dbg.nospace() << "N::Extensions::ExtensionBase(call=" << (void*)ex.call << ", data=" << ex.data << ")";
    return dbg.space();
}