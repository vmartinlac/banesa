
#pragma once

#include "banesa_core.h"

class SE3Value : public Value
{
public:

    SE3Value(ValueFactoryPtr factory) : Value(factory)
    {
        myTranslationX = 0.0;
        myTranslationY = 0.0;
        myTranslationZ = 0.0;
        myQuaternionW = 0.0;
        myQuaternionI = 0.0;
        myQuaternionJ = 0.0;
        myQuaternionK = 0.0;
    }

    double& refTranslationX()
    {
        return myTranslationX;
    }

    double& refTranslationY()
    {
        return myTranslationY;
    }

    double& refTranslationZ()
    {
        return myTranslationZ;
    }

    double& refQuaternionW()
    {
        return myQuaternionW;
    }

    double& refQuaternionI()
    {
        return myQuaternionI;
    }

    double& refQuaternionJ()
    {
        return myQuaternionJ;
    }

    double& refQuaternionK()
    {
        return myQuaternionK;
    }

    void bind(sqlite3_stmt* stmt, int& offset) override
    {
        sqlite3_bind_double(stmt, offset+0, myTranslationX);
        sqlite3_bind_double(stmt, offset+1, myTranslationY);
        sqlite3_bind_double(stmt, offset+2, myTranslationZ);
        sqlite3_bind_double(stmt, offset+3, myQuaternionW);
        sqlite3_bind_double(stmt, offset+4, myQuaternionI);
        sqlite3_bind_double(stmt, offset+5, myQuaternionJ);
        sqlite3_bind_double(stmt, offset+6, myQuaternionK);
        offset += 7;
    }

protected:

    double myTranslationX;
    double myTranslationY;
    double myTranslationZ;
    double myQuaternionW;
    double myQuaternionI;
    double myQuaternionJ;
    double myQuaternionK;
};

class SE3ValueFactory : public ValueFactory
{
public:

    SE3ValueFactory(const std::string& name) : ValueFactory(name)
    {
    }

    void getSqlFieldNames(std::vector<std::string>& names) override
    {
        names.clear();
        names.push_back( getName() + "_translation_x" );
        names.push_back( getName() + "_translation_y" );
        names.push_back( getName() + "_translation_z" );
        names.push_back( getName() + "_quaternion_w" );
        names.push_back( getName() + "_quaternion_i" );
        names.push_back( getName() + "_quaternion_j" );
        names.push_back( getName() + "_quaternion_k" );
    }

    void getSqlFieldTypes(std::vector<std::string>& sqltypes) override
    {
        sqltypes.assign({"FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT", "FLOAT"});
    }

    ValuePtr createValue() override
    {
        return std::make_shared<SE3Value>(shared_from_this());
    }
};

