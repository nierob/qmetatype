#pragma once

#include <QtCore>
#include "metatype_fwd.h"

namespace N {

namespace Extensions
{

template<class T>
class Ex
{
    // Used only for unique address range, allows 8 operations
    // TODO double check if there is no better option then alignment
    Q_DECL_ALIGN(8) static char offset_;
    static inline size_t offset() { return (size_t)&offset_; }
public:
    typedef Ex<T> Base;
    static inline bool isAccepted(size_t tag)
    {
         return tag == (size_t)&offset_;
    }
    static void Call(TypeId id, quint8 operation, size_t argc, void **argv)
    {
        auto metaTypeCall = static_cast<P::QtMetTypeCall>(id);
        if (!metaTypeCall(operation + offset(), argc, argv))
            qWarning() << "WARN Requested metatype extension is not registed for this type"; // TODO we can do better msg.
    }
};

template<class T> char Ex<T>::offset_;

} // namespace Extensions

} // namespace N