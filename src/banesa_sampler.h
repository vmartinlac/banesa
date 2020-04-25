
#pragma once

#include "banesa_core.h"

class Sampler
{
public:

    void run( const std::vector<NodePtr>& graph, int num_samples, const std::string& db_path, bool multithread=false);

private:

    struct ValueTable
    {
        std::vector<ValuePtr> values;
        int sample;
    };

    using ValueTablePtr = std::shared_ptr<ValueTable>;

    class SourceBody;
    class SamplerBody;
    class ExportBody;

private:

    static bool initializeDatabase(const std::vector<NodePtr>& graph, sqlite3*& db, const std::string& db_path);
    static bool createInsertionStatement(sqlite3* db, std::vector<ValueFactoryPtr>& values, sqlite3_stmt** stmt);
    static bool reorderNodes(const std::vector<NodePtr>& graph, std::vector<NodePtr>& ordered_nodes);
    static bool saveSample(sqlite3_stmt* insert_stmt, const std::vector<ValuePtr>& values);
};

