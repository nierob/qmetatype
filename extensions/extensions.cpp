#include "name.h"

QRegularExpression N::Extensions::Name_dlsym::nameFromTemplate{QLatin1String("with T = (.+)[;\\]]+.")};