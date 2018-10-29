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
#include "metatype.h"

/**
* Example code.
*
* Used for creating custom types dynamically. We want give a user power of controlling the life time
* of such data, as it allows to optimize allocations, maybe support plugins.
*
* TODO currently it inherits N::P::TypeIdData, which is private. We may consider making it public or
* change the interface.
*
* TODO usage of tuple just a shortcut, for which I do not have workaround. In general we want to
* co-allocate all the extension data and iterate over them in initializeType
*
* TODO maybe it is possible to merge it into initializeType function. In perfect world a user would
* need to just call a function with all the init data for extensions data and would get back customly allocated
* type. In reality I guess we need to split it into two steps, allocation and initialization.
*/
struct RuntimeData: N::P::TypeIdData
{
    std::tuple<N::Extensions::Name_hash::RuntimeData, N::Extensions::Allocation::RuntimeData> extensions;
};

int main(int argc, char** argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    qDebug() << "----------------Int--------------------------";
    {
        auto intId = N::qTypeId<int>();
        qDebug() << "Succesfull geting type id of int:" << intId;
        qDebug() << "sizeof(int):" << sizeof(int) << "while metatype says that the size is:" << N::Extensions::Allocation::sizeOf(intId);
        qDebug() << "alignof(int):" << alignof(int) << "while metatype says that the align is:" << N::Extensions::Allocation::alignOf(intId);
        int intCopy = 6;
        auto createdInt = N::Extensions::Allocation::create(intId, &intCopy);
        int *i = static_cast<int*>(createdInt.get());
        qDebug() << "create(int):" << *i;
        qDebug() << "try to call qdebug stream (int), should result in error";
        N::Extensions::QDebugStream::qDebugStream(intId, qDebug() << "   Testing qdebug stream output, should result in error ^", i);
        qDebug() << "let's try to re-register the type with new a extension";
        N::qTypeId<int, N::Extensions::QDebugStream>();
        qDebug() << "try to call qdebug stream (int):";
        N::Extensions::QDebugStream::qDebugStream(intId, qDebug() << "   Testing qdebug stream output after registration of QDebug extension value:", i);
    }

    qDebug() << "----------------QString--------------------------";
    {
        auto qstringId = N::qTypeId<QString>();
        qDebug() << "Succesfull geting type id of QString:" << qstringId << "as named type" << N::Extensions::Name_dlsym::name(qstringId);
        qDebug() << "sizeof(QString):" << sizeof(QString) << "while metatype says that the size is:" << N::Extensions::Allocation::sizeOf(qstringId);
        qDebug() << "alignof(QString):" << alignof(QString) << "while metatype says that the align is:" << N::Extensions::Allocation::alignOf(qstringId);
        QString stringCopy = QLatin1String("String");
        auto createdString = N::Extensions::Allocation::create(qstringId, &stringCopy);
        QString *string = static_cast<QString*>(createdString.get());
        qDebug() << "create(QString):" << *string;
    }

    qDebug() << "----------------Unsigned int--------------------------";
    {
        auto preRegistrationIntId = N::Extensions::Name_dlsym::fromName(QStringLiteral("unsigned"));
        qDebug() << "Succesfull geting type id of unsigned before using the qTypeId:" << preRegistrationIntId;
        auto uintId = N::qTypeId<unsigned, N::Extensions::Name_dlsym>();
        qDebug() << "All ids are in sync:" << (uintId == preRegistrationIntId);
        qDebug() << "Unsigned int by default is known as:" << N::Extensions::Name_dlsym::name(uintId);
    }

    qDebug() << "----------------Char--------------------------";
    {
        auto charId = N::qTypeId<char, N::Extensions::Name_hash>();
        qDebug() << "Succesfull geting type id of char:" << charId;
        qDebug() << "Lookup type id by name:" << N::Extensions::Name_hash::fromName("char");
        qDebug() << "Lookup type id by name returned correct id:" << (N::Extensions::Name_hash::fromName("char") == charId);
        qDebug() << "Char by default is known as:" << N::Extensions::Name_hash::name(charId);
    }

    qDebug() << "----------------Runtime--------------------------";
    {
        auto runtimeAdditionalData = new RuntimeData{{}, { N::Extensions::Name_hash::RuntimeData{{"RuntimeTypeName"}},
                                                           N::Extensions::Allocation::RuntimeData{std::size_t{12}, std::align_val_t{4}}}};
        auto runtimeTypeHandle = N::Extensions::initializeType(runtimeAdditionalData);
        auto runtimeTypeId = runtimeTypeHandle.id();
        qDebug() << "Custom type was created:" << runtimeTypeId;
        qDebug() << "Custom type size(12) and align(4) is:" << N::Extensions::Allocation::sizeOf(runtimeTypeId) << N::Extensions::Allocation::alignOf(runtimeTypeId);
    }

    qDebug() << "----------------Runtime with stack allocated definiton--------------------------";
    {
        RuntimeData runtimeAdditionalData{{}, { N::Extensions::Name_hash::RuntimeData{{"RuntimeTypeName"}},
                                                N::Extensions::Allocation::RuntimeData{std::size_t{2}, std::align_val_t{1}}}};
        auto runtimeTypeHandle = N::Extensions::initializeType<RuntimeData, N::Extensions::EmptyTypeIdHandleDeleter>(&runtimeAdditionalData);
        auto runtimeTypeId = runtimeTypeHandle.id();
        qDebug() << "Custom type was created:" << runtimeTypeId;
        qDebug() << "Custom type size(2) and align(1) is:" << N::Extensions::Allocation::sizeOf(runtimeTypeId) << N::Extensions::Allocation::alignOf(runtimeTypeId);
    }

    qDebug() << "----------------Extensions are also registered--------------------------";
    {
        auto nameId = N::Extensions::Name_hash::fromName("N::Extensions::Name_hash");
        qDebug() << "For example name recovering extensions id is there:" << nameId;
        auto allocId = N::Extensions::Name_hash::fromName("N::Extensions::Allocation");
        qDebug() << "Same for allocation:" << allocId;
    }
    return 0;
}
