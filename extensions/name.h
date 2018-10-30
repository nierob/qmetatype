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
#include <string_view>
#include <dlfcn.h>
#include "extensions.h"

/*!
 * Name extension
 *
 * It captures the type name and offers some operations around it. Main known uses cases in Qt:
 *  - error formating (qDebug and friends)
 *  - mapping by name to external types, like for example in sql, dbus
 *  - datastream protocol uses type names
 *  - old singal and slot mechanizm dispatch arguments through type names
 *  - odd stuff like QVariant being too smart about implict conversions for enumerations
 *    and other types, that one should go away with Qt6
 *  - tooling/code generators, type ids changes on every run, so generators needs to create
 *    code that uses type names
 *  - pointer detections (names that ends with "*"), that use case is invalid even if used
 *    in many places. We would need a flag instead
 *  - accessing type id for type name, QML while reading an input file, needs to convert
 *    type name to id so it can later on construct it
 *
 * This file shows some play around the problem. It doesn't implement the final solution, but it
 * explores a few possibilities. The main problem is that we would like to avoid storing global set
 * of name -> type id hash. It is tricky because of QMetaType::fromName function that can __not__
 * be remove nor deprecated.
 *
 * Explored ideas:
 *  - using dlsym to mmap own application and calling the right function
 *  - using typeid
 *  - abusing __PRETTY_FUNCTION__
 *  - storing all data in a custom section/segment that can be later scanned for strings and ids
 *    (I have failed to implement it, it may work for plugins)
 *
 * Conclusion:
 * ===========
 * There is no clear winner. I think the solution is to use a mixture of typeid and __PRETTY_FUNCTION__ ideas.
 * If we can get the 2nd to be used as constexpr then we should use it, on platforms that it is not possible in a sane fashion,
 * we should fallback to typeid.
 *
 * There is no way around for the global hash, as we offer aliases, that means that two different type names
 * can map to the same type id.
 *
 * Name_hash is probably the right base for Qt6 implementation
 */

namespace N::Extensions
{
    namespace QtPrivate
    {
        template<class QtTypeToIntrospect> constexpr std::string_view typeNameFromType_()
        {
            constexpr size_t offset = sizeof("constexpr std::string_view N::Extensions::QtPrivate::typeNameFromType() [with QtTypeToIntrospect = ");
            constexpr size_t tail = sizeof("; std::string_view = std::basic_string_view<char>]");
            constexpr size_t len = sizeof(__PRETTY_FUNCTION__);
            // TODO As for gcc this code is storing the full signature in the code because, we really would need to shorten the name or find another
            // way to cut the size, maybe a trick with const char [] could help? For others ensure it is compile time.
            return std::string_view{__PRETTY_FUNCTION__ + offset, len - offset - tail};
        }

        template<class T> QString typeNameFromType()
        {
            // TODO as an alternative we can use typeid(T).name(), but...
            // - it requires RTTI
            // - it requires +1 allocation, because it ignores CV qualifiers
            // - it may return different names for the same type in some cases (results are not stable across the calls)
            // - it may require demangling, depending on the compiler
            // ... but it doesn't depend on compiler extensions (ignoring demangling isssue).
            // We could use it as a fallback if __PRETTY_FUNCTION__ is not usable.
            std::string_view str = typeNameFromType_<T>();
            return QString::fromLocal8Bit(str.data(), str.length());
        }
    }

/*!
* \brief The Name_dlsym struct
*
* Experimantal class providing a type name handling.
* Mapping type id => name is based on typeNameFromType<T>()
* Mapping name => type id is based on dlsym and scanning own binary in hope for findind the rigth callback
*
* Summary:
* It is scary, but mostly works.
*   Pros:
*     - If a type was not used yet, it implicitly calls qTypeInfo
*     - It uses exsiting low level functionality instead of creating own solution from scratch
*   Cons:
*     - It still needs some global hash for aliases.
*     - It will not work with totally stripped binaries
*     - It require linking with rdynamic
*     - It is hard to find mangling function (demangling is there, but we would need to do the opposite)
*     - May not work with typedefs
*/
struct Name_dlsym: public Ex<Name_dlsym>
{
    enum Operations {GetName, RegisterAlias};

    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        switch (operation)
        {
            case GetName: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                *static_cast<QString*>(result) = QtPrivate::typeNameFromType<T>();
                break;
            }
        }
    }

    static QString name(TypeId id)
    {
        QString name;
        void *argv[] = {&name};
        Base::Call(id, GetName, 1, argv);
        return name;
    }

    static TypeId fromName(const QString &name)
    {
        // This algorithm will work only with -rdynamic and only if binaries
        // are not totally stripped. In addition it is platform specific.
        // We prefer the first value but any of _ZN1N13qTypeId would do,
        // so maybe we should just scan all symbols (TODO how exactly?)
        // TODO add more templates or figure out a better way of finding the call.
        static QVector<QString> symbolsTemplates = {
            QStringLiteral("_ZN1N13qTypeIdI7%1EEPFbmmPPvEv"),
            QStringLiteral("_ZN1N13qTypeIdI%1EEPFbmmPPvEv"),
            QStringLiteral("_ZN1N13qTypeIdI7%1NS_10Extensions10AllocationEJNS2_10DataStreamEEEEPFbmmPPvEv")
        };
        for (auto symbolTemplate: symbolsTemplates) {
            TypeId (*registrationFunctionPointer)();
            auto symbolName = symbolTemplate.arg(name).toLatin1();
            if ((*(void **)(&registrationFunctionPointer) = dlsym(RTLD_DEFAULT, symbolName.constData())))
                return registrationFunctionPointer();
        }
        // Check for aliases
        // TODO make it correct, that is just a mockup
        if (name == "int")
            return fromName("i");
        if (name == "unsigned" || name == "unsigned int")
           return fromName("j");
        // Not found
        return nullptr;
    }
};

