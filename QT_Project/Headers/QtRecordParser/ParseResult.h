#pragma once

#include <QMap>
#include <QString>
#include <QVariant>

/**
 * @file ParseResult.h
 * @brief Defines parsed records and structured parser failures.
 */

namespace QtRecordParser
{
enum class ParseError
{
    None,
    InvalidConfiguration,
    PatternMismatch,
    ConversionFailed
};

/**
 * @struct ParsedRecord
 * @brief Stores dynamically named converted values.
 */
struct ParsedRecord {
        QMap<QString, QVariant> values;
        QString source;

        [[nodiscard]] auto value(const QString& field_id) const -> QVariant
        {
            return values.value(field_id);
        }

        [[nodiscard]] auto contains(const QString& field_id) const -> bool
        {
            return values.contains(field_id);
        }
};

/**
 * @struct ParseResult
 * @brief Contains either a parsed record or structured failure information.
 */
struct ParseResult {
        ParsedRecord record;
        ParseError error{ParseError::None};
        QString error_field;
        QString error_message;

        [[nodiscard]] auto succeeded() const -> bool
        {
            return error == ParseError::None;
        }
};
}  // namespace QtRecordParser
