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

namespace N::Extensions {
namespace QtPrivate {
template<class T, class Stream>
class HasLoadStreamOperator
{
    struct Yes { char unused[1];};
    struct No { char unused[2];};
    Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));
    template <class C>
    static decltype(std::declval<Stream &>().operator>>(std::declval<C &>()), Yes()) load(int);
    template <class C>
    static decltype(operator>>(std::declval<Stream &>(), std::declval<C &>()), Yes()) load(int);
    template <class C>
    static No load(...);
public:
    static constexpr bool Value = (sizeof(load<T>(0)) == sizeof(Yes));;
};

template<class T, class Stream>
class HasSaveStreamOperator
{
    struct Yes { char unused[1];};
    struct No { char unused[2];};
    Q_STATIC_ASSERT(sizeof(Yes) != sizeof(No));
    template <class C>
    static decltype(operator<<(std::declval<Stream &>(), std::declval<const C &>()), Yes()) save(int);
    template <class C>
    static decltype(std::declval<Stream &>().operator<<(std::declval<const C &>()), Yes()) save(int);
    template <class C>
    static No save(...);
public:
    static constexpr bool Value = (sizeof(save<T>(0)) == sizeof(Yes));
};

template<class T, class Stream>
struct HasStreamOperator
{
    static constexpr bool LoadValue = HasLoadStreamOperator<T, Stream>::Value;
    static constexpr bool SaveValue = HasSaveStreamOperator<T, Stream>::Value;
    static constexpr bool Value = LoadValue && SaveValue;
};

} // namespace QtPrivate

template<class T, bool = QtPrivate::HasStreamOperator<T, QDataStream>::Value>
class DataStreamImpl
{
public:
    static inline void Call(quint8 operation, size_t argc, void **argv);
};
template<class T>
class DataStreamImpl<T, false>
{
public:
    static void Call(quint8 operation, size_t argc, void **argv)
    {
        Q_UNUSED(operation);
        Q_UNUSED(argc);
        Q_UNUSED(argv);
    }
};

struct DataStream: public Ex<DataStream>
{
    enum Operations {SaveData, LoadData};
    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_UNUSED(data);
        DataStreamImpl<T>::Call(operation, argc, argv);
    }
};

template<class T, bool hasStreamOperator>
inline void DataStreamImpl<T, hasStreamOperator>::Call(quint8 operation, size_t argc, void **argv)
{
    switch (operation)
    {
        case DataStream::SaveData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDataStream*>(argv[0]);
            auto data = static_cast<const T*>(argv[1]);
            *stream << *data;
            break;
        } case DataStream::LoadData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDataStream*>(argv[0]);
            auto data = static_cast<T*>(argv[1]);
            *stream >> *data;
            break;
        }
    }
}

template<class T, bool = QtPrivate::HasSaveStreamOperator<T, QDebug>::Value>
class QDebugStreamImpl
{
public:
    static inline void Call(quint8 operation, size_t argc, void **argv);
};
template<class T>
class QDebugStreamImpl<T, false>
{
public:
    static void Call(quint8 operation, size_t argc, void **argv)
    {
        Q_UNUSED(operation);
        Q_UNUSED(argc);
        Q_UNUSED(argv);
    }
};

struct QDebugStream: public Ex<QDebugStream>
{
    enum Operations {SaveData};
    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        return QDebugStreamImpl<T>::Call(operation, argc, argv);
    }

    static void qDebugStream(TypeId id, QDebug &dbg, const void *data)
    {
        void *argv[] = {&dbg, const_cast<void*>(data)};
        Base::Call(id, SaveData, 2, argv);
    }
};

template<class T, bool hasStreamOperator>
inline void QDebugStreamImpl<T, hasStreamOperator>::Call(quint8 operation, size_t argc, void **argv)
{
    switch (operation)
    {
        case QDebugStream::SaveData: {
            Q_ASSERT(argc == 2);
            auto stream = static_cast<QDebug*>(argv[0]);
            auto data = static_cast<const T*>(argv[1]);
            *stream << *data;
        }
    }
}

}  // N::Extensions
