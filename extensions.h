#pragma once

#include <QtCore>
#include "metatype_fwd.h"

namespace N {

namespace Extensions
{

template<class T>
struct Ex
{
    typedef Ex<T>  Base;
    // Used only for unique address range, allows 8 operations
    // TODO double check if there is no better option then alignment
    Q_DECL_ALIGN(8) static char offset_;
    static inline size_t offset() { return (size_t)&offset_; }
    static void* Call(TypeId id, quint8 operation, size_t argc, void **argv)
    {
        auto metaTypeCall = static_cast<P::QtMetTypeCall>(id);
        return metaTypeCall(operation + offset(), argc, argv);
    }
};

template<class T> char Ex<T>::offset_;

struct Allocation : public Ex<Allocation>
{
    enum Operations {Create, Destroy, Construct, Destruct, SizeOf, AlignOf};

    template<class T>
    static void* Call(size_t functionType, size_t argc, void **argv)
    {
        switch (functionType)
        {
            case Create: {
                Q_ASSERT(argc <= 1);
                if (!argc)
                    return new T{};
                auto copy = static_cast<const T*>(argv[0]);
                return new T{*copy};
            }
            case Destroy: {
                Q_ASSERT(argc == 1);
                auto obj = static_cast<T*>(argv[0]);
                delete obj;
                return obj;
            }
            case Construct: {
                Q_ASSERT(argc && argc <= 2);
                if (argc == 1)
                    return new (argv[0]) T{};
                auto copy = static_cast<const T*>(argv[0]);
                return new (argv[0]) T{*copy};
            }
            case Destruct: {
                Q_ASSERT(argc == 1);
                auto obj = static_cast<T*>(argv[0]);
                obj->~T();
                return obj;
            }
            case SizeOf: Q_ASSERT(!argc); return (void*)sizeof(T);
            case AlignOf: Q_ASSERT(!argc); return (void*)alignof(T);
            default: return nullptr;
        }
    }

    static size_t sizeOf(TypeId id)
    {
        return (size_t)Base::Call(id, SizeOf, 0, nullptr);
    }
    static size_t alignOf(TypeId id)
    {
        return (size_t)Base::Call(id, AlignOf, 0, nullptr);
    }
    static void* create(TypeId id, const void *copy = nullptr)
    {
        void *argv[] = {const_cast<void*>(copy)};
        return Base::Call(id, Create, copy ? 1 : 0, argv);
    }
    static void destroy(TypeId id, void *obj)
    {
        void *argv[] = {obj};
        Base::Call(id, Destroy, 1, argv);
    }
    static void* construct(TypeId id, void *where, const void *copy = nullptr)
    {
        void *argv[] = {where, const_cast<void*>(copy)};
        return Base::Call(id, Construct, copy ? 2 : 1, argv);
    }
    static void destruct(TypeId id, void *obj)
    {
        void *argv[] = {obj};
        Base::Call(id, Destruct, 1, argv);
    }
};


namespace P {

template<class T>
class HasStreamOperator
{
    struct Yes { char unused[1];};
    struct No { char unused[2];};
    Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));
    template <class C>
    static decltype(std::declval<QDataStream &>().operator>>(std::declval<C &>()), Yes()) load(int);
    template <class C>
    static decltype(operator>>(std::declval<QDataStream &>(), std::declval<C &>()), Yes()) load(int);
    template <class C>
    static No load(...);
    template <class C>
    static decltype(operator<<(std::declval<QDataStream &>(), std::declval<const C &>()), Yes()) save(int);
    template <class C>
    static decltype(std::declval<QDataStream &>().operator<<(std::declval<const C &>()), Yes()) save(int);
    template <class C>
    static No save(...);
    static constexpr bool LoadValue = (sizeof(load<T>(0)) == sizeof(Yes));
    static constexpr bool SaveValue = (sizeof(save<T>(0)) == sizeof(Yes));
public:
    static constexpr bool Value = LoadValue && SaveValue;
};

} // namespace P

template<class T, bool = P::HasStreamOperator<T>::Value>
class DataStreamImpl
{
public:
    static inline void* Call(size_t functionType, size_t argc, void **argv);
};
template<class T>
class DataStreamImpl<T, false>
{
public:
    static void* Call(size_t functionType, size_t argc, void **argv)
    {
        Q_UNUSED(functionType);
        Q_UNUSED(argc);
        Q_UNUSED(argv);
        return nullptr;
    }
};

struct DataStream: public Ex<DataStream>
{
    enum Operations {SaveData, LoadData};
    template<class T>
    static void* Call(size_t functionType, size_t argc, void **argv)
    {
        return DataStreamImpl<T>::Call(functionType, argc, argv);
    }
};

template<class T, bool hasStreamOperator>
inline void* DataStreamImpl<T, hasStreamOperator>::Call(size_t functionType, size_t argc, void **argv)
{
    switch (functionType)
    {
        case DataStream::SaveData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDataStream*>(argv[0]);
            auto data = static_cast<const T*>(argv[1]);
            *stream << *data;
            return stream;
        } case DataStream::LoadData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDataStream*>(argv[0]);
            auto data = static_cast<T*>(argv[1]);
            *stream >> *data;
            return stream;
        }
        default: return nullptr;
    }
}

} // namespace Extensions

} // namespace N