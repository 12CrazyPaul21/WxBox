#include <utils/common.h>

wb_config::SafeYamlNodeIterator::value_type wb_config::SafeYamlNodeIterator::operator*() const
{
    return SafeYamlNodeIterator::base::operator*();
}

wb_config::SafeYamlNodeConstIterator::value_type wb_config::SafeYamlNodeConstIterator::operator*() const
{
    return SafeYamlNodeConstIterator::base::operator*();
}