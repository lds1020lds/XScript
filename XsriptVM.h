#pragma once 
#include "Commonfunc.h"
#include <AccCtrl.h>
#include <map>
#include "MacroDefs.h"


#define		MAX_GLOBAL_DATASIZE				1024
#define		MAX_STACK_SIZE					0xffff
#define		MAX_LUA_CALL_STACK_DEPTH		1024


#define		MAX_FUNC_REG			8


enum 
{
	MS_White = 1 << 0,
	MS_Black = 1 << 1,
	MS_Gray = 1 << 2,
	MS_Fixed = 1 << 3,
};

enum HookEvent
{
	HE_HookLine = 0,
	HE_HookCall,
	HE_HookRet,
};

#define	 MASK_HOOKLINE	(1 << 0)
#define	 MASK_HOOKCALL	(1 << 1)
#define	 MASK_HOOKRET	(1 << 2)



#define  resoveStackIndex(index)		(index < 0 ? (mCurXScriptState->mFrameIndex + index) : index)

#define	 GC(o)							((CGObject*)o)
#define	 GC_SetBlack(o)					((CGObject*)o)->marked = MS_Black;
#define	 GC_SetWhite(o)					((CGObject*)o)->marked = MS_White;
#define	 GC_SetFixed(o)					((CGObject*)o)->marked = MS_Fixed;

#define  GCCommonHeader					CGObject* next;unsigned char type;unsigned char marked;		


#define	 IsValueCFunction(o)					((o)->type == OP_TYPE_FUNC && (o)->func->isCFunc)
#define	 IsValueLuaFunction(o)					((o)->type == OP_TYPE_FUNC && !(o)->func->isCFunc)

#define	 IsValueThread(o)					((o)->type == OP_TYPE_THREAD)
#define	 IsValueFunction(o)					((o)->type == OP_TYPE_FUNC)
#define	 IsValueString(o)					((o)->type == OP_TYPE_STRING)
#define	 IsValueNumber(o)					((o)->type == OP_TYPE_INT || (o)->type == OP_TYPE_FLOAT)

#define	 IsValueNil(o)						((o)->type == OP_TYPE_NIL)
#define	 IsValueInt(o)						((o)->type == OP_TYPE_INT)
#define	 IsValueFloat(o)					((o)->type == OP_TYPE_FLOAT)
#define	 IsValueTable(o)					((o)->type == OP_TYPE_TABLE)

#define	 MakeGloablIndex(stackIndex, nameIndex)	((stackIndex << 16) + nameIndex )
#define	 GloablVarStackIndex(o)						(o >> 16)
#define	 GloablVarNameIndex(o)						(o & 0xffff)


#define  EXEC_OP_ERROR(op, op1, op2)		ExecError("attempt to perform %s operator on %s and %s", #op, GetOperatorName(op1).c_str(), GetOperatorName(op2).c_str());

#define  ExecArgsCheck(cond,op, expect)	\
		if (!(cond))	\
			ExecError("Bad argument #%d to \"%s\"(%s)", op, GetOperatorName(op).c_str(), (expect));

#define  PopFrame(v)				mCurXScriptState->mTopIndex -= v;

#define FAST_PUSH(v)	\
	CopyValue(&mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex], v);	\
	mCurXScriptState->mTopIndex++;

#define  ResoveStackIndexWithEnv(index)		( index < 0 ? &mCurXScriptState->mStackElements[(mCurXScriptState->mFrameIndex + index)] : (( mEnvTable != NULL ) ? GetEnvValue(index) :  &mGlobalStackElements[GloablVarStackIndex(index)]))

#define		stringRawValue(o)				((const char*)&((o)->stringValue->value)) //((const char*)(&((o)->stringValue)).value))
#define		stringRawLen(o)					((o)->stringValue->len)

#define		CheckStrBuffer(size)		\
		if (mStrBufferSize < size)	\
		{	\
			delete []mStrBuffer;	\
			mStrBufferSize = size;	\
			mStrBuffer = new char[mStrBufferSize];	\
		}	



