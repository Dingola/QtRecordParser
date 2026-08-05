/**
 * @file FormatRecordParser.cpp
 * @brief Implements configurable placeholder-based record parsing.
 */

#include "QtRecordParser/FormatRecordParser.h"

#include <QSet>
#include <QStringView>
#include <utility>

#include "QtRecordParser/BuiltInConverters.h"

namespace QtRecordParser
{
/**
 * @brief Constructs a parser from configuration and converter registry.
 * @param configuration Serializable parser configuration.
 * @param registry Registry containing built-in and custom converters.
 */
FormatRecordParser::FormatRecordParser(ParserConfiguration configuration,
                                       ConverterRegistry registry)
    : m_configuration(std::move(configuration)), m_registry(std::move(registry))
{
    build_parser();
}

/**
 * @brief Parses one complete input record.
 * @param input Raw input record.
 * @param source File, stream or dataset identifier.
 * @return Parsed values or structured failure information.
 */
auto FormatRecordParser::parse(const QString& input, const QString& source) const -> ParseResult
{
    ParseResult result;
    result.record.source = source;

    if (!is_valid())
    {
        result.error = ParseError::InvalidConfiguration;
        result.error_message = m_configuration_error;
    }
    else
    {
        const QRegularExpressionMatch match = m_pattern.match(input);

        if (!match.hasMatch())
        {
            result.error = ParseError::PatternMismatch;
            result.error_message =
                QStringLiteral("Input does not match the configured record format.");
        }
        else
        {
            for (qsizetype index = 0; index < m_bindings.size() && result.succeeded(); ++index)
            {
                const FieldBinding& binding = m_bindings.at(index);

                QString captured = match.captured(binding.capture_name);

                if (binding.field.trim_value)
                {
                    captured = captured.trimmed();
                }

                const ConversionResult conversion = binding.converter->convert(
                    QStringView(captured), binding.field.converter_options);

                if (conversion.success)
                {
                    result.record.values.insert(binding.field.id, conversion.value);
                }
                else
                {
                    result.error = ParseError::ConversionFailed;
                    result.error_field = binding.field.id;
                    result.error_message =
                        conversion.error_message.isEmpty()
                            ? QStringLiteral("Could not convert field '%1'.").arg(binding.field.id)
                            : conversion.error_message;

                    result.record.values.clear();
                }
            }
        }
    }

    return result;
}

/**
 * @brief Returns the serializable parser configuration.
 * @return Current parser configuration.
 */
auto FormatRecordParser::get_configuration() const -> const ParserConfiguration&
{
    return m_configuration;
}

/**
 * @brief Replaces the parser configuration and rebuilds its pattern.
 * @param configuration New parser configuration.
 */
auto FormatRecordParser::set_configuration(const ParserConfiguration& configuration) -> void
{
    m_configuration = configuration;
    build_parser();
}

/**
 * @brief Returns fields used by the format in placeholder order.
 * @return Resolved field configurations.
 */
auto FormatRecordParser::get_resolved_fields() const -> const QVector<FieldConfiguration>&
{
    return m_resolved_fields;
}

/**
 * @brief Returns the generated anchored regular expression.
 * @return Pattern used by parse().
 */
auto FormatRecordParser::get_pattern() const -> QRegularExpression
{
    return m_pattern;
}

/**
 * @brief Returns whether the parser configuration is usable.
 * @return True when format, fields, converters and pattern are valid.
 */
auto FormatRecordParser::is_valid() const -> bool
{
    return m_configuration_error.isEmpty() && m_pattern.isValid();
}

/**
 * @brief Returns the parser configuration error.
 * @return Error message or an empty string when valid.
 */
auto FormatRecordParser::get_configuration_error() const -> QString
{
    return m_configuration_error;
}

/**
 * @brief Rebuilds resolved fields, capture bindings and pattern.
 */
auto FormatRecordParser::build_parser() -> void
{
    m_resolved_fields.clear();
    m_bindings.clear();
    m_pattern = QRegularExpression();
    m_configuration_error = validate_configured_fields();

    QString pattern_text;
    qsizetype previous_end = 0;
    QSet<QString> used_placeholders;

    const QRegularExpression placeholder_pattern(QStringLiteral(R"(\{([A-Za-z_][A-Za-z0-9_]*)\})"));

    QRegularExpressionMatchIterator placeholders =
        placeholder_pattern.globalMatch(m_configuration.format);

    while (placeholders.hasNext() && m_configuration_error.isEmpty())
    {
        const QRegularExpressionMatch placeholder = placeholders.next();

        const qsizetype placeholder_start = placeholder.capturedStart();

        const qsizetype placeholder_end = placeholder.capturedEnd();

        const QString field_id = placeholder.captured(1);

        pattern_text += QRegularExpression::escape(
            m_configuration.format.mid(previous_end, placeholder_start - previous_end));

        if (used_placeholders.contains(field_id))
        {
            m_configuration_error =
                QStringLiteral("Duplicate placeholder '%1' is ambiguous.").arg(field_id);
        }
        else
        {
            const FieldConfiguration* configured_field = find_configured_field(field_id);

            FieldConfiguration resolved_field;

            if (configured_field != nullptr)
            {
                resolved_field = *configured_field;
            }
            else if (m_configuration.allow_unknown_fields)
            {
                resolved_field = create_default_field(field_id);
            }
            else
            {
                m_configuration_error =
                    QStringLiteral("Placeholder '%1' has no field configuration.").arg(field_id);
            }

            if (m_configuration_error.isEmpty())
            {
                const std::shared_ptr<const ValueConverter> converter =
                    m_registry.get(resolved_field.converter_id);

                if (converter == nullptr)
                {
                    m_configuration_error =
                        QStringLiteral("Converter '%1' for field '%2' is not registered.")
                            .arg(resolved_field.converter_id, resolved_field.id);
                }
                else
                {
                    const QString capture_name = QStringLiteral("field_%1").arg(m_bindings.size());

                    pattern_text += QStringLiteral("(?<%1>%2)")
                                        .arg(capture_name, resolved_field.capture_pattern);

                    m_resolved_fields.append(resolved_field);

                    m_bindings.append(FieldBinding{resolved_field, capture_name, converter});

                    used_placeholders.insert(field_id);
                }
            }
        }

        previous_end = placeholder_end;
    }

    if (m_configuration_error.isEmpty())
    {
        pattern_text += QRegularExpression::escape(m_configuration.format.mid(previous_end));

        m_pattern = QRegularExpression(QStringLiteral("^%1$").arg(pattern_text));

        if (!m_pattern.isValid())
        {
            m_configuration_error = QStringLiteral("Generated parser pattern is invalid: %1")
                                        .arg(m_pattern.errorString());
        }
    }
}

/**
 * @brief Validates explicitly configured fields.
 * @return Error message or an empty string when valid.
 */
auto FormatRecordParser::validate_configured_fields() const -> QString
{
    QString error;
    QSet<QString> field_ids;

    for (const FieldConfiguration& field: m_configuration.fields)
    {
        if (error.isEmpty() && field.id.trimmed().isEmpty())
        {
            error = QStringLiteral("Field identifiers must not be empty.");
        }

        if (error.isEmpty() && field_ids.contains(field.id))
        {
            error = QStringLiteral("Field '%1' is configured more than once.").arg(field.id);
        }

        if (error.isEmpty() && field.capture_pattern.isEmpty())
        {
            error = QStringLiteral("Field '%1' has an empty capture pattern.").arg(field.id);
        }

        if (error.isEmpty() && !m_registry.contains(field.converter_id))
        {
            error = QStringLiteral("Converter '%1' for field '%2' is not registered.")
                        .arg(field.converter_id, field.id);
        }

        if (error.isEmpty())
        {
            const QRegularExpression field_pattern(
                QStringLiteral("^(?:%1)$").arg(field.capture_pattern));

            if (!field_pattern.isValid())
            {
                error = QStringLiteral("Capture pattern for field '%1' is invalid: %2")
                            .arg(field.id, field_pattern.errorString());
            }
        }

        field_ids.insert(field.id);
    }

    return error;
}

/**
 * @brief Finds an explicitly configured field.
 * @param field_id Placeholder identifier.
 * @return Field configuration or nullptr when not configured.
 */
auto FormatRecordParser::find_configured_field(const QString& field_id) const
    -> const FieldConfiguration*
{
    const FieldConfiguration* result = nullptr;

    for (const FieldConfiguration& field: m_configuration.fields)
    {
        if (result == nullptr && field.id == field_id)
        {
            result = &field;
        }
    }

    return result;
}

/**
 * @brief Creates the default configuration for an unknown placeholder.
 * @param field_id Placeholder identifier.
 * @return Text field using the default text converter.
 */
auto FormatRecordParser::create_default_field(const QString& field_id) -> FieldConfiguration
{
    return FieldConfiguration{field_id,          field_id,      QStringLiteral(".*?"),
                              ConverterId::Text, QVariantMap(), false};
}
}  // namespace QtRecordParser
