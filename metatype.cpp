#include <QtCore>

#include "metatype_impl.h"
#include "extensions/name.h"

namespace {
using DataExtension = N::P::TypeIdDataExtended<1>::ExtArray;
static const DataExtension *findInitialExtensionInTypeIdData(const N::P::TypeIdData *data, N::TypeId extensionId)
{
    auto fakeTypedItData = static_cast<const N::P::TypeIdDataExtended<1>*>(data); // Just to get the offset
    auto firstInitialExtension = fakeTypedItData->initialExtensions;
    for (auto extension = firstInitialExtension; extension != firstInitialExtension + data->extCount; ++extension) {
        if (extension->id == extensionId)
            return extension;
    }
    return nullptr;
}
} // namespace

bool N::P::TypeIdData::call(TypeId extensionId, quint8 operation, size_t argc, void **argv)
{
    if (auto extension = findInitialExtensionInTypeIdData(this, extensionId)) {
        extension->extension(operation, argc, argv);
        return true;
    }
    auto exIter = knownExtensions.find(extensionId);
    if (exIter == knownExtensions.cend())
        return false;
    exIter->second(operation, argc, argv);
    return true;
}

bool N::P::TypeIdData::isExtensionKnown(TypeId extensionId) const
{
    return findInitialExtensionInTypeIdData(this, extensionId) || knownExtensions.find(extensionId) != knownExtensions.end();
}

QDebug operator<<(QDebug &dbg, N::TypeId id)
{
    if (!id) {
        return dbg << "TypeId(Unknown)";
    }
    // TODO allow warning less name access
    auto typeName = N::Extensions::Name_hash::name(id);
    dbg.nospace() << "TypeId(";
    if (!typeName.isEmpty())
        dbg << typeName << ',' << ' ';
    dbg << (void*)id << ")";
    return dbg.space();
}