#pragma once

#include <QMap>
#include <QString>
#include <memory>

#include "QtRecordParser/ValueConverter.h"

/**
 * @file ConverterRegistry.h
 * @brief Declares a registry resolving serializable converter identifiers.
 */

namespace QtRecordParser
{
/**
 * @class ConverterRegistry
 * @brief Stores immutable converters under stable identifiers.
 */
class ConverterRegistry
{
    public:
        /**
         * @brief Creates a registry containing the built-in converters.
         * @return Configured registry.
         */
        [[nodiscard]] static auto create_default() -> ConverterRegistry;

        /**
         * @brief Adds or replaces a converter.
         * @param converter Immutable converter instance.
         * @return True when a valid converter was registered.
         */
        auto add(std::shared_ptr<const ValueConverter> converter) -> bool;

        /**
         * @brief Resolves a converter.
         * @param converter_id Stable converter identifier.
         * @return Shared converter or nullptr when unknown.
         */
        [[nodiscard]] auto get(const QString& converter_id) const
            -> std::shared_ptr<const ValueConverter>;

        /**
         * @brief Returns whether a converter identifier is registered.
         * @param converter_id Stable converter identifier.
         * @return True when the converter exists.
         */
        [[nodiscard]] auto contains(const QString& converter_id) const -> bool;

    private:
        QMap<QString, std::shared_ptr<const ValueConverter>> m_converters;
};
}  // namespace QtRecordParser
