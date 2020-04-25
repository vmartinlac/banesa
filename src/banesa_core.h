
#pragma once

#include <vector>
#include <memory>
#include <sqlite3.h>

class ValueFactory;

using ValueFactoryPtr = std::shared_ptr<ValueFactory>;

class Value
{
public:

    Value(ValueFactoryPtr factory) : myFactory(factory)
    {
    }

    ValueFactoryPtr getFactory()
    {
        return myFactory;
    }

    virtual void bind(sqlite3_stmt* stmt, int& offset) = 0;

private:

    ValueFactoryPtr myFactory;
};

using ValuePtr = std::shared_ptr<Value>;

class ValueFactory : public std::enable_shared_from_this<ValueFactory>
{
public:

    ValueFactory(const std::string& name) : myName(name)
    {
    }

    std::string getName()
    {
        return myName;
    }

    virtual void getSqlFieldNames(std::vector<std::string>& names) = 0;
    virtual void getSqlFieldTypes(std::vector<std::string>& sqltypes) = 0;

    virtual ValuePtr createValue() = 0;

private:

    std::string myName;
};

class Node
{
public:

    Node()
    {
    }

    std::string getName()
    {
        return myName;
    }

    const std::vector<std::string>& refDependencies()
    {
        return myDependencies;
    }

    const std::vector<ValueFactoryPtr>& refValueFactories()
    {
        return myValueFactories;
    }

    virtual void getSample(const std::vector<ValuePtr>& input, std::vector<ValuePtr>& output) = 0;

protected:

    void setName(const std::string& name)
    {
        myName = name;
    }

    void registerDependency(const std::string& dependency)
    {
        myDependencies.push_back(dependency);
    }

    void registerValueFactory(ValueFactoryPtr factory)
    {
        myValueFactories.push_back(std::move(factory));
    }

private:

    std::string myName;
    std::vector<ValueFactoryPtr> myValueFactories;
    std::vector<std::string> myDependencies;
};

using NodePtr = std::shared_ptr<Node>;

