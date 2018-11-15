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
#include <QtTest>
#include "metatype.h"

class tst_Ids: public QObject
{
    Q_OBJECT
private slots:
    void idCoherence();
    void lazyExtensions();
};

void tst_Ids::idCoherence()
{
    // Check that N::qTypeId<int>() returns the same everywhere
    // in particular it should be the same if called from the library
    // as from the app side.
    auto intIdFromName = N::Extensions::Name_hash::fromName("int");
    auto intIdFromQt5Support = N::QMetaType_Type::Int;
    auto intId = N::qTypeId<int>();
    QCOMPARE(intId, intIdFromName);
    QCOMPARE(intId, intIdFromQt5Support);
}

struct EmptyExtension : public N::Extensions::Ex<EmptyExtension>
{
    template<class T>
    static void Call(quint8, size_t, void **, void * = nullptr) {}
};

void tst_Ids::lazyExtensions()
{
    auto intId = N::qTypeId<int>();
    QVERIFY(!intId->isExtensionKnown(EmptyExtension::typeId()));
    auto newIntId = N::qTypeId<int, EmptyExtension>();
    QCOMPARE(intId, newIntId);
    QVERIFY(intId->isExtensionKnown(EmptyExtension::typeId()));
}

QTEST_GUILESS_MAIN(tst_Ids);

#include "main.moc"
