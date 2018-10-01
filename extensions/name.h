#pragma once
#include <string_view>
#include <dlfcn.h>
#include "extensions.h"

namespace N::Extensions
{

namespace P
{
template<class QtTypeToIntrospect> constexpr std::string_view typeNameFromType()
{
    constexpr size_t offset = strlen("constexpr std::string_view N::Extensions::P::typeNameFromType() [with QtTypeToIntrospect = ");
    constexpr size_t tail = strlen("; std::string_view = std::basic_string_view<char>]");
    // TODO That could be const expression but apparently it is not
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66639 sugests that it works on clang and msvc.
    // As for gcc this code is storing the full signature in the code because, we really would need to shorten the name.
    /*constexpr*/ size_t len = strlen(__PRETTY_FUNCTION__);
    return {Q_FUNC_INFO + offset, len - offset - tail};
}
}

struct Name_dlsym: public Ex<Name_dlsym>
{
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
                constexpr std::string_view str = P::typeNameFromType<T>();
                *static_cast<QString*>(result) = QString::fromLocal8Bit(str.data(), str.length());
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

    static TypeId fromName(const QString &name) Q_REQUIRED_RESULT
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


struct Name_hash: public Name_dlsym
{
    static TypeId fromName(const QString &name) Q_REQUIRED_RESULT
    {
        lock.lockForRead();
        auto id = nameToId[name];
        lock.unlock();
        return id;
    }

    template<class T>
    static void PreRegisterAction()
    {
        if (firstPreCall<T>())
            lock.lockForWrite();
    }

    template<class T>
    static void PostRegisterAction(TypeId id)
    {
        if (firstPostCall<T>()) {
            constexpr std::string_view str = P::typeNameFromType<T>();
            nameToId[QString::fromLocal8Bit(str.data(), str.length())] = id;
            lock.unlock();
        }
    }

private:
    static QReadWriteLock lock;
    static QHash<QString, TypeId> nameToId;

    template<class T>
    static bool firstPreCall()
    {
        bool result = false;
        static bool marker = ((result = true), false);
        Q_UNUSED(marker);
        return result;
    }
    template<class T>
    static bool firstPostCall()
    {
        bool result = false;
        static bool marker = ((result = true), false);
        Q_UNUSED(marker);
        return result;
    }
};


}  // N::Extensions
