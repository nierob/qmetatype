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
#include "metatype_fwd.h"
#include <tuple>
#include <utility>
#include <string_view>

namespace N {

namespace Extensions
{
template<class Extension>
class Ex : public ExtensionBase
{
    template<class T, class... Extensions> friend TypeId N::qTypeId();
protected:
    typedef Ex<Extension> Base;

    // Can be overridden in Extension. Can be called multiple times per T, needs to be thread safe
    template<class T> constexpr static void PreRegisterAction() {}
    // Can be overridden in Extension. Can be called multiple times per T, needs to be thread safe
    template<class T> constexpr static void PostRegisterAction(TypeId id) { Q_UNUSED(id); }

    static void Call(TypeId id, quint8 operation, size_t argc, void **argv);

    template<class T>
    static inline Extension createExtension()
    {
        return std::move(createFromBasicData<T>());
    }
    template<class T=void>
    static inline Extension createFromBasicData(decltype(ExtensionBase::call) call = Extension::template Call<T>, void *data = nullptr)
    {
        Extension extension;
        extension.call = call;
        extension.data = data;
        return std::move(extension);
    }
public:
    static inline TypeId typeId();
};

template<class T>
struct DefaultTypeIdHandleDeleter: public std::default_delete<T>
{
    void operator()(N::P::TypeIdData* ptr) const
    {
        std::default_delete<T>::operator()(static_cast<T*>(ptr));
    }
};

struct EmptyTypeIdHandleDeleter
{
    void operator()(N::P::TypeIdData*) const {}
};


template<class Deleter>
struct TypeIdHandle: private std::unique_ptr<N::P::TypeIdData, Deleter>
{
    using Base = std::unique_ptr<N::P::TypeIdData, Deleter>;
    TypeIdHandle(TypeId id, Deleter deleter = Deleter()) : Base(id, deleter) {}
    TypeId id() { return Base::get(); }
};

template<class TypeData, class Deleter=DefaultTypeIdHandleDeleter<TypeData>>
TypeIdHandle<Deleter> initializeType(TypeData *data)
{
    // TODO we probably need to add way to pass deleter to the TypeIdHandle
    static_assert(std::is_base_of_v<N::P::TypeIdData, TypeData>);
    auto registerExtension = [data](auto&... ex) {
        (data->registerExtensions(ex.createExtensionBase(data)), ...);
    };
    std::apply(registerExtension, data->extensions);
    return data;
}
} // namespace Extensions

} // namespace N
QDebug operator<<(QDebug &dbg, const N::Extensions::ExtensionBase &ex);

template<class T>
QDebug operator<<(QDebug &dbg, const N::Extensions::TypeIdHandle<T> &handle)
{
    dbg.nospace() << "TypeHandle(" << handle.id << ")";
    return dbg.space();
}