class FuncState;

class CSymbolTable;
class CMidCode;

class Function;
struct ValuePair;
struct Value;
class TableValue;
struct TableNode;
class XScriptState;
enum CoroutineStatus
{
	CS_Normal,
	CS_Running,
	CS_Suspend,
	CS_Dead,
};

class CGObject
{
public:
	GCCommonHeader;
};

struct XString
{
	GCCommonHeader;

	unsigned int	hash;
	int				len;
	char			value[1];
};


struct Value
{
	int type;
	union
	{
		int				iIntValue;
		float			fFloatValue;
		int				iFunctionValue;
		int				iInstrIndex;
		int				iRegIndex;
		XString*		stringValue;
		Function*		func;
		TableValue*		tableData;
		void*			userData;
		XScriptState*	threadData;
	};

	Value()
	{
		type = -1;
		userData = NULL;
		iIntValue = 0;
	}
};


struct TableKey
{
	Value		keyVal;
	TableNode*	next;

	TableKey()
		: next(NULL)
	{

	}
};

struct TableNode
{
	TableKey	key;
	Value		value;
};

class TableValue
{
public:
	GCCommonHeader;
	int			mArraySize;
	Value*		mArrayData;
	//ValuePair*	nextPair;


	int			mNodeCapacity;
	TableNode*	mNodeData;
	int			lastFreePos;
	TableValue()
		: mArraySize(0)
		, mArrayData(NULL)
	//	, nextPair(NULL)
		, mNodeData(NULL)
		, mNodeCapacity(0)
		, lastFreePos(0)
	{

	}

};



class UpValue
{
public:
	GCCommonHeader;

	Value*		pValue;
	UpValue*	nextValue;
	Value		value;
	UpValue()
		: nextValue(NULL)
		, pValue(NULL)
	{
		
		
	}
	
};


class HostFunction
{
public:
	std::string funcName;
	int			numParams;
	HOST_FUNC	pfnAddr;

	HostFunction(const std::string&	 _funcName, int _numParam, HOST_FUNC _pfn)
		: funcName(_funcName)
		, pfnAddr(_pfn)
		, numParams(_numParam)
	{

	}
};

class LuaFunction
{
public:
	FuncState*					proto;
	UpValue**					mUpVals;
	int							mNumUpVals;
	LuaFunction()
		: proto(NULL)
		, mUpVals(NULL)
		, mNumUpVals(0)
	{

	}
};


class Function
{
public:
	GCCommonHeader;

	bool				isCFunc;
	LuaFunction			luaFunc;
	int					hostFuncIndex;
	Function()
	{
		isCFunc = false;
		hostFuncIndex = -1;
	}
};


struct RuntimeOperator
{
	int type;
	union
	{
		int			iIntValue;
		float		fFloatValue;
		int			iStackIndex;
		int			iFunctionValue;
		int			iInstrIndex;
		XString*	stringValue;
		int			iRegIndex;
	};
	int				tableIndexType;
	union    
	{
		int			iIntTableValue;
		float		fFloatTableValue;
		XString*	strTableValue;
	};

	int		varNameIndex;

	RuntimeOperator()
	{
		varNameIndex = -1;
		type = -1;
		tableIndexType = 0;
		iIntTableValue = 0;
	}
};



struct ValuePair
{
	Value key;
	Value value;
	ValuePair* nextPair;
	ValuePair()
	{
		nextPair = NULL;
	}
};

struct Instr
{
	int opType;
	int numOpCount;
	RuntimeOperator* mOpList;
	int lineIndex;
};

class InstrStream
{
public:
	int numInstr;
	Instr* instrs;
	bool mHasJump;
};




class FuncState
{
public:
	GCCommonHeader;

	int	iIndex;
	InstrStream mInstrStream;
	int localDataSize;
	int localParamNum;
	int stackFrameSize;
	bool hasVarArgs;
	std::string funcName;
	std::string	sourceFileName;
	std::vector<FuncState*>	m_subFuncVec;
	FuncState*				m_parentFunc;
	std::vector<UpValueST>	m_upValueVec;

