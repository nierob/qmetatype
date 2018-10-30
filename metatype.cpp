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

#include <QtCore>

#include "metatype_impl.h"
#include "extensions/name.h"

namespace {
using DataExtension = N::QtPrivate::TypeIdDataExtended<1>::ExtArray;
static const DataExtension *findInitialExtensionInTypeIdData(const N::QtPrivate::TypeIdData *data, N::TypeId extensionId)
{
    auto fakeTypedItData = static_cast<const N::QtPrivate::TypeIdDataExtended<1>*>(data); // Just to get the offset
    auto firstInitialExtension = fakeTypedItData->initialExtensions;
    for (auto extension = firstInitialExtension; extension != firstInitialExtension + data->extCount; ++extension) {
        if (extension->id == extensionId)
            return extension;
    }
    return nullptr;
}
} // namespace

bool N::QtPrivate::TypeIdData::call(TypeId extensionId, quint8 operation, size_t argc, void **argv)
{
    if (auto extension = findInitialExtensionInTypeIdData(this, extensionId)) {
        extension->extension(operation, argc, argv);
        return true;
    }
    auto exIter = knownExtensions.find(extensionId);
    if (exIter == knownExtensions.cend())
        return false;
    exIter->second(operation, argc, argv);
    return true;
}

bool N::QtPrivate::TypeIdData::isExtensionKnown(TypeId extensionId) const
{
    return findInitialExtensionInTypeIdData(this, extensionId) || knownExtensions.find(extensionId) != knownExtensions.end();
}

QDebug operator<<(QDebug &dbg, N::TypeId id)
{
    if (!id) {
        return dbg << "TypeId(Unknown)";
    }
    // TODO allow warning less name access
    auto typeName = N::Extensions::Name_hash::name(id);
    dbg.nospace() << "TypeId(";
    if (!typeName.isEmpty())
        dbg << typeName << ',' << ' ';
    dbg << (void*)id << ")";
    return dbg.space();
}
