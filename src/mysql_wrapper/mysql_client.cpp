#include "mysql_client.h"

#include <chrono>

void MysqlClient::Execute(const std::string& query)
{
#ifndef LOG_MYSQL_QUERY
    auto query_res = RealQuery(query);
#else
    auto query_res = LoggedRealQuery(query);
#endif//LOG_MYSQL_QUERY
    if (!query_res) {
        auto ec = query_res.error();
        LOG_ERROR << "Error executing MySQL query \"" << "\": " << ec.message() << " (" << ec.value() << ")";
        return;
    }
}

MysqlClient::ResultRowPtr MysqlClient::QueryOne(const std::string& query)
{
#ifndef LOG_MYSQL_QUERY
    auto query_res = RealQuery(query);
#else
    auto query_res = LoggedRealQuery(query);
#endif//LOG_MYSQL_QUERY
    if (!query_res) {
        auto ec = query_res.error();

        LOG_ERROR << "Error executing MySQL query \"" << "\": " << ec.message() << " (" << ec.value() << ")";
        return {};
    }

    if (!query_res.value()) return {};

    auto* res = query_res.value().get();

    // get column info and give it to field validator,
    // which should throw if it doesn't like the columns
    unsigned int filed = mysql_num_fields(res);
    if (filed == 0) return {};
   
    if (MYSQL_ROW row = mysql_fetch_row(res)) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        return std::make_unique<ResultRow>(row, lengths, query_res.value().release(), filed);
    }

    return {};
}

void MysqlClient::Query(const std::string& query, const RowProcessor& processor)
{
#ifndef LOG_MYSQL_QUERY
    auto query_res = RealQuery(query);
#else
    auto query_res = LoggedRealQuery(query);
#endif//LOG_MYSQL_QUERY
    if (!query_res) {
        auto ec = query_res.error();

        LOG_ERROR << "Error executing MySQL query \"" << query.c_str() << "\": " << ec.message() << " (" << ec.value() << ")";
        return;
    }

    if (!query_res.value()) return;

    auto* res = query_res.value().get();

    // get column info and give it to field validator,
    // which should throw if it doesn't like the columns
    
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        uint32_t filed = mysql_num_fields(res);
        unsigned long* lengths = mysql_fetch_lengths(res);
        if (!processor(row, lengths, filed)) break;
    }
}

void MysqlClient::QueryResultRowProcessor(const std::string& query, const ResultRowProcessor& processor)
{
#ifndef LOG_MYSQL_QUERY
    auto query_res = RealQuery(query);
#else
    auto query_res = LoggedRealQuery(query);
#endif//LOG_MYSQL_QUERY
    if (!query_res) {
        auto ec = query_res.error();

        LOG_ERROR << "Error executing MySQL query \"" << "\": " << ec.message() << " (" << ec.value() << ")";
        return;
    }

    if (!query_res.value()) return;

    auto* res = query_res.value().get();

    // get column info and give it to field validator,
    // which should throw if it doesn't like the columns

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        uint32_t filed = mysql_num_fields(res);
        unsigned long* lengths = mysql_fetch_lengths(res);
        if (!processor(std::make_unique<ResultRow>(row, lengths, nullptr, filed))) break;
    }
}

uint64_t MysqlClient::LastInsertId()
{
    return mysql_insert_id(connection_);
}

MysqlClient::MysqlResultExpected MysqlClient::LoggedRealQuery(const std::string& q)
{
    using clock_type = std::chrono::steady_clock;

    auto start = clock_type::now();
    auto query_res = RealQuery(q);
    auto dur = clock_type::now() - start;
    auto msg =
        std::string(" (") +
        std::to_string(
            std::chrono::duration_cast<std::chrono::microseconds>(dur).count()) +
        " us)> " ;
    if (query_res) {
        auto const* res = query_res.value().get();

        msg += " // OK";
        if (res) {
            msg += " " + std::to_string(res->row_count) + " row" +
                (res->row_count != 1 ? "s" : "");
        }
    }
    else {
        auto err = query_res.error();
        msg += " // ERROR: " + std::to_string(err.value()) + " " + err.message();
    }
    LOG_INFO << msg;

    return query_res;
}

MysqlError make_mysql_error_code(MYSQL* m) {
    return { mysql_errno(m), mysql_error(m), mysql_sqlstate(m) };
}

MysqlClient::MysqlResultExpected MysqlClient::RealQuery(const std::string& q)
{
    auto query_res = mysql_real_query(connection(), q.data(), (uint32_t)q.size());
    if ( 0 != query_res)
    {
        return stdx::make_unexpected(make_mysql_error_code(connection()));
    }
    MysqlResult res{ mysql_store_result(connection()) };
    if (!res)
    {
        // no error, but also no resultset
        if (mysql_errno(connection()) == 0) return {};

        return stdx::make_unexpected(make_mysql_error_code(connection()));
    }
#if defined(__SUNPRO_CC)
    // ensure sun-cc doesn't try to the copy-constructor on a move-only type
    return std::move(res);
#else
    return res;
#endif
}


