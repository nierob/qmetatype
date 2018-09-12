# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"

namespace N {

namespace P {

template<class T>
bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    Q_UNUSED(functionType);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return false;
}

template<class T, class Ext, class... Exts>
bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = (std::numeric_limits<size_t>::max() >> 3) << 3;
    auto extensionOffset = functionType & extensionMask;
    auto functionId = functionType ^ extensionOffset;
    if (extensionOffset == Ext::offset()) {
        Ext::template Call<T>(functionId, argc, argv);
        return true;
    }
    return metaTypeCallImpl<T, Exts...>(functionType, argc, argv);
}

}  // namespace P

template<class T>
TypeId qRegisterTypeImpl(P::QtMetTypeCall info)
{
    static P::QtMetTypeCall typeInfo{info};
    if (typeInfo != info) {
        qDebug() << "UNIMPLEMENTED would register a new one extension for this type"; // TODO
    }
    return typeInfo;
}

template<class T, class Extension, class... Extensions>
TypeId qRegisterType()
{
    return qRegisterTypeImpl<T>(P::metaTypeCallImpl<T, Extension, Extensions...>);
}

template<class T>
TypeId qRegisterType()
{
    return qRegisterType<T, Extensions::Allocation, Extensions::DataStream>();
}

}  // namespace N