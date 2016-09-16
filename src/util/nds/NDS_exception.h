#ifndef _NDS_EXCEPTION_H
#define _NDS_EXCEPTION_H

#include <string>
#include <iostream>
#include <exception>
#include <cerrno>

using namespace std;

namespace NDS
{

class NDS_exception: public exception
{
protected:
	int retcode;
	string message;
	string filestr;
	int linestr;
public:
	NDS_exception(const string &file, const int line, const string &mesg="General error", const int ret=-1) throw()
	:retcode(ret),message(mesg),filestr(file),linestr(line)
	{
	}
	NDS_exception(const NDS_exception &e) throw()
	:exception(e),retcode(e.retcode),message(e.message),filestr(e.filestr),linestr(e.linestr)
	{
	}
	virtual ~NDS_exception() throw()
	{
	}
	int get_retcode() const
	{
		return retcode;
	}
	const string &get_message() const
	{
		return message;
	}
	virtual const char *what() const throw()
	{
		return message.c_str();
	}
	virtual void print(ostream &out) const throw()
	{
		out<<"Exception in <"<<filestr<<">, line <"<<linestr<<">, retcode=<"<<retcode<<">, <"<<message<<">"<<endl;
	}
};

class NDS_errno_exception:public NDS_exception
{
public:
	NDS_errno_exception(const string &file, const int line, const string &mesg="", const int ret=errno) throw()
	:NDS_exception(file,line,mesg+" : "+strerror(ret),ret)
	{
	}
};

}

#endif

