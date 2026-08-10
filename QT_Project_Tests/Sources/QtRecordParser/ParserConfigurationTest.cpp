/**
 * @file ParserConfigurationTest.cpp
 * @brief Tests parser configuration JSON conversion.
 */

#include "QtRecordParser/ParserConfigurationTest.h"

#include <QJsonArray>

#include "QtRecordParser/BuiltInConverters.h"
#include "QtRecordParser/ParserConfiguration.h"

/**
 * @brief Verifies serialization of all configuration and field properties.
 */
TEST_F(ParserConfigurationTest, SerializesCompleteConfiguration)
{
    QtRecordParser::ParserConfiguration configuration;
    configuration.format = QStringLiteral("{identifier};{name}");
    configuration.allow_unknown_fields = false;
    configuration.fields = {
        {QStringLiteral("identifier"), QStringLiteral("Identifier"), QStringLiteral(R"([0-9a-f]+)"),
         QtRecordParser::ConverterId::Integer, QVariantMap{{QStringLiteral("base"), 16}}, true},
        {QStringLiteral("name"), QStringLiteral("Name"), QStringLiteral(R"([^;]+)"),
         QtRecordParser::ConverterId::Text, QVariantMap(), false}};

    const QJsonObject object = configuration.to_json();

    EXPECT_EQ(object.value(QStringLiteral("format")).toString(), configuration.format);
    EXPECT_FALSE(object.value(QStringLiteral("allow_unknown_fields")).toBool());

    const QJsonArray fields = object.value(QStringLiteral("fields")).toArray();
    ASSERT_EQ(fields.size(), 2);

    const QJsonObject identifier = fields.at(0).toObject();
    EXPECT_EQ(identifier.value(QStringLiteral("id")).toString(), QStringLiteral("identifier"));
    EXPECT_EQ(identifier.value(QStringLiteral("display_name")).toString(),
              QStringLiteral("Identifier"));
    EXPECT_EQ(identifier.value(QStringLiteral("pattern")).toString(),
              QStringLiteral(R"([0-9a-f]+)"));
    EXPECT_EQ(identifier.value(QStringLiteral("converter")).toString(),
              QtRecordParser::ConverterId::Integer);
    EXPECT_EQ(identifier.value(QStringLiteral("converter_options"))
                  .toObject()
                  .value(QStringLiteral("base"))
                  .toInt(),
              16);
    EXPECT_TRUE(identifier.value(QStringLiteral("trim")).toBool());
}

/**
 * @brief Verifies deserialization of a complete valid configuration.
 */
TEST_F(ParserConfigurationTest, DeserializesCompleteConfiguration)
{
    const QJsonObject field{{QStringLiteral("id"), QStringLiteral("active")},
                            {QStringLiteral("display_name"), QStringLiteral("Active")},
                            {QStringLiteral("pattern"), QStringLiteral("yes|no")},
                            {QStringLiteral("converter"), QtRecordParser::ConverterId::Boolean},
                            {QStringLiteral("converter_options"),
                             QJsonObject{{QStringLiteral("case_sensitive"), true}}},
                            {QStringLiteral("trim"), true}};

    const QJsonObject object{{QStringLiteral("format"), QStringLiteral("{active}")},
                             {QStringLiteral("allow_unknown_fields"), false},
                             {QStringLiteral("fields"), QJsonArray{field}}};

    QString error = QStringLiteral("previous error");

    const QtRecordParser::ParserConfiguration configuration =
        QtRecordParser::ParserConfiguration::from_json(object, &error);

    EXPECT_TRUE(error.isEmpty());
    EXPECT_EQ(configuration.format, QStringLiteral("{active}"));
    EXPECT_FALSE(configuration.allow_unknown_fields);
    ASSERT_EQ(configuration.fields.size(), 1);

    const QtRecordParser::FieldConfiguration& restored = configuration.fields.first();
    EXPECT_EQ(restored.id, QStringLiteral("active"));
    EXPECT_EQ(restored.display_name, QStringLiteral("Active"));
    EXPECT_EQ(restored.capture_pattern, QStringLiteral("yes|no"));
    EXPECT_EQ(restored.converter_id, QtRecordParser::ConverterId::Boolean);
    EXPECT_TRUE(restored.converter_options.value(QStringLiteral("case_sensitive")).toBool());
    EXPECT_TRUE(restored.trim_value);
}

