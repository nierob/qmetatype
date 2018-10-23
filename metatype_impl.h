# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"
#include "extensions/name.h"

namespace N {

template<class Extension> TypeId Extensions::Ex<Extension>::typeId()
{
    // Used for unique address range and extensions handling should allows X operations
    // TODO check operation count and eforce it
    // TODO double check if there is no better option then alignment
    return qTypeId<Extension, Name_hash>();
}

namespace P {

bool TypeIdData::call(TypeId extensionId, quint8 operation, size_t argc, void **argv)
{
    using DataExtension = TypeIdDataExtended<1>::ExtArray;
    auto fakeTypedItData = static_cast<TypeIdDataExtended<1>*>(this); // Just to get the offset
    const auto firstInitialExtension = fakeTypedItData->initialExtensions;
    for (auto extension = firstInitialExtension; extension != firstInitialExtension + extCount; ++extension) {
        if (extension->id == extensionId) {
            extension->extension(operation, argc, argv);
            return true;
        }
    }
    auto exIter = knownExtensions.find(extensionId);
    if (exIter == knownExtensions.cend())
        return false;
    exIter->second(operation, argc, argv);
    return true;
}

bool TypeIdData::isExtensionKnown(TypeId extensionId) const
{
    return knownExtensions.find(extensionId) != knownExtensions.end();
}

template<class Extension, class... Extensions>
void TypeIdData::registerExtensions(Extension extension, Extensions... extensions)
{
    registerExtension(Extension::typeId(), extension);
    if constexpr (bool(sizeof...(Extensions)))
        registerExtensions(extensions...);
}

void TypeIdData::registerExtension(TypeId extensionId, Extensions::ExtensionBase extension)
{
    knownExtensions.try_emplace(extensionId, extension);
}

}  // namespace P

template<class T>
TypeId qTypeIdImpl(TypeId newId)
{
    static TypeId id = newId;
    return id;
}

template<class T, class Extension, class... Extensions>
TypeId qTypeId()
{
    N::Extensions::P::PreRegisterAction<T, Extension, Extensions...>();
    using ExtendedTypeIdData = N::P::TypeIdDataExtended<sizeof...(Extensions) + 1>;
    static ExtendedTypeIdData typeData{{sizeof...(Extensions) + 1, {}},
                                       {{Extension::typeId(), Extension::template createExtension<T>()},
                                        {Extensions::typeId(), Extensions::template createExtension<T>()}...}};
    auto id = qTypeIdImpl<T>(&typeData);
    if (id != &typeData) {
        // We are adding extensions
        // TODO try to re-use typeData
        id->registerExtensions(Extension::template createExtension<T>(), Extensions::template createExtension<T>()...);
    }
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
inline void Extensions::Ex<Extension>::Call(TypeId id, quint8 operation, size_t argc, void **argv)
{
    if (!id->call(typeId(), operation, argc, argv)) {
        auto extensionName = Name_dlsym::name(qTypeId<Extension, Name_dlsym>());
        // TODO depending on our name registration strategy we can get the type name too. Otherwise we would could fallback
        // to dladdr as the typeId is a function pointer so we may be able to parse it if debug symbols are there.
        // TODO Think when we need the warning, sometimes we want just to probe if it is possible to do stuff
        qWarning() << QLatin1String("WARN Requested metatype extension ") + extensionName + QLatin1String(" is not registed for this type:")
                      << id;
    }
}

}  // namespace N
