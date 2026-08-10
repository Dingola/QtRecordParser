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

    configuration.fields = {{.id = QStringLiteral("id"),
                             .display_name = QStringLiteral("Identifier"),
                             .capture_pattern = QStringLiteral(R"(\d+)"),
                             .converter_id = QtRecordParser::ConverterId::Integer,
                             .converter_options = {},
                             .trim_value = true},
                            {.id = QStringLiteral("name"),
                             .display_name = QStringLiteral("Name"),
                             .capture_pattern = QStringLiteral(R"([^;]+)"),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = true},
                            {.id = QStringLiteral("amount"),
                             .display_name = QStringLiteral("Amount"),
                             .capture_pattern = QStringLiteral(R"([+-]?\d+(?:\.\d+)?)"),
                             .converter_id = QtRecordParser::ConverterId::FloatingPoint,
                             .converter_options = {},
                             .trim_value = true},
                            {.id = QStringLiteral("paid"),
                             .display_name = QStringLiteral("Paid"),
                             .capture_pattern = QStringLiteral(R"(true|false|yes|no|1|0)"),
                             .converter_id = QtRecordParser::ConverterId::Boolean,
                             .converter_options = {},
                             .trim_value = true}};

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

    configuration.fields = {{.id = QStringLiteral("value"),
                             .display_name = QStringLiteral("Value"),
                             .capture_pattern = QStringLiteral(".+"),
                             .converter_id = QStringLiteral("uppercase"),
                             .converter_options = {},
                             .trim_value = true}};

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

    configuration.fields = {{.id = QStringLiteral("number"),
                             .display_name = QStringLiteral("Number"),
                             .capture_pattern = QStringLiteral(R"(\S+)"),
                             .converter_id = QtRecordParser::ConverterId::Integer,
                             .converter_options = {},
                             .trim_value = true}};

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
 * @brief Verifies structured failure when the input does not match the generated pattern.
 */
TEST_F(FormatRecordParserTest, ReportsPatternMismatch)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{number}");
    configuration.allow_unknown_fields = false;
    configuration.fields = {{.id = QStringLiteral("number"),
                             .display_name = QStringLiteral("Number"),
                             .capture_pattern = QStringLiteral(R"(\d+)"),
                             .converter_id = QtRecordParser::ConverterId::Integer,
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("not-a-number"), QStringLiteral("source"));

    EXPECT_FALSE(result.succeeded());
    EXPECT_EQ(result.error, QtRecordParser::ParseError::PatternMismatch);
    EXPECT_TRUE(result.error_field.isEmpty());
    EXPECT_FALSE(result.error_message.isEmpty());
    EXPECT_TRUE(result.record.values.isEmpty());
}

/**
 * @brief Verifies rejection of an unconfigured placeholder when unknown fields are disabled.
 */
TEST_F(FormatRecordParserTest, RejectsMissingFieldConfiguration)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{missing}");
    configuration.allow_unknown_fields = false;

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("value"), QStringLiteral("source"));

    EXPECT_FALSE(parser.is_valid());
    EXPECT_FALSE(parser.get_configuration_error().isEmpty());
    EXPECT_EQ(result.error, QtRecordParser::ParseError::InvalidConfiguration);
    EXPECT_FALSE(result.error_message.isEmpty());
}

/**
 * @brief Verifies rejection of a field referencing an unavailable converter.
 */