/**
 * @brief Verifies default values for omitted optional field properties.
 */
TEST_F(ParserConfigurationTest, AppliesDefaultsToOptionalProperties)
{
    const QJsonObject object{
        {QStringLiteral("format"), QStringLiteral("{value}")},
        {QStringLiteral("fields"),
         QJsonArray{QJsonObject{{QStringLiteral("id"), QStringLiteral("value")}}}}};

    QString error;

    const QtRecordParser::ParserConfiguration configuration =
        QtRecordParser::ParserConfiguration::from_json(object, &error);

    EXPECT_TRUE(error.isEmpty());
    EXPECT_TRUE(configuration.allow_unknown_fields);
    ASSERT_EQ(configuration.fields.size(), 1);

    const QtRecordParser::FieldConfiguration& field = configuration.fields.first();
    EXPECT_EQ(field.id, QStringLiteral("value"));
    EXPECT_TRUE(field.display_name.isEmpty());
    EXPECT_EQ(field.capture_pattern, QStringLiteral(".*?"));
    EXPECT_EQ(field.converter_id, QtRecordParser::ConverterId::Text);
    EXPECT_TRUE(field.converter_options.isEmpty());
    EXPECT_FALSE(field.trim_value);
}

/**
 * @brief Verifies rejection when the fields member is missing or not an array.
 */
TEST_F(ParserConfigurationTest, RejectsNonArrayFieldsMember)
{
    const QJsonObject object{{QStringLiteral("format"), QStringLiteral("{value}")},
                             {QStringLiteral("fields"), QJsonObject()}};

    QString error;

    const QtRecordParser::ParserConfiguration configuration =
        QtRecordParser::ParserConfiguration::from_json(object, &error);

    EXPECT_FALSE(error.isEmpty());
    EXPECT_TRUE(configuration.format.isEmpty());
    EXPECT_TRUE(configuration.fields.isEmpty());
    EXPECT_TRUE(configuration.allow_unknown_fields);
}

/**
 * @brief Verifies rejection of field entries that are not JSON objects.
 */
TEST_F(ParserConfigurationTest, RejectsNonObjectFieldDefinition)
{
    const QJsonObject object{{QStringLiteral("format"), QStringLiteral("{value}")},
                             {QStringLiteral("fields"), QJsonArray{QStringLiteral("value")}}};

    QString error;

    const QtRecordParser::ParserConfiguration configuration =
        QtRecordParser::ParserConfiguration::from_json(object, &error);

    EXPECT_FALSE(error.isEmpty());
    EXPECT_TRUE(configuration.format.isEmpty());
    EXPECT_TRUE(configuration.fields.isEmpty());
}

/**
 * @brief Verifies rejection of empty and whitespace-only field identifiers.
 */
TEST_F(ParserConfigurationTest, RejectsBlankFieldIdentifier)
{
    const QJsonObject object{
        {QStringLiteral("format"), QStringLiteral("{value}")},
        {QStringLiteral("allow_unknown_fields"), false},
        {QStringLiteral("fields"),
         QJsonArray{QJsonObject{{QStringLiteral("id"), QStringLiteral("   ")}}}}};

    QString error;

    const QtRecordParser::ParserConfiguration configuration =
        QtRecordParser::ParserConfiguration::from_json(object, &error);

    EXPECT_FALSE(error.isEmpty());
    EXPECT_TRUE(configuration.format.isEmpty());
    EXPECT_TRUE(configuration.fields.isEmpty());
    EXPECT_TRUE(configuration.allow_unknown_fields);
}
