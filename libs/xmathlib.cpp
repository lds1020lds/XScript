#include <math.h>
#include "XsriptVM.h"
#include "xmathlib.h"

void init_math_lib()
{
	std::vector<HostFunction> funcVec;
	funcVec.push_back(HostFunction("random", 0, host_random));
	funcVec.push_back(HostFunction("acos", 1, host_acos));
	funcVec.push_back(HostFunction("asin", 1, host_asin));
	funcVec.push_back(HostFunction("atan", 1, host_atan));
	funcVec.push_back(HostFunction("cos", 1, host_cos));
	funcVec.push_back(HostFunction("exp", 1, host_exp));
	funcVec.push_back(HostFunction("log", 1, host_log));
	funcVec.push_back(HostFunction("log10", 1, host_log10));
	funcVec.push_back(HostFunction("sin", 1, host_sin));
	funcVec.push_back(HostFunction("sqrt", 1, host_sqrt));
	funcVec.push_back(HostFunction("tan", 1, host_tan));
	gScriptVM.RegisterHostLib("math", funcVec);
}

void  host_random(XScriptVM* vm)
{
	int r = rand();
	vm->setReturnAsInt(r);
}


void  host_acos(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(acos(fValue));
	}
}


void  host_asin(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(asin(fValue));
	}
}


void  host_atan(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(atan(fValue));
	}
	
}


void  host_cos(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(cos(fValue));
	}
	
}


void  host_exp(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(exp(fValue));
	}

}


void  host_log(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(log(fValue));
	}

}


void  host_log10(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(log10(fValue));
	}
	
}


void  host_sin(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(sin(fValue));
	}

}


void  host_sqrt(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(sqrt(fValue));
	}
}


void  host_tan(XScriptVM* vm)
{
	float fValue;
	if (vm->getParamAsFloat(0, fValue))
	{
		vm->setReturnAsfloat(tan(fValue));
	}
	
}
