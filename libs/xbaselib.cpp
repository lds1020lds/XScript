#include "XsriptVM.h"
#include "xbaselib.h"
#include "Xutility.h"
#include <AccCtrl.h>
void init_base_lib()
{
	gScriptVM.RegisterHostApi("system_pause",	host_pause);
	gScriptVM.RegisterHostApi("sleep",			host_sleep);
	gScriptVM.RegisterHostApi("type",			host_type);
	gScriptVM.RegisterHostApi("toString",		host_toString);
	gScriptVM.RegisterHostApi("toNumber",		host_toNumber);
	gScriptVM.RegisterHostApi("array",			host_array);
	gScriptVM.RegisterHostApi("printf",			 host_print);
	gScriptVM.RegisterHostApi("prints",			host_prints);

	gScriptVM.RegisterHostApi("getCurrentTime",  host_GetCurrentTime);

	gScriptVM.RegisterHostApi("inext",			 host_inext);
	gScriptVM.RegisterHostApi("ipairs",			 host_ipairs);

	gScriptVM.RegisterHostApi("gettable",  host_gettable);
	gScriptVM.RegisterHostApi("settable",  host_settable);

	gScriptVM.RegisterHostApi("garbageCollect", host_gc);

	gScriptVM.RegisterHostApi("require", host_require);
	gScriptVM.RegisterHostApi("pcall",	 host_pcall);
	gScriptVM.RegisterHostApi("xpcall",	 host_xpcall);

	gScriptVM.RegisterHostApi("setenvtable",  host_setEnvTable);
	gScriptVM.RegisterHostApi("getenvtable",  host_getEnvTable);
	gScriptVM.RegisterHostApi("loadstring", host_loadstring);

	gScriptVM.RegisterHostApi("setmetatable", host_setmetadata);
	gScriptVM.RegisterHostApi("getmetatable", host_getmetadata);

	std::vector<HostFunction> coVec;

	coVec.push_back(HostFunction("create", host_coCreate));
	coVec.push_back(HostFunction("resume", host_coResume));
	coVec.push_back(HostFunction("yield",  host_coYield));
	coVec.push_back(HostFunction("status", host_coStatus));
	coVec.push_back(HostFunction("wrap", host_coWrapCreate));

	gScriptVM.RegisterHostLib("coroutine", coVec);

	std::vector<HostFunction> structVec;

	structVec.push_back(HostFunction("pack", xstruct_pack));
	structVec.push_back(HostFunction("unpack", xstruct_unpack));
	structVec.push_back(HostFunction("calFormatSize", xstruct_calFormatSize));

	gScriptVM.RegisterHostLib("struct", structVec);
}

void host_coWrapResume(XScriptVM* vm)
{
	Function* func = vm->GetCurCFunction();
	XScriptState* threadData = func->funcUnion.cFunc.mUpVal[0].threadData;
	vm->ResumeCoroutie(threadData, 0);
}