/*!
* \brief The Name_hash struct
*
* Experimantal class providing a type name handling.
* Mapping type id => name is based on typeNameFromType<T>()
* Mapping name => type id is based on global hash (similar to Qt5)
*
* Summary:
* It doesn't offer anything more over what we have in Qt5
*   Pros:
*    - It is simple
*    - It supports aliases out of the box
*   Cons:
*    - It requires lock-free hash or mutex
*    - It likely duplicates the data, if based on QStrings
*/
struct Name_hash: public Ex<Name_hash>
{
    enum Operations {GetName, RegisterAlias};

    struct RuntimeData
    {
        QString name;
        // TODO hide it, maybe through friend function
        Name_hash createExtensionBase(TypeId id)
        {
            registerName(id, name);
            auto Call = [](quint8 operation, size_t argc, void **argv, void *data)
            {
                Q_ASSERT(data);
                auto typedData = static_cast<RuntimeData*>(data);
                switch (operation)
                {
                    case GetName: {
                        Q_ASSERT(argc == 1);
                        void *&result = argv[0];
                        *static_cast<QString*>(result) = typedData->name;
                        break;
                    }
                }
            };
            return createFromBasicData(Call, this);
        }
    };

    static QString name(TypeId id)
    {
        QString name;
        void *argv[] = {&name};
        Base::Call(id, GetName, 1, argv);
        return name;
    }

    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        switch (operation)
        {
            case GetName: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                *static_cast<QString*>(result) = QtPrivate::typeNameFromType<T>();
                break;
            }
        }
    }

    static void registerName(TypeId id, const QString &name)
    {
        lock.lockForWrite();
        nameToId[name] = id;
        lock.unlock();
    }

    static TypeId fromName(const QString &name)
    {
        lock.lockForRead();
        auto id = nameToId[name];
        lock.unlock();
        return id;
    }

    template<class T>
    static void PreRegisterAction()
    {
        if (firstPreCall<T>())
            lock.lockForWrite();
    }

    template<class T>
    static void PostRegisterAction(TypeId id)
    {
        if (firstPostCall<T>()) {
            nameToId[QtPrivate::typeNameFromType<T>()] = id;
            lock.unlock();
        }
    }

private:
    static QReadWriteLock lock;
    static QHash<QString, TypeId> nameToId;

    template<class T>
    static bool firstPreCall()
    {
        bool result = false;
        static bool marker = ((result = true), false);
        Q_UNUSED(marker);
        return result;
    }
    template<class T>
    static bool firstPostCall()
    {
        bool result = false;
        static bool marker = ((result = true), false);
        Q_UNUSED(marker);
        return result;
    }
};

}  // N::Extensions

template<> N::TypeId N::qTypeId<N::Extensions::Name_hash, N::Extensions::Name_hash>();
