#include "xdebuglib.h"
#include "XsriptVM.h"

void init_debug_lib()
{
	std::vector<HostFunction> debugFuncVec;

	debugFuncVec.push_back(HostFunction("stackbacktrace", 0, host_stackbacktrace));
	debugFuncVec.push_back(HostFunction("getlocalvar", 2, host_getLocalvar));
	debugFuncVec.push_back(HostFunction("setlocalvar", 3, host_setLocalvar));

	debugFuncVec.push_back(HostFunction("getlocalvarbyname", 2, host_getLocalvarByName));
	debugFuncVec.push_back(HostFunction("setlocalvarbyname", 3, host_setLocalvarByName));

	debugFuncVec.push_back(HostFunction("debug", 0, host_debug));
	debugFuncVec.push_back(HostFunction("sethook", 1, host_hook));
	debugFuncVec.push_back(HostFunction("getlocalvars", 1, host_getLocalvars));
	debugFuncVec.push_back(HostFunction("getdebuginfo", 1, host_getDebugInfo));
	debugFuncVec.push_back(HostFunction("getglobalvar", 1, host_getGlobalVar));
	debugFuncVec.push_back(HostFunction("getstackdepth", 0, host_getStackDepth));
	gScriptVM.RegisterHostLib("debug", debugFuncVec);
}

