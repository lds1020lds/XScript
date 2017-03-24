#pragma once 
#include "VMDefs.h"

class XScriptState
{
public:
	UpValue*								mNextRefUpVals;
	CallInfo*								mCallInfoBase;
	int										mCurCallIndex;

	FuncState*								mCurFunctionState;
	Function*								mCurFunction;

	Value*									mStackElements;
	int										mStackSize;
	int										mTopIndex;
	int										mFrameIndex;

	int										mInstrIndex;
	int										mCCallIndex;
	int										mStatus;
	Function*								mCurrentCFunction;
	Function*								mStartFunction;
};

typedef TableValue*			TABLE;

class	XScriptVM
{
public:
	XScriptVM();

	bool			init();
	void			loadHostLibs();
	bool			doFile(const std::string& fileName);

	void			ConvertMidCodeToInstr(CSymbolTable &symbolTable, CMidCode &midCode, const std::string& fileName, std::map<int, FuncState*>& funcMap);

	void			RegisterGlobalValues(CSymbolTable &symbolTable);
	Value			GetEnvValue(int index);
	
	FuncState*		CompileString(const std::string& code);
	void			SetEnvTable(TableValue* envTable) { mEnvTable = envTable; }
	TableValue*		GetEnvTable() { return mEnvTable; }

	int				GetUnusedVarIndex();
	void			ReleaseUsedVarIndex(int index);
	int				GetGloabVarIndex(VariantST* var);
	Value*			GetGlobalValue(const std::string& varName);

	void			setGloablStackValue(int index, const Value& value);
	const Value&    getStackValue(int index);
	Value*		    getStackValueRef(int index);

	Value			ConstructValue(XInt value);
	Value			ConstructValue(XFloat value);

	Value			ConstructValue(const char* value);
	Value			ConstructValue(XScriptState* state);
	Value			ConstructValue(XString* str);
	Value			ConstructValue(TABLE value);
	Value			ConstructValue(FuncState* mainFunc);

	TABLE			newTable(int arraySize = 0);
	bool			getTableValue(TABLE table, const  char* key, XInt& value);
	bool			getTableValue(TABLE table, const char* key, XFloat& value);
	bool			getTableValue(TABLE table, const char* key, char* &value);
	bool			getTableValue(TABLE table, const  char* key, TABLE& value);

	bool			getTableValue(TABLE table, XInt key, XInt& value);
	bool			getTableValue(TABLE table, XInt key, XFloat& value);
	bool			getTableValue(TABLE table, XInt key, char* &value);
	bool			getTableValue(TABLE table, XInt key, TABLE& value);

	void			setTableValue(TABLE table, const char* key, XInt value);
	void			setTableValue(TABLE table, const char* key, XFloat value);
	void			setTableValue(TABLE table, const char* key, const char* str);
	void			setTableValue(TABLE table, const char* key, TABLE t);

	void			setTableValue(TABLE table, XInt key, XInt value);
	void			setTableValue(TABLE table, XInt key, XFloat value);
	void			setTableValue(TABLE table, XInt key, const char* str);
	void			setTableValue(TABLE table, XInt key, TABLE t);
	void			setTableValue(TABLE table, const Value& key, const Value& value);


	bool			getTableValue(TableValue* table, const Value &keyValue, Value& resultValue);
	Value*			GetTableValueRef(TableValue* tableDate, const Value &keyValue);


	void			resetScriptState(XScriptState* state);
	void			RegisterHostApi(const char* apiName, HOST_FUNC pfnAddr);
	
	void			RegisterUserClass(const char* className, const char* bassClassName, const std::vector<HostFunction>& funcVec);

	int				getNumParam();
	int				getParamType(int paramIndex);

	Value			getParamValue(int paramIndex);
	int				getParamStackIndex(int paramIndex);

	Value*			getReturnRegValue(int index);

	bool			getParamAsInt(int paramIndex, XInt& value);
	bool			getParamAsFloat(int paramIndex, XFloat& value);
	bool			getParamAsString(int paramIndex, char* &value);
	void*			getParamAsObj(int paramIndex, char* userType);
	bool			getParamAsTable(int paramIndex, TABLE& table);

