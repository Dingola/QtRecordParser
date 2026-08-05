#pragma once

#include <QString>

#include "QtRecordParser/ParseResult.h"
#include "QtRecordParser/ParserConfiguration.h"

/**
 * @file RecordParser.h
 * @brief Declares the generic record parser interface.
 */

namespace QtRecordParser
{
/**
 * @class RecordParser
 * @brief Interface for parsers that produce dynamically structured records.
 *
 * Implementations determine how one complete input record is decoded. Reading
 * files, assembling multiline records and adapting values to application models
 * are intentionally outside this interface.
 */
class RecordParser
{
    public:
        /**
         * @brief Destroys the parser.
         */
        virtual ~RecordParser() = default;

        /**
         * @brief Parses one complete input record.
         * @param input Raw input record.
         * @param source File, stream or dataset identifier.
         * @return Parsed values or structured failure information.
         */
        [[nodiscard]] virtual auto parse(const QString& input,
                                         const QString& source) const -> ParseResult = 0;

        /**
         * @brief Returns the serializable parser configuration.
         * @return Current parser configuration.
         */
        [[nodiscard]] virtual auto get_configuration() const -> const ParserConfiguration& = 0;
};
}  // namespace QtRecordParser
