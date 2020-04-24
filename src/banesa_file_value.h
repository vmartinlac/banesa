
#pragma once

#include "banesa_core.h"

template<typename T>
class FileValue : public Value
{
public:

    FileValue(ValueFactoryPtr factory) : Value(factory)
    {
    }

    T& ref()
    {
        return myValue;
    }

    void setPath(const std::string& path)
    {
        myPath = path;
    }

    void bind(sqlite3_stmt* stmt, int& offset) override
    {
        sqlite3_bind_text(stmt, offset, myPath.c_str(), -1, SQLITE_TRANSIENT);
        offset++;
    }

protected:

    std::string myPath;
    T myValue;
};

template<typename T>
class FileValueFactory : public ValueFactory
{
public:

    FileValueFactory(const std::string& name) : ValueFactory(name)
    {
    }

    void getSqlFieldNames(std::vector<std::string>& names) override
    {
        const std::string field_name = getName() + "_path";
        names.assign({ field_name });
    }

    void getSqlFieldTypes(std::vector<std::string>& sqltypes) override
    {
        sqltypes.assign({"TEXT"});
    }

    ValuePtr createValue() override
    {
        return std::make_shared< FileValue<T> >(shared_from_this());
    }
};

