#pragma once

#include <QtCore>
#include "metatype_fwd.h"

namespace N {

namespace Extensions
{

namespace P {
template<class QtTypeToIntrospect> constexpr std::string_view typeNameFromType()
{
    constexpr size_t offset = strlen("constexpr std::string_view N::Extensions::P::typeNameFromType() [with QtTypeToIntrospect = ");
    constexpr size_t tail = strlen("; std::string_view = std::basic_string_view<char>]");
    // TODO That could be const expression but apparently it is not
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66639 sugests that it works on clang and msvc.
    // As for gcc this code is storing the full signature in the code because, we really would need to shorten the name.
    /*constexpr*/ size_t len = strlen(__PRETTY_FUNCTION__);
    return {Q_FUNC_INFO + offset, len - offset - tail};
}

template<class T, class Extension, class... Extensions>
void PreRegisterAction()
{
    Extension::template PreRegisterAction<T>();
    if constexpr (sizeof...(Extensions))
        PreRegisterAction<T, Extensions...>();
}

template<class T, class Extension, class... Extensions>
void PostRegisterAction(TypeId id)
{
    Extension::template PostRegisterAction<T>(id);
    if constexpr (sizeof...(Extensions))
        PostRegisterAction<T, Extensions...>(id);
}

} // namespace P

template<class Extension>
class Ex
{
    // Used only for unique address range, allows 8 operations
    // TODO double check if there is no better option then alignment
    Q_DECL_ALIGN(8) static char offset_;
    static inline size_t offset() { return (size_t)&offset_; }
public:
    typedef Ex<Extension> Base;
    static inline bool isAccepted(size_t tag)
    {
         return tag == (size_t)&offset_;
    }
    static void Call(TypeId metaTypeCall, quint8 operation, size_t argc, void **argv)
    {
        if (!metaTypeCall(operation + offset(), argc, argv)) {
            auto extensionName = P::typeNameFromType<Extension>();
            // TODO depending on our name registration strategy we can get the type name too. Otherwise we would could fallback
            // to dladdr as the typeId is a function pointer so we may be able to parse it if debug symbols are there.
            qWarning() << QLatin1String("WARN Requested metatype extension '") +
                          QString::fromLocal8Bit(extensionName.data(), extensionName.length()) + QLatin1String("' is not registed for this type:")
                          << (void*)metaTypeCall;
        }
    }
    template<class T> constexpr static void PreRegisterAction() {}
    template<class T> constexpr static void PostRegisterAction(TypeId id) { Q_UNUSED(id); }
};

template<class Extension> Q_DECL_ALIGN(8) char Ex<Extension>::offset_;

} // namespace Extensions

} // namespace N