void host_getLocalvar(XScriptVM* vm)
{
	int stackIndex = 0;
	int varIndex = 0;
	vm->getParamAsInt(0, stackIndex);
	vm->getParamAsInt(1, varIndex);

	std::string name;
	Value* value = vm->GetStackValueByIndex(stackIndex, varIndex, name);
	if (value != NULL)
	{
		vm->setReturnAsInt(1, 0);
		vm->setReturnAsStr(name.c_str(), 1);
		Value retValue;
		CopyValue(&retValue, *value);
		vm->setReturnAsValue(retValue, 2);
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

void host_stackbacktrace(XScriptVM* vm)
{
	std::string stackBack = vm->stackBackTrace();
	vm->setReturnAsStr(stackBack.c_str());
}

void host_getStackDepth(XScriptVM* vm)
{
	vm->setReturnAsInt(vm->GetStackDepth());
}

void host_getLocalvars(XScriptVM* vm)
{
	int stackIndex = 0;
	if (vm->getParamAsInt(0, stackIndex))
	{
		int callIndex = vm->GetStackDepth() - stackIndex;
		if (callIndex >= 0 && callIndex <= vm->GetStackDepth())
		{
			bool hasFound = false;
			FuncState* funcState = vm->GetCallInfo(callIndex)->mCurFunctionState;

			TableValue* table = vm->newTable();

			for (int i = 0; i < funcState->m_localVarVec.size(); i++)
			{
				if (i >= 0 && i < funcState->stackFrameSize)
				{
					int stackPos = vm->GetCallInfo(callIndex)->mFrameIndex + i - funcState->stackFrameSize;
					Value stackValue = vm->getStackValue(stackPos);
					Value newValue;
					CopyValue(&newValue, stackValue);
					vm->setTableValue(table, vm->ConstructValue(funcState->m_localVarVec[i].c_str()), newValue);
				}

			}

			vm->setReturnAsInt(1, 0);
			vm->setReturnAsTable(table, 1);

		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}
}

void host_getDebugInfo(XScriptVM* vm)
{
	Value* pValue = vm->getParamValue(0);
	if (pValue != NULL)
	{
		if (pValue->type == OP_TYPE_INT)
		{
			int callIndex = vm->GetStackDepth() - pValue->iIntValue;
			if (callIndex >= 0 && callIndex <= vm->GetStackDepth())
			{
				FuncState* funcState = vm->GetCallInfo(callIndex)->mCurFunctionState;
				TABLE table = vm->newTable();
				vm->setTableValue(table, (char*)"source", (char*)funcState->sourceFileName.c_str());
				vm->setTableValue(table, (char*)"what", (char*)"lua");
				vm->setTableValue(table, (char*)"name", (char*)funcState->funcName.c_str());
				vm->setTableValue(table, (char*)"nups", (int)funcState->m_upValueVec.size());
				vm->setReturnAsInt(1, 0);
				vm->setReturnAsValue(vm->ConstructValue(table), 1);
			}
			else
			{
				vm->setReturnAsInt(0, 0);
			}
		}
		else if (pValue->type == OP_TYPE_FUNC)
		{
			TABLE table = vm->newTable();
			if (pValue->func->isCFunc)
			{
				vm->setTableValue(table, (char*)"what", (char*)"C");
				vm->setTableValue(table, (char*)"name", (char*)vm->GetHostFunction(pValue->func->hostFuncIndex).funcName.c_str());
			}
			else
			{
				FuncState* funcState = pValue->func->luaFunc.proto;
				vm->setTableValue(table, (char*)"source", (char*)funcState->sourceFileName.c_str());
				vm->setTableValue(table, (char*)"what", (char*)"lua");
				vm->setTableValue(table, (char*)"name", (char*)funcState->funcName.c_str());
				vm->setTableValue(table, (char*)"nups", (int)funcState->m_upValueVec.size());
			}

			vm->setReturnAsInt(1, 0);
			vm->setReturnAsValue(vm->ConstructValue(table), 1);
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
}

void host_hook(XScriptVM* vm)
{
	int hookIndex = vm->RegisterGlobalValue("_hook");

	Value* pValue = vm->getParamValue(0);
	if (pValue != NULL && pValue->type == OP_TYPE_FUNC)
	{
		vm->setGloablStackValue(hookIndex, *pValue);
	}
}

void host_getGlobalVar(XScriptVM* vm)
{
	char* varName = 0;
	if (vm->getParamAsString(0, varName))
	{
		Value* pvalue = vm->GetGlobalValue(varName);
		if (pvalue != NULL)
		{
			Value newValue;
			CopyValue(&newValue, *pvalue);
			vm->setReturnAsInt(1, 0);
			vm->setReturnAsValue(newValue, 1);
		}
		else
		{
			vm->setReturnAsInt(0);
		}
	}
	else
	{
		vm->setReturnAsInt(0);
	}

}

void host_debug(XScriptVM* vm)
{
	while (true)
	{
		char buffer[250];
		fputs("xs_debug> ", stderr);
		if (fgets(buffer, sizeof(buffer), stdin) == 0 ||
			strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "\n") == 0)
			break;
		/*
		if (vm->loadString(buffer))
		{
			for (int i = 0; i < MAX_FUNC_REG; i++)
			{
				Value* p = vm->getReturnRegValue(i);
				if (p->type != OP_TYPE_NIL)
				{
					printf(getValueDescString(vm, p).c_str());
					printf("\t");
				}
			}

			printf("\n");
		}
		*/

	}
}

void host_getLocalvarByName(XScriptVM* vm)
{
	int stackIndex = 0;
	char* varName = 0;
	if (vm->getParamAsInt(0, stackIndex) && vm->getParamAsString(1, varName))
	{
		Value* pValue = vm->GetStackValueByName(stackIndex, varName);
		if (pValue != NULL)
		{
			vm->setReturnAsInt(1, 0);
			Value retValue;
			CopyValue(&retValue, *pValue);
			vm->setReturnAsValue(retValue, 1);
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}

}

void host_setLocalvarByName(XScriptVM* vm)
{
	int stackIndex = 0;
	char* varName = 0;
	if (vm->getParamAsInt(0, stackIndex) && vm->getParamAsString(1, varName))
	{
		Value* setValue = vm->getParamValue(2);
		Value* pValue = vm->GetStackValueByName(stackIndex, varName);
		if (setValue != NULL && pValue != NULL)
		{
			vm->setReturnAsInt(1, 0);
			CopyValue(pValue, *setValue);
		}
		else
		{
			vm->setReturnAsInt(0, 0);
		}
	}
	else
	{
		vm->setReturnAsInt(0, 0);
	}
}

void host_setLocalvar(XScriptVM* vm)
{
	int stackIndex = 0;
	int varIndex = 0;
	vm->getParamAsInt(0, stackIndex);
	vm->getParamAsInt(1, varIndex);
	Value* setValue = vm->getParamValue(2);

	std::string name;
	Value* value = vm->GetStackValueByIndex(stackIndex, varIndex, name);
	if (value != NULL)
	{
		CopyValue(value, *setValue);
		vm->setReturnAsInt(1, 0);
		vm->setReturnAsStr(name.c_str(), 1);
	}
	else
	{
		vm->setReturnAsInt(0);
	}
}