void host_coWrapCreate(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	if (IsValueLuaFunction(&value))
	{
		XScriptState* threadData = vm->CreateCoroutie(value.func);
		Function* func = vm->CreateCFunction(1);
		func->funcUnion.cFunc.mUpVal[0] = vm->ConstructValue(threadData);
		func->funcUnion.cFunc.pfnAddr = host_coWrapResume;
		Value funcValue;
		funcValue.func = func;
		funcValue.type = OP_TYPE_FUNC;
		vm->setReturnAsValue(funcValue, 0);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void host_coStatus(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	//ExecArgsCheck(IsValueThread(stackValue), 0, "coroutine expect");

	if (IsValueThread(&stackValue))
	{
		const char* status = vm->GetCoroutieStatusName(stackValue.threadData);
		vm->setReturnAsStr(status);
	}
}

void host_coCreate(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	if (IsValueLuaFunction(&value))
	{
		XScriptState* xsState = vm->CreateCoroutie(value.func);
		vm->setReturnAsValue(vm->ConstructValue(xsState));
	}
	else
	{
		vm->setReturnAsNil(0);
	}
	
}

void host_coResume(XScriptVM* vm)
{
	Value value = vm->getParamValue(0);
	if (IsValueThread(&value))
	{
		vm->ResumeCoroutie(value.threadData, 1);
	}
	
}

void host_coYield(XScriptVM* vm)
{
	vm->YieldCoroutie();
}

void host_setEnvTable(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	if ( IsValueTable(&stackValue) )
	{
		vm->SetEnvTable(stackValue.tableData);
	}
	else if (IsValueNil(&stackValue))
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

std::string  getValueDescString(XScriptVM* vm, const Value& value)
{
	std::string desc;
	char text[256] = { 0 };
	if (value.type == OP_TYPE_INT)
	{
		snprintf(text, 256, XIntConFmt, value.iIntValue);
		desc = text;
	}
	else if (value.type == OP_TYPE_FLOAT)
	{
		snprintf(text, 256, XFloatConFmt, value.fFloatValue);
		desc = text;
	}
	else if (value.type == OP_TYPE_STRING)
	{
		//snprintf(text, 256, "\"%s\"", stringRawValue(&value));
		desc = "\"";
		desc += stringRawValue(&value) ;
		desc += "\"";
	}
	else if (IsValueFunction(&value))
	{
		if (value.func->isCFunc)
		{
			snprintf(text, 256, "C function: 0x%p", value.func->funcUnion.cFunc.pfnAddr);
		}
		else
		{
			snprintf(text, 256, "lua function: %s", value.func->funcUnion.luaFunc.proto->funcName.c_str());
		}
		desc = text;
	}
	else if (IsUserType(value.type))
	{
		std::string localType = vm->GetString(UserDataType(value.type));
		snprintf(text, 256, "User Type: %s, 0x%p", localType.c_str(), value.lightUserData);
		desc = text;
	}
	else if (value.type == OP_TYPE_TABLE)
	{
		desc += "{";
		
		for (int i = 0; i < value.tableData->mArraySize; i++)
		{
			desc += ConvertToString(i);
			desc += "=";

			desc += getValueDescString(vm, value.tableData->mArrayData[i]);
			if (i < value.tableData->mArraySize - 1 || value.tableData->mNodeCapacity > 0)
			{
				desc += ",";
			}
		}

		for (int i = 0; i < value.tableData->mNodeCapacity; i++)
		{
			if (!IsValueNil(&value.tableData->mNodeData[i].key.keyVal))
			{
				desc += getValueDescString(vm, value.tableData->mNodeData[i].key.keyVal);
				desc += "=";
				desc += getValueDescString(vm, value.tableData->mNodeData[i].value);

				if (i < value.tableData->mNodeCapacity - 1)
					desc += ",";
			}
			
		}

		desc += "}";
	}
	else if (value.type == OP_TYPE_NIL)
	{
		desc = "nil";
	}

	return desc;
}



void host_pcall(XScriptVM* vm)
{
	int numParam = vm->getNumParam();
	Value fValue = vm->getParamValue(0);
	if (numParam > 0 &&  IsValueFunction(&fValue))
	{
		for (int i = 1; i < numParam; i++)
		{
			vm->push(vm->getParamValue(i));
		}
		std::string errorDesc;
		int ret = vm->ProtectCallFunction(fValue.func, numParam - 1, errorDesc);
		vm->setReturnAsInt(ret, 0);
		vm->setReturnAsStr(errorDesc.c_str(), 1);
	}
}

void host_xpcall(XScriptVM* vm)
{
	Value fValue1 = vm->getParamValue(0);
	Value fValue2 = vm->getParamValue(1);

	if ( IsValueFunction(&fValue1) && IsValueFunction(&fValue2))
	{
		std::string errorDesc;
		int errorFuncIndex = vm->getParamStackIndex(1);
		int ret = vm->ProtectCallFunction(fValue1.func, 0, errorDesc, errorFuncIndex);
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

void host_settable(XScriptVM* vm)
{
	TABLE table;
	vm->getParamAsTable(0, table);
	Value key = vm->getParamValue(1);
	Value value = vm->getParamValue(2);
	vm->setTableValue(table, key, value);
}

void host_gettable(XScriptVM* vm)
{
	TABLE table;
	vm->getParamAsTable(0, table);
	Value key = vm->getParamValue(1);
	Value ret;
	vm->getTableValue(table, key, ret);
	vm->setReturnAsValue(ret);
}

void host_inext(XScriptVM* vm)
{
	TABLE table;
	vm->getParamAsTable(0, table);
	Value key = vm->getParamValue(1);

	Value nextKey, nextValue;

	if (vm->GetNextKey(table, key, nextKey, nextValue))
	{
		vm->setReturnAsValue(nextKey, 0);
		vm->setReturnAsValue(nextValue, 1);
	}
	else
	{
		vm->setReturnAsNil(0);
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
		vm->setReturnAsNil(2);
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
	__int64 startCounters, contersPersecond;
	QueryPerformanceCounter((LARGE_INTEGER *)&startCounters);
	QueryPerformanceFrequency((LARGE_INTEGER *)&contersPersecond);

	float fTime = (startCounters * 1.0f) / contersPersecond;
	vm->setReturnAsfloat(fTime);
}


void host_sleep(XScriptVM* vm)
{
	XInt spleepTime = 0;
	vm->getParamAsInt(0, spleepTime);
	::Sleep((int)spleepTime);
}

void	host_array(XScriptVM* vm)
{
	XInt arraySize = 0;
	if (vm->getParamAsInt(0, arraySize) && arraySize > 0)
	{
		TableValue* table = vm->newTable((int)arraySize);
		vm->setReturnAsTable(table);
	}
}


void host_toNumber(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	if (IsValueNumber(&stackValue))
	{
		vm->setReturnAsInt((XInt)PNumberValue(stackValue));
	}
	else if (IsValueString(&stackValue))
	{
		vm->setReturnAsInt(StrToXInt(stringRawValue(&stackValue)));
	}
	else
		vm->setReturnAsNil(0);

}

void host_toString(XScriptVM* vm)
{
	Value stackValue = vm->getParamValue(0);
	std::string desc = getValueDescString(vm, stackValue);
	vm->setReturnAsStr(desc.c_str());
}

void host_pause(XScriptVM* vm)
{
	system("pause");
}

void host_getmetadata(XScriptVM* vm)
{
	Value param1 = vm->getParamValue(0);
	TableValue* metaTable = NULL;
	if (IsValueUserData(&param1))
	{
		metaTable = param1.userData->mMetaTable;
	}
	else if (IsValueTable(&param1))
	{
		metaTable = param1.tableData->mMetaTable;
		
	}
	
	if (metaTable != NULL)
	{
		vm->setReturnAsTable(param1.tableData->mMetaTable);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
	
}

void host_setmetadata(XScriptVM* vm)
{
	CheckParam(setmetatable, 1, param2, OP_TYPE_TABLE);

	Value param1 = vm->getParamValue(0);
	if (IsValueUserData(&param1))
	{
		param1.userData->mMetaTable = param2.tableData;
	}
	else if (IsValueTable(&param1))
	{
		param1.tableData->mMetaTable = param2.tableData;
	}
	
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

int	CalFormatSize(const Value& format)
{
	int bufferLen = 0;
	int lastCount = 1;
	int formatLen = (int)stringRealLen(&format);
	for (int i = 0; i < formatLen; i++)
	{
		if (isdigit(stringRawValue(&format)[i]))
		{
			lastCount = stringRawValue(&format)[i] - '0';

			while (i < formatLen - 1 && isdigit(stringRawValue(&format)[i + 1]))
			{
				lastCount = lastCount * 10 + (stringRawValue(&format)[i + 1] - '0');
				i++;
			}
		}
		else
		{
			switch (stringRawValue(&format)[i])
			{
			case 'c':
			{
				bufferLen += lastCount * sizeof(char);
			}
			break;
			case 'h':
			{
				bufferLen += lastCount * sizeof(short);
			}
			break;
			case 'i':
			{
				bufferLen += lastCount * sizeof(int);
			}
			break;
			case 'l':
			{
				bufferLen += lastCount * sizeof(long long);
			}
			break;
			case 'f':
			{
				bufferLen += lastCount * sizeof(float);
			}
			break;
			case 'd':
			{
				bufferLen += lastCount * sizeof(double);
			}
			break;
			case 's':
			{
				bufferLen += lastCount * sizeof(char);
			}
			break;
			}

			lastCount = 1;
		}

	}

	return bufferLen;
}

void xstruct_pack(XScriptVM* vm)
{
	CheckParam(struct.pack, 0, format, OP_TYPE_STRING);
	int len = CalFormatSize(format);

	char* buffer = new char[len];
	char* p = buffer;

	int lastCount = 1;
	int curParamIndex = 1;
	int formatLen = (int)stringRealLen(&format);
	for (int i = 0; i < formatLen; i++)
	{
		if (isdigit(stringRawValue(&format)[i]))
		{
			lastCount = stringRawValue(&format)[i] - '0';

			while (i < formatLen -1 && isdigit(stringRawValue(&format)[i + 1]))
			{
				lastCount = lastCount * 10 + (stringRawValue(&format)[i + 1] - '0');
				i++;
			}
		}
		else
		{
			switch (stringRawValue(&format)[i])
			{
			case 'c':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex+index, value, OP_TYPE_INT);
					*(char*)p = (char)value.iIntValue;
					p += sizeof(char);
				}

				curParamIndex += lastCount;
			}
			break;
			case 'h':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex+index, value, OP_TYPE_INT);
					*(short*)p = (short)value.iIntValue;
					p += sizeof(short);
				}

				curParamIndex += lastCount;
			}
			break;
			case 'i':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex+index, value, OP_TYPE_INT);
					*(int*)p = (int)value.iIntValue;
					p += sizeof(int);
				}

				curParamIndex += lastCount;
			}
			break;
			case 'l':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex + index, value, OP_TYPE_INT);
					*(long long*)p = (long long)value.iIntValue;
					p += sizeof(long long);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'f':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex + index, value, OP_TYPE_FLOAT);
					*(float*)p = (float)value.fFloatValue;
					p += sizeof(float);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'd':
			{
				for (int index = 0; index < lastCount; index++)
				{
					CheckParam(struct.pack, curParamIndex + index, value, OP_TYPE_FLOAT);
					*(double*)p = (double)value.fFloatValue;
					p += sizeof(double);
				}
				curParamIndex += lastCount;
			}
			break;
			case 's':
			{
				CheckParam(struct.pack, curParamIndex, value, OP_TYPE_STRING);

				memset(p, 0, sizeof(lastCount));
				int strLen = stringRealLen(&value);
				memcpy(p, stringRawValue(&value), lastCount < strLen ? lastCount : strLen);
				p += lastCount;
				curParamIndex++;
			}
			break;
			default:
				vm->ExecError("struct.pack: error format");
				break;
			}

			lastCount = 1;
		}
		
	}

	vm->setReturnAsValue(vm->ConstructValue(vm->NewXString(buffer, len)));
}

