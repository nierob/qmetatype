#include <qt5/QtCore/qstring.h>
#include "name.h"

QReadWriteLock N::Extensions::Name_hash::lock;
QHash<QString, N::TypeId> N::Extensions::Name_hash::nameToId;
