# pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"

namespace N {

namespace P {

constexpr size_t RegisterExtension = 0;
struct ExtensionNode
{
    QAtomicPointer<ExtensionNode> next;
    bool (*Call)(size_t functionType, size_t argc, void **argv);
    bool (*isAccepted)(size_t tag);
    static bool CallIfAcceptedInChain(ExtensionNode *node, size_t extensionTag, size_t functionType, size_t argc, void **argv);
    static void AppendToTheChain(QAtomicPointer<ExtensionNode> &first, ExtensionNode *newNode);
};

template<class T>
bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = (std::numeric_limits<size_t>::max() >> 3) << 3;
    auto extensionTag = functionType & extensionMask;
    auto functionId = functionType ^ extensionTag;

    static QAtomicPointer<ExtensionNode> first{};
    if (ExtensionNode::CallIfAcceptedInChain(first.load(), extensionTag, functionType, argc, argv))
        return true;
    if (functionType == RegisterExtension) {
        Q_ASSERT(argc == 1);
        ExtensionNode *newNode = static_cast<ExtensionNode*>(argv[0]);
        ExtensionNode::AppendToTheChain(first, newNode);
    }
    return false;
}

template<class T, class Ext, class... Exts>
bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = (std::numeric_limits<size_t>::max() >> 3) << 3;
    auto extensionTag = functionType & extensionMask;
    auto functionId = functionType ^ extensionTag;
    if (Ext::isAccepted(extensionTag)) {
        Ext::template Call<T>(functionId, argc, argv);
        return true;
    }
    return metaTypeCallImpl<T, Exts...>(functionType, argc, argv);
}

template<class Extension, class... Extensions>
bool areExtensionsAccepting(size_t tag) {
    if (Extension::isAccepted(tag))
        return true;
    if constexpr (sizeof...(Extensions))
        return areExtensionsAccepting<Extensions...>(tag);
    return false;
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
    auto proposedTypeInfo = P::metaTypeCallImpl<T, Extension, Extensions...>;
    auto typeInfo = qRegisterTypeImpl<T>(proposedTypeInfo);
    if (typeInfo != proposedTypeInfo) {
        // This check is a bit too broad as order of the extensions should not matter
        // and this allows to re-register some Extensions multiple times.
        static N::P::ExtensionNode node {nullptr, proposedTypeInfo, N::P::areExtensionsAccepting<Extension, Extensions...>};
        void *argv[] = {&node};
        typeInfo(N::P::RegisterExtension, 1, argv);
    }
    return typeInfo;
}

template<class T>
TypeId qRegisterType()
{
    return qRegisterType<T, Extensions::Allocation, Extensions::DataStream>();
}

}  // namespace N