	std::vector<std::string>		m_localVarVec;
	FuncState()
	{
		iIndex = -1;
		localDataSize = 0;
		localParamNum = 0;
		stackFrameSize = 0;
		hasVarArgs = false;
		m_parentFunc = NULL;
	}
};

class UserClassData
{
public:
	std::string					mClassName;
	std::string					mBassClassName;
	std::vector<HostFunction>	mHostFunctions;
};

class MoudleData
{
public:
	std::string								mModuleName;
	std::vector<FuncState>					mFunctions;
	std::map<std::string, int>				mMoudleVarMap;
};

class CallInfo
{
public:
	FuncState*		mCurFunctionState;
	Function*		mCurFunction;
	int				mInstrIndex;
	int				mCurLine;
	int				mFrameIndex;
};

class XScriptState
{
public:
	UpValue*								mNextRefUpVals;
	CallInfo*								mCallInfoBase;
	int										mCurBaseCallIndex;
	int										mCurCallIndex;
	FuncState*								mCurFunctionState;
	Function*								mCurFunction;
	Value*									mStackElements;
	int										mMaxSize;
	int										mTopIndex;
	int										mFrameIndex;
	int										mInstrIndex;
	int										mCCallIndex;
	int										mStatus;

	Function*								mStartFunction;
};

typedef TableValue*			TABLE;


class	XScriptVM
{
public:
	XScriptVM();

	bool			init();
	void			loadBuildinLibs();
	bool			doFile(const std::string& fileName);

	void			ConvertMidCodeToInstr(CSymbolTable &symbolTable, CMidCode &midCode, const std::string& fileName, std::map<int, FuncState*>& funcMap);

	void			RegisterGlobalValues(CSymbolTable &symbolTable);
	Value*			GetEnvValue(int index);
	
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

	Value			ConstructValue(int value);
	Value			ConstructValue(float value);
	Value			ConstructValue(const char* value);
	Value			ConstructValue(XScriptState* state);

	Value			ConstructValue(XString* str);
	Value			ConstructValue(TABLE value);
	Value			ConstructValue(FuncState* mainFunc);

	TABLE			newTable();
	bool			getTableValue(TABLE table, char* key, int& value);
	bool			getTableValue(TABLE table, char* key, float& value);
	bool			getTableValue(TABLE table, char* key, char* &value);
	bool			getTableValue(TABLE table, char* key, TABLE& value);
	bool			getTableValue(TABLE table, int key, int& value);
	bool			getTableValue(TABLE table, int key, float& value);
	bool			getTableValue(TABLE table, int key, char* &value);
	bool			getTableValue(TABLE table, int key, TABLE& value);
	bool			getTableValue(TABLE table, const Value& keyValue, Value& value);
	void			setTableValue(TABLE table, char* key, int value);
	void			setTableValue(TABLE table, char* key, float value);
	void			setTableValue(TABLE table, char* key, char* str);
	void			setTableValue(TABLE table, char* key, TABLE t);
	void			setTableValue(TABLE table, int key, int value);
	void			setTableValue(TABLE table, int key, float value);
	void			setTableValue(TABLE table, int key, char* str);
	void			setTableValue(TABLE table, int key, TABLE t);
	void			setTableValue(TABLE table, const Value& key, const Value& value);

	void			resetScriptState(XScriptState* state);
	void			registerHostApi(const char* apiName, int numParams, HOST_FUNC pfnAddr);
	
	bool			RegisterUserClass(const char* className, const std::string& bassClassName, const std::vector<HostFunction>& funcVec);

	int				getNumParam();
	int				getParamType(int paramIndex);

	Value*			getParamValue(int paramIndex);
	int				getStackIndex(Value* value);

	Value*			getReturnRegValue(int index);

	bool			getParamAsInt(int paramIndex, int& value);
	bool			getParamAsFloat(int paramIndex, float& value);
	bool			getParamAsString(int paramIndex, char* &value);
	void*			getParamAsObj(int paramIndex, char* userType);
	bool			getParamAsTable(int paramIndex, TABLE& table);

