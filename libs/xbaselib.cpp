#include "XsriptVM.h"
#include "xbaselib.h"

void init_base_lib()
{
	gScriptVM.registerHostApi("system_pause",	0, host_pause);
	gScriptVM.registerHostApi("sleep",			1, host_sleep);
	gScriptVM.registerHostApi("type",			1, host_type);
	gScriptVM.registerHostApi("toString",		1, host_toString);
	gScriptVM.registerHostApi("toNumber",		1, host_toNumber);
	gScriptVM.registerHostApi("array",			1, host_array);
	gScriptVM.registerHostApi("printf",			-1, host_print);
	gScriptVM.registerHostApi("prints",			1, host_prints);
	gScriptVM.registerHostApi("getCurrentTime", 0, host_GetCurrentTime);

	gScriptVM.registerHostApi("inext",			2, host_inext);
	gScriptVM.registerHostApi("ipairs",			1, host_ipairs);
	gScriptVM.registerHostApi("garbageCollect", 0, host_gc);

	gScriptVM.registerHostApi("require",		1, host_require);
	gScriptVM.registerHostApi("pcall",			-1, host_pcall);
	gScriptVM.registerHostApi("xpcall",			2, host_xpcall);

	gScriptVM.registerHostApi("setenvtable", 1, host_setEnvTable);
	gScriptVM.registerHostApi("getenvtable", 0, host_getEnvTable);
	gScriptVM.registerHostApi("loadstring", 1, host_loadstring);

	std::vector<HostFunction> coVec;

	coVec.push_back(HostFunction("create", 1, host_coCreate));
	coVec.push_back(HostFunction("resume", -1, host_coResume));
	coVec.push_back(HostFunction("yield", -1, host_coYield));
	coVec.push_back(HostFunction("status", -1, host_coStatus));
	gScriptVM.RegisterHostLib("coroutine", coVec);
}

void host_coStatus(XScriptVM* vm)
{
	Value* stackValue = vm->getParamValue(0);
	//ExecArgsCheck(IsValueThread(stackValue), 0, "coroutine expect");

	if (IsValueThread(stackValue))
	{
		const char* status = vm->GetCoroutieStatusName(stackValue->threadData);
		vm->setReturnAsStr(status);
	}
}

void host_coCreate(XScriptVM* vm)
{
	vm->CreateCoroutie();
}

void host_coResume(XScriptVM* vm)
{
	vm->ResumeCoroutie();
}

void host_coYield(XScriptVM* vm)
{
	vm->YieldCoroutie();
}

void host_setEnvTable(XScriptVM* vm)
{
	Value* stackValue = vm->getParamValue(0);
	if ( IsValueTable(stackValue) )
	{
		vm->SetEnvTable(stackValue->tableData);
	}
	else if (IsValueNil(stackValue))
	{
		vm->SetEnvTable(NULL);
	}
}

void host_getEnvTable(XScriptVM* vm)
{
	TABLE table = vm->GetEnvTable();
	vm->setReturnAsTable(table);
}

