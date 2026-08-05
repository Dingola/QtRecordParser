#pragma once

#include <QString>
#include <QStringView>
#include <QVariant>
#include <QVariantMap>

/**
 * @file ValueConverter.h
 * @brief Defines conversion contracts used by QtRecordParser.
 */

namespace QtRecordParser
{
/**
 * @struct ConversionResult
 * @brief Contains a converted value or an explanatory error.
 */
struct ConversionResult {
        QVariant value;
        QString error_message;
        bool success{false};
};

/**
 * @class ValueConverter
 * @brief Converts captured text into an application-independent value.
 *
 * Implementations are immutable and may be shared between parser instances
 * and worker threads.
 */
class ValueConverter
{
    public:
        /**
         * @brief Destroys the converter.
         */
        virtual ~ValueConverter() = default;

        /**
         * @brief Returns the stable identifier used in parser configurations.
         * @return Converter identifier.
         */
        [[nodiscard]] virtual auto get_id() const -> QString = 0;

        /**
         * @brief Converts captured text using field-specific options.
         * @param input Captured field text.
         * @param options Serializable converter options.
         * @return Conversion result.
         */
        [[nodiscard]] virtual auto convert(QStringView input, const QVariantMap& options) const
            -> ConversionResult = 0;
};
}  // namespace QtRecordParser
