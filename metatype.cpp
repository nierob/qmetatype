#include <QtCore>

#include "metatype_impl.h"

bool N::P::ExtensionNode::CallIfAcceptedInChain(ExtensionNode *node, size_t extensionTag, size_t functionType, size_t argc, void **argv)
{
    while (node) {
        if (node->isAccepted(extensionTag)) {
            node->Call(functionType, argc, argv);
            return true;
        }
        node = node->next.load();
    }
    return false;
}

void N::P::ExtensionNode::AppendToTheChain(QAtomicPointer<ExtensionNode> &first, ExtensionNode *newNode)
{
    ExtensionNode *node = nullptr;
    if (!first.testAndSetOrdered(nullptr, newNode, node))
            while (node->next.testAndSetOrdered(nullptr, newNode, node)) {}
    // It may happen that the same node is being inserted from many threads
    // so the above algorithm could potentially cause an insertion of
    // self-referencing node (inifinit loop). Just in case let's try to [un]break it.
    newNode->next.testAndSetRelease(newNode, nullptr);
}

bool N::P::metaTypeCallImpl(QAtomicPointer<ExtensionNode> &first, size_t functionType, size_t argc, void **argv)
{
    // TODO this bit fidling should be in automatically sync with alignof(Extensions::Ex<void>::offset_)
    constexpr auto extensionMask = (std::numeric_limits<size_t>::max() >> 3) << 3;
    auto extensionTag = functionType & extensionMask;

    if (ExtensionNode::CallIfAcceptedInChain(first.load(), extensionTag, functionType, argc, argv))
        return true;
    if (functionType == RegisterExtension) {
        Q_ASSERT(argc == 1);
        ExtensionNode *newNode = static_cast<ExtensionNode*>(argv[0]);
        ExtensionNode::AppendToTheChain(first, newNode);
    }
    return false;
}