TEST_F(FormatRecordParserTest, RejectsUnknownConverter)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{value}");
    configuration.fields = {{.id = QStringLiteral("value"),
                             .display_name = QStringLiteral("Value"),
                             .capture_pattern = QStringLiteral(".*"),
                             .converter_id = QStringLiteral("unknown"),
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser parser(configuration);

    EXPECT_FALSE(parser.is_valid());
    EXPECT_FALSE(parser.get_configuration_error().isEmpty());
}

/**
 * @brief Verifies rejection of duplicate explicitly configured field identifiers.
 */
TEST_F(FormatRecordParserTest, RejectsDuplicateFieldConfigurations)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{value}");
    configuration.fields = {{.id = QStringLiteral("value"),
                             .display_name = QStringLiteral("First"),
                             .capture_pattern = QStringLiteral(".*"),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = false},
                            {.id = QStringLiteral("value"),
                             .display_name = QStringLiteral("Second"),
                             .capture_pattern = QStringLiteral(".*"),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser parser(configuration);

    EXPECT_FALSE(parser.is_valid());
    EXPECT_FALSE(parser.get_configuration_error().isEmpty());
}

/**
 * @brief Verifies rejection of empty and syntactically invalid capture patterns.
 */
TEST_F(FormatRecordParserTest, RejectsInvalidCapturePatterns)
{
    QtRecordParser::ParserConfiguration empty_pattern;
    empty_pattern.format = QStringLiteral("{value}");
    empty_pattern.fields = {{.id = QStringLiteral("value"),
                             .display_name = QStringLiteral("Value"),
                             .capture_pattern = QString(),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser empty_parser(empty_pattern);

    QtRecordParser::ParserConfiguration invalid_pattern;
    invalid_pattern.format = QStringLiteral("{value}");
    invalid_pattern.fields = {{.id = QStringLiteral("value"),
                               .display_name = QStringLiteral("Value"),
                               .capture_pattern = QStringLiteral("("),
                               .converter_id = QtRecordParser::ConverterId::Text,
                               .converter_options = {},
                               .trim_value = false}};

    const QtRecordParser::FormatRecordParser invalid_parser(invalid_pattern);

    EXPECT_FALSE(empty_parser.is_valid());
    EXPECT_FALSE(empty_parser.get_configuration_error().isEmpty());
    EXPECT_FALSE(invalid_parser.is_valid());
    EXPECT_FALSE(invalid_parser.get_configuration_error().isEmpty());
}

/**
 * @brief Verifies trimming and forwarding of field-specific converter options.
 */
TEST_F(FormatRecordParserTest, AppliesTrimmingAndConverterOptions)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{number}");
    configuration.fields = {{.id = QStringLiteral("number"),
                             .display_name = QStringLiteral("Number"),
                             .capture_pattern = QStringLiteral(".*"),
                             .converter_id = QtRecordParser::ConverterId::Integer,
                             .converter_options = {{QStringLiteral("base"), 16}},
                             .trim_value = true}};

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("  ff  "), QStringLiteral("source"));

    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(result.record.value(QStringLiteral("number")).toLongLong(), 255);
}

/**
 * @brief Verifies that the supplied source identifier is preserved in successful results.
 */
TEST_F(FormatRecordParserTest, PreservesRecordSource)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{value}");

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("content"), QStringLiteral("records/input.txt"));

    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(result.record.source, QStringLiteral("records/input.txt"));
}

/**
 * @brief Verifies that fixed format text is escaped instead of interpreted as regex syntax.
 */
