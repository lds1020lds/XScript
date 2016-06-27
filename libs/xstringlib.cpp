#include "xstringlib.h"
#include "XsriptVM.h"

void init_string_lib()
{
	std::vector<HostFunction> strfuncVec;
	strfuncVec.push_back(HostFunction("len", 1, host_str_len));
	strfuncVec.push_back(HostFunction("find", 3, host_str_find));
	strfuncVec.push_back(HostFunction("sub", 3, host_str_sub));
	gScriptVM.RegisterHostLib("string", strfuncVec);
}

void host_str_len(XScriptVM* vm)
{
	char* str = NULL;
	if (vm->getParamAsString(0, str))
	{
		vm->setReturnAsInt(strlen(str));
	}
	else
	{
		vm->setReturnAsInt(0);
	}

}

void host_str_find(XScriptVM* vm)
{
	char* str = NULL;
	char* subStr = NULL;
	int startPos = 0;
	int ret = -1;

	if (vm->getParamAsString(0, str) && vm->getParamAsString(1, subStr) && vm->getParamAsInt(2, startPos))
	{
		const char* szPos = strstr(str + startPos, subStr);
		if (szPos != NULL)
		{
			ret = szPos - str;
		}
	}

	vm->setReturnAsInt(ret);
}

void host_str_sub(XScriptVM* vm)
{
	char* str = NULL;
	int startPos = 0;
	int len = -1;

	if (vm->getParamAsString(0, str) && vm->getParamAsInt(1, startPos) && vm->getParamAsInt(2, len))
	{
		if (len == -1)
		{
			len = strlen(str) - startPos;
		}

		if (len < 0)
			len = 0;

		char* szRet = new char[len + 1];
		szRet[len] = 0;
		strncpy(szRet, str + startPos, len);

		vm->setReturnAsStr(szRet, 0);

		delete []szRet;
	}
	else
		vm->setReturnAsNil(0);
}
