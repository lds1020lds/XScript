#pragma once
#ifndef VMDefs_h__
#define VMDefs_h__

#include "Commonfunc.h"
#include <map>
#include "MacroDefs.h"
#include <AccCtrl.h>

#define		MAX_GLOBAL_DATASIZE				1024
#define		MAX_STACK_SIZE					0xffff
#define		MIN_STACK_SIZE					256
#define		MAX_LUA_CALL_STACK_DEPTH		1024

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

#define	 XSetIntValue(o, n)					(o)->type = OP_TYPE_INT;(o)->iIntValue = n;
#define	 SetFloatValue(o, n)				(o)->type = OP_TYPE_FLOAT;(o)->fFloatValue = n;
#define	 SetUserDataValue(o, n)				(o)->type = OP_TYPE_USERDATA;(o)->userData = n;

#define	 IsValueLightUserData(o)			(((o)->type >> 16) == OP_LIGHTUSERDATA)
#define	 IsValueUserData(o)					((o)->type == OP_TYPE_USERDATA)

#define	 MakeGloablIndex(stackIndex, nameIndex)	((stackIndex << 16) + nameIndex )
#define	 GloablVarStackIndex(o)						(o >> 16)
#define	 GloablVarNameIndex(o)						(o & 0xffff)


#define  EXEC_OP_ERROR(op, op1, op2)		ExecError("attempt to perform %s operator on %s and %s", #op, GetOperatorName(op1).c_str(), GetOperatorName(op2).c_str());

#define  ExecArgsCheck(cond,op, expect)	\
		if (!(cond))	\
			ExecError("Bad argument #%d to \"%s\"(%s)", op, GetOperatorName(op).c_str(), (expect));

#define  ResoveStackIndexWithEnv(index)		( index < 0 ? mCurXScriptState->mStackElements[(mCurXScriptState->mFrameIndex + index)] : (( mEnvTable != NULL ) ? GetEnvValue(index) :  mGlobalStackElements[GloablVarStackIndex(index)]))

#define  ResoveStackIndex(index)		( index < 0 ? &mCurXScriptState->mStackElements[(mCurXScriptState->mFrameIndex + index)] : &mGlobalStackElements[GloablVarStackIndex(index)])

#define		stringRawValue(o)				((const char*)&((o)->stringValue->value))  

#define		stringRealLen(o)				strlen((const char*)&((o)->stringValue->value))  

#define		stringRawLen(o)					((o)->stringValue->len)


#define		CheckStrBuffer(size)		\
		if (mStrBufferSize < size)	\
		{	\
			delete []mStrBuffer;	\
			mStrBufferSize = size;	\
			mStrBuffer = new char[mStrBufferSize];	\
		}	


#define CheckParam(F, N, PN, T)	\
	Value PN = vm->getParamValue(N);	\
	if (PN.type != T)	\
	{	\
		vm->ExecError("invalid #%d param for %s, should %s, but %s", N, #F, getTypeName(T), getTypeName(PN.type));	\
	}

#define CheckUserTypeParam(F, N, PN, T, TN)	\
	T* PN = (T*)(vm->getParamAsObj(N, TN));	\
	if (PN == NULL)	\
	{	\
		vm->ExecError("invalid %d param for %s, should be user type %s", N, #F, TN);	\
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


struct UserData
{
	GCCommonHeader;
	TableValue*		mMetaTable;
	int				mSize;
	char			value[1];
};

struct Value
{
	int type;
	union
	{
		XInt			iIntValue;
		XFloat			fFloatValue;

		int				iFunctionValue;
		int				iInstrIndex;
		int				iRegIndex;
		XString*		stringValue;
		Function*		func;
		TableValue*		tableData;
		void*			lightUserData;
		XScriptState*	threadData;
		UserData*		userData;
	};

	Value()
	{
		type = OP_TYPE_NIL;
		lightUserData = NULL;
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
	XInt		mArraySize;
	Value*		mArrayData;
	//ValuePair*	nextPair;


	int			mNodeCapacity;
	TableNode*	mNodeData;
	int			lastFreePos;

	TableValue*	mMetaTable;
	TableValue()
		: mArraySize(0)
		, mArrayData(NULL)
		//	, nextPair(NULL)
		, mNodeData(NULL)
		, mNodeCapacity(0)
		, lastFreePos(0)
		, mMetaTable(NULL)
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
	HOST_FUNC	pfnAddr;

	HostFunction(const std::string&	 _funcName, HOST_FUNC _pfn)
		: funcName(_funcName)
		, pfnAddr(_pfn)
	{

	}
};

class CFunction
{
public:
	Value*				mUpVal;
	int					mNumUpVal;
	HOST_FUNC			pfnAddr;

};

class LuaFunction
{
public:
	FuncState*					proto;
	UpValue**					mUpVals;
	int							mNumUpVals;
};

typedef union FuncUnion
{
	CFunction		cFunc;
	LuaFunction		luaFunc;
}FuncUnion;


class Function
{
public:
	GCCommonHeader;

	bool				isCFunc;
	FuncUnion			funcUnion;
	Function()
	{
		isCFunc = false;
	}
};


struct RuntimeOperator
{
	int type;
	union
	{
		XInt		iIntValue;
		XFloat		fFloatValue;
		int			iStackIndex;
		int			iFunctionValue;
		int			iInstrIndex;
		XString*	stringValue;
		int			iRegIndex;
	};
	int				tableIndexType;
	union
	{
		XInt		iIntTableValue;
		XFloat		fFloatTableValue;
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

class CallInfo
{
public:
	FuncState*		mCurFunctionState;
	Function*		mCurFunction;
	int				mInstrIndex;
	int				mCurLine;
	int				mFrameIndex;
};

#endif // VMDefs_h__