TEST_F(FormatRecordParserTest, EscapesFixedFormatText)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("[{value}].+");
    configuration.allow_unknown_fields = false;
    configuration.fields = {{.id = QStringLiteral("value"),
                             .display_name = QStringLiteral("Value"),
                             .capture_pattern = QStringLiteral(R"(\w+)"),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QtRecordParser::ParseResult exact =
        parser.parse(QStringLiteral("[text].+"), QStringLiteral("source"));
    const QtRecordParser::ParseResult regex_like =
        parser.parse(QStringLiteral("[text]anything"), QStringLiteral("source"));

    EXPECT_TRUE(exact.succeeded());
    EXPECT_EQ(exact.record.value(QStringLiteral("value")).toString(), QStringLiteral("text"));
    EXPECT_EQ(regex_like.error, QtRecordParser::ParseError::PatternMismatch);
}

/**
 * @brief Verifies that parsing requires the complete input to match.
 */
TEST_F(FormatRecordParserTest, AnchorsGeneratedPatternToCompleteInput)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{number}");
    configuration.fields = {{.id = QStringLiteral("number"),
                             .display_name = QStringLiteral("Number"),
                             .capture_pattern = QStringLiteral(R"(\d+)"),
                             .converter_id = QtRecordParser::ConverterId::Integer,
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser parser(configuration);

    EXPECT_TRUE(parser.parse(QStringLiteral("42"), QStringLiteral("source")).succeeded());
    EXPECT_EQ(parser.parse(QStringLiteral("x42"), QStringLiteral("source")).error,
              QtRecordParser::ParseError::PatternMismatch);
    EXPECT_EQ(parser.parse(QStringLiteral("42x"), QStringLiteral("source")).error,
              QtRecordParser::ParseError::PatternMismatch);
}

/**
 * @brief Verifies that replacing the configuration rebuilds the parser state.
 */
TEST_F(FormatRecordParserTest, RebuildsParserAfterConfigurationChange)
{
    QtRecordParser::ParserConfiguration numeric_configuration;
    numeric_configuration.format = QStringLiteral("{number}");
    numeric_configuration.fields = {{.id = QStringLiteral("number"),
                                     .display_name = QStringLiteral("Number"),
                                     .capture_pattern = QStringLiteral(R"(\d+)"),
                                     .converter_id = QtRecordParser::ConverterId::Integer,
                                     .converter_options = {},
                                     .trim_value = false}};

    QtRecordParser::FormatRecordParser parser(numeric_configuration);

    ASSERT_TRUE(parser.parse(QStringLiteral("42"), QStringLiteral("source")).succeeded());

    QtRecordParser::ParserConfiguration boolean_configuration;
    boolean_configuration.format = QStringLiteral("active={active}");
    boolean_configuration.fields = {{.id = QStringLiteral("active"),
                                     .display_name = QStringLiteral("Active"),
                                     .capture_pattern = QStringLiteral("yes|no"),
                                     .converter_id = QtRecordParser::ConverterId::Boolean,
                                     .converter_options = {},
                                     .trim_value = false}};

    parser.set_configuration(boolean_configuration);

    const QtRecordParser::ParseResult result =
        parser.parse(QStringLiteral("active=yes"), QStringLiteral("source"));

    ASSERT_TRUE(parser.is_valid());
    ASSERT_TRUE(result.succeeded());
    EXPECT_TRUE(result.record.value(QStringLiteral("active")).toBool());
    EXPECT_EQ(parser.get_configuration().format, boolean_configuration.format);
    EXPECT_EQ(parser.parse(QStringLiteral("42"), QStringLiteral("source")).error,
              QtRecordParser::ParseError::PatternMismatch);
}

/**
 * @brief Verifies that resolved fields follow placeholder order rather than configuration order.
 */
TEST_F(FormatRecordParserTest, ResolvesFieldsInPlaceholderOrder)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{second}|{first}|{unknown}");
    configuration.allow_unknown_fields = true;
    configuration.fields = {{.id = QStringLiteral("first"),
                             .display_name = QStringLiteral("First"),
                             .capture_pattern = QStringLiteral(R"([^|]+)"),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = false},
                            {.id = QStringLiteral("second"),
                             .display_name = QStringLiteral("Second"),
                             .capture_pattern = QStringLiteral(R"([^|]+)"),
                             .converter_id = QtRecordParser::ConverterId::Text,
                             .converter_options = {},
                             .trim_value = false}};

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QVector<QtRecordParser::FieldConfiguration>& fields = parser.get_resolved_fields();

    ASSERT_TRUE(parser.is_valid());
    ASSERT_EQ(fields.size(), 3);
    EXPECT_EQ(fields.at(0).id, QStringLiteral("second"));
    EXPECT_EQ(fields.at(1).id, QStringLiteral("first"));
    EXPECT_EQ(fields.at(2).id, QStringLiteral("unknown"));
    EXPECT_EQ(fields.at(2).converter_id, QtRecordParser::ConverterId::Text);
}

/**
 * @brief Verifies literal-only formats without any captured fields.
 */
TEST_F(FormatRecordParserTest, ParsesFormatWithoutPlaceholders)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("fixed.record");

    const QtRecordParser::FormatRecordParser parser(configuration);
    const QtRecordParser::ParseResult exact =
        parser.parse(QStringLiteral("fixed.record"), QStringLiteral("source"));
    const QtRecordParser::ParseResult different =
        parser.parse(QStringLiteral("fixedXrecord"), QStringLiteral("source"));

    ASSERT_TRUE(parser.is_valid());
    EXPECT_TRUE(exact.succeeded());
    EXPECT_TRUE(exact.record.values.isEmpty());
    EXPECT_EQ(different.error, QtRecordParser::ParseError::PatternMismatch);
}

/**
 * @brief Verifies configuration serialization and deserialization.
 */
TEST_F(FormatRecordParserTest, ConfigurationRoundTripPreservesFields)
{
    QtRecordParser::ParserConfiguration original;

    original.format = QStringLiteral("{timestamp} {message}");

    original.allow_unknown_fields = false;

    original.fields = {{.id = QStringLiteral("timestamp"),
                        .display_name = QStringLiteral("Timestamp"),
                        .capture_pattern = QStringLiteral(".+?"),
                        .converter_id = QtRecordParser::ConverterId::DateTime,
                        .converter_options = {{QStringLiteral("formats"),
                                               QStringList{QStringLiteral("yyyy-MM-dd HH:mm:ss")}}},
                        .trim_value = true},
                       {.id = QStringLiteral("message"),
                        .display_name = QStringLiteral("Message"),
                        .capture_pattern = QStringLiteral(".*"),
                        .converter_id = QtRecordParser::ConverterId::Text,
                        .converter_options = {},
                        .trim_value = true}};

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
