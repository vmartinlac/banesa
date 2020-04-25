#include <iostream>
#include <sstream>
#include <stack>
#include <set>
#include <map>
#include <tbb/flow_graph.h>
#include "banesa.h"

class Sampler::SourceBody
{
public:

    SourceBody(int num_samples)
    {
        myNumSamples = num_samples;
        myNextSample = 0;
    }

    bool operator()(int& msg)
    {
        bool ret = false;

        if(myNextSample < myNumSamples)
        {
            ret = true;
            msg = myNextSample;
            myNextSample++;
        }

        return ret;
    }

protected:

    int myNumSamples;
    int myNextSample;
};

class Sampler::SamplerBody
{
public:

    SamplerBody(
        const std::vector<NodePtr>& ordered_nodes,
        const std::map<std::string,NodePtr>& node_map,
        const std::map<std::string,size_t>& offset,
        const std::vector<ValueFactoryPtr>& value_factories) :

        myOrderedNodes(ordered_nodes),
        myNodeMap(node_map),
        myOffset(offset),
        myValueFactories(value_factories)
    {
    }

    ValueTablePtr operator()(int sample)
    {
        std::vector<ValuePtr> input_values;
        std::vector<ValuePtr> output_values;

        ValueTablePtr ret = std::make_shared<ValueTable>();

        ret->sample = sample;

        for(ValueFactoryPtr factory : myValueFactories)
        {
            ret->values.push_back(factory->createValue());
        }

        for(NodePtr node : myOrderedNodes)
        {
            // gather input values.

            input_values.clear();

            for(std::string other_node_name : node->refDependencies())
            {
                NodePtr other_node = myNodeMap.find(other_node_name)->second;

                std::copy(
                    ret->values.begin() + myOffset.find(other_node_name)->second,
                    ret->values.begin() + myOffset.find(other_node_name)->second + other_node->refValueFactories().size(),
                    std::back_inserter(input_values));
            }

            // gather output values.

            output_values.clear();

            std::copy(
                ret->values.begin() + myOffset.find(node->getName())->second,
                ret->values.begin() + myOffset.find(node->getName())->second + node->refValueFactories().size(),
                std::back_inserter(output_values));

            // call sampling function.

            node->getSample(input_values, output_values);
        }

        return ret;
    }

protected:

    const std::vector<NodePtr>& myOrderedNodes;
    const std::map<std::string,NodePtr>& myNodeMap;
    const std::map<std::string,size_t>& myOffset;
    const std::vector<ValueFactoryPtr>& myValueFactories;
};

class Sampler::ExportBody
{
public:

    ExportBody(sqlite3_stmt* insert_stmt)
    {
        myInsertStatement = insert_stmt;
    }

    tbb::flow::continue_msg operator()(const ValueTablePtr& value_table)
    {
        const bool ok = saveSample(myInsertStatement, value_table->values);

        if(ok == false)
        {
            std::cout << "Could not insert sample to database!" << std::endl;
        }

        return tbb::flow::continue_msg();
    }

protected:

    sqlite3_stmt* myInsertStatement;
};

bool Sampler::initializeDatabase(const std::vector<NodePtr>& graph, sqlite3*& db, const std::string& db_path)
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

    bool ok = true;

    if(ok)
    {
        ok = (SQLITE_OK == sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, nullptr));
    }

    if(ok)
    {
        ok = (SQLITE_OK == sqlite3_exec(db, query.str().c_str(), nullptr, nullptr, nullptr));
    }

    return ok;
}

