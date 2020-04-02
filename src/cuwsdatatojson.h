#ifndef CUWSDATATOJSON_H
#define CUWSDATATOJSON_H

class CuData;
#include <QString>

class CuWsDataToJson
{
public:
    QString toJson(const CuData& d);
};

#endif // CUWSDATATOJSON_H
