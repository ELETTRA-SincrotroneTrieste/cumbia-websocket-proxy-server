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

QString CuWsDataToJson::toJson(const CuData &d)
{
    QString j;
    QJsonObject o;
    QJsonObject data_o;
    o["src"] = QuString(d, "src");
    QMap<QString, QStringList> slmap;
    if(d.has("type", "property")) {
        QStringList keys = QStringList () << "abs_change" << "archive_abs_change" << "archive_period"
                                          << "archive_rel_change" <<  "description" <<  "disp_level" << "display_unit"
                                          << "format"  <<  "label"<<  "max" <<  "max_alarm"<<   "max_dim_x"  << "max_dim_y"   <<     "max_warning"   << "min"
                                          << "min_alarm"   <<   "min_warning"<<  "periodic_period"  << "rel_change"   <<      "root_attr_name"
                                          <<    "standard_unit" << "writable_attr_name" << "delta_t";
        foreach(const QString& k, keys) {
            const std::string& s = k.toStdString();
            if(d.containsKey(s)) {
                QJsonValue v(QuString(d, s.c_str()));
                data_o[s.c_str()] = v;
            }
        }
        if(d.containsKey("properties")) {
            slmap["properties"] = QuStringList(d["properties"].toStringVector());
        }

        // to int
        QStringList iks = QStringList() << "data_type" << "data_format";
        foreach(const QString& ik, iks) {
            if(d.containsKey(ik.toStdString()))
                data_o[ik] = d[ik.toStdString()].toInt();
        }
    }

    // skip "event", it can break decoding on cumbia-websocket client side
    // "event" is used by canoned to name the source
    QStringList keys = QStringList() << "msg" << "type" << "activity"
                                     << "quality_string" << "state_string" << "quality_color" << "state_color"
                                     << "data_format" << "data_format_str" << "device" << "mode" << "point" << "rmode";

    foreach(QString k, keys) {
        if(d.containsKey(k.toStdString())) {
            const std::string& s = k.toStdString();
            QJsonValue v(QuString(d, s.c_str()));
            data_o[s.c_str()] = v;
        }
    }
    // non string values
    if(d.containsKey("argins"))
        slmap["argins"] = QuStringList(d["argins"].toStringVector());

    // timestamp and error
    data_o["timestamp_ms"] = static_cast<qint64>(d["timestamp_ms"].toLongInt());
    data_o["timestamp_us"] = d["timestamp_us"].toDouble();
    data_o["err"] = d["err"].toBool();

    if(d.containsKey("quality"))
        data_o["quality"] = d["quality"].toInt();

    QStringList vas = QStringList () << "value" << "w_value";
    foreach(const QString& vk, vas) {
        if(d.containsKey(vk.toStdString())) {
            CuVariant v = d[vk.toStdString()];
            if(v.getFormat() == CuVariant::Scalar) {
                if(v.getType() == CuVariant::Boolean)
                    data_o[vk] = v.toBool();
                else if(v.isInteger()) {
                    long long iv;
                    v.to<long long>(iv);
                    data_o[vk] = iv;
                }
                else if(v.isFloatingPoint()) {
                    double dv;
                    v.to<double>(dv);
                    data_o[vk] = dv;
                }
                else if(v.getType() == CuVariant::String)
                    data_o[vk] = QuString(d, qstoc(vk));
            }
        }
    }
    o["data"] = data_o;
    QJsonDocument jdo(o);
    j = jdo.toJson();
    return j;
}