	void			resetReturnValue();

	void			setReturnAsNil(int regIndex);
	void			setReturnAsTable(const TABLE& table, int regIndex = 0);
	void			setReturnAsValue(const Value& table, int regIndex = 0);

	void			setReturnAsInt(XInt iResult, int regIndex = 0);
	void			setReturnAsfloat(XFloat fResult, int regIndex = 0);
	void			setReturnAsStr(const char* strResult, int regIndex = 0);

	void			setReturnAsUserData(const char* className, void* pThis, int regIndex = 0);

	void			SetHookMask(int mask) { mHookMask = mask; }

	void			beginCall();
	void			endCall();

	void			pushIntParam(XInt iValue);
	void			pushFloatParam(XFloat fValue);
	void			pushStrParam(const char* strValue);

	const			std::string&	GetString(int index) { return index < (int)m_stringVec.size() ? m_stringVec[index] : mEmptyStr; }

	int				AddString(const std::string& funcName);

	int				RegisterGlobalValue(const std::string& name);

	void			RegisterHostLib(const char* libName, std::vector<HostFunction>& hostFuncs);

	void			CreateFunctionTable(const std::vector<HostFunction> &hostFuncVec, TABLE table);

	void			GarbageCollect();
	void			RequireMoudle(const char* moudleName);
	
	std::string		stackBackTrace();

	Value*			GetStackValueByName(int stackIndex, const std::string& name);
	Value*			GetStackValueByIndex(int stackIndex, int varIndex, std::string& name);

	void			CallHookFunction(int event, int curLine);
	const HostFunction&	GetHostFunction(int index) { return mHostFunctions[index]; }
	int				GetStackDepth() { return mCurXScriptState->mCurCallIndex; }
	CallInfo*		GetCallInfo(int index) { return &mCurXScriptState->mCallInfoBase[index]; }
	int				ProtectCallFunction(Function* firstValue, int numParam, std::string& errorDesc, int errorFunc = -1);

	int				ProtectResume( std::string & errorDesc);

	void			push(const Value& value);
	Value			pop();
	bool			EvalConditionString(const char* condstr);

	XScriptState*	CreateCoroutie(Function* func);
	void			ResumeCoroutie(XScriptState* threadState, int offset);
	void			YieldCoroutie();
	CoroutineStatus	GetCoroutieStatus(XScriptState* xsState);
	const char*		GetCoroutieStatusName(XScriptState* xsState);

	void			GrowStack(XScriptState* xsState, int growSize);
	Function*		CreateCFunction(int numUpvals);
	Function*		GetCurCFunction() { return mCurXScriptState->mCurrentCFunction; }


	bool			CalByTagMethod(Value* result, Value* value1, Value* value2, MetaMethodType mmtType );

	const char*		MetaMetodString(MetaMethodType type);

	Value			GetMetaMethod(Value* value, MetaMethodType type);

	bool			GetNextKey(TableValue* tableData, const Value &keyValue, Value& nextKey, Value& nextValue);

	void			ExecError(const char* errorStr, ...);
	XString*		NewXString(const char* str);
	XString*		NewXString(const char* str, int len);

private:
	std::string		GetOperatorName(RuntimeOperator* op, Value* value);

	std::string		GetOperatorName(int opIndex);

	bool			CastStrToFloat(const char *s, XFloat *result);

	HostFunction*	getClassFuncByName(const std::string & className, const std::string& funcName);

	void			ExecuteFunction(int numLuaCallCount);

	void			ExecInstr_Func();

	void			ExecInstr_Concat_To();

	void			ExecInstr_Logic_Not();

	void			ExecInstr_Logic_And();

	void			ExecInstr_Logic_Or();

	void			ExecInstr_JMP();

	void			ExecInstr_Concat();

	void			ExecInstr_RET();

	void			RemoveStackUpVals(int topIndex);

	void			ExecInstr_Neg();
	

	void			CallLuaFunction(Function* func);
	void			ExecLuaFunction(Function* func);

	void			CallFunctionInLua(Function* firstValue, int numParam);
	void			CallFunction(Function* funcValue, int numParam);

