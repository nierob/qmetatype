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

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"
#include "extensions/name.h"

namespace N {

template<class Extension> TypeId Extensions::Ex<Extension>::typeId()
{
    return qTypeId<Extension, Name_hash>();
}

namespace P {

template<class... Extensions>
void TypeIdData::registerExtensions(Extensions... extensions)
{
    (registerExtension(Extensions::typeId(), extensions), ...);
}

void TypeIdData::registerExtension(TypeId extensionId, Extensions::ExtensionBase extension)
{
    knownExtensions.try_emplace(extensionId, extension);
}

}  // namespace P

template<class T>
TypeId qTypeIdImpl(TypeId newId)
{
    static TypeId id = newId;
    return id;
}

template<class T, class... Extensions>
TypeId qTypeId()
{
    if constexpr (!bool(sizeof...(Extensions))) {
        // Register default stuff, Qt should define minimal useful set, DataStream is probably not in :-)
        // Every usage of metatype can call qTypeId with own minimal set of extensions.
        return qTypeId<T, N::Extensions::Allocation, N::Extensions::DataStream, N::Extensions::Name_dlsym, N::Extensions::Name_hash>();
    }
    (Extensions::template PreRegisterAction<T>(), ...);
    using ExtendedTypeIdData = N::P::TypeIdDataExtended<sizeof...(Extensions) + 1>;
    static ExtendedTypeIdData typeData{{sizeof...(Extensions) + 1, {}},
                                       {{Extensions::typeId(), Extensions::template createExtension<T>()}...}};
    auto id = qTypeIdImpl<T>(&typeData);
    if (id != &typeData) {
        // We are adding extensions
        // TODO try to re-use typeData
        id->registerExtensions(Extensions::template createExtension<T>()...);
    }
    (Extensions::template PostRegisterAction<T>(id), ...);
    return id;
}

template<class Extension>
inline void Extensions::Ex<Extension>::Call(TypeId id, quint8 operation, size_t argc, void **argv)
{
    if (!id->call(typeId(), operation, argc, argv)) {
        auto extensionName = Name_dlsym::name(qTypeId<Extension, Name_dlsym>());
        // TODO Think when we need the warning, sometimes we want just to probe if it is possible to do stuff
        qWarning() << QLatin1String("WARN Requested metatype extension ") + extensionName + QLatin1String(" is not registed for this type:")
                      << id;
    }
}

}  // namespace N
