#pragma once
#include "extensions.h"
#include <new>

namespace N::Extensions
{

struct Allocation : public Ex<Allocation>
{
    Q_STATIC_ASSERT(sizeof(void*) >= sizeof(size_t));

    static void RuntimeCall(quint8 operation, size_t argc, void **argv, void *data)
    {
        // TODO Mybe we should also have some init functions in the RuntimeData
        auto typeData = static_cast<RuntimeData*>(data);
        switch (operation) {
            case Create: {
                Q_ASSERT_X(argc == 1, "N::Extensions::Allocation", "This type can not be copy constructed"); // TODO think about that case, do we need extra protocol?
                void *&result = argv[0];
                result = ::operator new(typeData->size, typeData->align);
                break;
            }
            case Destroy: {
                Q_ASSERT(argc == 1);
                ::operator delete(argv[0], typeData->size, typeData->align);
                break;
            }
            case SizeOf: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                result = (void*)(typeData->size);
                break;
            }
            case AlignOf: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                result = (void*)(typeData->align);
                break;
            }
        }
    }

public:
    enum Operations {Create, Destroy, Construct, Destruct, SizeOf, AlignOf};
    struct RuntimeData
    {
        std::size_t size;
        std::align_val_t align;
        Allocation createExtensionBase(TypeId id)
        {
            Q_UNUSED(id);
            return {RuntimeCall, this};
        }
    };

    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        switch (operation)
        {
            case Create: {
                Q_ASSERT(argc <= 2);
                void *&result = argv[0];
                if (argc == 2) {
                    auto copy = static_cast<const T*>(argv[1]);
                    result = new T{*copy};
                } else {
                    result = new T{};
                }
                break;
            }
            case Destroy: {
                Q_ASSERT(argc == 1);
                delete static_cast<T*>(argv[0]);
                break;
            }
            case Construct: {
                Q_ASSERT(argc > 1 && argc <= 3);
                void *&result = argv[0];
                auto storage = argv[1];
                if (argc == 2) {
                    result = new (storage) T{};
                } else {
                    auto copy = static_cast<const T*>(argv[2]);
                    result = new (storage) T{*copy};
                }
                break;
            }
            case Destruct: {
                Q_ASSERT(argc == 1);
                auto obj = static_cast<T*>(argv[0]);
                obj->~T();
                break;
            }
            case SizeOf: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                result = (void*)sizeof(T);
                break;
            }
            case AlignOf: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                result = (void*)alignof(T);
                break;
            }
        }
    }

    static size_t sizeOf(TypeId id)
    {
        void *argv[] = {nullptr};
        Base::Call(id, SizeOf, 1, argv);
        return (size_t)argv[0];
    }
    static size_t alignOf(TypeId id)
    {
        void *argv[] = {nullptr};
        Base::Call(id, AlignOf, 1, argv);
        return (size_t)argv[0];
    }
    static void* create(TypeId id, const void *copy = nullptr)
    {
        // TODO consider std::unique_ptr with destroy as deleter as a return type
        void *argv[] = {nullptr, const_cast<void*>(copy)};
        Base::Call(id, Create, copy ? 2 : 1, argv);
        return argv[0];
    }
    static void destroy(TypeId id, void *obj)
    {
        void *argv[] = {obj};
        Base::Call(id, Destroy, 1, argv);
    }
    static void* construct(TypeId id, void *where, const void *copy = nullptr)
    {
        void *argv[] = {nullptr, where, const_cast<void*>(copy)};
        Base::Call(id, Construct, copy ? 3 : 2, argv);
        return argv[0];
    }
    static void destruct(TypeId id, void *obj)
    {
        void *argv[] = {obj};
        Base::Call(id, Destruct, 1, argv);
    }
};

}  // N::Extensions