	void			CallHostFunc(Function* func, HOST_FUNC pfnAddr, int numParam);

	Value			resolveOpValue(int index);

	void			SetOpValue(int index, const Value& value);

	int				resolveOpAsInstrIndex(int index);

	void			pushFrame(int frameSize);
	void			popFrame(int frameSize);

	Value			resolveTableValue(RuntimeOperator* value);

	void			resolveSetTableValue(RuntimeOperator* value, const Value &tableVale);

	void			SetTableValue(TableValue* table, const Value &keyValue, const Value& value);

	XInt			FindKeyIndex(TableValue* tableData, const Value &keyValue);

	TableValue*		CreateTable();
	Function*		CreateFunction();
	

	FuncState*		CreateFunctionState();
	UpValue*		CreateUpVal();

	void			MarkValue(Value* value);
	void			MarkTable(TABLE table);
	void			MarkProto(FuncState* func);
	void			MarkObjects();
	void			SweepObjects();
	void			FreeObject(CGObject* obj);

	TableNode*		NewTableKey(TABLE table, const Value* key);
	void			RehashTable(TABLE table);


	UserData*		CreateUserData( int size );

	void			ResizeHashTable();
private:
	struct XScript_LongJmp
	{
		XScript_LongJmp *previous;
		jmp_buf			j;
		int				errorCode;
		int				errorFunc;
		std::string		mErrorDesc;
	};

	CGObject*								mRootCG;
	std::vector<std::string>				m_stringVec;
	std::string								mEmptyStr;
	Instr*									mCurInstr;
	std::vector<HostFunction>				mHostFunctions;

	XString**								mStringHashTable;
	int										mStringHashSize;
	int										mStringHashUsedSize;
	std::map<std::string, UserClassData>	mUserClassDataMap;
	std::map<std::string, int>				mGloablVarMap;
	bool									mGloablUseMap[MAX_GLOBAL_DATASIZE];
	Value									mRegValue[MAX_FUNC_REG];
	Value*									mNilValue;
	XScript_LongJmp*						mLongJmp;
	char*									mStrBuffer;
	int										mStrBufferSize;
	TABLE									mEnvTable;
	TABLE									mModuleTable;
	TABLE									mMetaTable;
	int										mHookMask;
	bool									mAllowHook;
	int										mNumHostFuncParam;
	bool									mIsInCallScriptFunc;
	int										mNumScriptFuncParams;

	Value*									mGlobalStackElements;

	XScriptState							mMainXScriptState;
	XScriptState*							mCurXScriptState;
};

const char* getTypeName(int type);

bool isValueEqual(const Value& value1, const Value& value2);

inline int 	XScriptVM::AddString(const std::string& funcName)
{
	for (int i = 0; i < (int)m_stringVec.size(); i++)
	{
		if (funcName == m_stringVec[i])
			return i;
	}

	m_stringVec.push_back(funcName);
	return (m_stringVec.size() - 1);

}




__forceinline Value   XScriptVM::resolveOpValue(int index)
{
	//const Instr& instr = mCurFunction->mInstrStream.instrs[mCurFunction->mInstrStream.curInstr];
	switch (mCurInstr->mOpList[index].type)
	{
	case ROT_Stack_Index:
	{
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;
		return ResoveStackIndexWithEnv(stackIndex);
	}
	case ROT_Table:
	case ROT_UpValue_Table:
	{
		return resolveTableValue(&mCurInstr->mOpList[index]);
	}
	case ROT_Reg:
	{
		return mRegValue[mCurInstr->mOpList[index].iRegIndex];
	}
	case ROT_UpVal_Index:
	{
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;
		return *mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[stackIndex]->pValue;
	}
	default:
		return *(Value*)(&mCurInstr->mOpList[index]);
	}
}

