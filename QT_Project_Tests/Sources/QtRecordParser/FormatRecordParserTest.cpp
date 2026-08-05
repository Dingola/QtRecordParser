/**
 * @file FormatRecordParserTest.cpp
 * @brief Tests format parsing, conversion and configuration round-trips.
 */

#include "QtRecordParser/FormatRecordParserTest.h"

#include <QJsonObject>
#include <memory>

#include "QtRecordParser/BuiltInConverters.h"
#include "QtRecordParser/FormatRecordParser.h"

/**
 * @class UppercaseConverter
 * @brief Test converter demonstrating application-defined conversion.
 */
class UppercaseConverter final: public QtRecordParser::ValueConverter
{
    public:
        [[nodiscard]] auto get_id() const -> QString override
        {
            return QStringLiteral("uppercase");
        }

        [[nodiscard]] auto convert(QStringView input, const QVariantMap& options) const
            -> QtRecordParser::ConversionResult override
        {
            Q_UNUSED(options);

            return QtRecordParser::ConversionResult{input.toString().toUpper(), QString(), true};
        }
};

/**
 * @brief Verifies a non-log business record with several value types.
 */
TEST_F(FormatRecordParserTest, ParsesTypedBusinessRecord)
{
    QtRecordParser::ParserConfiguration configuration;

    configuration.format = QStringLiteral("{id};{name};{amount};{paid}");

    configuration.fields = {
        {QStringLiteral("id"), QStringLiteral("Identifier"), QStringLiteral(R"(\d+)"),
         QtRecordParser::ConverterId::Integer, QVariantMap(), true},

        {QStringLiteral("name"), QStringLiteral("Name"), QStringLiteral(R"([^;]+)"),
         QtRecordParser::ConverterId::Text, QVariantMap(), true},

        {QStringLiteral("amount"), QStringLiteral("Amount"),
         QStringLiteral(R"([+-]?\d+(?:\.\d+)?)"), QtRecordParser::ConverterId::FloatingPoint,
         QVariantMap(), true},

        {QStringLiteral("paid"), QStringLiteral("Paid"), QStringLiteral(R"(true|false|yes|no|1|0)"),
         QtRecordParser::ConverterId::Boolean, QVariantMap(), true}};

    const QtRecordParser::FormatRecordParser parser(configuration);

    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("42;Example;19.95;yes"), QStringLiteral("orders.csv"));

    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(result.record.value(QStringLiteral("id")).toLongLong(), 42);
    EXPECT_EQ(result.record.value(QStringLiteral("name")).toString(), QStringLiteral("Example"));
    EXPECT_DOUBLE_EQ(result.record.value(QStringLiteral("amount")).toDouble(), 19.95);
    EXPECT_TRUE(result.record.value(QStringLiteral("paid")).toBool());
}

/**
 * @brief Verifies automatic text handling for unknown placeholders.
 */
TEST_F(FormatRecordParserTest, ResolvesUnknownFieldsAsText)
{
    QtRecordParser::ParserConfiguration configuration;

    configuration.format = QStringLiteral("{left}|{right}");

    configuration.allow_unknown_fields = true;

    const QtRecordParser::FormatRecordParser parser(configuration);

    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("alpha|beta"), QStringLiteral("source"));

    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(result.record.value(QStringLiteral("left")).toString(), QStringLiteral("alpha"));
    EXPECT_EQ(result.record.value(QStringLiteral("right")).toString(), QStringLiteral("beta"));
}

/**
 * @brief Verifies use of an application-defined converter.
 */
TEST_F(FormatRecordParserTest, UsesCustomConverter)
{
    QtRecordParser::ConverterRegistry registry =
        QtRecordParser::ConverterRegistry::create_default();

    ASSERT_TRUE(registry.add(std::make_shared<UppercaseConverter>()));

    QtRecordParser::ParserConfiguration configuration;

    configuration.format = QStringLiteral("{value}");

    configuration.fields = {{QStringLiteral("value"), QStringLiteral("Value"), QStringLiteral(".+"),
                             QStringLiteral("uppercase"), QVariantMap(), true}};

    const QtRecordParser::FormatRecordParser parser(configuration, registry);

    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("hello"), QStringLiteral("source"));

    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(result.record.value(QStringLiteral("value")).toString(), QStringLiteral("HELLO"));
}

/**
 * @brief Verifies structured conversion failure information.
 */
TEST_F(FormatRecordParserTest, ReportsConversionFailure)
{
    QtRecordParser::ParserConfiguration configuration;

    configuration.format = QStringLiteral("{number}");

    configuration.fields = {{QStringLiteral("number"), QStringLiteral("Number"),
                             QStringLiteral(R"(\S+)"), QtRecordParser::ConverterId::Integer,
                             QVariantMap(), true}};

    const QtRecordParser::FormatRecordParser parser(configuration);

    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("invalid"), QStringLiteral("source"));

    EXPECT_FALSE(result.succeeded());
    EXPECT_EQ(result.error, QtRecordParser::ParseError::ConversionFailed);
    EXPECT_EQ(result.error_field, QStringLiteral("number"));
    EXPECT_TRUE(result.record.values.isEmpty());
}

/**
 * @brief Verifies rejection of duplicate placeholders.
 */
TEST_F(FormatRecordParserTest, RejectsDuplicatePlaceholders)
{
    QtRecordParser::ParserConfiguration configuration;

    configuration.format = QStringLiteral("{value}:{value}");

    const QtRecordParser::FormatRecordParser parser(configuration);

    EXPECT_FALSE(parser.is_valid());
    EXPECT_FALSE(parser.get_configuration_error().isEmpty());
}

/**
 * @brief Verifies configuration serialization and deserialization.
 */
TEST_F(FormatRecordParserTest, ConfigurationRoundTripPreservesFields)
{
    QtRecordParser::ParserConfiguration original;

    original.format = QStringLiteral("{timestamp} {message}");

    original.allow_unknown_fields = false;

    original.fields = {{QStringLiteral("timestamp"), QStringLiteral("Timestamp"),
                        QStringLiteral(".+?"), QtRecordParser::ConverterId::DateTime,
                        QVariantMap{{QStringLiteral("formats"),
                                     QStringList{QStringLiteral("yyyy-MM-dd HH:mm:ss")}}},
                        true},

                       {QStringLiteral("message"), QStringLiteral("Message"), QStringLiteral(".*"),
                        QtRecordParser::ConverterId::Text, QVariantMap(), true}};

    const QJsonObject json = original.to_json();

    QString error;

    const QtRecordParser::ParserConfiguration restored =
        QtRecordParser::ParserConfiguration::from_json(json, &error);

    EXPECT_TRUE(error.isEmpty());
    EXPECT_EQ(restored.format, original.format);
    EXPECT_EQ(restored.allow_unknown_fields, original.allow_unknown_fields);

    ASSERT_EQ(restored.fields.size(), 2);
    EXPECT_EQ(restored.fields.at(0).id, QStringLiteral("timestamp"));
    EXPECT_EQ(restored.fields.at(0).converter_id, QtRecordParser::ConverterId::DateTime);
    EXPECT_EQ(restored.fields.at(1).id, QStringLiteral("message"));
}
