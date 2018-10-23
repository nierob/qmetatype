#pragma once

#include <QtCore>
#include "metatype_fwd.h"
#include <tuple>
#include <utility>
#include <string_view>

namespace N {

namespace Extensions
{
template<class Extension>
class Ex : public ExtensionBase
{
public:
    typedef Ex<Extension> Base;

    static inline TypeId typeId();

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

template<class T>
struct DefaultTypeIdHandleDeleter: public std::default_delete<T>
{
    void operator()(N::P::TypeIdData* ptr) const
    {
        std::default_delete<T>::operator()(static_cast<T*>(ptr));
    }
};

struct EmptyTypeIdHandleDeleter
{
    void operator()(N::P::TypeIdData*) const {}
};


template<class Deleter>
struct TypeIdHandle: private std::unique_ptr<N::P::TypeIdData, Deleter>
{
    using Base = std::unique_ptr<N::P::TypeIdData, Deleter>;
    TypeIdHandle(TypeId id, Deleter deleter = Deleter()) : Base(id, deleter) {}
    TypeId id() { return Base::get(); }
};

template<class TypeData, class Deleter=DefaultTypeIdHandleDeleter<TypeData>>
TypeIdHandle<Deleter> initializeType(TypeData *data)
{
    // TODO we probably need to add way to pass deleter to the TypeIdHandle
    static_assert(std::is_base_of_v<N::P::TypeIdData, TypeData>);
    // TODO Add additional check:it has to be the first class because of the up cast in TypeIdHandle deleter
    auto registerExtension = [data](auto&... ex) {
        (data->registerExtensions(ex.createExtensionBase(data)), ...);
    };
    std::apply(registerExtension, data->extensions);
    return data;
}
} // namespace Extensions

} // namespace N
QDebug operator<<(QDebug &dbg, const N::Extensions::ExtensionBase &ex);

template<class T>
QDebug operator<<(QDebug &dbg, const N::Extensions::TypeIdHandle<T> &handle)
{
    dbg.nospace() << "TypeHandle(" << handle.id << ")";
    return dbg.space();
}