inline void		XScriptVM::SetOpValue(int index, const Value& value)
{
	//const Instr& instr = mCurFunction->mInstrStream.instrs[mCurFunction->mInstrStream.curInstr];
	switch (mCurInstr->mOpList[index].type)
	{
	case ROT_Stack_Index:
	{
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;
		Value* pValue = ResoveStackIndex(stackIndex);
		*pValue = value;
		break;
	}
	case ROT_Table:
	case ROT_UpValue_Table:
	{
		resolveSetTableValue(&mCurInstr->mOpList[index], value);
		break;
	}
	case ROT_Reg:
	{
		mRegValue[mCurInstr->mOpList[index].iRegIndex] = value;
		break;
	}
	case ROT_UpVal_Index:
	{
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;
		Value* pValue = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[stackIndex]->pValue;
		*pValue = value;
		break;
	}
	}
}


inline int     XScriptVM::resolveOpAsInstrIndex(int index)
{
	const Value& value = resolveOpValue(index);
	return value.iInstrIndex;

}


inline void   XScriptVM::push(const Value& value)
{
	CopyValue(&mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex], value);
	mCurXScriptState->mTopIndex++;
}


inline Value   XScriptVM::pop()
{
	mCurXScriptState->mTopIndex--;
	return mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex];
}


inline void     XScriptVM::pushFrame(int frameSize)
{
	for(int i = 0; i < frameSize; i++)
	{
		mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex + i].type = OP_TYPE_NIL;
		mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex + i].iIntValue = 0;
	}

	mCurXScriptState->mTopIndex += frameSize;
	mCurXScriptState->mFrameIndex = mCurXScriptState->mTopIndex;
}


inline void     XScriptVM::popFrame(int frameSize)
{
	mCurXScriptState->mTopIndex -= frameSize;
}

inline void     XScriptVM::setGloablStackValue(int index, const Value& value)
{
	mGlobalStackElements[index] = value;
}


inline const Value&    XScriptVM::getStackValue(int index)
{
	return mCurXScriptState->mStackElements[resoveStackIndex(index)];
}

inline Value*		  XScriptVM::getStackValueRef(int index)
{
	return &mCurXScriptState->mStackElements[resoveStackIndex(index)];
}

inline int   XScriptVM::getNumParam()
{
	return mNumHostFuncParam;
}

inline void  XScriptVM::beginCall()
{
	_ASSERT(!mIsInCallScriptFunc);
	mIsInCallScriptFunc = true;
	mNumScriptFuncParams = 0;
}

inline void  XScriptVM::endCall()
{
	_ASSERT(mIsInCallScriptFunc);
	mIsInCallScriptFunc = false;
}

inline void  XScriptVM::pushIntParam(XInt iValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;
	push(ConstructValue(iValue));
	mNumScriptFuncParams++;
}

inline void  XScriptVM::pushFloatParam(XFloat fValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;

	push(ConstructValue(fValue));
	mNumScriptFuncParams++;
}


inline void  XScriptVM::pushStrParam(const char* strValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;

	push(ConstructValue(strValue));
	mNumScriptFuncParams++;
}


inline HostFunction*	XScriptVM::getClassFuncByName(const std::string & className, const std::string& funcName)
{
	HostFunction* function = NULL;

	std::string searchClassName = className;
	while (!searchClassName.empty() && mUserClassDataMap.find(searchClassName) != mUserClassDataMap.end())
	{
		UserClassData& userClassData = mUserClassDataMap[searchClassName];
		for (int i = 0; i < (int)userClassData.mHostFunctions.size(); i++)
		{
			if (userClassData.mHostFunctions[i].funcName == funcName)
			{
				return &userClassData.mHostFunctions[i];
			}

		}
		searchClassName = userClassData.mBassClassName;
	}

	return NULL;
}