void host_loadstring(XScriptVM* vm)
{
	char* code = NULL;
	if (vm->getParamAsString(0, code))
	{
		FuncState* func = vm->CompileString(code);
		if (func != NULL)
		{
			vm->setReturnAsInt(1);
			vm->setReturnAsValue(vm->ConstructValue(func), 1);
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

std::string  getValueDescString(XScriptVM* vm, Value* value)
{
	std::string desc;
	char text[256] = { 0 };
	if (value->type == OP_TYPE_INT)
	{
		snprintf(text, 256, "%d", value->iIntValue);
		desc = text;
	}
	else if (value->type == OP_TYPE_FLOAT)
	{
		snprintf(text, 256, "%f", value->fFloatValue);
		desc = text;
	}
	else if (value->type == OP_TYPE_STRING)
	{
		snprintf(text, 256, "\"%s\"", stringRawValue(value));
		desc = text;
	}
	else if (IsValueFunction(value))
	{
		if (value->func->isCFunc)
		{
			snprintf(text, 256, "C function: %s", vm->GetHostFunction(value->func->hostFuncIndex).funcName.c_str());
		}
		else
		{
			snprintf(text, 256, "lua function: %s", value->func->luaFunc.proto->funcName.c_str());
		}
		desc = text;
	}
	else if (IsUserType(value->type))
	{
		std::string localType = vm->GetString(USERDATA_TYPE(value->type));
		snprintf(text, 256, "User Type: %s, 0x%x", localType.c_str(), value->userData);
		desc = text;
	}
	else if (value->type == OP_TYPE_TABLE)
	{
		desc += "{";
		
		for (int i = 0; i < value->tableData->mArraySize; i++)
		{
			desc += ConvertToString(i);
			desc += "=";

			desc += getValueDescString(vm, &value->tableData->mArrayData[i]);
			if (i < value->tableData->mArraySize - 1 || value->tableData->mNodeCapacity > 0)
			{
				desc += ",";
			}
		}

		for (int i = 0; i < value->tableData->mNodeCapacity; i++)
		{
			desc += getValueDescString(vm, &value->tableData->mNodeData[i].key.keyVal);
			desc += "=";
			desc += getValueDescString(vm, &value->tableData->mNodeData[i].value);

			if (i < value->tableData->mNodeCapacity - 1)
				desc += ",";
		}

		desc += "}";
	}
	else if (value->type == OP_TYPE_NIL)
	{
		desc = "nil";
	}

	return desc;
}



void host_pcall(XScriptVM* vm)
{
	int numParam = vm->getNumParam();
	Value* fValue = vm->getParamValue(0);
	if (numParam > 0 && fValue != NULL && fValue->type == OP_TYPE_FUNC)
	{
		for (int i = 1; i < numParam; i++)
		{
			Value newValue;
			CopyValue(&newValue, *vm->getParamValue(i));
			vm->push(newValue);
		}
		std::string errorDesc;
		int ret = vm->ProtectCallFunction(fValue->func, numParam - 1, errorDesc);
		vm->setReturnAsInt(ret, 0);
		vm->setReturnAsStr(errorDesc.c_str(), 1);
	}
}

void host_xpcall(XScriptVM* vm)
{
	Value* fValue1 = vm->getParamValue(0);
	Value* fValue2 = vm->getParamValue(1);

	if (fValue1 != NULL && fValue2 != NULL
		&& fValue1->type == OP_TYPE_FUNC && fValue2->type == OP_TYPE_FUNC)
	{
		std::string errorDesc;
		int errorFuncIndex = vm->getStackIndex(fValue2);
		int ret = vm->ProtectCallFunction(fValue1->func, 0, errorDesc, errorFuncIndex);
		vm->setReturnAsInt(ret, 0);
		vm->setReturnAsStr(errorDesc.c_str(), 1);
	}

	//vm->xpcall();
}

void host_require(XScriptVM* vm)
{
	char* moudleName = NULL;
	if (vm->getParamAsString(0, moudleName))
	{
		vm->RequireMoudle(moudleName);
	}
	
}

void host_inext(XScriptVM* vm)
{
	TABLE table;
	int key = -1;
	if (vm->getParamAsTable(0, table) && vm->getParamAsInt(1, key))
	{
		key += 1;
		bool hasFound = false;
		if (key < table->mArraySize)
		{
			//if (!IsValueNil(&table->mArrayData[key]))
			{
				vm->setReturnAsInt(key, 0);
				vm->setReturnAsValue(table->mArrayData[key], 1);
				hasFound = true;
			//	break;
			}
// 			else
// 			{
// 				key++;
// 			}
		}

		if (!hasFound)
		{
			int newIndex = key - table->mArraySize;
			if (newIndex < table->mNodeCapacity)
			{
				//if (!IsValueNil(&table->mNodeData[newIndex].value))
				{
					vm->setReturnAsInt(key, 0);
					vm->setReturnAsValue(table->mNodeData[newIndex].value, 1);
					hasFound = true;
// 					break;
				}
// 				else
// 				{
// 					newIndex++;
// 					key++;
// 				}
			}
		}
		

		if (!hasFound)
		{
			vm->setReturnAsNil(0);
		}
	}
}

void host_ipairs(XScriptVM* vm)
{
	TABLE table;
	if (vm->getParamAsTable(0, table))
	{
		Value* nextFunc = vm->GetGlobalValue("inext");
		if (nextFunc != NULL)
		{
			vm->setReturnAsValue(*nextFunc, 0);
		}
		else
		{
			vm->setReturnAsNil(0);
		}
		vm->setReturnAsTable(table, 1);
		vm->setReturnAsInt(-1, 2);
	}

}

void host_prints(XScriptVM* vm)
{
	char* str = NULL;
	if (vm->getParamAsString(0, str))
	{
		printf(str);
		printf("\n");
	}
}


void  host_print(XScriptVM* vm)
{
	std::string desc;
	int numParam = vm->getNumParam();
	for (int i = 0; i < numParam; i++)
	{
		if (i > 0)
			desc += "\t";

		desc += getValueDescString(vm, vm->getParamValue(i));
	}
	desc += "\n";
	printf(desc.c_str());
}

void  host_GetCurrentTime(XScriptVM* vm)
{
	__int64 startCounters, contersPersecond, currentCounter;
	QueryPerformanceCounter((LARGE_INTEGER *)&startCounters);
	QueryPerformanceFrequency((LARGE_INTEGER *)&contersPersecond);

	float fTime = (startCounters * 1.0f) / contersPersecond;
	vm->setReturnAsfloat(fTime);
}


void host_sleep(XScriptVM* vm)
{
	int spleepTime = 0;
	vm->getParamAsInt(0, spleepTime);
	::Sleep(spleepTime);
}

void	host_array(XScriptVM* vm)
{
	int arraySize = 0;
	if (vm->getParamAsInt(0, arraySize) && arraySize > 0)
	{
		TableValue* table = vm->newTable();
		table->mArraySize = arraySize;
		table->mArrayData = new Value[arraySize];
		vm->setReturnAsTable(table);
	}
}


void host_toNumber(XScriptVM* vm)
{
	Value* stackValue = vm->getParamValue(0);
	if (IsValueNumber(stackValue))
	{
		vm->setReturnAsInt(PNumberValue(stackValue));
	}
	else if (IsValueString(stackValue))
	{
		vm->setReturnAsInt(atoi(stringRawValue(stackValue)));
	}
	else
		vm->setReturnAsNil(0);

}

void host_toString(XScriptVM* vm)
{
	Value* stackValue = vm->getParamValue(0);
	if (stackValue != NULL)
	{
		std::string desc = getValueDescString(vm, stackValue);
		vm->setReturnAsStr(desc.c_str());
	}
	
}

void host_pause(XScriptVM* vm)
{
	system("pause");
}

void host_type(XScriptVM* vm)
{
	int type = vm->getParamType(0);
	vm->setReturnAsStr(getTypeName(type));
}

void host_gc(XScriptVM* vm)
{
	vm->GarbageCollect();
}