
#pragma once

#include "banesa_core.h"

template<typename T>
class PrimitiveValue : public Value
{
public:

    PrimitiveValue(ValueFactoryPtr factory) : Value(factory)
    {
        myValue = T();
    }

    void bind(sqlite3_stmt* stmt, int& offset) override;

    T& ref()
    {
        return myValue;
    }

protected:

    T myValue;
};

template<>
inline void PrimitiveValue<int>::bind(sqlite3_stmt* stmt, int& offset)
{
    sqlite3_bind_int(stmt, offset, myValue);
    offset++;
}

template<>
inline void PrimitiveValue<double>::bind(sqlite3_stmt* stmt, int& offset)
{
    sqlite3_bind_double(stmt, offset, myValue);
    offset++;
}

template<typename T>
class PrimitiveValueFactory : public ValueFactory
{
public:

    PrimitiveValueFactory(const std::string& name) : ValueFactory(name)
    {
    }

    void getSqlFieldNames(std::vector<std::string>& names) override
    {
        names.assign({getName()});
    }

    void getSqlFieldTypes(std::vector<std::string>& sqltypes) override;

    ValuePtr createValue() override
    {
        return std::make_shared< PrimitiveValue<T> >(shared_from_this());
    }
};

template<>
inline void PrimitiveValueFactory<int>::getSqlFieldTypes(std::vector<std::string>& sqltypes)
{
    sqltypes.assign({"INTEGER"});
}

template<>
inline void PrimitiveValueFactory<double>::getSqlFieldTypes(std::vector<std::string>& sqltypes)
{
    sqltypes.assign({"FLOAT"});
}

using RealValue = PrimitiveValue<double>;
using RealValueFactory = PrimitiveValueFactory<double>;

using IntegerValue = PrimitiveValue<int>;
using IntegerValueFactory = PrimitiveValueFactory<int>;

