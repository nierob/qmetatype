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

#include <limits>
#include <tuple>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"
#include "extensions/name.h"

namespace N {

template<class Extension> TypeId Extensions::Ex<Extension>::typeId()
{
    return qTypeId<Extension, Name_hash>();
}

namespace QtPrivate {

template<class... Extensions>
void TypeIdData::registerExtensions(Extensions... extensions)
{
    (registerExtension(Extensions::typeId(), extensions), ...);
}

void TypeIdData::registerExtension(TypeId extensionId, Extensions::ExtensionBase extension)
{
    knownExtensions.try_emplace(extensionId, extension);
}

/*!
 * Trick to have always at most one type id per type.
 *
 * When we call the function the first time the proposed newId would be
 * set as the one used across the application. On the second call the newId would be ignored. It
 * is up to called to figure-out, which case it was.
 */
template<class T>
TypeId qTypeIdImpl(TypeId newId)
{
    static TypeId id = newId;
    return id;
}
}  // namespace QtPrivate

/*!
 * The function to get the type id, more or less equivalent of qMetaTypeId.
 *
 * The function creates or gets id for a given type. It also can register Extensions
 * if there are new given.
 *
 * If called without Extensions, then a default set of Extensions is used.
 *
 * TODO The id is not cached as in qRegisterMetaType, that could be a problem, but not
 * necessarily as we do recommend to call qRegisterMetaType only once. So it should not
 * be in a hot code path and the cost is minor.
 * TODO Maybe a bit expensive if many qTypeId<T> with different extensions are used, because
 * static typeData possibly would be initialized but unused.
 * TODO We may consider change qTypeIdImpl to hold an atomic TypeId and check for nullptr
 * to see if a type already has an id or not, something to measure, as atomic access would
 * likely be more expensive then static access.
 * TODO think about lifetime of Extensions in context of plugins
 */

namespace QtPrivate {
template<class T, class First, class... Tail>
struct FilterUsableDefaultExtensions {
    using TailType = typename FilterUsableDefaultExtensions<T, Tail...>::Type;
    using Type = std::conditional_t<First::template WorksForType<T>(),
                                    decltype(std::tuple_cat(std::tuple<First>(), TailType())),
                                    TailType>;
};

template<class T, class First>
struct FilterUsableDefaultExtensions<T, First>
{
    using Type = std::conditional_t<First::template WorksForType<T>(),
                                    std::tuple<First>,
                                    std::tuple<>>;
};

template<class T, class ValidExtensionsTuple, size_t... I> TypeId qTypeIdDefault(std::index_sequence<I...>)
{
    return qTypeId<T, std::tuple_element_t<I, ValidExtensionsTuple>...>();
}
} // namespace QtPrivate

template<class T, class... Extensions>
TypeId qTypeId()
{
    if constexpr (!bool(sizeof...(Extensions))) {
        using namespace N::Extensions;
        // Register default stuff, Qt should define minimal useful set, DataStream is probably not in :-)
        // Every usage of metatype can call qTypeId with own minimal set of extensions.

        // List of extensions that WorksForType<T> returns true
        using UsableExtensionsTuple = typename N::QtPrivate::FilterUsableDefaultExtensions<T, Allocation, DataStream, Name_dlsym, Name_hash>::Type;
        auto extensionsCount = std::make_index_sequence<std::tuple_size_v<UsableExtensionsTuple>>();
        return N::QtPrivate::qTypeIdDefault<T, UsableExtensionsTuple>(extensionsCount);
    }
    static_assert((Extensions::template WorksForType<T, N::Extensions::OperationMode::AssertTrue>() && ...), "One of extensions is not valid for this type");
    (Extensions::template PreRegisterAction<T>(), ...);
    using ExtendedTypeIdData = N::QtPrivate::TypeIdDataExtended<sizeof...(Extensions) + 1>;
    static ExtendedTypeIdData typeData{{sizeof...(Extensions) + 1, {}},
                                       {{Extensions::typeId(), Extensions::template createExtension<T>()}...}};
    auto id = N::QtPrivate::qTypeIdImpl<T>(&typeData);
    if (id != &typeData) {
        // We are adding extensions
        // TODO try to re-use typeData, maybe we should just link them?
        id->registerExtensions(Extensions::template createExtension<T>()...);
    }
    (Extensions::template PostRegisterAction<T>(id), ...);
    return id;
}

template<class Extension>
inline void Extensions::Ex<Extension>::Call(TypeId id, quint8 operation, size_t argc, void **argv)
{
    if (!id->call(typeId(), operation, argc, argv)) {
        // TODO Think when we need the warning, sometimes we want just to probe if it is possible to do stuff
        warnAboutFailedCall(qTypeId<Extension, Name_dlsym>(), id);
    }
}

/*! Enum replacing QMetaType::Type
 *
 * We need to keep as much of source compatibility as possible therefore some hacks are needed.
 * This struct shows a proposal how to implement replacement of QMetaType::Type enum. As we do not
 * static ids anymore, we can not re-construct the enum. In ideal world such code:
 *
 * if (variant.userType == QMetaType::Void) ...
 *
 * could be replaced by:
 *
 * if (variant.userType == qTypeId<void>()) ...
 *
 * but we could simplify migration by temporally providing the proposed struct. The struct is has a cost
 * and it can be affected by global static initialization order so it is not perfect, but it should
 * be good enough as "Qt5 support mode".
 *
 * The const id takes also part of QDataStream format, so we would need some kind of mapping anyway, unless
 * we drop old versions support there.
 */

struct QMetaType_Type
{
    #define CASE(CurrentEnumName, CurrentEnumValue, Type, ...) \
        static const TypeId CurrentEnumName;
    QT_FOR_EACH_STATIC_TYPE(CASE)
    #undef CASE

    TypeId from(int type);
    TypeId from(QVariant::Type type) { return from(static_cast<int>(type)); }
    TypeId from(QMetaType::Type type) { return from(static_cast<int>(type)); }
};

}  // namespace N
