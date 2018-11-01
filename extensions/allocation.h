/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once
#include <new>
#include <memory>
#include <type_traits>

#include "extensions.h"

/*!
* Allocation extension
*
* It allows to create and destroy instances of the given type. Used everywhere where such operations is required,
* well known places in Qt:
* - MetaObject system
* - QML in many places
* - QVariant and through it's interface a lot of other usages comes into play (I guess sql, activeqt, dbus)
* - Remote objects
*/
namespace N::Extensions
{

/*!
* \brief The Allocation struct
*
* Implements base for QMetaType::create / construct / delete / destruct / size and maybe others.
* Nothing spectacular here, it just worked out of the box. As opposite to the current solution
* it automatically picks the right new operator and would have no problem with higher alignments.
*
* From API perspecitve it makes create function a bit safer by using unique_ptr with the right deleter.
*/
struct Allocation : public Ex<Allocation>
{
    Q_STATIC_ASSERT(sizeof(void*) >= sizeof(size_t));

    template<class T, OperationMode Mode = OperationMode::Query> constexpr static bool WorksForType()
    {
        constexpr auto isNotVoid= !std::is_same_v<T, void>;
        static_assert(Mode != OperationMode::AssertTrue || isNotVoid, "Void can not be used with Allocation extension.");
        constexpr auto isDefaultConstructible = std::is_default_constructible_v<T>;
        static_assert(Mode != OperationMode::AssertTrue || isDefaultConstructible, "Type needs to be default constructible");
        constexpr auto isCopyConstructible = std::is_copy_constructible_v<T>;
        static_assert(Mode != OperationMode::AssertTrue || isCopyConstructible, "Type needs to be copy constructible");
        constexpr auto isDestructible = std::is_destructible_v<T>;
        static_assert(Mode != OperationMode::AssertTrue || isDestructible, "Type needs to be destructible");
        return isDefaultConstructible && isCopyConstructible && isDestructible && isNotVoid;
    }

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
                // TODO clang doesn't like that, but it should... anyway it is not a show stopper
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
    /*!
     * \brief The RuntimeData struct
     * Helper for creating runtime types.
     *
     * TODO / UNIMPLEMENTED it needs to be extended with functions wrapping callback, otherwise it is useless.
     * On the other hand complexity of the class would grow significantly therefore maybe it is just better
     * to expose some way to override RuntimeCall.
     */
    struct RuntimeData
    {
        std::size_t size;
        std::align_val_t align;
        Allocation createExtensionBase(TypeId id)
        {
            Q_UNUSED(id);
            return {{{RuntimeCall, this}}};
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

    struct Deleter
    {
        TypeId id;
        void operator()(void *ptr) const
        {
            destroy(id, ptr);
        }
    };

    static std::unique_ptr<void, Deleter> create(TypeId id, const void *copy = nullptr)
    {
        void *argv[] = {nullptr, const_cast<void*>(copy)};
        Base::Call(id, Create, copy ? 2 : 1, argv);
        return std::unique_ptr<void, Deleter>{argv[0], Deleter{id}};
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
