This is QMetaType for Qt6 prototype
===================================


During Qt5 we learned many thing about type information that we need and that we collect.
The current system is quite good and fast, but it has some deficiencies.

A. Usability:
- It is impossible to register types that are not constructible, the original requirement comes from
  QVariant and has no sense in QMetaType context.
- Unloading plugins is not supported by the QMetaType. We agreed that Qt is not supporting that case,
  but people still do it and the current implementation leaks memory. Same happens for dynamically
  created types, there is no way to "unregister" a type, even if such function exists.
- Q_DECLARE_METATYPE is confusing for users, it got better as now we strongly recommend to place it
  after the class declaration, but still it is not great.
- It is impossible for a user to extend collected data. That is example of QML in which
  a parallel structure has to be created. Similar problem is shared with data stream operators
  registration, and the code that tries to handle that.
- It is not always obvious when qRegisterMetaType needs to be called.

B. Code quality:
- The current implementation is quite fragile from BC perspective, we store data that may not
  match the reality after a while.
- Distinction between built-ins and custom types is confusing and is responsible for growing binary size.
- The implementation is tight to int as a type id, this blocks some optimizations that could be applied even
  with the current design.
- Because of the centralized design testing happens also in just one test and it is hard to test
  part of the functionality without depending on the internals.

C. Performance
- The whole registry is kept behind a mutex and it is very central, the mutex usage actually
  shows on profilers.
- Remapping type id hides some optimizations opportunities.
- Access to builtin types is supper fast at cost of the significant binary size. While it is great to
  have fast code, it is doubtable if anyone really depends on it (who creates millions QIcons through QMetaType?).


Hard requirements, without them the there is no point in changing anything:
- Allow future changes: type id should be extended from int to void* size
- Simplify the code: do not have distinction between built-ins and custom types
- Avoid code bloat: do not have distinction between built-ins and custom types

My focus:
- extensions - so we are not fighting anymore about every bit and not every single type has to be registered
- no Q_DECLARE_METATYPE - so user rarly would need to do anything (qt implicitly would call qTypeId in most cases)

The proposed solution in short
==============================

We introduce a metatype call similar to QObject metacall. Type id is just an address to a structure containing that function
plus some data used for runtime created types. Access to any stored data or functionality would be achieved always through
the call, just with different arguments. That creates small, but I believe acceptable, overhead of dispatching the calls.
In return we get quite a lot of flexibility and BC safety for modifications. Similar concept is used in QObject metatcall.

Main concepts
=============

* Type information => Every static data "attached" to the type. For example it can be it's size, name but also functionality, like
for example information how to construct the type or how to stream it to qDebug.

* Type id => Identifier that uniquely identifies the type. Every type can have at most one type id and two distinct types
can not share the id.

* Distinct type => Whatever C++ defines as distinct type. In particular typedef is not a separate type and pointer to a type
is different from the type it is pointing to. Currently CV qualifiers also introduces new types, we may re-consider that in Qt6. // TODO

* Metatype extension => Structure containing type information is pluggable, every "pluggin", that delivers some type information,
can be called as metatype extension.

* Metatype call => The main entry to the type information. Every type information can be retrieved or used through the call.
Users should not use it directly, but through user friendly API, most likely delivered by metatype extension.

* Runtime type => Type that doesn't exist at compilation time, for example most of QML types are created in runtime.


Why not use just pure const data access, aka why to pay for an indirect function call?
--------------------------------------------------------------------------------------

I think that is the first thought people would have and to be honest it is a very sensible
question. The short answer is: code bloat and compiler optimizations. Imagine the current setup:

struct QMetaTypeInterface
{
    QMetaType::Constructor constructor;
    QMetaType::Destructor destructor;
    QMetaType::TypedConstructor typedConstructor;
    QMetaType::TypedDestructor typedDestructor;
    ...
};

Usage of most of the data here is actually a indirect call. As it is not possible legally to get
address of a constructor we need to use a wrapper function:

static void *Construct(void *where, const void *t)
{
    if (t)
        return new (where) T(*static_cast<const T*>(t));
    return new (where) T;
}

QMetaTypeInterface::constructor stores a function pointer to a wrapper, so even if one had access
to the data the performance should be comparable as in:

void metaTypeCall(size_t functionType, size_t argc, void **argv)
{
    switch (functionType) {
        case Construct:
            void *&result = argv[0];
            auto storage = argv[1];
            if (argc == 2) {
                result = new (storage) T{};
            } else {
                auto copy = static_cast<const T*>(argv[0]);
                result = new (storage) T{*copy};
            }
        }
        ...
    }
}

Now, what happens if we want to add new optional argument to the Construct? With the data based
approach we would need to extend the data size by another pointer and increase some version counter,
with function it is just a matter of handling additional arguments. Keeping an old code working is easier
as it is required to handle the same functionality, not the same data layout. Moreover bigger handlers allow
compiler to reuse some context allowing to optimize the code better.

Both data and function based solution can be extensible, but versioning and optional information in data based
solution would need to be handled by the information consumer, which may create certain maintenance costs.

In addition there are some minor bonuses like reduction in functions wrapper counts, which may speedup compilation
and improve resulting code.


Why not use just a function pointer as id?
--------------------------------------------------------------------------------------
Because we need to support types defined at runtime. That means that we need to attach
some data to the call. The data may be known only at runtime, so even template functions
would not work, we could use std::function to support for example lambdas, but that one
is not only quite big and could cause memory allocations at "registration" time, but
also it was hard to define the data ownership, which was causing memory leaks.
