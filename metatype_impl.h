# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions.h"

namespace N {

namespace P {

void* metaTypeCallImplTerminator(const char *name);

template<class T>
void* metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    Q_UNUSED(functionType);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    return metaTypeCallImplTerminator(__PRETTY_FUNCTION__); // TODO Add parsing
}

template<class T, class Ext, class... Exts>
void* metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = (std::numeric_limits<size_t>::max() >> 3) << 3;
    auto extensionOffset = functionType & extensionMask;
    auto functionId = functionType ^ extensionOffset;
    if (extensionOffset == Ext::offset())
        return Ext::template Call<T>(functionId, argc, argv);
    return metaTypeCallImpl<T, Exts...>(functionType, argc, argv);
}

}  // namespace P

template<class T>
TypeId qRegisterTypeImpl(P::QtMetTypeCall info)
{
    static P::QtMetTypeCall typeInfo{info};
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