# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"
#include "extensions/name.h"

namespace N {

namespace P {

constexpr size_t RegisterExtension = 0;
struct ExtensionNode
{
    QAtomicPointer<ExtensionNode> next;
    bool (*Call)(size_t functionType, size_t argc, void **argv, void *data);
    bool (*isAccepted)(size_t tag);
    static bool CallIfAcceptedInChain(ExtensionNode *node, size_t extensionTag, size_t functionType, size_t argc, void **argv, void *data);
    static void AppendToTheChain(QAtomicPointer<ExtensionNode> &first, ExtensionNode *newNode);
};

bool metaTypeCallImpl(QAtomicPointer<ExtensionNode> &first, size_t functionType, size_t argc, void **argv, void *data);
template<class T>
inline bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv, void *data)
{
    static QAtomicPointer<ExtensionNode> first{};
    return metaTypeCallImpl(first, functionType, argc, argv, data);
}

template<class T, class Ext, class... Exts>
bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv, void *data = nullptr)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = (std::numeric_limits<size_t>::max() >> 3) << 3;
    auto extensionTag = functionType & extensionMask;
    auto functionId = functionType ^ extensionTag;
    if (Ext::isAccepted(extensionTag)) {
        Ext::template Call<T>(functionId, argc, argv);
        return true;
    }
    return metaTypeCallImpl<T, Exts...>(functionType, argc, argv, data);
}

template<class Extension, class... Extensions>
bool areExtensionsAccepting(size_t tag) {
    if (Extension::isAccepted(tag))
        return true;
    if constexpr (bool(sizeof...(Extensions)))
        return areExtensionsAccepting<Extensions...>(tag);
    return false;
}

}  // namespace P

template<class T>
TypeId qTypeIdImpl(P::QtMetTypeCall info)
{
    static P::TypeIdData typeData{nullptr, info};
    return &typeData;
}

template<class T, class Extension, class... Extensions>
TypeId qTypeId()
{
    N::Extensions::P::PreRegisterAction<T, Extension, Extensions...>();
    auto proposedTypeInfo = P::metaTypeCallImpl<T, Extension, Extensions...>;
    auto typeInfo = qTypeIdImpl<T>(proposedTypeInfo);
    N::Extensions::P::PostRegisterAction<T, Extension, Extensions...>(typeInfo);
    if (!typeInfo->isExtensionKnown(proposedTypeInfo)) {
        static N::P::ExtensionNode node {nullptr, proposedTypeInfo, N::P::areExtensionsAccepting<Extension, Extensions...>};
        void *argv[] = {&node};
        typeInfo->call(N::P::RegisterExtension, 1, argv);
    }
    return typeInfo;
}

template<class T>
TypeId qTypeId()
{
    // Register default stuff, Qt should define minimal useful set, DataStream
    // is probably not in :-)
    // Every usage of metatype can call qTypeId with own minimal set of
    // extensions.
    return qTypeId<T, Extensions::Allocation, Extensions::DataStream, Extensions::Name_dlsym>();
}

}  // namespace N