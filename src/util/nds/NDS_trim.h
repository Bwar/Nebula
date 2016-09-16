#ifndef _NDS_TRIM_H
#define _NDS_TRIM_H

#include <string>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cctype>
using namespace std;

namespace NDS
{

inline string& ltrim(string &ss, int (*pf)(int)=isspace)
{
	string::iterator p=find_if(ss.begin(),ss.end(),not1(ptr_fun(pf)));
	ss.erase(ss.begin(),p);
	return ss;
}

inline string& rtrim(string &ss, int (*pf)(int)=isspace)
{
	string::reverse_iterator p=find_if(ss.rbegin(),ss.rend(),not1(ptr_fun(pf)));
	ss.erase(p.base(),ss.end());
	return ss;
}

inline string& trim(string &st)
{
	ltrim(rtrim(st));
	return st;
}

inline void stringupper(string& str)
{
	for (string::iterator i=str.begin();i!=str.end();i++)
		*i=toupper(*i);
}

inline void stringlower(string& str)
{
	for (string::iterator i=str.begin();i!=str.end();i++)
		*i=tolower(*i);
}

template <typename T> inline const string to_string(const T& v)
{
	ostringstream os;
	os<<v;
	return os.str();
}

template <typename T> inline const T from_string(const string& v)
{
	istringstream is(v);
	T t;
	is>>t;
	return t;
}

template <typename T> inline void from_string(T& t, const string& v)
{
	istringstream is(v);
	is>>t;
}

}

#endif

