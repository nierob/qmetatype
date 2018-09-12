#pragma once

#include <QtCore>

namespace N {

namespace P {


typedef bool (*QtMetTypeCall)(size_t functionType, size_t argc, void **argv);
template<class T> bool metaTypeCallImpl(size_t functionType, size_t argc, void **argv);

}  // namespace P

using TypeId = P::QtMetTypeCall;

template<class T> TypeId qRegisterType();

}  // namespace N

