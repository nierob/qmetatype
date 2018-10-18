# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"
#include "extensions/name.h"

namespace N {

namespace P {

struct TypeIdData
{
    // TODO make it thread safe
    // TODO it can be partially filled on compile time, maybe the map is not the right struct
    // the point here is that we need a low cost mapping
    // TODO Split API into public and private as it seems tht TypeIdData needs to be private
    // otherwise we can not construct type at runtime
    std::unordered_map<N::Extensions::Tag, N::Extensions::ExtensionBase> knownExtensions;

    bool call(N::Extensions::Tag tag, quint8 operation, size_t argc, void **argv)
    {
        auto exIter = knownExtensions.find(tag);
        if (exIter == knownExtensions.cend())
            return false;
        exIter->second(operation, argc, argv);
        return true;
    }

    bool isExtensionKnown(N::Extensions::Tag tag) const
    {
        return knownExtensions.find(tag) != knownExtensions.end();
    }

    template<class Extension, class... Extensions>
    void registerExtensions(Extension extension, Extensions... extensions)
    {
        registerExtension(Extension::tag(), extension);
        if constexpr (bool(sizeof...(Extensions)))
            registerExtensions(extensions...);
    }

    void registerExtension(N::Extensions::Tag tag, N::Extensions::ExtensionBase extension)
    {
        knownExtensions.try_emplace(tag, extension);
    }
};

}  // namespace P

template<class T>
TypeId qTypeIdImpl()
{
    static P::TypeIdData typeData;
    return &typeData;
}

template<class T, class Extension, class... Extensions>
TypeId qTypeId()
{
    N::Extensions::P::PreRegisterAction<T, Extension, Extensions...>();
    auto id = qTypeIdImpl<T>();
    id->registerExtensions(Extension::template createExtension<T>(), Extensions::template createExtension<T>()...);
    N::Extensions::P::PostRegisterAction<T, Extension, Extensions...>(id);
    return id;
}

template<class T>
TypeId qTypeId()
{
    // Register default stuff, Qt should define minimal useful set, DataStream
    // is probably not in :-)
    // Every usage of metatype can call qTypeId with own minimal set of
    // extensions.
    return qTypeId<T, Extensions::Allocation, Extensions::DataStream, Extensions::Name_dlsym, Extensions::Name_hash>();
}

template<class Extension>
inline void N::Extensions::Ex<Extension>::Call(TypeId id, quint8 operation, size_t argc, void **argv)
{
    if (!id->call(tag(), operation, argc, argv)) {
        auto extensionName = P::typeNameFromType<Extension>();
        // TODO depending on our name registration strategy we can get the type name too. Otherwise we would could fallback
        // to dladdr as the typeId is a function pointer so we may be able to parse it if debug symbols are there.
        // TODO Think when we need the warning, sometimes we want just to probe if it is possible to do stuff
        qWarning() << QLatin1String("WARN Requested metatype extension '") +
                      QString::fromLocal8Bit(extensionName.data(), extensionName.length()) + QLatin1String("' is not registed for this type:")
                      << (void*)id;
    }
}


}  // namespace N
