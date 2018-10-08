#pragma once

#include <QtCore>

namespace N {

namespace P {


typedef bool (*QtMetTypeCall)(size_t functionType, size_t argc, void **argv, void *data);
template<class T> bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv, void *data);

struct TypeIdData
{
    void *data;
    P::QtMetTypeCall _call;
    bool call(size_t functionType, size_t argc, void **argv)
    {
        return _call(functionType, argc, argv, data);
    }
    bool isExtensionKnown(QtMetTypeCall extension) const
    {
        // This check is a bit too broad as order of the extensions should not matter
        // and this allows to re-register some Extensions multiple times.
        return _call == extension;
    }
};

}  // namespace P

using TypeId = P::TypeIdData*;

template<class T> TypeId qTypeId();

}  // namespace N

