/**
 * @file ConverterRegistryTest.cpp
 * @brief Tests converter registration and lookup.
 */

#include "QtRecordParser/ConverterRegistryTest.h"

#include <memory>
#include <utility>

#include "QtRecordParser/BuiltInConverters.h"
#include "QtRecordParser/ConverterRegistry.h"

namespace
{
/**
 * @class TestConverter
 * @brief Minimal configurable converter used by registry tests.
 */
class TestConverter final: public QtRecordParser::ValueConverter
{
    public:
        explicit TestConverter(QString id, QString value = QStringLiteral("converted"))
            : m_id(std::move(id)), m_value(std::move(value))
        {}

        [[nodiscard]] auto get_id() const -> QString override
        {
            return m_id;
        }

        [[nodiscard]] auto convert(QStringView input, const QVariantMap& options) const
            -> QtRecordParser::ConversionResult override
        {
            Q_UNUSED(input);
            Q_UNUSED(options);

            return QtRecordParser::ConversionResult{m_value, QString(), true};
        }

    private:
        QString m_id;
        QString m_value;
};
}  // namespace

/**
 * @brief Verifies that every built-in converter is registered by default.
 */
TEST_F(ConverterRegistryTest, ContainsAllBuiltInConverters)
{
    const QtRecordParser::ConverterRegistry registry =
        QtRecordParser::ConverterRegistry::create_default();

    EXPECT_TRUE(registry.contains(QtRecordParser::ConverterId::Text));
    EXPECT_TRUE(registry.contains(QtRecordParser::ConverterId::Integer));
    EXPECT_TRUE(registry.contains(QtRecordParser::ConverterId::FloatingPoint));
    EXPECT_TRUE(registry.contains(QtRecordParser::ConverterId::Boolean));
    EXPECT_TRUE(registry.contains(QtRecordParser::ConverterId::DateTime));

    EXPECT_NE(registry.get(QtRecordParser::ConverterId::Text), nullptr);
    EXPECT_NE(registry.get(QtRecordParser::ConverterId::Integer), nullptr);
    EXPECT_NE(registry.get(QtRecordParser::ConverterId::FloatingPoint), nullptr);
    EXPECT_NE(registry.get(QtRecordParser::ConverterId::Boolean), nullptr);
    EXPECT_NE(registry.get(QtRecordParser::ConverterId::DateTime), nullptr);
}

/**
 * @brief Verifies that unknown identifiers are not reported or resolved.
 */
TEST_F(ConverterRegistryTest, ReturnsNullForUnknownConverter)
{
    const QtRecordParser::ConverterRegistry registry =
        QtRecordParser::ConverterRegistry::create_default();

    const QString unknown_id = QStringLiteral("unknown");

    EXPECT_FALSE(registry.contains(unknown_id));
    EXPECT_EQ(registry.get(unknown_id), nullptr);
}

/**
 * @brief Verifies registration and lookup of an application-defined converter.
 */
TEST_F(ConverterRegistryTest, RegistersCustomConverter)
{
    QtRecordParser::ConverterRegistry registry;

    const auto converter = std::make_shared<TestConverter>(QStringLiteral("custom"));

    ASSERT_TRUE(registry.add(converter));
    EXPECT_TRUE(registry.contains(QStringLiteral("custom")));
    EXPECT_EQ(registry.get(QStringLiteral("custom")), converter);
}

/**
 * @brief Verifies that a later converter replaces an existing identifier.
 */
TEST_F(ConverterRegistryTest, ReplacesConverterWithSameIdentifier)
{
    QtRecordParser::ConverterRegistry registry;

    const auto original =
        std::make_shared<TestConverter>(QStringLiteral("replaceable"), QStringLiteral("original"));

    const auto replacement = std::make_shared<TestConverter>(QStringLiteral("replaceable"),
                                                             QStringLiteral("replacement"));

    ASSERT_TRUE(registry.add(original));
    ASSERT_TRUE(registry.add(replacement));

    const std::shared_ptr<const QtRecordParser::ValueConverter> resolved =
        registry.get(QStringLiteral("replaceable"));

    ASSERT_EQ(resolved, replacement);

    const QtRecordParser::ConversionResult result =
        resolved->convert(QStringLiteral("input"), QVariantMap());

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.value.toString(), QStringLiteral("replacement"));
}

/**
 * @brief Verifies that a null converter is rejected without changing the registry.
 */
TEST_F(ConverterRegistryTest, RejectsNullConverter)
{
    QtRecordParser::ConverterRegistry registry;

    const std::shared_ptr<const QtRecordParser::ValueConverter> converter;

    EXPECT_FALSE(registry.add(converter));
    EXPECT_FALSE(registry.contains(QString()));
}

/**
 * @brief Verifies rejection of empty and whitespace-only converter identifiers.
 */
TEST_F(ConverterRegistryTest, RejectsBlankConverterIdentifiers)
{
    QtRecordParser::ConverterRegistry registry;

    const auto empty_id = std::make_shared<TestConverter>(QString());
    const auto whitespace_id = std::make_shared<TestConverter>(QStringLiteral("   "));

    EXPECT_FALSE(registry.add(empty_id));
    EXPECT_FALSE(registry.add(whitespace_id));
    EXPECT_FALSE(registry.contains(QString()));
    EXPECT_FALSE(registry.contains(QStringLiteral("   ")));
}
