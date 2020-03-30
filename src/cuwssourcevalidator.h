#ifndef CUWS_SOURCEVALIDATOR_H
#define CUWS_SOURCEVALIDATOR_H

#include <QString>

#ifdef QUMBIA_TANGO_CONTROLS_VERSION
#include <cutango-world.h>
#endif

class CuWsSourceValidator
{
public:
    CuWsSourceValidator();

    bool isValid(const QString& src) const;
};

#endif // SOURCEVALIDATOR_H
