#include "cuwsdatatojson.h"
#include <cudata.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QStringList>
#include <QMap>
#include <qustring.h>
#include <qustringlist.h>
#include <QtDebug>

QString CuWsDataToJson::toJson(const CuData &d, const char* atype)
{
    QString j;
    QJsonObject o;
    QJsonObject data_o;
    o["src"] = QuString(d, "src");
    o["atype"] = QString(atype);

    // copy everything from CuData to JSon data object
    //
    // Json converts numbers into double
    const std::vector<std::string>& keys = d.keys();
    foreach(const std::string &k , keys) {
        const CuVariant &v = d[k];
        // treat timestamp_ms and timestamp_us separately
        if(k.find("timestamp") == std::string::npos && v.isValid()) {
            if(v.getFormat() == CuVariant::Scalar) {
                if(v.getType() == CuVariant::Boolean)
                    data_o[k.c_str()] = v.toBool();
                else if(v.getType() == CuVariant::String)
                    data_o[k.c_str()] = QuString(v.toString());
                else if(v.isInteger()) {
                    int i;
                    v.to<int>(i);
                    data_o[k.c_str()] = i;
                }
                else if(v.isFloatingPoint()) {
                    // float and double become double (QJsonValue has only toDouble)
                    double d;
                    v.to<double>(d);
                    data_o[k.c_str()] = d;
                }
            }
            else if(v.getFormat() == CuVariant::Vector) {
                QJsonArray a;
                if(v.getType() == CuVariant::Boolean) {
                    std::vector<bool> bv = v.toBoolVector();
                    foreach(bool b, bv)
                        a.push_back(b);
                }
                else if(v.getType() == CuVariant::String) {
                    a = QJsonArray::fromStringList(QuStringList(v.toStringVector()));
                }
                else if(v.isInteger()) {
                    std::vector<int> vi;
                    v.toVector<int>(vi);
                    foreach(int i, vi)
                        a.push_back(i);
                }
                else if(v.isFloatingPoint()) {
                    // float and double become double (QJsonValue has only toDouble)
                    std::vector<double> dv;
                    v.toVector<double>(dv);
                    foreach(double d, dv)
                        a.push_back(d);
                }
                data_o[k.c_str()] = a;
            }
        }
    }

    // timestamp. Use double version of timestamp_ms to stay safe
    // if talking to a 32 bit system (WebAssembly)
    data_o["timestamp_ms"] = static_cast<double>(d["timestamp_ms"].toLongInt());
    data_o["timestamp_us"] = d["timestamp_us"].toDouble();

    //        QJsonObject o;
    //        QJsonObject data_o;
    //        o["src"] = QuString(d, "src");
    //        o["atype"] = QString(atype);
    //        QMap<QString, QStringList> slmap;
    //        if(d.has("type", "property")) {
    //            QStringList keys = QStringList () << "abs_change" << "archive_abs_change" << "archive_period"
    //                                              << "archive_rel_change" <<  "description" <<  "disp_level" << "display_unit"
    //                                              << "format"  <<  "label"<<  "max" <<  "max_alarm"<<   "max_dim_x"  << "max_dim_y"   <<     "max_warning"   << "min"
    //                                              << "min_alarm"   <<   "min_warning"<<  "periodic_period"  << "rel_change"   <<      "root_attr_name"
    //                                              <<    "standard_unit" << "writable_attr_name" << "delta_t";
    //            foreach(const QString& k, keys) {
    //                const std::string& s = k.toStdString();
    //                if(d.containsKey(s)) {
    //                    QJsonValue v(QuString(d, s.c_str()));
    //                    data_o[s.c_str()] = v;
    //                }
    //            }
    //            if(d.containsKey("properties")) {
    //                slmap["properties"] = QuStringList(d["properties"].toStringVector());
    //            }

    //            // to int
    //            QStringList iks = QStringList() << "data_type" << "data_format";
    //            foreach(const QString& ik, iks) {
    //                if(d.containsKey(ik.toStdString()))
    //                    data_o[ik] = d[ik.toStdString()].toInt();
    //            }
    //        }

    //        // skip "event", it can break decoding on cumbia-websocket client side
    //        // "event" is used by canoned to name the source
    //        QStringList keys = QStringList() << "msg" << "type" << "activity"
    //                                         << "quality_string" << "state_string" << "quality_color" << "state_color" << "writable"
    //                                         << "data_format" << "data_format_str" << "device" << "mode" << "point" << "rmode";

    //        foreach(QString k, keys) {
    //            if(d.containsKey(k.toStdString())) {
    //                const std::string& s = k.toStdString();
    //                QJsonValue v(QuString(d, s.c_str()));
    //                data_o[s.c_str()] = v;
    //            }
    //        }
    //        // non string values
    //        if(d.containsKey("argins"))
    //            slmap["argins"] = QuStringList(d["argins"].toStringVector());


    //        data_o["err"] = d["err"].toBool();

    //        // to int
    //        QStringList itypes = QStringList() << "format" << "writable" << "data_type";
    //        foreach(QString ityp, itypes)
    //            if(d.containsKey(ityp.toStdString()))
    //                data_o[ityp] = d[ityp.toStdString()].toInt();

    //        QStringList vas = QStringList () << "value" << "w_value";
    //        foreach(const QString& vk, vas) {
    //            if(d.containsKey(vk.toStdString())) {
    //                CuVariant v = d[vk.toStdString()];
    //                if(v.getFormat() == CuVariant::Scalar) {
    //                    if(v.getType() == CuVariant::Boolean)
    //                        data_o[vk] = v.toBool();
    //                    else if(v.isInteger()) {
    //                        long long iv;
    //                        v.to<long long>(iv);
    //                        data_o[vk] = iv;
    //                    }
    //                    else if(v.isFloatingPoint()) {
    //                        double dv;
    //                        v.to<double>(dv);
    //                        data_o[vk] = dv;
    //                    }
    //                    else if(v.getType() == CuVariant::String)
    //                        data_o[vk] = QuString(d, qstoc(vk));
    //                }
    //            }
    //        }
    o["data"] = data_o;
    QJsonDocument jdo(o);
    j = jdo.toJson();
    return j;
}
