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

#include <QtCore>
#include <unordered_map>

namespace N {
namespace P {
typedef void (*QtMetTypeCall)(quint8 operation, size_t argc, void **argv, void *data);
struct TypeIdData;
}  // namespace P

using TypeId = P::TypeIdData*;

namespace Extensions
{
    struct ExtensionBase
    {
        N::P::QtMetTypeCall call = nullptr;
        void *data = nullptr;

        void operator()(quint8 operation, size_t argc, void** argv) const
        {
            call(operation, argc, argv, data);
        }
    protected:
        static void warnAboutFailedCall(TypeId extensionId, TypeId id);
    };
}

namespace P {

struct TypeIdData
{
    // TODO make it thread safe
    // TODO Split API into public and private as it seems tht TypeIdData needs to be public
    // otherwise we can not construct type at runtime
    const size_t extCount = 0;
    std::unordered_map<N::TypeId, N::Extensions::ExtensionBase> knownExtensions;

    bool call(N::TypeId extensionId, quint8 operation, size_t argc, void **argv);
    bool isExtensionKnown(N::TypeId extensionId) const;
    template<class... Extensions>
    inline void registerExtensions(Extensions... extensions);
    inline void registerExtension(N::TypeId extensionId, N::Extensions::ExtensionBase extension);
};

template<size_t InitialExtensionsCount>
struct TypeIdDataExtended: public TypeIdData
{
    struct ExtArray
    {
        TypeId id;
        N::Extensions::ExtensionBase extension;
    } initialExtensions[InitialExtensionsCount];
};

}  // namespace P

template<class T, class... Extensions> TypeId qTypeId();

}  // namespace N

QDebug operator<<(QDebug &dbg, N::TypeId id);
