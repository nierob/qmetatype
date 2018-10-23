#include <QtCore>

#include "metatype_impl.h"
#include "extensions/name.h"

bool N::P::TypeIdData::call(TypeId extensionId, quint8 operation, size_t argc, void **argv)
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

bool N::P::TypeIdData::isExtensionKnown(TypeId extensionId) const
{
    return knownExtensions.find(extensionId) != knownExtensions.end();
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