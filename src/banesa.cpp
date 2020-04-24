#include <iostream>
#include <sstream>
#include <map>
#include "banesa.h"

bool Sampler::createTable(sqlite3* db, const std::vector<NodePtr>& graph)
{
    std::vector<std::string> field_names;
    std::vector<std::string> field_types;
    std::vector<std::string> local_field_names;
    std::vector<std::string> local_field_types;

    field_names.assign({"id"});
    field_types.assign({"INTEGER PRIMARY KEY"});

    for(NodePtr n : graph)
    {
        for( const ValueFactoryPtr vf : n->refValueFactories() )
        {
            vf->getSqlFieldNames(local_field_names);
            vf->getSqlFieldTypes(local_field_types);

            field_names.insert( field_names.end(), local_field_names.begin(), local_field_names.end() );
            field_types.insert( field_types.end(), local_field_types.begin(), local_field_types.end() );
        }
    }

    std::stringstream query;
    query << "DROP TABLE IF EXISTS samples;";
    query << "CREATE TABLE samples(";
    for(size_t i=0; i<field_names.size(); i++)
    {
        if( i > 0 )
        {
            query << ", ";
        }

        query << field_names[i] << " " << field_types[i];
    }
    query << ");";

    return (SQLITE_OK == sqlite3_exec(db, query.str().c_str(), nullptr, nullptr, nullptr));
}

bool Sampler::createInsertionStatement(sqlite3* db, std::vector<ValuePtr>& values, sqlite3_stmt** stmt)
{
    std::stringstream sql;
    size_t field_count = 0;
    std::vector<std::string> local_field_names;

    sql << "INSERT INTO samples(";

    for(size_t i=0; i<values.size(); i++)
    {
        values[i]->getFactory()->getSqlFieldNames(local_field_names);

        for(std::string& field_name : local_field_names)
        {
            if(field_count > 0)
            {
                sql << ", ";
            }

            sql << field_name;

            field_count++;
        }
    }

    sql << ") VALUES (";

    for(size_t i=0; i<field_count; i++)
    {
        if( i > 0 )
        {
            sql << ", ";
        }
        sql << "?";
    }
    sql << ")";

    return (SQLITE_OK == sqlite3_prepare_v2(db, sql.str().c_str(), -1, stmt, nullptr));
}

void Sampler::run( const std::vector<NodePtr>& graph, int num_samples, const std::string& db_path)
{
    bool ok = true;
    const char* err = "";
    sqlite3* db = nullptr;
    sqlite3_stmt* insert_stmt = nullptr;

    std::vector<ValuePtr> values;
    std::map<std::string, size_t> offset;
    std::map<std::string, NodePtr> node_map;

    std::vector<NodePtr> ordered_nodes;

    if(ok)
    {
        ok = (SQLITE_OK == sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, nullptr));
        err = "Could not open database!";
    }

    if(ok)
    {
        ok = createTable(db, graph);
        err = "Could not create table!";
    }

    // allocate values.

    if(ok)
    {
        values.clear();
        offset.clear();
        node_map.clear();

        for(NodePtr n : graph)
        {
            offset[n->getName()] = values.size();
            node_map[n->getName()] = n;

            for( ValueFactoryPtr vf : n->refValueFactories() )
            {
                values.push_back(vf->createValue());
            }
        }
    }

    // create statement to save a record to database.

    if(ok)
    {
        ok = createInsertionStatement(db, values, &insert_stmt);
        err = "Could not create insertion statement!";
    }

    // Compute in which order to process the nodes.

    {
        ordered_nodes = graph; // TODO
    }

    // proceed with sampling.

    std::vector<ValuePtr> input_values;
    std::vector<ValuePtr> output_values;
    for(int i=0; ok && i<num_samples; i++)
    {
        // compute sample.

        for(NodePtr node : ordered_nodes)
        {
            // gather input values.

            input_values.clear();

            for(std::string other_node_name : node->refDependencies())
            {
                NodePtr other_node = node_map[other_node_name];

                std::copy(
                    values.begin() + offset[other_node_name],
                    values.begin() + offset[other_node_name] + other_node->refValueFactories().size(),
                    std::back_inserter(input_values));
            }

            // gather output values.

            output_values.clear();

            std::copy(
                values.begin() + offset[node->getName()],
                values.begin() + offset[node->getName()] + node->refValueFactories().size(),
                std::back_inserter(output_values));

            node->getSample(input_values, output_values);
        }

        // save sample to database.

        {
            sqlite3_reset(insert_stmt);

            int field_offset = 1;

            for(ValuePtr v : values)
            {
                v->bind(insert_stmt, field_offset);
            }

            ok = (SQLITE_DONE == sqlite3_step(insert_stmt));
            err = "Could not insert sample to database!";
        }
    }

    if(ok)
    {
        sqlite3_close_v2(db);
        db = nullptr;
    }

    if(ok == false)
    {
        std::cout << err << std::endl;
        exit(1);
    }
}

