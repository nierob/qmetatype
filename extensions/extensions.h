#pragma once

#include <QtCore>
#include "metatype_fwd.h"
#include <tuple>
#include <utility>

namespace N {

namespace Extensions
{

    using Tag = size_t;

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

struct ExtensionBase
{
    N::P::QtMetTypeCall call = nullptr;
    void *data = nullptr;

    void operator()(quint8 operation, size_t argc, void** argv)
    {
        call(operation, argc, argv, data);
    }
};

template<class Extension>
class Ex : public ExtensionBase
{
public:
    typedef Ex<Extension> Base;

    static inline Tag tag()
    {
        // Used only for unique address range, allows 8 operations
        // TODO double check if there is no better option then alignment
        // TODO can we change it to be typeId? That would be meta cool
        Q_DECL_ALIGN(8) static char offset_;
        return (Tag)&offset_;
    }

    template<class T> constexpr static void PreRegisterAction() {}
    template<class T> constexpr static void PostRegisterAction(TypeId id) { Q_UNUSED(id); }

    static void Call(TypeId id, quint8 operation, size_t argc, void **argv);

    template<class T>
    static inline Extension createExtension()
    {
        return std::move(createFromBasicData<T>());
    }
    template<class T=void>
    static inline Extension createFromBasicData(decltype(ExtensionBase::call) call = Extension::template Call<T>, void *data = nullptr)
    {
        Extension extension;
        extension.call = call;
        extension.data = data;
        return std::move(extension);
    }
};

template<class TypeData>
TypeId initializeType(TypeData *data)
{
    static_assert(std::is_base_of_v<N::P::TypeIdData, TypeData>);
    auto registerExtension = [data](auto&... ex) {
        (data->registerExtensions(ex.createExtensionBase(data)), ...);
    };
    std::apply(registerExtension, data->extensions);
    return data;
}
} // namespace Extensions

} // namespace N
QDebug operator<<(QDebug &dbg, const N::Extensions::ExtensionBase &ex);