inline void	XScriptVM::resolveSetTableValue(RuntimeOperator* value, const Value &tableValue)
{
	Value* table = NULL;
	if (value->type == ROT_UpValue_Table)
	{
		table = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iStackIndex]->pValue;
	}
	else
	{
		table = ResoveStackIndex(value->iStackIndex);
	}

	Value keyValue;
	switch (value->tableIndexType)
	{
	case ROT_Int:
		keyValue.type = OP_TYPE_INT;
		keyValue.iIntValue = value->iIntTableValue;
		break;
	case ROT_Float:
		keyValue.type = OP_TYPE_FLOAT;
		keyValue.fFloatValue = value->fFloatTableValue;
		break;
	case ROT_String:
		keyValue.type = OP_TYPE_STRING;
		keyValue.stringValue = value->strTableValue;
		break;
	case ROT_Stack_Index:
		keyValue = ResoveStackIndexWithEnv((int)value->iIntTableValue);;
		break;
	case ROT_Reg:
		keyValue = mRegValue[value->iIntTableValue];
		break;
	case ROT_UpVal_Index:
		keyValue = *mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iIntTableValue]->pValue;
	default:
		break;
	}

	Value curTable = *table;
	for (int i = 0; i < MAX_TAGMETHOD_LOOP; i++)
	{
		Value tagMethod;
		if (curTable.type == OP_TYPE_TABLE)
		{
			Value* tvalRef = GetTableValueRef(curTable.tableData, keyValue);

			if (tvalRef != NULL)
			{
				*tvalRef = tableValue;
				return;
			}
			else
			{
				tagMethod = GetMetaMethod(&curTable, MMT_NewIndex);
				if (IsValueNil(&tagMethod))
				{
					SetTableValue(curTable.tableData, keyValue, tableValue);
					return;
				}
			}
		}
		else
		{
			tagMethod = GetMetaMethod(&curTable, MMT_NewIndex);
			if (IsValueNil(&tagMethod))
			{
				ExecError("Do index operation on %s", GetOperatorName(value, table).c_str());
			}
		}

		if (IsValueFunction(&tagMethod))
		{
			push(curTable);
			push(keyValue);
			push(tableValue);
			Instr* savedInstr = mCurInstr;
			CallFunction(tagMethod.func, 3);
			mCurInstr = savedInstr;
			return;
		}
		else
		{
			curTable = tagMethod;
		}
	}

	ExecError("too many loops in __index on %s", GetOperatorName(value, table).c_str());
}


inline Value   XScriptVM::resolveTableValue(RuntimeOperator* value)
{
	Value* table = NULL;
	if (value->type == ROT_UpValue_Table)
	{
		table = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iStackIndex]->pValue;
	}
	else
	{
		table = ResoveStackIndex(value->iStackIndex);
	}

	Value keyValue;
	switch (value->tableIndexType)
	{
	case ROT_Int:
		keyValue.type = OP_TYPE_INT;
		keyValue.iIntValue = value->iIntTableValue;
		break;
	case ROT_Float:
		keyValue.type = OP_TYPE_FLOAT;
		keyValue.fFloatValue = value->fFloatTableValue;
		break;
	case ROT_String:
		keyValue.type = OP_TYPE_STRING;
		keyValue.stringValue = value->strTableValue;
		break;
	case ROT_Stack_Index:
		keyValue = ResoveStackIndexWithEnv((int)value->iIntTableValue);;
		break;
	case ROT_Reg:
		keyValue = mRegValue[value->iIntTableValue];
		break;
	case ROT_UpVal_Index:
		keyValue = *mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[value->iIntTableValue]->pValue;
	default:
		break;
	}



	Value curTable = *table;
	for (int i = 0; i < MAX_TAGMETHOD_LOOP; i++)
	{
		Value tagMethod;

		if (curTable.type == OP_TYPE_TABLE)
		{
			Value tval;
			
			if (getTableValue(curTable.tableData, keyValue, tval))
			{
				return tval;
			}
			else
			{
				tagMethod = GetMetaMethod(&curTable, MMT_Index);
				if (IsValueNil(&tagMethod))
				{
					return *mNilValue;
				}
			}
		}
		else
		{
			tagMethod = GetMetaMethod(&curTable, MMT_Index);
			if (IsValueNil(&tagMethod))
			{
				ExecError("Do index operation on %s", GetOperatorName(value, table).c_str());
			}
		}

		if (IsValueFunction(&tagMethod))
		{
			push(curTable);
			push(keyValue);
			Instr* savedInstr = mCurInstr;
			CallFunction(tagMethod.func, 2);
			mCurInstr = savedInstr;
			return mRegValue[0];
		}
		else
		{
			curTable = tagMethod;
		}
	}

	ExecError("too many loops in __index");
	return *mNilValue;
}

inline int		XScriptVM::getParamStackIndex(int paramIndex)
{
	return mCurXScriptState->mTopIndex - (mNumHostFuncParam - paramIndex);
}


