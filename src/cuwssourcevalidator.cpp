#include "cuwssourcevalidator.h"
#include <cutango-world.h>
#include <QRegularExpression>

CuWsSourceValidator::CuWsSourceValidator()
{

}

bool CuWsSourceValidator::isValid(const QString &src) const
{
#ifdef QUMBIA_TANGO_CONTROLS_VERSION
    QString s(src);
    QRegularExpression re("ws[s]{0,1}://");
    s.remove(re);
    CuTangoWorld w;
    return w.source_valid(s.toStdString());
#else
    return true;
#endif
}
