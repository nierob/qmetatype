#pragma once
#include "extensions.h"

namespace N::Extensions {
namespace P {
template<class T, class Stream>
class HasLoadStreamOperator
{
    struct Yes { char unused[1];};
    struct No { char unused[2];};
    Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));
    template <class C>
    static decltype(std::declval<Stream &>().operator>>(std::declval<C &>()), Yes()) load(int);
    template <class C>
    static decltype(operator>>(std::declval<Stream &>(), std::declval<C &>()), Yes()) load(int);
    template <class C>
    static No load(...);
public:
    static constexpr bool Value = (sizeof(load<T>(0)) == sizeof(Yes));;
};

template<class T, class Stream>
class HasSaveStreamOperator
{
    struct Yes { char unused[1];};
    struct No { char unused[2];};
    Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));
    template <class C>
    static decltype(operator<<(std::declval<Stream &>(), std::declval<const C &>()), Yes()) save(int);
    template <class C>
    static decltype(std::declval<Stream &>().operator<<(std::declval<const C &>()), Yes()) save(int);
    template <class C>
    static No save(...);
public:
    static constexpr bool Value = (sizeof(save<T>(0)) == sizeof(Yes));
};

template<class T, class Stream>
struct HasStreamOperator
{
    static constexpr bool LoadValue = HasLoadStreamOperator<T, Stream>::Value;
    static constexpr bool SaveValue = HasSaveStreamOperator<T, Stream>::Value;
    static constexpr bool Value = LoadValue && SaveValue;
};

} // namespace P

template<class T, bool = P::HasStreamOperator<T, QDataStream>::Value>
class DataStreamImpl
{
public:
    static inline void Call(quint8 operation, size_t argc, void **argv);
};
template<class T>
class DataStreamImpl<T, false>
{
public:
    static void Call(quint8 operation, size_t argc, void **argv)
    {
        Q_UNUSED(operation);
        Q_UNUSED(argc);
        Q_UNUSED(argv);
    }
};

struct DataStream: public Ex<DataStream>
{
    enum Operations {SaveData, LoadData};
    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_UNUSED(data);
        DataStreamImpl<T>::Call(operation, argc, argv);
    }
};

template<class T, bool hasStreamOperator>
inline void DataStreamImpl<T, hasStreamOperator>::Call(quint8 operation, size_t argc, void **argv)
{
    switch (operation)
    {
        case DataStream::SaveData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDataStream*>(argv[0]);
            auto data = static_cast<const T*>(argv[1]);
            *stream << *data;
            break;
        } case DataStream::LoadData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDataStream*>(argv[0]);
            auto data = static_cast<T*>(argv[1]);
            *stream >> *data;
            break;
        }
    }
}

template<class T, bool = P::HasSaveStreamOperator<T, QDebug>::Value>
class QDebugStreamImpl
{
public:
    static inline void Call(quint8 operation, size_t argc, void **argv);
};
template<class T>
class QDebugStreamImpl<T, false>
{
public:
    static void Call(quint8 operation, size_t argc, void **argv)
    {
        Q_UNUSED(operation);
        Q_UNUSED(argc);
        Q_UNUSED(argv);
    }
};

struct QDebugStream: public Ex<QDebugStream>
{
    enum Operations {SaveData};
    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        return QDebugStreamImpl<T>::Call(operation, argc, argv);
    }

    static void qDebugStream(TypeId id, QDebug &dbg, const void *data)
    {
        void *argv[] = {&dbg, const_cast<void*>(data)};
        Base::Call(id, SaveData, 2, argv);
    }
};

template<class T, bool hasStreamOperator>
inline void QDebugStreamImpl<T, hasStreamOperator>::Call(quint8 operation, size_t argc, void **argv)
{
    switch (operation)
    {
        case QDebugStream::SaveData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDebug*>(argv[0]);
            auto data = static_cast<const T*>(argv[1]);
            *stream << *data;
        }
    }
}

}  // N::Extensions
