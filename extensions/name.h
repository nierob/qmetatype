#pragma once
#include <string_view>
#include <dlfcn.h>
#include "extensions.h"

namespace N::Extensions
{
    namespace P
    {
        template<class QtTypeToIntrospect> constexpr std::string_view typeNameFromType_()
        {
            constexpr size_t offset = sizeof("constexpr std::string_view N::Extensions::P::typeNameFromType() [with QtTypeToIntrospect = ") - 1;
            constexpr size_t tail = sizeof("; std::string_view = std::basic_string_view<char>]");
            constexpr size_t len = sizeof(__PRETTY_FUNCTION__);
            // TODO As for gcc this code is storing the full signature in the code because, we really would need to shorten the name or find another
            // way to cut the size, maybe a trick with const char [] could help? For others ensure it is compile time.
            return std::string_view{__PRETTY_FUNCTION__ + offset, len - offset - tail};
        }

        template<class T> QString typeNameFromType()
        {
            // TODO as an alternative we can use typeid(T).name(), but...
            // - it requires RTTI
            // - it requires +1 allocation, because it ignores CV qualifiers
            // - it may require demangling, depending on the compiler
            // ... but it doesn't depend on compiler extensions (ignoring demangling isssue).
            // We could use it as a fallback if __PRETTY_FUNCTION__ is not usable.
            std::string_view str = typeNameFromType_<T>();
            return QString::fromLocal8Bit(str.data(), str.length());
        }
    }

struct Name_dlsym: public Ex<Name_dlsym>
{
    enum Operations {GetName, RegisterAlias};

    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        switch (operation)
        {
            case GetName: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                *static_cast<QString*>(result) = P::typeNameFromType<T>();
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

        // We prefer the first value but any of _ZN1N13qTypeId would do,
        // so maybe we should just scan all symbols (TODO how exactly?)
        // TODO add more templates or figure out a better way of finding the call.
        static QVector<QString> symbolsTemplates = {
            QStringLiteral("_ZN1N13qTypeIdI7%1EEPFbmmPPvEv"),
            QStringLiteral("_ZN1N13qTypeIdI%1EEPFbmmPPvEv"),
            QStringLiteral("_ZN1N13qTypeIdI7%1NS_10Extensions10AllocationEJNS2_10DataStreamEEEEPFbmmPPvEv")
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


struct Name_hash: public Ex<Name_hash>
{
    enum Operations {GetName, RegisterAlias};

    struct RuntimeData
    {
        QString name;
        // TODO hide it, maybe through friend function
        Name_hash createExtensionBase(TypeId id)
        {
            registerName(id, name);
            auto Call = [](quint8 operation, size_t argc, void **argv, void *data)
            {
                Q_ASSERT(data);
                auto typedData = static_cast<RuntimeData*>(data);
                switch (operation)
                {
                    case GetName: {
                        Q_ASSERT(argc == 1);
                        void *&result = argv[0];
                        *static_cast<QString*>(result) = typedData->name;
                        break;
                    }
                }
            };
            return createFromBasicData(Call, this);
        }
    };

    static QString name(TypeId id)
    {
        QString name;
        void *argv[] = {&name};
        Base::Call(id, GetName, 1, argv);
        return name;
    }

    template<class T>
    static void Call(quint8 operation, size_t argc, void **argv, void *data = nullptr)
    {
        Q_ASSERT(!data);
        switch (operation)
        {
            case GetName: {
                Q_ASSERT(argc == 1);
                void *&result = argv[0];
                *static_cast<QString*>(result) = P::typeNameFromType<T>();
                break;
            }
        }
    }

    static void registerName(TypeId id, const QString &name)
    {
        lock.lockForWrite();
        nameToId[name] = id;
        lock.unlock();
    }

    static TypeId fromName(const QString &name)
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
            nameToId[P::typeNameFromType<T>()] = id;
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

template<> N::TypeId N::qTypeId<N::Extensions::Name_hash, N::Extensions::Name_hash>();
