# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions.h"

namespace N {

namespace P {

template<class T>
void* metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = ((std::numeric_limits<size_t>::max() >> 3) << 3);
    auto extensionOffset = functionType & extensionMask;
    auto functionId = functionType ^ extensionOffset;
    {
        using Ext = Extensions::Allocation;
        if (extensionOffset == Ext::offset())
            return Ext::Call<T>(functionId, argc, argv);
    }
    {
        using Ext = Extensions::DataStream;
        if (extensionOffset == Ext::offset())
            return Ext::Call<T>(functionId, argc, argv);
    }
    // TODO improve the message with the type names
    qWarning() << "Metatype extension was not registerd for this type";
    return nullptr;
}

}  // namespace P

template<class T, >
TypeId qRegisterType()
{
    static P::QtMetTypeCall typeInfo{P::metaTypeCallImpl<T>};
    return typeInfo;
}

}  // namespace N