#pragma once

#include <QtCore>
#include <unordered_map>

namespace N {

namespace P {


typedef void (*QtMetTypeCall)(quint8 operation, size_t argc, void **argv, void *data);

struct TypeIdData;

}  // namespace P

using TypeId = P::TypeIdData*;

template<class T> TypeId qTypeId();

}  // namespace N

QDebug operator<<(QDebug &dbg, N::TypeId id);
