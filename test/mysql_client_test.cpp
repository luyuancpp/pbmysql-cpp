#include <gtest/gtest.h>

#include "src/mysql_wrapper/mysql_database.h"

#include "common_proto/mysql_database_test.pb.h"
#include "common_proto/mysql_database_table.pb.h"


MysqlClientPtr query_database;
database_server_db query_database_param;
std::string main_acount("lu hailong");
std::string main_password("lu yaun");
std::string main_where_case = std::string("account = '") + main_acount + std::string("'");

TEST(MysqlClientTest, ConectMysql)
{
    MysqlDatabase client;
    client.Connect(query_database_param);
   
    query_database->AddTable(account_database_one_test::default_instance());
    client.Init();
}

TEST(MysqlClientTest, QueryOptionMessage)
{
    account_database_one_test save_message;
    save_message.set_account(main_acount);
    save_message.set_password(main_password);
    query_database->SaveOne(save_message);
    account_database_one_test load_message;
    query_database->LoadOne(load_message);
    EXPECT_EQ(save_message.account(), load_message.account());
    EXPECT_EQ(save_message.password(), load_message.password());
    account_database_one_test load_message_where;
    query_database->LoadOne(load_message_where, main_where_case);
    EXPECT_EQ(save_message.account(), load_message_where.account());
    EXPECT_EQ(save_message.password(), load_message_where.password());
}

TEST(MysqlClientTest, QueryRepeatedMessage)
{
    account_database_all_test save_message;
    auto first =  save_message.mutable_account_password()->Add();
    first->set_account("lu hailong1");
    first->set_password("luyuan ");
    auto second = save_message.mutable_account_password()->Add();
    second->set_account("luh ailong1");
    second->set_password("lu yuan ");
    query_database->SaveAll<::account_database_one_test>(save_message);
    account_database_all_test load_message;
    query_database->LoadAll<::account_database_one_test>(load_message);
}

TEST(MysqlClientTest, QueryDelete)
{
    account_database_one_test save_message;
    save_message.set_account(main_acount);
    save_message.set_password(main_acount);
    account_database_one_test load_message_where;
    query_database->LoadOne(load_message_where, main_where_case);

    account_database_one_test delete_message_where;
    query_database->Delete(delete_message_where, main_where_case);
    load_message_where.Clear();
    query_database->LoadOne(load_message_where, main_where_case);
    EXPECT_TRUE(load_message_where.account().empty());
    EXPECT_TRUE(load_message_where.password().empty());
}

//TEST(MysqlClientTest, QueryDeleteAll)
//{
//    account_database_one_test delete_message_where;
//    delete_message_where.set_account("lu hailong1");
//    query_database->Delete(delete_message_where);
//    delete_message_where.set_account("luh ailong1");
//    query_database->Delete(delete_message_where);
//    account_database_all_test load_message;
//    query_database->LoadAll<::account_database_one_test>(load_message);
//    EXPECT_EQ(0, load_message.account_password_size());
//}

TEST(MysqlClientTest, QueryWhereAll)
{
    account_database_one_test delete_message_where;
    delete_message_where.set_account("1");
    query_database->SaveOne(delete_message_where);
    delete_message_where.set_account("2");
    query_database->SaveOne(delete_message_where);
    account_database_all_test load_message;
    query_database->LoadAll<::account_database_one_test>(load_message, "account = '1' or account = '2'");
    EXPECT_EQ(2, load_message.account_password_size());
}

TEST(MysqlClientTest, LastId)
{
    EXPECT_EQ(0, query_database->LastInsertId());
}

int main(int argc, char** argv)
{
    std::string droptable = std::string("drop table ") + account_database_one_test::default_instance().GetTypeName();
    
    query_database_param.set_db_host("127.0.0.1");
    query_database_param.set_db_user("root");
    query_database_param.set_db_password("lhl616586");
    query_database_param.set_db_port(3306);
    query_database_param.set_db_dbname("game");
    query_database = std::make_unique<MysqlDatabase>();
    query_database->Connect(query_database_param);
    query_database->QueryOne(droptable);
    query_database->AddTable(account_database_one_test::default_instance());
    query_database->SetDBName("game");
    query_database->Init();       
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}