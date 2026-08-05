#pragma once

#include <QRegularExpression>
#include <QString>
#include <QVector>
#include <memory>

#include "QtRecordParser/ConverterRegistry.h"
#include "QtRecordParser/RecordParser.h"

/**
 * @file FormatRecordParser.h
 * @brief Declares the configurable placeholder-based record parser.
 */

namespace QtRecordParser
{
/**
 * @class FormatRecordParser
 * @brief Parses records using a serializable format and converter configuration.
 *
 * Placeholders such as `{timestamp}` are replaced with named regular-expression
 * capture groups. Each captured value is converted by a converter resolved from
 * ConverterRegistry.
 *
 * Unknown placeholders may optionally be accepted as text fields. The parser is
 * immutable while parse() runs and can therefore be copied into worker threads.
 */
class FormatRecordParser final: public RecordParser
{
    public:
        /**
         * @brief Constructs a parser from configuration and converter registry.
         * @param configuration Serializable parser configuration.
         * @param registry Registry containing built-in and custom converters.
         */
        explicit FormatRecordParser(
            ParserConfiguration configuration,
            ConverterRegistry registry = ConverterRegistry::create_default());

        /**
         * @brief Parses one complete input record.
         * @param input Raw input record.
         * @param source File, stream or dataset identifier.
         * @return Parsed values or structured failure information.
         */
        [[nodiscard]] auto parse(const QString& input,
                                 const QString& source) const -> ParseResult override;

        /**
         * @brief Returns the serializable parser configuration.
         * @return Current parser configuration.
         */
        [[nodiscard]] auto get_configuration() const -> const ParserConfiguration& override;

        /**
         * @brief Replaces the parser configuration and rebuilds its pattern.
         * @param configuration New parser configuration.
         */
        auto set_configuration(const ParserConfiguration& configuration) -> void;

        /**
         * @brief Returns fields used by the format in placeholder order.
         * @return Resolved field configurations.
         */
        [[nodiscard]] auto get_resolved_fields() const -> const QVector<FieldConfiguration>&;

        /**
         * @brief Returns the generated anchored regular expression.
         * @return Pattern used by parse().
         */
        [[nodiscard]] auto get_pattern() const -> QRegularExpression;

        /**
         * @brief Returns whether the parser configuration is usable.
         * @return True when format, fields, converters and pattern are valid.
         */
        [[nodiscard]] auto is_valid() const -> bool;

        /**
         * @brief Returns the parser configuration error.
         * @return Error message or an empty string when valid.
         */
        [[nodiscard]] auto get_configuration_error() const -> QString;

    private:
        /**
         * @struct FieldBinding
         * @brief Associates a field with one capture group and converter.
         */
        struct FieldBinding {
                FieldConfiguration field;
                QString capture_name;
                std::shared_ptr<const ValueConverter> converter;
        };

        /**
         * @brief Rebuilds resolved fields, capture bindings and pattern.
         */
        auto build_parser() -> void;

        /**
         * @brief Validates explicitly configured fields.
         * @return Error message or an empty string when valid.
         */
        [[nodiscard]] auto validate_configured_fields() const -> QString;

        /**
         * @brief Finds an explicitly configured field.
         * @param field_id Placeholder identifier.
         * @return Field configuration or nullptr when not configured.
         */
        [[nodiscard]] auto find_configured_field(const QString& field_id) const
            -> const FieldConfiguration*;

        /**
         * @brief Creates the default configuration for an unknown placeholder.
         * @param field_id Placeholder identifier.
         * @return Text field using the default text converter.
         */
        [[nodiscard]] static auto create_default_field(const QString& field_id)
            -> FieldConfiguration;

    private:
        ParserConfiguration m_configuration;
        ConverterRegistry m_registry;
        QVector<FieldConfiguration> m_resolved_fields;
        QVector<FieldBinding> m_bindings;
        QRegularExpression m_pattern;
        QString m_configuration_error;
};
}  // namespace QtRecordParser