bool Sampler::createInsertionStatement(sqlite3* db, std::vector<ValueFactoryPtr>& value_factories, sqlite3_stmt** stmt)
{
    std::stringstream sql;
    size_t field_count = 0;
    std::vector<std::string> local_field_names;

    sql << "INSERT INTO samples(";

    for(size_t i=0; i<value_factories.size(); i++)
    {
        value_factories[i]->getSqlFieldNames(local_field_names);

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

void Sampler::run( const std::vector<NodePtr>& graph, int num_samples, const std::string& db_path, bool multithread)
{
    bool ok = true;
    const char* err = "";
    sqlite3* db = nullptr;
    sqlite3_stmt* insert_stmt = nullptr;

    std::vector<ValuePtr> values;
    std::vector<ValueFactoryPtr> value_factories;
    std::map<std::string, size_t> offset;
    std::map<std::string, NodePtr> node_map;

    std::vector<NodePtr> ordered_nodes;

    std::vector<ValuePtr> input_values;
    std::vector<ValuePtr> output_values;

    if(ok)
    {
        ok = initializeDatabase(graph, db, db_path);
        err = "Could not initialize database!";
    }

    // allocate values.

    if(ok)
    {
        values.clear();
        offset.clear();
        node_map.clear();

        for(NodePtr n : graph)
        {
            offset[n->getName()] = value_factories.size();
            node_map[n->getName()] = n;

            for( ValueFactoryPtr vf : n->refValueFactories() )
            {
                value_factories.push_back(vf);
            }

            if( multithread == false )
            {
                for( ValueFactoryPtr vf : n->refValueFactories() )
                {
                    values.push_back(vf->createValue());
                }
            }
        }
    }

    // create statement to save a record to database.

    if(ok)
    {
        ok = createInsertionStatement(db, value_factories, &insert_stmt);
        err = "Could not create insertion statement!";
    }

    // Compute in which order to process the nodes.

    if(ok)
    {
        ok = reorderNodes(graph, ordered_nodes);
        err = "Incorrect graph!";
    }

    // proceed with sampling.

    if(ok)
    {
        if(multithread)
        {
            tbb::flow::graph g;

            tbb::flow::source_node<int> source_node(g, SourceBody(num_samples), false);
            tbb::flow::limiter_node<int> limiter_node(g, 10);

            tbb::flow::function_node<int, ValueTablePtr> sampler_node(g, 0, SamplerBody(ordered_nodes, node_map, offset, value_factories));
            tbb::flow::function_node<ValueTablePtr, tbb::flow::continue_msg> export_node(g, 1, ExportBody(insert_stmt));

            make_edge(source_node, limiter_node);
            make_edge(limiter_node, sampler_node);
            make_edge(sampler_node, export_node);
            make_edge(export_node, limiter_node.decrement);

            source_node.activate();
            g.wait_for_all();
        }
        else
        {
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

                    // call sampling function.

                    node->getSample(input_values, output_values);
                }

                // save sample to database.

                ok = saveSample(insert_stmt, values);
                err = "Could not insert sample to database!";
            }
        }
    }

    if(ok)
    {
        ok = (SQLITE_OK == sqlite3_finalize(insert_stmt));
        err = "Could not release insertion statement!";
    }

    if(ok)
    {
        ok = (SQLITE_OK == sqlite3_close_v2(db));
        err = "Could not close database!";
        db = nullptr;
    }

    if(ok == false)
    {
        std::cout << err << std::endl;
        exit(1);
    }
}

bool Sampler::saveSample(sqlite3_stmt* insert_stmt, const std::vector<ValuePtr>& values)
{
    sqlite3_reset(insert_stmt);

    int field_offset = 1;

    for(ValuePtr v : values)
    {
        v->bind(insert_stmt, field_offset);
    }

    return (SQLITE_DONE == sqlite3_step(insert_stmt));
}

bool Sampler::reorderNodes(const std::vector<NodePtr>& graph, std::vector<NodePtr>& ordered_nodes)
{
    std::map< std::string, std::vector<NodePtr> > children;
    std::set< std::string > processed;
    std::stack<NodePtr> stack;

    for(NodePtr node : graph)
    {
        for(std::string parent : node->refDependencies())
        {
            children[parent].push_back(node);
        }
    }

    ordered_nodes.clear();

    for(NodePtr root_node : graph)
    {
        if( processed.count(root_node->getName()) == 0 && root_node->refDependencies().empty() )
        {
            stack.push(root_node);
            ordered_nodes.push_back(root_node);
            processed.insert(root_node->getName());

            while(stack.empty() == false)
            {
                NodePtr node = stack.top();
                stack.pop();

                for(NodePtr child : children[node->getName()])
                {
                    if(processed.count(child->getName()) == 0)
                    {
                        bool all_dependencies_available = true;
                        for(std::string parent_name : child->refDependencies())
                        {
                            all_dependencies_available = all_dependencies_available && (processed.count(parent_name) > 0);
                        }

                        if(all_dependencies_available)
                        {
                            stack.push(child);
                            ordered_nodes.push_back(child);
                            processed.insert(child->getName());
                        }
                    }
                }
            }
        }
    }

    /*
    for(NodePtr n : graph) std::cout << n->getName() << " ";
    std::cout << std::endl;
    for(NodePtr n : ordered_nodes) std::cout << n->getName() << " ";
    std::cout << std::endl;
    */

    return true;
}