inline bool	XScriptVM::getTableValue(TABLE table, XInt key, XInt& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_INT)
		{
			value = subValue.iIntValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, XInt key, XFloat& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_FLOAT)
		{
			value = subValue.fFloatValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, XInt key, char* &value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_STRING)
		{
			stringRawValue(&subValue);
			return true;
		}
	}

	return false;
}

inline Value	XScriptVM::ConstructValue(XString* str)
{
	Value tableValue;
	tableValue.stringValue = str;
	tableValue.type = OP_TYPE_STRING;
	return tableValue;
}

inline Value	XScriptVM::ConstructValue(TABLE value)
{
	Value tableValue;
	tableValue.tableData = value;
	tableValue.type = OP_TYPE_TABLE;
	return tableValue;
}

inline bool	XScriptVM::getTableValue(TABLE table, XInt key, TABLE& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_TABLE)
		{
			value = subValue.tableData;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, XInt& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_INT)
		{
			value = subValue.iIntValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, XFloat& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_FLOAT)
		{
			value = subValue.fFloatValue;
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, char* &value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_STRING)
		{
			stringRawValue(&subValue);
			return true;
		}
	}

	return false;
}

inline bool	XScriptVM::getTableValue(TABLE table, const char* key, TABLE& value)
{
	Value subValue;
	if (getTableValue(table, ConstructValue(key), subValue))
	{
		if (subValue.type == OP_TYPE_TABLE)
		{
			value = subValue.tableData;
			return true;
		}
	}

	return false;
}


inline Value	XScriptVM::ConstructValue(XInt value)
{
	Value subValue;
	subValue.type = OP_TYPE_INT;
	subValue.iIntValue = value;
	return subValue;
}


inline Value XScriptVM::ConstructValue(FuncState* mainFunc)
{
	Function* func = CreateFunction();
	func->isCFunc = false;
	func->funcUnion.luaFunc.proto = mainFunc;
	Value fValue;
	fValue.type = OP_TYPE_FUNC;
	fValue.func = func;
	return fValue;
}

inline Value	XScriptVM::ConstructValue(XFloat value)
{
	Value subValue;
	subValue.type = OP_TYPE_FLOAT;
	subValue.fFloatValue = value;
	return subValue;
}

inline Value	XScriptVM::ConstructValue(const char* value)
{
	Value subValue;
	subValue.type = OP_TYPE_STRING;
	subValue.stringValue = NewXString(value);
	return subValue;
}

inline Value	XScriptVM::ConstructValue(XScriptState* state)
{
	Value subValue;
	subValue.type = OP_TYPE_THREAD;
	subValue.threadData = state;
	return subValue;
}


inline TABLE	XScriptVM::newTable(int arraySize)
{
	TABLE table = CreateTable();
	if (arraySize > 0)
	{
		table->mArraySize = arraySize;
		table->mArrayData = new Value[arraySize];
	}

	return table;
}

inline void	XScriptVM::setTableValue(TABLE table, const Value& key, const Value& value)
{
	SetTableValue(table, key, value);
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, XInt value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, XFloat value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, const char* str)
{
	setTableValue(table, ConstructValue(key), ConstructValue(str));
}

inline void	XScriptVM::setTableValue(TABLE table, XInt key, TABLE t)
{
	setTableValue(table, ConstructValue(key), ConstructValue(t));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, XInt value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, XFloat value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, const char* str)
{
	setTableValue(table, ConstructValue(key), ConstructValue(str));
}

inline void	XScriptVM::setTableValue(TABLE table, const char* key, TABLE t)
{
	setTableValue(table, ConstructValue(key), ConstructValue(t));
}

inline Value*	XScriptVM::GetGlobalValue(const std::string& varName)
{
	std::map<std::string, int>::iterator it = mGloablVarMap.find(varName);
	if (it != mGloablVarMap.end())
	{
		return &mGlobalStackElements[it->second];
	}

	return NULL;
}

inline Value*	XScriptVM::getReturnRegValue(int index)
{
	return &mRegValue[index];
}



extern XScriptVM gScriptVM;