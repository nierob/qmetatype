#pragma once

#include <QtCore>
#include "metatype_fwd.h"

namespace N {

namespace Extensions
{

namespace P {

template<class T, class Extension, class... Extensions>
void PreRegisterAction()
{
    Extension::template PreRegisterAction<T>();
    if constexpr (sizeof...(Extensions))
        PreRegisterAction<T, Extensions...>();
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
        if (!metaTypeCall(operation + offset(), argc, argv))
            qWarning() << "WARN Requested metatype extension is not registed for this type"; // TODO we can do better msg.
    }
    template<class T> constexpr static void PreRegisterAction() {}
};

template<class Extension> Q_DECL_ALIGN(8) char Ex<Extension>::offset_;

} // namespace Extensions

} // namespace N