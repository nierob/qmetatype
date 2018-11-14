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
#include "extensions.h"
#include <unordered_map>
#include <tuple>
#include <QtCore/qreadwritelock.h>

namespace N::Extensions
{

struct Convertion: public Ex<Convertion>
{
    typedef bool (*TypeLessConverter) (const void *fromData, void *toData);
    static void _convert(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, void *&result, const void *fromData, void *toData, TypeId toId);
    static void _register(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, void *&result, TypeId toId, TypeLessConverter converter);
    static void _unregister(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, TypeId toId);
    static void _isRegisterd(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, void *&result, TypeId toId);

public:
    enum Operations {Convert, RegisterConvertion, UnregisterConvertion, HasRegisteredConvertion};

    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        static QReadWriteLock lock;
        static std::unordered_map<TypeId, TypeLessConverter> converters;

        switch (operation)
        {
            case Convert: {
                Q_ASSERT(argc == 4);
                void *&result = argv[0];
                auto toId = static_cast<TypeId>(argv[3]);
                _convert(lock, converters, result, argv[1], argv[2], toId);
                break;
            }
            case RegisterConvertion: {
                Q_ASSERT(argc == 3);
                void *&result = argv[0];
                auto toId = static_cast<TypeId>(argv[1]);
                auto converter = reinterpret_cast<TypeLessConverter>(argv[2]);
                _register(lock, converters, result, toId, converter);
                break;
            }
            case UnregisterConvertion: {
                Q_ASSERT(argc == 1);
                auto toId = static_cast<TypeId>(argv[0]);
                _unregister(lock, converters, toId);
                break;
            }
            case HasRegisteredConvertion: {
                Q_ASSERT(argc == 2);
                void *&result = argv[0];
                auto toId = static_cast<TypeId>(argv[1]);
                _isRegisterd(lock, converters, result, toId);
                break;
            }
        }
    }

    static bool convert(const void *fromData, TypeId fromId, void *toData, TypeId toId)
    {
        void *argv[] = {nullptr, const_cast<void*>(fromData), toData, toId};
        Base::Call(fromId, Convert, 4, argv);
        return argv[0];
    }

    // implicit conversion supported like double -> float
    template<typename From, typename To>
    static bool registerConverter()
    {
        auto converter = [](const void *fromData, void *toData) {
            auto typedToData = static_cast<To *>(toData);
            auto typedFromData = static_cast<const From*>(fromData);
            *typedToData = *typedFromData;
            return true;
        };
        return registerConverter(qTypeId<From>(), qTypeId<To>(), converter);
    }

    template<typename From, typename To, To(From::*Function)() const>
    static bool registerConverter()
    {
        TypeLessConverter converter = [](const void *fromData, void *toData) {
            auto typedToData = static_cast<To *>(toData);
            auto typedFromData = static_cast<const From*>(fromData);
            *typedToData = (typedFromData->*Function)();
            return true;
        };
        return registerConverter(qTypeId<From>(), qTypeId<To>(), converter);
    }

    template<typename From, typename To, To(From::*Function)(bool *) const>
    static bool registerConverter()
    {
        TypeLessConverter converter = [](const void *fromData, void *toData) {
            auto typedToData = static_cast<To *>(toData);
            auto typedFromData = static_cast<const From*>(fromData);
            bool ok = false;
            *typedToData = (typedFromData->*Function)(&ok);
            return ok;
        };
        return registerConverter(qTypeId<From>(), qTypeId<To>(), converter);
    }

    template<typename From, typename To, To(*Functor)(const From&)>
    static bool registerConverter()
    {
        TypeLessConverter converter = [](const void *fromData, void *toData) {
            auto typedToData = static_cast<To *>(toData);
            auto typedFromData = static_cast<const From*>(fromData);
            *typedToData = Functor(typedFromData);
            return true;
        };
        return registerConverter(qTypeId<From>(), qTypeId<To>(), converter);
    }

    template<typename From, typename To, TypeLessConverter converter>
    static bool registerConverter()
    {
        return registerConverter(qTypeId<From>(), qTypeId<To>(), converter);
    }

    static bool registerConverter(TypeId fromId, TypeId toId, TypeLessConverter converter)
    {
        static_assert(sizeof(void*) == sizeof(TypeLessConverter));
        void *argv[] = {nullptr, toId, reinterpret_cast<void*>(converter)};
        Base::Call(fromId, RegisterConvertion, 3, argv);
        return argv[0];
    }

    static void unregisterConvertion(TypeId fromId, TypeId toId)
    {
        void *argv[] = {toId};
        Base::Call(fromId, UnregisterConvertion, 1, argv);
    }

    template<typename From, typename To>
    static bool hasRegisteredConverterFunction()
    {
        return hasRegisteredConverterFunction(qTypeId<From>(), qTypeId<To>());
    }

    static bool hasRegisteredConverterFunction(TypeId fromId, TypeId toId)
    {
        void *argv[] = {nullptr, toId};
        Base::Call(fromId, HasRegisteredConvertion, 2, argv);
        return argv[0];
    }
};

}  // N::Extensions
