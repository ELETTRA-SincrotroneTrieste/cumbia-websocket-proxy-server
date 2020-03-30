#include "cuwssourcevalidator.h"
#include <cutango-world.h>

CuWsSourceValidator::CuWsSourceValidator()
{

}

bool CuWsSourceValidator::isValid(const QString &src) const
{
#ifdef QUMBIA_TANGO_CONTROLS_VERSION
    CuTangoWorld w;
    return w.source_valid(src.toStdString());
#else
    return false;
#endif
}
