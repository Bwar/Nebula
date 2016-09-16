/*******************************************************************************
* Project:  DataAnalysis
* File:     Dbi.hpp
* Description: DB操作接口类
* Author:        bwarliao
* Created date:  2010-12-21
* Modify history:
*******************************************************************************/

#ifndef DBI_HPP_
#define DBI_HPP_

#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>


namespace neb
{

enum E_DB_TYPE                              // 数据库类型
{
	UNDEFINE = 0,							// 未定义
    MYSQL_DB = 1,                           // MySQL数据库
    ORACLE_DB = 2,                          // ORACLE数据库
    SQL_SERVER_DB = 3,                      // SQL Server数据库
    DATA_AGENT = 9,                         // 数据代理（非真正意义上的DB）
};

const int g_iMaxSqlBufSize = 2048;

typedef ::std::vector < ::std::string > T_vecDict;		// 数据字典
typedef ::std::map < ::std::string, ::std::string > T_mapRow;
typedef ::std::vector < ::std::string > T_vecRow;
typedef ::std::vector < T_mapRow > T_vecResultSet;
typedef ::std::vector < T_vecRow > T_vecResultSetWithoutDict;

struct tagDbConnInfo							// DB连接配置
{
    char m_szDbCharSet[16];                 	// DB字符集
    char m_szDbHost[32];                    	// DB所在服务器IP
    char m_szDbUser[32];                    	// DB用户名
    char m_szDbPwd[32];                     	// DB用户密码
    char m_szDbName[32];                    	// DB库名
    unsigned int m_uiDbPort;                	// DB端口
    unsigned int m_uiIdcId;                     // 虚拟IDC ID，用于访问数据库时选择数据代理，如果是直连访问，无需理会此配置
    unsigned int uiTimeOut;

    tagDbConnInfo()
    {
        memset(m_szDbCharSet, 0, sizeof(m_szDbCharSet));
        memset(m_szDbHost, 0, sizeof(m_szDbHost));
        memset(m_szDbUser, 0, sizeof(m_szDbUser));
        memset(m_szDbPwd, 0, sizeof(m_szDbPwd));
        memset(m_szDbName, 0, sizeof(m_szDbName));
        m_uiDbPort = 0;
        m_uiIdcId = 0;
        uiTimeOut = 3;
    }

    tagDbConnInfo(const tagDbConnInfo& stDbConnInfo)
    {
        strncpy(m_szDbCharSet, stDbConnInfo.m_szDbCharSet, sizeof(m_szDbCharSet));
        strncpy(m_szDbHost, stDbConnInfo.m_szDbHost, sizeof(m_szDbHost));
        strncpy(m_szDbUser, stDbConnInfo.m_szDbUser, sizeof(m_szDbUser));
        strncpy(m_szDbPwd, stDbConnInfo.m_szDbPwd, sizeof(m_szDbPwd));
        strncpy(m_szDbName, stDbConnInfo.m_szDbName, sizeof(m_szDbName));
        m_uiDbPort = stDbConnInfo.m_uiDbPort;
        m_uiIdcId = stDbConnInfo.m_uiIdcId;
        uiTimeOut = stDbConnInfo.uiTimeOut;
    }

    tagDbConnInfo& operator = (const tagDbConnInfo& stDbConnInfo)
    {
        strncpy(m_szDbCharSet, stDbConnInfo.m_szDbCharSet, sizeof(m_szDbCharSet));
        strncpy(m_szDbHost, stDbConnInfo.m_szDbHost, sizeof(m_szDbHost));
        strncpy(m_szDbUser, stDbConnInfo.m_szDbUser, sizeof(m_szDbUser));
        strncpy(m_szDbPwd, stDbConnInfo.m_szDbPwd, sizeof(m_szDbPwd));
        strncpy(m_szDbName, stDbConnInfo.m_szDbName, sizeof(m_szDbName));
        m_uiDbPort = stDbConnInfo.m_uiDbPort;
        m_uiIdcId = stDbConnInfo.m_uiIdcId;
        uiTimeOut = stDbConnInfo.uiTimeOut;
        return(*this);
    }
};

struct tagDbConfDetail                      	// 源DB信息配置
{
    unsigned char m_ucAccess;               	// 访问方式,见 E_DB_ACCESS
    unsigned char m_ucDbType;               	// DB类型
    tagDbConnInfo m_stDbConnInfo;				// DB连接信息

    tagDbConfDetail()
    {
        m_ucAccess = 0;
        m_ucDbType = 0;
    }

    tagDbConfDetail(const tagDbConfDetail& stDbConfDetail)
    {
        m_ucAccess = stDbConfDetail.m_ucAccess;
        m_ucDbType = stDbConfDetail.m_ucDbType;
        m_stDbConnInfo = stDbConfDetail.m_stDbConnInfo;
    }

    tagDbConfDetail& operator = (const tagDbConfDetail& stDbConfDetail)
    {
        m_ucAccess = stDbConfDetail.m_ucAccess;
        m_ucDbType = stDbConfDetail.m_ucDbType;
        m_stDbConnInfo = stDbConfDetail.m_stDbConnInfo;
        return(*this);
    }
};


class CDbi
{
public:
    CDbi(){};
    virtual ~CDbi(){};

    virtual int InitDbConn(const tagDbConfDetail& stDbConfDetail) = 0;

    // 本应统一使用InitDbConn(const tagDbConnInfo& stDbConnInfo)的，
    // 因数据访问数据库安全需要，走数据代理的连接不传输DB连接信息
//    virtual int InitDbConn(const tagAgentNode& stAgentNode)
//    {
//    	return 0;
//    }
    virtual int ExecSql(const std::string& strSql) = 0;

    // 一次性把结果集取到客户端（适用于结果集比较小的应用场景）
    virtual int GetResultSet(T_vecResultSet& vecResultSet) = 0;

    // 把结果集放在服务端（适用于结果集太大，客户端无法存储完整结果集场景）
    virtual int GetResultDict(T_vecDict& vecDict) = 0;
    virtual int GetResultRow(T_mapRow& mapRow) = 0;

    // 如果查询结果集太大，先将数据缓存在文件中
    virtual int GetResultToFile() = 0;

    //取上一个数据库操作的错误信息
    virtual int GetErrno() const = 0;
    virtual const std::string& GetError() const = 0;

public:
	// 字符串中特殊字符处理，如mysql中可调用 mysql_real_escape_string，每种dbi如需使用该函数则必须实现
	virtual int EscapeString(
			char* szTo,
			const char* szFrom,
			unsigned long uiLength)
	{
		return 0;
	}

protected:
	/* 每个数据库连接保存一个临时文件名，
	 * 该文件用于缓存大结果集，
	 * 需要保证文件名在任意时刻唯一，
	 * 在构造函数创建文件，在析构函数删除文件
	 */
	std::string m_strTempFileName;
};

}

#endif /* DBI_HPP_ */