	void			resetReturnValue();

	void			setReturnAsNil(int regIndex);
	void			setReturnAsTable(const TABLE& table, int regIndex = 0);
	void			setReturnAsValue(const Value& table, int regIndex = 0);

	void			setReturnAsInt(int iResult, int regIndex = 0);
	void			setReturnAsfloat(float fResult, int regIndex = 0);
	void			setReturnAsStr(const char* strResult, int regIndex = 0);

	void			setReturnAsUserData(const std::string& className, void* pThis, int regIndex = 0);


	void			beginCall();
	void			endCall();

	bool			call(const char* funcName);

	void			pushIntParam(int iValue);
	void			pushFloatParam(float fValue);
	void			pushStrParam(const char* strValue);

	int				getReturnType(int regIndex = 0);
	int				getReturnAsInt(int regIndex = 0);
	float			getReturnAsFloat(int regIndex = 0);
	char*			getReturnAsStr(int regIndex = 0);

	const			std::string&	GetString(int index) { return index < (int)m_stringVec.size() ? m_stringVec[index] : mEmptyStr; }

	int				AddString(const std::string& funcName);

	int				RegisterGlobalValue(const std::string& name);

	void			RegisterHostLib(const char* libName, std::vector<HostFunction>& hostFuncs);
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


	void			CreateCoroutie();
	void			ResumeCoroutie();
	void			YieldCoroutie();
	CoroutineStatus	GetCoroutieStatus(XScriptState* xsState);
	const char*		GetCoroutieStatusName(XScriptState* xsState);
private:
	std::string		GetOperatorName(RuntimeOperator* op, Value* value);

	std::string		GetOperatorName(int opIndex);

	bool			CastStrToFloat(const char *s, float *result);

	HostFunction*	getClassFuncByName(const std::string & className, const std::string& funcName);

	void			ExecuteFunction(int numLuaCallCount);

	void			ExecInstr_Func();

	void			ExecInstr_Concat_To();

	void			ExecInstr_Test_G();

	void			ExecInstr_Test_NE();

	void			ExecInstr_Test_E();

	void			ExecInstr_Logic_Not();

	void			ExecInstr_Test_L();

	void			ExecInstr_Test_GE();

	void			ExecInstr_Test_LE();

	void			ExecInstr_Logic_And();

	void			ExecInstr_Logic_Or();

	void			ExecInstr_SHRTO();

	void			ExecInstr_SHLTO();

	void			ExecInstr_ExpTo();

	void			ExecInstr_MODTO();

	void			ExecInstr_XORTO();

	void			ExecInstr_ORTO();

	void			ExecInstr_AndTO();

	void			ExecInstr_DIVTO();

	void			ExecInstr_MULTO();

	void			ExecInstr_SubTo();

	void			ExecInstr_AddTo();

	void			ExecInstr_JMP();

	void			ExecInstr_Concat();

	bool			ExecInstr_CallClassFunc();

	void			ExecInstr_CallStaticClassFunc();

	void			ExecInstr_RET();

	void			RemoveStackUpVals(int topIndex);

	void			ExecInstr_CALL();

	void			ExecInstr_JLE();

	void			ExecInstr_JL();

	void			ExecInstr_JGE();

	void			ExecInstr_JG();

	void			ExecInstr_JNE();

	void			ExecInstr_JE();

	void			ExecInstr_SHR();

	void			ExecInstr_SHL();

	void			ExecInstr_Not();

	void			ExecInstr_Xor();

	void			ExecInstr_Or();

	void			ExecInstr_And();

	void			ExecInstr_Dec();

	void			ExecInstr_Inc();

	void			ExecInstr_Neg();

	void			ExecInstr_Pow();

	void			ExecInstr_Mod();

	void			ExecInstr_Div();

	void			ExecInstr_Mul();

	void			ExecInstr_Sub();

	void			ExecInstr_Add();

