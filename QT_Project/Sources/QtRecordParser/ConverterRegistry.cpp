/**
 * @file ConverterRegistry.cpp
 * @brief Implements converter registration and lookup.
 */

#include "QtRecordParser/ConverterRegistry.h"

#include "QtRecordParser/BuiltInConverters.h"

namespace QtRecordParser
{
auto ConverterRegistry::create_default() -> ConverterRegistry
{
    ConverterRegistry registry;

    registry.add(std::make_shared<TextConverter>());
    registry.add(std::make_shared<IntegerConverter>());
    registry.add(std::make_shared<FloatingPointConverter>());
    registry.add(std::make_shared<BooleanConverter>());
    registry.add(std::make_shared<DateTimeConverter>());

    return registry;
}

auto ConverterRegistry::add(std::shared_ptr<const ValueConverter> converter) -> bool
{
    const bool valid = converter != nullptr && !converter->get_id().trimmed().isEmpty();

    if (valid)
    {
        m_converters.insert(converter->get_id(), std::move(converter));
    }

    return valid;
}

auto ConverterRegistry::get(const QString& converter_id) const
    -> std::shared_ptr<const ValueConverter>
{
    return m_converters.value(converter_id);
}

auto ConverterRegistry::contains(const QString& converter_id) const -> bool
{
    return m_converters.contains(converter_id);
}
}  // namespace QtRecordParser
