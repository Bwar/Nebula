#ifndef _NDS_SO_H
#define _NDS_SO_H

#include <string>
#include <functional>
using namespace std;

#include <dlfcn.h>

#include <NDS_exception.h>
using namespace NDS;

template <typename PF> class NDS_so
{
private:
	void *handle;
	NDS_so();
	NDS_so(const NDS_so&);
public:
	NDS_so(const string& soname, int flag=RTLD_LAZY)
	{
		if ((handle=dlopen(soname.c_str(),flag))==NULL)
			throw NDS_exception(__FILE__,__LINE__,dlerror());
	}
	~NDS_so()
	{
		if (dlclose(handle)!=0)
			throw NDS_exception(__FILE__,__LINE__,dlerror());
	}
	PF operator[](const string& funname)
	{
		void *pfun=dlsym(handle,funname.c_str());
		if (pfun==NULL)
			throw NDS_exception(__FILE__,__LINE__,dlerror());
		return (PF)pfun;
//		return static_cast<PF>(pfun);
	}
};

#endif

