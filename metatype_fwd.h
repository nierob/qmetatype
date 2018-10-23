#pragma once

#include <QtCore>
#include <unordered_map>

namespace N {
namespace P {
typedef void (*QtMetTypeCall)(quint8 operation, size_t argc, void **argv, void *data);
struct TypeIdData;
}  // namespace P

using TypeId = P::TypeIdData*;

namespace Extensions
{
    struct ExtensionBase
    {
        N::P::QtMetTypeCall call = nullptr;
        void *data = nullptr;

        void operator()(quint8 operation, size_t argc, void** argv) const
        {
            call(operation, argc, argv, data);
        }
    };
}

namespace P {

struct TypeIdData
{
    // TODO make it thread safe
    // TODO Split API into public and private as it seems tht TypeIdData needs to be public
    // otherwise we can not construct type at runtime
    const size_t extCount = 0;
    std::unordered_map<N::TypeId, N::Extensions::ExtensionBase> knownExtensions;

    bool call(N::TypeId extensionId, quint8 operation, size_t argc, void **argv);
    bool isExtensionKnown(N::TypeId extensionId) const;
    template<class... Extensions>
    inline void registerExtensions(Extensions... extensions);
    inline void registerExtension(N::TypeId extensionId, N::Extensions::ExtensionBase extension);
};

template<size_t InitialExtensionsCount>
struct TypeIdDataExtended: public TypeIdData
{
    struct ExtArray
    {
        TypeId id;
        N::Extensions::ExtensionBase extension;
    } initialExtensions[InitialExtensionsCount];
};

}  // namespace P

template<class T> TypeId qTypeId();
template<class T, class Extension, class... Extensions> TypeId qTypeId();

}  // namespace N

QDebug operator<<(QDebug &dbg, N::TypeId id);
