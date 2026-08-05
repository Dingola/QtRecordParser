#pragma once

#include "QtRecordParser/ValueConverter.h"

/**
 * @file BuiltInConverters.h
 * @brief Declares the standard QtRecordParser value converters.
 */

namespace QtRecordParser
{
namespace ConverterId
{
inline const QString Text{QStringLiteral("text")};
inline const QString Integer{QStringLiteral("integer")};
inline const QString FloatingPoint{QStringLiteral("floating_point")};
inline const QString Boolean{QStringLiteral("boolean")};
inline const QString DateTime{QStringLiteral("datetime")};
}  // namespace ConverterId

/**
 * @class TextConverter
 * @brief Returns captured text as QString.
 */
class TextConverter final: public ValueConverter
{
    public:
        [[nodiscard]] auto get_id() const -> QString override;

        [[nodiscard]] auto convert(QStringView input,
                                   const QVariantMap& options) const -> ConversionResult override;
};

/**
 * @class IntegerConverter
 * @brief Converts text into a signed 64-bit integer.
 *
 * Supported option:
 * - `base`: integer base from 2 through 36; defaults to 10.
 */
class IntegerConverter final: public ValueConverter
{
    public:
        [[nodiscard]] auto get_id() const -> QString override;

        [[nodiscard]] auto convert(QStringView input,
                                   const QVariantMap& options) const -> ConversionResult override;
};

/**
 * @class FloatingPointConverter
 * @brief Converts text into a double-precision number.
 */
class FloatingPointConverter final: public ValueConverter
{
    public:
        [[nodiscard]] auto get_id() const -> QString override;

        [[nodiscard]] auto convert(QStringView input,
                                   const QVariantMap& options) const -> ConversionResult override;
};

/**
 * @class BooleanConverter
 * @brief Converts configurable true and false tokens into bool.
 *
 * Supported options:
 * - `true_values`: string list; defaults to true, 1, yes and on.
 * - `false_values`: string list; defaults to false, 0, no and off.
 * - `case_sensitive`: defaults to false.
 */
class BooleanConverter final: public ValueConverter
{
    public:
        [[nodiscard]] auto get_id() const -> QString override;

        [[nodiscard]] auto convert(QStringView input,
                                   const QVariantMap& options) const -> ConversionResult override;
};

/**
 * @class DateTimeConverter
 * @brief Converts ISO-8601 or configured date-time formats into QDateTime.
 *
 * Supported options:
 * - `accept_iso`: defaults to true.
 * - `formats`: ordered string list passed to QDateTime::fromString.
 */
class DateTimeConverter final: public ValueConverter
{
    public:
        [[nodiscard]] auto get_id() const -> QString override;

        [[nodiscard]] auto convert(QStringView input,
                                   const QVariantMap& options) const -> ConversionResult override;
};
}  // namespace QtRecordParser