	void			ExecError(const char* errorStr, ...);

	void			CallLuaFunction(Function* func);
	void			ExecLuaFunction(Function* func);

	void			CallFunctionInLua(Function* firstValue, int numParam);
	void			CallFunction(Function* funcValue, int numParam);

	void			CallHostFunc(HostFunction* func, int numParam);

	const Value&	resolveOpValue(int index);
	Value*			resolveOpPointer(int index);

	int				resolveOpAsInstrIndex(int index);

	int				castValueToInt(const Value& value);
	float			castValueTofloat(const Value& value);

	void			pushFrame(int frameSize);
	void			popFrame(int frameSize);

	Value*			resolveTableValue(RuntimeOperator* value, bool create);

	Value*			GetTableValue(TableValue* table, const Value &keyValue, bool create);

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

	XString*		NewXString(const char* str);

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




__forceinline const Value&   XScriptVM::resolveOpValue(int index)
{
	return *resolveOpPointer(index);
}


inline Value*   XScriptVM::resolveOpPointer(int index)
{
	//const Instr& instr = mCurFunction->mInstrStream.instrs[mCurFunction->mInstrStream.curInstr];
	switch(mCurInstr->mOpList[index].type)
	{
	case ROT_Stack_Index:
		{
			int stackIndex = mCurInstr->mOpList[index].iStackIndex;
			return ResoveStackIndexWithEnv(stackIndex);
		}
	case ROT_Table:
	case ROT_UpValue_Table:
		{
			Value* tableValue = resolveTableValue(&mCurInstr->mOpList[index], true);
			return tableValue;
		}
	case ROT_Reg:
		{
			return &mRegValue[mCurInstr->mOpList[index].iRegIndex];
		}
	case ROT_UpVal_Index:
		{
			int stackIndex = mCurInstr->mOpList[index].iStackIndex;
			return mCurXScriptState->mCurFunction->luaFunc.mUpVals[stackIndex]->pValue;
		}
	default:
		return (Value*)(&mCurInstr->mOpList[index]);
	}
}


inline int     XScriptVM::resolveOpAsInstrIndex(int index)
{
	const Value& value = resolveOpValue(index);
	return value.iInstrIndex;

}

inline int   XScriptVM::castValueToInt(const Value& value)
{
	switch (value.type)
	{
	case OP_TYPE_INT:
		return value.iIntValue;
	case OP_TYPE_FLOAT:
		return (int)value.fFloatValue;
	case OP_TYPE_STRING:
		return atoi(stringRawValue(&value));
	}
	return 0;
}


inline float   XScriptVM::castValueTofloat(const Value& value)
{
	switch (value.type)
	{
	case OP_TYPE_INT:
		return (float)value.iIntValue;
	case OP_TYPE_FLOAT:
		return value.fFloatValue;
	case OP_TYPE_STRING:
		return (float)atof(stringRawValue(&value));
	}
	return 0;
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

inline void  XScriptVM::pushIntParam(int iValue)
{
	_ASSERT(mIsInCallScriptFunc);
	if (!mIsInCallScriptFunc)
		return;
	push(ConstructValue(iValue));
	mNumScriptFuncParams++;
}

inline void  XScriptVM::pushFloatParam(float fValue)
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

inline int   XScriptVM::getReturnType(int regIndex)
{
	_ASSERT(mIsInCallScriptFunc);
	return mRegValue[regIndex].type;
	
}

inline int   XScriptVM::getReturnAsInt(int regIndex)
{
	_ASSERT(mIsInCallScriptFunc);
	//_ASSERT(mRegValue.type == OP_TYPE_INT);
	return castValueToInt(mRegValue[regIndex]);
}


inline float XScriptVM::getReturnAsFloat(int regIndex)
{
	_ASSERT(mIsInCallScriptFunc); 
	return castValueTofloat(mRegValue[regIndex]);
}


inline char* XScriptVM::getReturnAsStr(int regIndex)
{
	_ASSERT(mIsInCallScriptFunc);
	if (mRegValue[regIndex].type == OP_TYPE_STRING)
	{
		return (char*)stringRawValue(&mRegValue[regIndex]);
	}
	return NULL;
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



inline Value*   XScriptVM::resolveTableValue(RuntimeOperator* value, bool create)
{
	Value* table = NULL;
	if (value->type == ROT_UpValue_Table)
	{
		table = mCurXScriptState->mCurFunction->luaFunc.mUpVals[value->iStackIndex]->pValue;
	}
	else
	{
		table = ResoveStackIndexWithEnv(value->iStackIndex);
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
		keyValue.iIntValue = value->fFloatTableValue;
		break;
	case ROT_String:
		keyValue.type = OP_TYPE_STRING;
		keyValue.stringValue = value->strTableValue;
		break;
	case ROT_Stack_Index:
		keyValue = *ResoveStackIndexWithEnv(value->iIntTableValue);;
		break;
	case ROT_Reg:
		keyValue = mRegValue[value->iIntTableValue];
		break;
	case ROT_UpVal_Index:
		keyValue = *mCurXScriptState->mCurFunction->luaFunc.mUpVals[value->iIntTableValue]->pValue;
	default:
		break;
	}

	if (table->type != OP_TYPE_TABLE)
	{
		ExecError("Do table operation on %s", GetOperatorName(value, table));
	}

	return GetTableValue(table->tableData, keyValue, create);
}

inline int		XScriptVM::getStackIndex(Value* value)
{
	int index = value - mCurXScriptState->mStackElements;
	if (index < 0 || index >= mCurXScriptState->mMaxSize)
	{
		return -1;
	}
	return index;
}


inline bool	XScriptVM::getTableValue(TABLE table, int key, int& value)
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

inline bool	XScriptVM::getTableValue(TABLE table, int key, float& value)
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

inline bool	XScriptVM::getTableValue(TABLE table, int key, char* &value)
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

inline bool	XScriptVM::getTableValue(TABLE table, int key, TABLE& value)
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

inline bool	XScriptVM::getTableValue(TABLE table, char* key, int& value)
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

inline bool	XScriptVM::getTableValue(TABLE table, char* key, float& value)
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

inline bool	XScriptVM::getTableValue(TABLE table, char* key, char* &value)
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

inline bool	XScriptVM::getTableValue(TABLE table, char* key, TABLE& value)
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


inline Value	XScriptVM::ConstructValue(int value)
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
	func->luaFunc.proto = mainFunc;
	Value fValue;
	fValue.type = OP_TYPE_FUNC;
	fValue.func = func;
	return fValue;
}

inline Value	XScriptVM::ConstructValue(float value)
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

inline bool	XScriptVM::getTableValue(TABLE table, const Value& keyValue, Value& value)
{
	Value* pValue = GetTableValue(table, keyValue, false);

	if (pValue != NULL)
	{
		value = *pValue;
		return true;
	}

	return false;
}


inline TABLE	XScriptVM::newTable()
{
	return CreateTable();
}

inline void	XScriptVM::setTableValue(TABLE table, const Value& key, const Value& value)
{
	Value* pValue = GetTableValue(table, key, true);
	if (pValue != NULL)
	{
		CopyValue(pValue, value);
	}
}

inline void	XScriptVM::setTableValue(TABLE table, int key, int value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, int key, float value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, int key, char* str)
{
	setTableValue(table, ConstructValue(key), ConstructValue(str));
}

inline void	XScriptVM::setTableValue(TABLE table, int key, TABLE t)
{
	setTableValue(table, ConstructValue(key), ConstructValue(t));
}

inline void	XScriptVM::setTableValue(TABLE table, char* key, int value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, char* key, float value)
{
	setTableValue(table, ConstructValue(key), ConstructValue(value));
}

inline void	XScriptVM::setTableValue(TABLE table, char* key, char* str)
{
	setTableValue(table, ConstructValue(key), ConstructValue(str));
}

inline void	XScriptVM::setTableValue(TABLE table, char* key, TABLE t)
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