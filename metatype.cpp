#include <QtCore>

#include "metatype_impl.h"

void* N::P::metaTypeCallImplTerminator(const char* name)
{
    qWarning() << "Metatype extension was not registerd for this type:" << name;
    return nullptr;
}
