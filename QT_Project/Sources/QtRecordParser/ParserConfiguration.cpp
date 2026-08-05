/**
 * @file ParserConfiguration.cpp
 * @brief Implements JSON conversion for parser configurations.
 */

#include "QtRecordParser/ParserConfiguration.h"

#include <QJsonArray>

namespace QtRecordParser
{
auto ParserConfiguration::to_json() const -> QJsonObject
{
    QJsonArray serialized_fields;

    for (const FieldConfiguration& field: fields)
    {
        QJsonObject serialized_field;

        serialized_field.insert(QStringLiteral("id"), field.id);

        serialized_field.insert(QStringLiteral("display_name"), field.display_name);

        serialized_field.insert(QStringLiteral("pattern"), field.capture_pattern);

        serialized_field.insert(QStringLiteral("converter"), field.converter_id);

        serialized_field.insert(QStringLiteral("converter_options"),
                                QJsonObject::fromVariantMap(field.converter_options));

        serialized_field.insert(QStringLiteral("trim"), field.trim_value);

        serialized_fields.append(serialized_field);
    }

    QJsonObject object;

    object.insert(QStringLiteral("format"), format);

    object.insert(QStringLiteral("allow_unknown_fields"), allow_unknown_fields);

    object.insert(QStringLiteral("fields"), serialized_fields);

    return object;
}

auto ParserConfiguration::from_json(const QJsonObject& object,
                                    QString* error_message) -> ParserConfiguration
{
    ParserConfiguration configuration;
    QString error;

    configuration.format = object.value(QStringLiteral("format")).toString();

    configuration.allow_unknown_fields =
        object.value(QStringLiteral("allow_unknown_fields")).toBool(true);

    const QJsonValue fields_value = object.value(QStringLiteral("fields"));

    if (!fields_value.isArray())
    {
        error = QStringLiteral("The 'fields' member must be an array.");
    }
    else
    {
        const QJsonArray serialized_fields = fields_value.toArray();

        for (const QJsonValue& field_value: serialized_fields)
        {
            if (error.isEmpty() && !field_value.isObject())
            {
                error = QStringLiteral("Every field definition must be an object.");
            }

            if (error.isEmpty())
            {
                const QJsonObject field_object = field_value.toObject();

                FieldConfiguration field;

                field.id = field_object.value(QStringLiteral("id")).toString();

                field.display_name = field_object.value(QStringLiteral("display_name")).toString();

                field.capture_pattern =
                    field_object.value(QStringLiteral("pattern")).toString(QStringLiteral(".*?"));

                field.converter_id = field_object.value(QStringLiteral("converter"))
                                         .toString(QStringLiteral("text"));

                field.converter_options = field_object.value(QStringLiteral("converter_options"))
                                              .toObject()
                                              .toVariantMap();

                field.trim_value = field_object.value(QStringLiteral("trim")).toBool(false);

                if (field.id.trimmed().isEmpty())
                {
                    error = QStringLiteral("Every field requires a non-empty identifier.");
                }
                else
                {
                    configuration.fields.append(field);
                }
            }
        }
    }

    if (error_message != nullptr)
    {
        *error_message = error;
    }

    if (!error.isEmpty())
    {
        configuration = ParserConfiguration();
    }

    return configuration;
}
}  // namespace QtRecordParser
