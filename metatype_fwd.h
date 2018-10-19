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

        void operator()(quint8 operation, size_t argc, void** argv)
        {
            call(operation, argc, argv, data);
        }
    };
}

namespace P {

struct TypeIdData
{
    // TODO make it thread safe
    // TODO it can be partially filled on compile time, maybe the map is not the right struct
    // the point here is that we need a low cost mapping
    // TODO Split API into public and private as it seems tht TypeIdData needs to be public
    // otherwise we can not construct type at runtime
    std::unordered_map<N::TypeId, N::Extensions::ExtensionBase> knownExtensions;

    inline bool call(N::TypeId extensionId, quint8 operation, size_t argc, void **argv);
    inline bool isExtensionKnown(N::TypeId extensionId) const;
    template<class Extension, class... Extensions>
    inline void registerExtensions(Extension extension, Extensions... extensions);
    inline void registerExtension(N::TypeId extensionId, N::Extensions::ExtensionBase extension);

};

}  // namespace P

template<class T> TypeId qTypeId();
template<class T, class Extension, class... Extensions> TypeId qTypeId();

}  // namespace N

QDebug operator<<(QDebug &dbg, N::TypeId id);
