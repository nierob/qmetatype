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

class tst_Convert: public QObject
{
    Q_OBJECT
private slots:
    void basicConversion();
    void hasRegisteredConverterFunction();
};

void tst_Convert::basicConversion()
{
    auto i16id = N::qTypeId<qint16, N::Extensions::Convertion>();
    auto stringId = N::qTypeId<QString, N::Extensions::Convertion>();

    using namespace N::Extensions;
    {
        // Implicit conversion
        auto i32id = N::qTypeId<qint32>();
        qint16 fromData = 43;
        qint32 toData = -1;
        QVERIFY((!Convertion::convert(&fromData, i16id, &toData, i32id)));
        QCOMPARE(fromData, 43);
        QCOMPARE(toData, -1);
        QVERIFY((Convertion::registerConverter<qint16, qint32>()));
        QVERIFY((Convertion::convert(&fromData, i16id, &toData, i32id)));
        QCOMPARE(fromData, toData);
    }
    {   // Memeber toXXX(bool*) conversion
        QVERIFY((!Convertion::hasRegisteredConverterFunction<QString, float>()));
        QVERIFY((Convertion::registerConverter<QString, float, &QString::toFloat>()));
        QVERIFY((Convertion::hasRegisteredConverterFunction<QString, float>()));
        QVERIFY((!Convertion::hasRegisteredConverterFunction<QString, int>()));
        auto floatId = N::qTypeId<float>();
        QString fromData = QLatin1String("43");
        float toData = 0;
        QVERIFY((Convertion::convert(&fromData, stringId, &toData, floatId)));
        fromData = QLatin1String("WillNotWork");
        QVERIFY((!Convertion::convert(&fromData, stringId, &toData, floatId)));
    }
}

void tst_Convert::hasRegisteredConverterFunction()
{
    QVERIFY((!N::Extensions::Convertion::hasRegisteredConverterFunction<QString, double>()));
    QVERIFY((N::Extensions::Convertion::registerConverter<QString, double, &QString::toDouble>()));
    QVERIFY((N::Extensions::Convertion::hasRegisteredConverterFunction<QString, double>()));
    N::Extensions::Convertion::unregisterConvertion(N::qTypeId<QString>(), N::qTypeId<double>());
}

QTEST_GUILESS_MAIN(tst_Convert);

#include "main.moc"
