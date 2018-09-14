#include "name.h"

QRegularExpression N::Extensions::Name::nameFromTemplate{QLatin1String("with T = (.+)[;\\]]+.")};