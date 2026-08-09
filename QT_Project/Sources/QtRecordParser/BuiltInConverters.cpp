/**
 * @file BuiltInConverters.cpp
 * @brief Implements the standard QtRecordParser value converters.
 */

#include "QtRecordParser/BuiltInConverters.h"

#include <QDateTime>
#include <QStringList>

/**
 * @brief Creates a failed conversion result.
 * @param message Failure description.
 * @return Failed result.
 */
namespace
{
[[nodiscard]] auto failed_conversion(const QString& message) -> QtRecordParser::ConversionResult
{
    return QtRecordParser::ConversionResult{QVariant(), message, false};
}
}  // namespace

namespace QtRecordParser
{
auto TextConverter::get_id() const -> QString
{
    return ConverterId::Text;
}

auto TextConverter::convert(QStringView input, const QVariantMap& options) const -> ConversionResult
{
    Q_UNUSED(options);

    return ConversionResult{input.toString(), QString(), true};
}

auto IntegerConverter::get_id() const -> QString
{
    return ConverterId::Integer;
}

auto IntegerConverter::convert(QStringView input,
                               const QVariantMap& options) const -> ConversionResult
{
    const int base = options.value(QStringLiteral("base"), 10).toInt();

    bool valid = false;
    qlonglong value = 0;

    if (base >= 2 && base <= 36)
    {
        value = input.toString().toLongLong(&valid, base);
    }

    ConversionResult result;

    if (valid)
    {
        result = ConversionResult{value, QString(), true};
    }
    else
    {
        result = failed_conversion(QStringLiteral("Value is not a valid integer."));
    }

    return result;
}

auto FloatingPointConverter::get_id() const -> QString
{
    return ConverterId::FloatingPoint;
}

auto FloatingPointConverter::convert(QStringView input,
                                     const QVariantMap& options) const -> ConversionResult
{
    Q_UNUSED(options);

    bool valid = false;
    const double value = input.toString().toDouble(&valid);

    ConversionResult result;

    if (valid)
    {
        result = ConversionResult{value, QString(), true};
    }
    else
    {
        result = failed_conversion(QStringLiteral("Value is not a valid floating-point number."));
    }

    return result;
}

auto BooleanConverter::get_id() const -> QString
{
    return ConverterId::Boolean;
}

auto BooleanConverter::convert(QStringView input,
                               const QVariantMap& options) const -> ConversionResult
{
    const QStringList default_true_values{QStringLiteral("true"), QStringLiteral("1"),
                                          QStringLiteral("yes"), QStringLiteral("on")};

    const QStringList default_false_values{QStringLiteral("false"), QStringLiteral("0"),
                                           QStringLiteral("no"), QStringLiteral("off")};

    const QStringList true_values =
        options.value(QStringLiteral("true_values"), default_true_values).toStringList();

    const QStringList false_values =
        options.value(QStringLiteral("false_values"), default_false_values).toStringList();

    const bool case_sensitive = options.value(QStringLiteral("case_sensitive"), false).toBool();

    const Qt::CaseSensitivity sensitivity =
        case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    const QString text = input.toString();
    bool is_true = false;
    bool is_false = false;

    for (const QString& candidate: true_values)
    {
        if (!is_true && text.compare(candidate, sensitivity) == 0)
        {
            is_true = true;
        }
    }

    for (const QString& candidate: false_values)
    {
        if (!is_false && text.compare(candidate, sensitivity) == 0)
        {
            is_false = true;
        }
    }

    ConversionResult result;

    if (is_true != is_false)
    {
        result = ConversionResult{is_true, QString(), true};
    }
    else
    {
        result = failed_conversion(QStringLiteral("Value is not a recognized boolean token."));
    }

    return result;
}

auto DateTimeConverter::get_id() const -> QString
{
    return ConverterId::DateTime;
}

auto DateTimeConverter::convert(QStringView input,
                                const QVariantMap& options) const -> ConversionResult
{
    const QString text = input.toString();

    const bool accept_iso = options.value(QStringLiteral("accept_iso"), true).toBool();

    const QStringList formats = options.value(QStringLiteral("formats")).toStringList();

    QDateTime date_time;

    if (accept_iso)
    {
        date_time = QDateTime::fromString(text, Qt::ISODateWithMs);

        if (!date_time.isValid())
        {
            date_time = QDateTime::fromString(text, Qt::ISODate);
        }
    }

    for (qsizetype index = 0; index < formats.size() && !date_time.isValid(); ++index)
    {
        date_time = QDateTime::fromString(text, formats.at(index));
    }

    ConversionResult result;

    if (date_time.isValid())
    {
        result = ConversionResult{date_time, QString(), true};
    }
    else
    {
        result = failed_conversion(QStringLiteral("Value is not a recognized date-time."));
    }

    return result;
}
}  // namespace QtRecordParser
