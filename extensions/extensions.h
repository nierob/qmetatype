#pragma once

#include <QtCore>
#include "metatype_fwd.h"

namespace N {

namespace Extensions
{

namespace P {
template<class QtTypeToIntrospect> constexpr std::string_view typeNameFromType()
{
    constexpr size_t offset = sizeof("constexpr std::string_view N::Extensions::P::typeNameFromType() [with QtTypeToIntrospect = ") - 1;
    constexpr size_t tail = sizeof("; std::string_view = std::basic_string_view<char>]");
    // As for gcc this code is storing the full signature in the code because, we really would need to shorten the name or find another
    // way to cut the size, maybe a trick with const char [] could help?
    constexpr size_t len = sizeof(__PRETTY_FUNCTION__);
    return std::string_view{__PRETTY_FUNCTION__ + offset, len - offset - tail};
}

template<class T, class Extension, class... Extensions>
void PreRegisterAction()
{
    Extension::template PreRegisterAction<T>();
    if constexpr (bool(sizeof...(Extensions)))
        PreRegisterAction<T, Extensions...>();
}

template<class T, class Extension, class... Extensions>
void PostRegisterAction(TypeId id)
{
    Extension::template PostRegisterAction<T>(id);
    if constexpr (bool(sizeof...(Extensions)))
        PostRegisterAction<T, Extensions...>(id);
}

} // namespace P

template<class Extension>
class Ex
{
    static inline size_t offset()
    {
        // Used only for unique address range, allows 8 operations
        // TODO double check if there is no better option then alignment
        Q_DECL_ALIGN(8) static char offset_;
        return (size_t)&offset_;
    }
public:
    typedef Ex<Extension> Base;
    static inline bool isAccepted(size_t tag)
    {
         return tag == offset();
    }
    static void Call(TypeId id, quint8 operation, size_t argc, void **argv)
    {
        if (!id->call(operation + offset(), argc, argv)) {
            auto extensionName = P::typeNameFromType<Extension>();
            // TODO depending on our name registration strategy we can get the type name too. Otherwise we would could fallback
            // to dladdr as the typeId is a function pointer so we may be able to parse it if debug symbols are there.
            qWarning() << QLatin1String("WARN Requested metatype extension '") +
                          QString::fromLocal8Bit(extensionName.data(), extensionName.length()) + QLatin1String("' is not registed for this type:")
                          << (void*)id;
        }
    }
    template<class T> constexpr static void PreRegisterAction() {}
    template<class T> constexpr static void PostRegisterAction(TypeId id) { Q_UNUSED(id); }
};

} // namespace Extensions

} // namespace N