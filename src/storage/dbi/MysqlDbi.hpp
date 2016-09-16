/*******************************************************************************
* Project:  DataAnalysis
* File:     MysqlDbi.hpp
* Description: MySQL DB操作接口类
* Author:        bwarliao
* Created date:  2010-12-21
* Modify history:
*******************************************************************************/

#ifndef MYSQLDBI_HPP_
#define MYSQLDBI_HPP_


#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mysql56/errmsg.h>
#include <mysql56/mysql.h>
#include "Dbi.hpp"

namespace neb
{

class CMysqlDbi : public CDbi
{
public:
    CMysqlDbi();
    CMysqlDbi(
            const char* szIp,
            const char* szUserName,
            const char* szUserPwd,
            const char* szDbName,
            const char* szCharacterSet = "utf8",
            unsigned int uiPort = 3306);
    virtual ~CMysqlDbi();

    //初始化连接实例
    virtual int InitDbConn(const tagDbConfDetail& stDbConfDetail);

    //执行SQL语句
    virtual int ExecSql(const std::string& strSql);

    virtual int GetResultSet(T_vecResultSet& vecResultSet);

    virtual int GetResultDict(T_vecDict& vecDict);

    virtual int GetResultRow(T_mapRow& mapRow);

public:
    virtual int MysqlInit();

    /**
     * @brief 设置mysql参数
     * @note  设置mysql参数，包括设置连接超时时间、设置传输数据压缩、设置是否允许LOAD FILE等.可参考
     * CMysqlDbi::InitDbConn(const tagDbConfDetail& stDbConfDetail)
     * 中的使用。
     */
    virtual int SetMysqlOptions(
            enum mysql_option option,
            const char* arg);

    virtual int MysqlRealConnect(
            const char* szIp,
            const char* szUserName,
            const char* szUserPwd,
            const char* szDbName,
            unsigned int uiPort,
            const char* szUnixSocket,
            unsigned long ulClientFlag);

    // 设置连接字符集
    virtual int SetDbConnCharacterSet(const char* szCharacterSet);

    virtual int EscapeString(
            char* szTo,
            const char* szFrom,
            unsigned long uiLength);

    virtual int MysqlPing();

    virtual int SetAutoCommit(my_bool mode);

    virtual int Commit();

    MYSQL* GetMysqlInstant() { return &m_dbInstance;}

    //执行SQL语句 ，并返回MYSQL结果集
    virtual int ExecSql(
            const std::string& strSql,
            MYSQL_RES* pQueryRes);

    //执行SQL语句 ，并返回自定义结果集
    virtual int ExecSql(
            const std::string& strSql,
            T_vecResultSet& vecRes);

    //执行插入SQL语句 ，并返回 last_insert_id
    virtual int ExecSql(
            const std::string& strSql,
            unsigned long long& ullInsertId);

    //返回上次UPDATE更改的行数，上次DELETE删除的行数，或上次INSERT语句插入的行数
    virtual unsigned int AffectRows();

    //使用mysql_store_result()方式取查询结果集
    virtual const MYSQL_RES* StoreResult();

    //使用mysql_use_result()方式取查询结果集
    virtual const MYSQL_RES* UseResult();

    //使用mysql_free_result()释放结果集
    void FreeResult();

    //返回结果集记录行
    virtual MYSQL_ROW GetRow();

    //返回结果集内当前行的列的长度
    virtual unsigned long* FetchLengths();

    //返回当前结果集列数量
    virtual unsigned int FetchFieldNum();

    //返回当前结果集列名
    virtual MYSQL_FIELD* FetchFields();

    //返回连接句柄结果集列数量
    virtual unsigned int FetchFieldCount();

    //取上一次数据库操作错误码
    int GetErrno() const
    {
        return m_iErrno;
    }

    //取上一次数据库操作错误信息
    const std::string& GetError() const
    {
        return m_strError;
    }

    const char* MysqlInfo();

    virtual int GetResultToFile();
    T_mapRow& GetRowFromFile();

private:
	//初始化数据成员，仅供构造函数使用
	void InitVariables();

	//执行SQL命令前的清理工作
    bool CleanBeforeExecSql();

private:
    MYSQL m_dbInstance;         //统计过程库连接实例
    MYSQL_RES* m_pQueryRes;     //查询结果集
    MYSQL_ROW m_stRow;          //查询结果集的记录行
    int m_iQueryResult;         //MySQL函数Query返回值
    int m_iErrno;       //上次调用的MySQL函数的错误编号
    std::string m_strError;  //上次调用的MySQL函数的错误消息
    T_vecDict m_vecDict;	//结果集数据字典
    T_mapRow m_mapRow;	//结果集行

    tagDbConfDetail m_stDbConfDetail; // 数据库连接信息
 };


}

#endif /* MYSQLDBI_HPP_ */
