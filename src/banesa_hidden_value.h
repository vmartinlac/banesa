
#pragma once

#include "banesa_core.h"

template<typename T>
class HiddenValue : public Value
{
public:

    HiddenValue(ValueFactoryPtr factory) : Value(factory)
    {
    }

    T& ref()
    {
        return myValue;
    }

    void bind(sqlite3_stmt* stmt, int& offset) override
    {
    }

protected:

    T myValue;
};

template<typename T>
class HiddenValueFactory : public ValueFactory
{
public:

    HiddenValueFactory(const std::string& name) : ValueFactory(name)
    {
    }

    void getSqlFieldNames(std::vector<std::string>& names) override
    {
        names.clear();
    }

    void getSqlFieldTypes(std::vector<std::string>& sqltypes) override
    {
        sqltypes.clear();
    }

    ValuePtr createValue() override
    {
        return std::make_shared< HiddenValue<T> >(shared_from_this());
    }
};

