#pragma once

#include <QtCore>

namespace N {

namespace P {


typedef bool (*QtMetTypeCall)(size_t functionType, size_t argc, void **argv);
template<class T> bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv);

struct TypeIdData
{
    void *data;
    P::QtMetTypeCall call;
};

}  // namespace P

using TypeId = P::TypeIdData*;

template<class T> TypeId qTypeId();

}  // namespace N

