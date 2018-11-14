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

#include <QtCore/qstring.h>
#include "name.h"
#include "convert.h"
#include "metatype_impl.h"

QReadWriteLock N::Extensions::Name_hash::lock {QReadWriteLock::Recursive};

QDebug operator<<(QDebug &dbg, const N::Extensions::ExtensionBase &ex)
{
    dbg.nospace() << "N::Extensions::ExtensionBase(call=" << (void*)ex.call << ", data=" << ex.data << ")";
    return dbg.space();
}

template<> N::TypeId N::qTypeId<N::Extensions::Name_hash, N::Extensions::Name_hash>()
{
    // HACK WARNING
    // Name_hash is used when storing metatype information about all extensions (for example name is used in
    // error messages), it means that to access type id of Name_hash we need to know it's id.
    // To break infinite recursion we pretend here that it is a runtime created type (even if created through
    // static initialization).
    static struct {
        QtPrivate::TypeIdData typeData;
#ifndef Q_CC_MSVC
        N::Extensions::Name_hash::RuntimeData data{QStringLiteral("N::Extensions::Name_hash")};
#else // msvc
        N::Extensions::Name_hash::RuntimeData data{QString("N::Extensions::Name_hash")};
#endif
    } d;
    auto id = &d.typeData;
    id->registerExtension(id, d.data.createExtensionBase(id));
    return id;
}

void N::Extensions::ExtensionBase::warnAboutFailedCall(TypeId extensionId, TypeId id)
{
    auto extensionName = Name_dlsym::name(extensionId);
    qWarning() << QLatin1String("WARN Requested metatype extension ") + extensionName + QLatin1String(" is not registed for this type:")
                  << id;
}

void N::Extensions::Convertion::_convert(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, void *&result, const void *fromData, void *toData, TypeId toId)
{
    lock.lockForRead();
    auto i = converters.find(toId);
    if (i != converters.cend() && i->second(fromData, toData))
        result = &result; // True
    lock.unlock();
}

void N::Extensions::Convertion::_register(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, void *&result, TypeId toId, Convertion::TypeLessConverter converter)
{
    lock.lockForWrite();
    bool r;
    std::tie(std::ignore, r) = converters.try_emplace(toId, converter);
    if (r)
        result = &result; // True
    lock.unlock();
}

void N::Extensions::Convertion::_unregister(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, TypeId toId)
{
    lock.lockForWrite();
    auto i = converters.find(toId);
    if (i != converters.cend())
        converters.erase(i);
    lock.unlock();
}

void N::Extensions::Convertion::_isRegisterd(QReadWriteLock &lock, std::unordered_map<TypeId, TypeLessConverter> &converters, void *&result, TypeId toId)
{
    lock.lockForRead();
    auto i = converters.find(toId);
    if (i != converters.cend())
        result = &result; // True
    lock.unlock();
}
