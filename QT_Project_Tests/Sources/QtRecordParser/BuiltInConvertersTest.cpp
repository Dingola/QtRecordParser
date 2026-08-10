/**
 * @file BuiltInConvertersTest.cpp
 * @brief Tests the standard QtRecordParser value converters.
 */

#include "QtRecordParser/BuiltInConvertersTest.h"

#include <QDateTime>
#include <QStringList>

#include "QtRecordParser/BuiltInConverters.h"

/**
 * @brief Verifies that text is returned without modification.
 */
TEST_F(BuiltInConvertersTest, PreservesTextValue)
{
    const QtRecordParser::TextConverter converter;

    const QtRecordParser::ConversionResult result =
        converter.convert(QStringLiteral("  unchanged text  "), QVariantMap());

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value.toString(), QStringLiteral("  unchanged text  "));
    EXPECT_TRUE(result.error_message.isEmpty());
}

/**
 * @brief Verifies decimal and configured hexadecimal integer conversion.
 */
TEST_F(BuiltInConvertersTest, ConvertsIntegersWithConfiguredBase)
{
    const QtRecordParser::IntegerConverter converter;

    const QtRecordParser::ConversionResult decimal =
        converter.convert(QStringLiteral("-42"), QVariantMap());

    const QtRecordParser::ConversionResult hexadecimal =
        converter.convert(QStringLiteral("ff"), QVariantMap{{QStringLiteral("base"), 16}});

    ASSERT_TRUE(decimal.success);
    EXPECT_EQ(decimal.value.toLongLong(), -42);

    ASSERT_TRUE(hexadecimal.success);
    EXPECT_EQ(hexadecimal.value.toLongLong(), 255);
}

/**
 * @brief Verifies that malformed integers and unsupported bases are rejected.
 */
TEST_F(BuiltInConvertersTest, RejectsInvalidIntegers)
{
    const QtRecordParser::IntegerConverter converter;

    const QtRecordParser::ConversionResult malformed =
        converter.convert(QStringLiteral("12x"), QVariantMap());

    const QtRecordParser::ConversionResult base_below_range =
        converter.convert(QStringLiteral("10"), QVariantMap{{QStringLiteral("base"), 1}});

    const QtRecordParser::ConversionResult base_above_range =
        converter.convert(QStringLiteral("10"), QVariantMap{{QStringLiteral("base"), 37}});

    EXPECT_FALSE(malformed.success);
    EXPECT_FALSE(malformed.error_message.isEmpty());
    EXPECT_FALSE(base_below_range.success);
    EXPECT_FALSE(base_below_range.error_message.isEmpty());
    EXPECT_FALSE(base_above_range.success);
    EXPECT_FALSE(base_above_range.error_message.isEmpty());
}

/**
 * @brief Verifies floating-point conversion and malformed input handling.
 */
TEST_F(BuiltInConvertersTest, ConvertsAndRejectsFloatingPointValues)
{
    const QtRecordParser::FloatingPointConverter converter;

    const QtRecordParser::ConversionResult valid =
        converter.convert(QStringLiteral("-19.75"), QVariantMap());

    const QtRecordParser::ConversionResult invalid =
        converter.convert(QStringLiteral("19.75.2"), QVariantMap());

    ASSERT_TRUE(valid.success);
    EXPECT_DOUBLE_EQ(valid.value.toDouble(), -19.75);
    EXPECT_FALSE(invalid.success);
    EXPECT_FALSE(invalid.error_message.isEmpty());
}

/**
 * @brief Verifies case-insensitive default boolean tokens.
 */
TEST_F(BuiltInConvertersTest, ConvertsDefaultBooleanTokensCaseInsensitively)
{
    const QtRecordParser::BooleanConverter converter;

    const QtRecordParser::ConversionResult true_result =
        converter.convert(QStringLiteral("YES"), QVariantMap());

    const QtRecordParser::ConversionResult false_result =
        converter.convert(QStringLiteral("Off"), QVariantMap());

    ASSERT_TRUE(true_result.success);
    EXPECT_TRUE(true_result.value.toBool());

    ASSERT_TRUE(false_result.success);
    EXPECT_FALSE(false_result.value.toBool());
}

/**
 * @brief Verifies custom boolean tokens and case-sensitive matching.
 */
TEST_F(BuiltInConvertersTest, AppliesConfiguredBooleanTokens)
{
    const QtRecordParser::BooleanConverter converter;

    const QVariantMap options{
        {QStringLiteral("true_values"), QStringList{QStringLiteral("enabled")}},
        {QStringLiteral("false_values"), QStringList{QStringLiteral("disabled")}},
        {QStringLiteral("case_sensitive"), true}};

    const QtRecordParser::ConversionResult enabled =
        converter.convert(QStringLiteral("enabled"), options);

    const QtRecordParser::ConversionResult wrong_case =
        converter.convert(QStringLiteral("ENABLED"), options);

    ASSERT_TRUE(enabled.success);
    EXPECT_TRUE(enabled.value.toBool());
    EXPECT_FALSE(wrong_case.success);
    EXPECT_FALSE(wrong_case.error_message.isEmpty());
}

/**
 * @brief Verifies ISO-8601 and configured date-time formats.
 */
TEST_F(BuiltInConvertersTest, ConvertsIsoAndConfiguredDateTimes)
{
    const QtRecordParser::DateTimeConverter converter;

    const QtRecordParser::ConversionResult iso =
        converter.convert(QStringLiteral("2026-08-09T13:45:12.123Z"), QVariantMap());

    const QtRecordParser::ConversionResult iso_without_milliseconds =
        converter.convert(QStringLiteral("2026-08-09T13:45:12Z"), QVariantMap());

    const QVariantMap custom_options{
        {QStringLiteral("accept_iso"), false},
        {QStringLiteral("formats"), QStringList{QStringLiteral("dd.MM.yyyy HH:mm")}}};

    const QtRecordParser::ConversionResult custom =
        converter.convert(QStringLiteral("09.08.2026 13:45"), custom_options);

    ASSERT_TRUE(iso.success);
    EXPECT_TRUE(iso.value.toDateTime().isValid());
    EXPECT_EQ(iso.value.toDateTime().date(), QDate(2026, 8, 9));

    ASSERT_TRUE(iso_without_milliseconds.success);
    EXPECT_TRUE(iso_without_milliseconds.value.toDateTime().isValid());
    EXPECT_EQ(iso_without_milliseconds.value.toDateTime().date(), QDate(2026, 8, 9));

    ASSERT_TRUE(custom.success);
    EXPECT_TRUE(custom.value.toDateTime().isValid());
    EXPECT_EQ(custom.value.toDateTime().date(), QDate(2026, 8, 9));
    EXPECT_EQ(custom.value.toDateTime().time(), QTime(13, 45));
}

/**
 * @brief Verifies rejection when no date-time format accepts the input.
 */
TEST_F(BuiltInConvertersTest, RejectsInvalidDateTime)
{
    const QtRecordParser::DateTimeConverter converter;

    const QVariantMap options{
        {QStringLiteral("accept_iso"), false},
        {QStringLiteral("formats"), QStringList{QStringLiteral("yyyy-MM-dd")}}};

    const QtRecordParser::ConversionResult result =
        converter.convert(QStringLiteral("not-a-date"), options);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.isEmpty());
    EXPECT_FALSE(result.value.isValid());
}
