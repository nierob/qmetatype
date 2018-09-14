#pragma once
#include <dlfcn.h>
#include "extensions.h"

namespace N::Extensions
{

struct Name: public Ex<Name>
{
    static QRegularExpression nameFromTemplate;
    enum Operations {GetName, RegisterAlias};

    template<class T>
    static void Call(size_t functionType, size_t argc, void **argv)
    {
        switch (functionType)
        {
            case GetName: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                // TODO make it compile time and return const char* probably.
                *static_cast<QString*>(result) = nameFromTemplate.match(Q_FUNC_INFO).captured(1);
                break;
            }
        }
    }

    static QString name(TypeId id)
    {
        QString name;
        void *argv[] = {&name};
        Base::Call(id, GetName, 1, argv);
        return name;
    }

    static TypeId fromName(const QString &name)
    {
        // TODO this algorithm will work only with -rdynamic and if binaries
        // are not totally stripped. In addition it is platform specific. We may
        // need a generic fallback based on a global hash as in Qt5 or we
        // can store the type name with a special markes just after typeId, then
        // scanning .data section would probably discover it, but we would need
        // to ensure that all internal statics are actually initialized (TODO how?).

        // We prefer the first value but any of _ZN1N13qRegisterType would do,
        // so maybe we should just scan all symbols (TODO how exactly?)
        // TODO add more templates or figure out a better way of finding the call.
        static QVector<QString> symbolsTemplates = {
            QStringLiteral("_ZN1N13qRegisterTypeI7%1EEPFbmmPPvEv"),
            QStringLiteral("_ZN1N13qRegisterTypeI%1EEPFbmmPPvEv"),
            QStringLiteral("_ZN1N13qRegisterTypeI7%1NS_10Extensions10AllocationEJNS2_10DataStreamEEEEPFbmmPPvEv")
        };
        for (auto symbolTemplate: symbolsTemplates) {
            TypeId (*registrationFunctionPointer)();
            auto symbolName = symbolTemplate.arg(name).toLatin1();
            if ((*(void **)(&registrationFunctionPointer) = dlsym(RTLD_DEFAULT, symbolName.constData())))
                return registrationFunctionPointer();
        }
        // Check for aliases
        // TODO make it correct, that is just a mockup
        if (name == "int")
            return fromName("i");
        if (name == "unsigned" || name == "unsigned int")
           return fromName("j");
        // Not found
        return nullptr;
    }
};

}  // N::Extensions