void xstruct_calFormatSize(XScriptVM* vm)
{
	CheckParam(struct.unpack, 0, format, OP_TYPE_STRING);

	vm->setReturnAsInt(CalFormatSize(format));
}

void xstruct_unpack(XScriptVM* vm)
{
	CheckParam(struct.unpack, 0, format, OP_TYPE_STRING);
	CheckParam(struct.unpack, 1, buffer, OP_TYPE_STRING);

	int bufferLen = CalFormatSize(format);

	if (stringRawLen(&buffer) < bufferLen)
	{
		vm->ExecError("struct.unpack: buffer length is not enough, expected %d, but %d", bufferLen, stringRawLen(&buffer));
	}

	const char* p = stringRawValue(&buffer);
	int lastCount = 1;
	int curParamIndex = 0;
	for (int i = 0; i < (int)stringRealLen(&format); i++)
	{
		if (isdigit(stringRawValue(&format)[i]))
		{
			lastCount = stringRawValue(&format)[i] - '0';
		}
		else
		{
			switch (stringRawValue(&format)[i])
			{
			case 'c':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(char*)p, curParamIndex + index);
					p += sizeof(char);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'h':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(short*)p, curParamIndex + index);
					p += sizeof(short);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'i':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(int*)p, curParamIndex + index);
					p += sizeof(int);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'l':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsInt(*(long long*)p, curParamIndex + index);
					p += sizeof(long long);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'f':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsfloat(*(float*)p, curParamIndex + index);
					p += sizeof(float);
				}
				curParamIndex += lastCount;
			}
			break;
			case 'd':
			{
				for (int index = 0; index < lastCount; index++)
				{
					vm->setReturnAsfloat(*(double*)p, curParamIndex + index);
					p += sizeof(double);
				}
				curParamIndex += lastCount;
			}
			break;
			case 's':
			{
				char* strBuf = new char[lastCount];
				memcpy(strBuf, p, lastCount);
				vm->setReturnAsValue(vm->ConstructValue(vm->NewXString(strBuf, lastCount)), curParamIndex);
				delete []strBuf;

				p += lastCount;
				curParamIndex++;
			}
			break;
			default:
				vm->ExecError("error format for struct.pack");
				break;
			}

			lastCount = 1;
		}

	}
}
