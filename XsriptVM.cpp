#include "windows.h"
#include "XSriptVM.h"
#include "Parser.h"
#include "MidCode.h"
#include "SymbolTable.h"
#include <math.h>
#include "libs/xmathlib.h"
#include "libs/xbaselib.h"
#include "libs/xstringlib.h"
#include "libs/xdebuglib.h"
#include "libs/xiolib.h"
#include "libs/xxmllib.h"
#include "libs/xoslib.h"
#include "SourceFile.h"
#include <setjmp.h>


XScriptVM gScriptVM;

XScriptVM::XScriptVM()
{

}

int		XScriptVM::GetUnusedVarIndex()
{
	for (int i = 0; i < MAX_GLOBAL_DATASIZE; i++)
	{
		if (mGloablUseMap[i] == false)
		{
			mGloablUseMap[i] = true;
			return i;
		}
	}

	return -1;
}


void	XScriptVM::ReleaseUsedVarIndex(int index)
{
	mGloablUseMap[index] = true;
}

int		XScriptVM::GetGloabVarIndex(VariantST* var)
{
	if (var->iScope >= 0)
		return var->stackIndex;

	if (mGloablVarMap.find(var->varName) != mGloablVarMap.end())
	{
		int globalIndex = mGloablVarMap[var->varName];
		int nameIndex = AddString(var->varName);
		return MakeGloablIndex(globalIndex, nameIndex);
	}

	return -1;
}


void  XScriptVM::loadHostLibs()
{
	init_math_lib();
	init_base_lib();
	init_string_lib();
	init_debug_lib();
	init_io_lib();
	init_xml_lib();
	init_os_lib();
}

int		XScriptVM::RegisterGlobalValue(const std::string& name)
{
	if (mGloablVarMap.find(name) == mGloablVarMap.end())
	{
		mGloablVarMap[name] = GetUnusedVarIndex();
	}

	return mGloablVarMap[name];
}

FuncState*	XScriptVM::CompileString(const std::string& code)
{
	CParser parser;
	CMidCode midCode;
	CSymbolTable symbolTable;
	symbolTable.AddFunction("CompileString", 0, -1);

	CSourceFile sourefile;
	sourefile.LoadFromString(code.c_str());
	bool succ = parser.ParseFile(&sourefile, this, &midCode, &symbolTable);
	if (!succ)
	{
		return NULL;
	}

	RegisterGlobalValues(symbolTable);

	map<int, FuncState*> FuncMap;

	ConvertMidCodeToInstr(symbolTable, midCode, "", FuncMap);

	FuncState* mainFunc = FuncMap.begin()->second;
	mainFunc->marked = MS_White;
	return mainFunc;
}

#define  OUTPUT_MIDCODE
bool  XScriptVM::doFile(const std::string& fileName)
{
	CParser parser;

	CMidCode midCode;
	CSymbolTable symbolTable;
	symbolTable.AddFunction("_main", 0, -1);

	std::string	moudleName = fileName;

	CSourceFile sourefile;

	if (!sourefile.LoadSourceFile(fileName.c_str()))
	{
		return false;
	}

	bool succ = parser.ParseFile(&sourefile, this, &midCode, &symbolTable);
	if (!succ)
	{
		return false;
	}

#ifdef OUTPUT_MIDCODE
	std::string midCodeFileName;
	int lastDot = fileName.find_last_of('.');
	if (lastDot > 0)
	{
		midCodeFileName = moudleName + ".code";
	}
	if (!midCodeFileName.empty())
	{
		parser.OutPutCode(&midCode, &symbolTable, (char*)midCodeFileName.c_str());
	}
#endif 

	RegisterGlobalValues(symbolTable);

	map<int, FuncState*> FuncMap;

	ConvertMidCodeToInstr(symbolTable, midCode, fileName, FuncMap);

	FuncState* mainFunc = FuncMap.begin()->second;
	Value fValue = ConstructValue(mainFunc);
	setTableValue(mModuleTable, ConstructValue(moudleName.c_str()), fValue);
	std::string errorDesc;
	if (ProtectCallFunction(fValue.func, 0, errorDesc) != 0)
	{
		printf(errorDesc.c_str());
		system("pause");
	}


	map<int, FuncState*>::iterator it = FuncMap.begin();
	for (; it != FuncMap.end(); it++)
	{
		it->second->marked = MS_White;
	}

	return true;
}


std::string	XScriptVM::GetOperatorName(RuntimeOperator* op, Value* value)
{
	std::string desc;
	if (op->varNameIndex >= 0)
	{
		desc = GetString(op->varNameIndex);
		desc += "(a " + std::string(getTypeName(value->type)) + " value)";
	}
	else
	{
		desc = getTypeName(value->type);
	}

	return desc;
}

std::string	XScriptVM::GetOperatorName(int opIndex)
{
	Value value = resolveOpValue(opIndex);
	std::string desc;
	if (mCurInstr->mOpList[opIndex].varNameIndex >= 0)
	{
		desc = GetString(mCurInstr->mOpList[opIndex].varNameIndex);
		desc += "(a " + std::string(getTypeName(value.type)) + " value)";
	}
	else
	{

		desc = getTypeName(value.type);
	}

	return desc;
}


std::string XScriptVM::stackBackTrace()
{
	std::string stackTraceBack;
	stackTraceBack = "stack traceback:\n";
	int callIndex = mCurXScriptState->mCurCallIndex;
	while (callIndex >= 0)
	{
		if (mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState != NULL)
		{
			stackTraceBack += "\t" + mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState->sourceFileName + ":line(";
			if (callIndex == mCurXScriptState->mCurCallIndex)
				stackTraceBack += ConvertToString(mCurInstr->lineIndex);
			else
				stackTraceBack += ConvertToString(mCurXScriptState->mCallInfoBase[callIndex + 1].mCurLine);

			stackTraceBack += "): in function \"" + mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState->funcName + "\"\n";
		}
		callIndex--;
	}
	stackTraceBack += "\t[C]:?\n";

	return stackTraceBack;
}

void XScriptVM::ConvertMidCodeToInstr(CSymbolTable &symbolTable, CMidCode &midCode, const std::string& fileName, std::map<int, FuncState*>& funcMap)
{
	for (int iFuncIndex = 0; iFuncIndex < (int)symbolTable.mFuncTable.size(); iFuncIndex++)
	{
		FuncState* funcState = CreateFunctionState();
		funcMap[symbolTable.mFuncTable[iFuncIndex].iIndex] = funcState;
		funcState->sourceFileName = fileName;
	}

	FuncState* mainFunc = NULL;

	for (int iFuncIndex = 0; iFuncIndex < (int)symbolTable.mFuncTable.size(); iFuncIndex++)
	{
		FunctionST funcST = symbolTable.mFuncTable[iFuncIndex];
		FuncState* funcState = funcMap[symbolTable.mFuncTable[iFuncIndex].iIndex];

		int functionIndex = funcST.iIndex;
		funcState->funcName = funcST.funcName;
		funcState->localDataSize = funcST.localDataSize;
		funcState->localParamNum = funcST.iParamSize;
		funcState->stackFrameSize = funcST.localDataSize + funcST.iParamSize;
		funcState->hasVarArgs = funcST.hasVarArgs;
		symbolTable.GetFunctionVars(functionIndex, funcState->m_localVarVec);
		if (mainFunc == NULL)
			mainFunc = funcState;

		for (int upValueIndex = 0; upValueIndex < (int)funcST.upValueVec.size(); upValueIndex++)
		{
			UpValueST up = funcST.upValueVec[upValueIndex];
			if (up.type == VLOCAL)
			{
				VariantST* var = symbolTable.GetVarByIndex(up.index);
				up.index = var->stackIndex;
			}

			funcState->m_upValueVec.push_back(up);
		}


		for (int subFuncIndex = 0; subFuncIndex < (int)funcST.subIndexVec.size(); subFuncIndex++)
		{
			funcState->m_subFuncVec.push_back(funcMap[funcST.subIndexVec[subFuncIndex]]);
		}

		if (funcST.parentIndex >= 0)
		{
			funcState->m_parentFunc = funcMap[funcST.parentIndex];
		}

		funcState->mInstrStream.numInstr = (int)midCode.mCodeList[funcST.iIndex].size();
		if (funcState->mInstrStream.numInstr > 0)
			funcState->mInstrStream.instrs = new Instr[funcState->mInstrStream.numInstr];


		for (int i = 0; i < (int)midCode.mCodeList[funcST.iIndex].size(); i++)
		{
			Code code = midCode.mCodeList[funcST.iIndex][i].code;
			funcState->mInstrStream.instrs[i].opType = code.iCodeOpr;
			funcState->mInstrStream.instrs[i].numOpCount = (int)code.oprList.size();
			if (code.oprList.size() > 0)
				funcState->mInstrStream.instrs[i].mOpList = new RuntimeOperator[code.oprList.size()];

			int lineIndex = midCode.mCodeList[funcST.iIndex][i].lineIndex;
			funcState->mInstrStream.instrs[i].lineIndex = lineIndex;

			for (int op = 0; op < (int)code.oprList.size(); op++)
			{
				Operand operand = code.oprList[op];
				RuntimeOperator* value = &funcState->mInstrStream.instrs[i].mOpList[op];
				value->varNameIndex = -1;
				//value->type = operand.operandType;
				switch (operand.operandType)
				{
				case OP_TYPE_INT:
				{
					value->type = ROT_Int;
					value->iIntValue = operand.iIntValue;
					break;
				}
				case OP_TYPE_FLOAT:
				{
					value->type = ROT_Float;
					value->fFloatValue = operand.fFloatValue;
					break;
				}
				case PST_String_Index:
				{
					value->type = ROT_String;
					value->stringValue = NewXString(symbolTable.mStringTable[operand.iStringIndex].str);
					GC_SetFixed(value->stringValue);
					break;
				}
				case PST_Var_Index:
				{
					if (operand.iSymbolIndex & UPVALMASK)
					{
						int upvalIndex = operand.iSymbolIndex - UPVALMASK;
						value->type = ROT_UpVal_Index;
						value->iStackIndex = upvalIndex;
						value->varNameIndex = AddString(CParser::FindVarNameBySymbolIndex(symbolTable, operand.iSymbolIndex, functionIndex));
					}
					else
					{
						value->type = ROT_Stack_Index;
						VariantST* var = symbolTable.GetVarByIndex(operand.iSymbolIndex);
						value->iStackIndex = GetGloabVarIndex(var);
						value->varNameIndex = AddString(var->varName);
					}

					break;
				}
				case PST_JumpTarget_Index:
				{
					value->type = ROT_Instr_Index;

					int jumpInstrIndex = -1;
					for (int j = 0; j < (int)midCode.mJumpList[functionIndex].size(); j++)
					{
						if (midCode.mJumpList[functionIndex][j].jumpIndex == operand.iJumppIndex)
						{
							jumpInstrIndex = midCode.mJumpList[functionIndex][j].codeIndex;
							break;
						}
					}
					value->iInstrIndex = jumpInstrIndex;
					break;
				}

				case PST_FuncIndex:
				{
					value->type = ROT_FuncValue;
					value->iFunctionValue = operand.iFunData;
					break;
				}
				case PST_Table:
				{
					if (operand.iSymbolIndex != -1)
					{
						std::string varName;
						if (operand.iSymbolIndex & UPVALMASK)
						{
							int upvalIndex = operand.iSymbolIndex - UPVALMASK;
							value->type = ROT_UpValue_Table;
							value->iStackIndex = upvalIndex;
							varName = CParser::FindVarNameBySymbolIndex(symbolTable, operand.iSymbolIndex, functionIndex);
						}
						else
						{
							VariantST* var = symbolTable.GetVarByIndex(operand.iSymbolIndex);
							value->iStackIndex = GetGloabVarIndex(var);
							varName = var->varName;

							value->type = ROT_Table;
						}

						char temp[64] = { 0 };
						std::string subName;
						value->tableIndexType = operand.tableIndexType;
						switch (operand.tableIndexType)
						{
						case ROT_Float:
							value->fFloatTableValue = operand.fFloatTableValue;
							snprintf(temp, 64, XFloatConFmt, operand.fFloatTableValue);
							subName = temp;
							break;
						case ROT_Int:
							value->iIntTableValue = operand.iIntTableValue;
							snprintf(temp, 64, XIntConFmt, operand.iIntTableValue);
							subName = temp;
							break;
						case ROT_String:
							value->strTableValue = NewXString(symbolTable.mStringTable[(int)operand.iIntTableValue].str);
							GC_SetFixed(value->strTableValue);
							break;
						case ROT_Stack_Index:
						{
							if (operand.iIntTableValue & UPVALMASK)
							{
								int upvalIndex = (int)operand.iIntTableValue - UPVALMASK;
								value->tableIndexType = ROT_UpVal_Index;
								value->iIntTableValue = upvalIndex;

								subName = "[" + std::string(CParser::FindVarNameBySymbolIndex(symbolTable, (int)operand.iIntTableValue, functionIndex)) + "]";
							}
							else
							{
								VariantST* var = symbolTable.GetVarByIndex((int)operand.iIntTableValue);
								value->iIntTableValue = GetGloabVarIndex(var);
								subName = "[" + std::string(var->varName) + "]";
							}
						}

						break;
						case ROT_Reg:
							value->iIntTableValue = operand.iIntTableValue;
							break;
						default:
							break;
						}

						varName += subName;
						value->varNameIndex = AddString(varName);
					}
					break;
				}
				case PST_Reg:
				{
					value->type = ROT_Reg;
					value->iRegIndex = operand.iRegIndex;
					break;
				}
				}
			}

		}

	}
}

void XScriptVM::RegisterGlobalValues(CSymbolTable &symbolTable)
{
	for (int iVarIndex = 0; iVarIndex < (int)symbolTable.mVarTable.size(); iVarIndex++)
	{
		VariantST& var = symbolTable.mVarTable[iVarIndex];
		if (var.iScope == -1)
		{
			RegisterGlobalValue(var.varName);
		}
	}
}

bool  XScriptVM::init()
{

	mStringHashSize = 32;
	mStringHashTable = new XString*[mStringHashSize];
	memset(mStringHashTable, 0, sizeof(XString*) * mStringHashSize);

	mStrBufferSize = 128;
	mStrBuffer = new char[mStrBufferSize];

	mStringHashUsedSize = 0;
	mEnvTable = NULL;
	mAllowHook = true;
	mHookMask = 0;
	mLongJmp = NULL;
	mNumHostFuncParam = 0;
	mIsInCallScriptFunc = false;
	mNumScriptFuncParams = 0;

	mHostFunctions.clear();

	for (int i = 0; i < MAX_GLOBAL_DATASIZE; i++)
		mGloablUseMap[i] = false;

	mCurXScriptState = &mMainXScriptState;
	resetScriptState(mCurXScriptState);

	mGlobalStackElements = new Value[MAX_GLOBAL_DATASIZE];
	memset(mGlobalStackElements, 0, sizeof(Value) * MAX_GLOBAL_DATASIZE);
	for (int i = 0; i < MAX_GLOBAL_DATASIZE; i++)
		mGlobalStackElements[i].type = OP_TYPE_NIL;

	mCurInstr = NULL;
	mRootCG = NULL;
	RegisterGlobalValue("nil");
	mNilValue = GetGlobalValue("nil");
	mNilValue->type = OP_TYPE_NIL;

	//RegisterGlobalValue("metatable");
	mMetaTable = CreateTable();
	mModuleTable = CreateTable();
	loadHostLibs();
	return true;
}


void  XScriptVM::resetScriptState(XScriptState* state)
{
	state->mCurrentCFunction = NULL;
	state->mStartFunction = NULL;
	state->mNextRefUpVals = NULL;
	state->mCurFunction = NULL;
	state->mCurFunctionState = NULL;
	state->mCurCallIndex = -1;
	state->mCallInfoBase = new CallInfo[MAX_LUA_CALL_STACK_DEPTH];
	state->mStatus = CS_Normal;
	state->mTopIndex = 0;
	state->mFrameIndex = 0;
	state->mStackSize = MIN_STACK_SIZE;
	state->mTopIndex = 0;
	state->mFrameIndex = 0;
	state->mCCallIndex = 0;
	state->mInstrIndex = 0;
	state->mStackElements = new Value[mCurXScriptState->mStackSize];
	memset(&state->mStackElements[0], 0, sizeof(Value) * state->mStackSize);
	for (int i = 0; i < mCurXScriptState->mStackSize; i++)
		state->mStackElements[i].type = OP_TYPE_NIL;

}


void  XScriptVM::ExecuteFunction(int numLuaCallCount)
{
	int lineIndex = -1;
	while (true)
	{
		int lastInstrIndex = mCurXScriptState->mInstrIndex;
		mCurInstr = &mCurXScriptState->mCurFunctionState->mInstrStream.instrs[lastInstrIndex];
		int lastCallIndex = mCurXScriptState->mCurCallIndex;
		// 		if (lineIndex == -1 && mAllowHook && (mHookMask & MASK_HOOKCALL))
		// 		{
		// 			CallHookFunction(HE_HookCall, lineIndex);
		// 		}

		if (mCurInstr->lineIndex != lineIndex && mCurInstr->lineIndex >= 0)
		{
			lineIndex = mCurInstr->lineIndex;

			if (mAllowHook && (mHookMask & MASK_HOOKLINE))
			{
				CallHookFunction(HE_HookLine, lineIndex);
			}
		}

		switch (mCurInstr->opType)
		{
		case INSTR_TYPE:
		{
			SetOpValue(0, ConstructValue(CreateTable()));
			break;
		}
		case INSTR_LOADNIL:
		{
			SetOpValue(0, *mNilValue);
		}
		break;
		case INSTR_MOV:
		{
			ResolveOpPointer(1, srcValue);
			SetOpValue(0, srcValue);
			break;
		}
		case INSTR_ADD_TO:
		{
			EXEC_INSTR_MATH_TO(+, MMT_Add);
			//ExecInstr_AddTo();
		}
		break;
		case INSTR_ADD:
		{
			EXEC_INSTR_MATH(+, MMT_Add);
			//ExecInstr_Add();
			break;
		}
		case INSTR_SUB_TO:
		{
			EXEC_INSTR_MATH_TO(-, MMT_Sub);
			//ExecInstr_SubTo();
		}
		break;
		case INSTR_SUB:
		{
			EXEC_INSTR_MATH(-, MMT_Sub);
			//ExecInstr_Sub();
			break;
		}
		case INSTR_MUL_TO:
		{
			EXEC_INSTR_MATH_TO(*, MMT_Mul);
			//ExecInstr_MULTO();
		}
		break;
		case INSTR_MUL:
		{
			EXEC_INSTR_MATH(*, MMT_Mul);
			//ExecInstr_Mul();
			break;
		}
		case INSTR_DIV_TO:
		{
			EXEC_INSTR_MATH_TO(/ , MMT_Div);
			//ExecInstr_DIVTO();
		}
		break;
		case INSTR_DIV:
		{
			EXEC_INSTR_MATH(/ , MMT_Div);
			//ExecInstr_Div();
			break;
		}
		case INSTR_MOD_TO:
		{
			EXEC_INSTR_MATH_INTOP_TO(XFMod, %, MMT_Mod);
			//ExecInstr_MODTO();
		}
		break;
		case INSTR_MOD:
		{
			EXEC_INSTR_MATH_INTOP(XFMod, %, MMT_Mod);
			//ExecInstr_Mod();
			break;
		}
		case INSTR_EXP_TO:
		{
			EXEC_INSTR_MATH_OP_TO(pow, MMT_Pow);
			//ExecInstr_ExpTo();
		}
		break;
		case INSTR_EXP:
		{
			EXEC_INSTR_MATH_OP(pow, MMT_Pow);
			//ExecInstr_Pow();
			break;
		}
		case INSTR_NEG:
		{
			ExecInstr_Neg();
			break;
		}
		case INSTR_INC:
		{
			EXEC_INSTR_MATH_INC(+);
			//ExecInstr_Inc();
			break;
		}
		case INSTR_DEC:
		{
			EXEC_INSTR_MATH_INC(-);
			//ExecInstr_Dec();
			break;
		}
		case INSTR_AND_TO:
		{
			EXEC_INSTR_LOGIC_OP_TO(&);
			//ExecInstr_AndTO();
		}
		break;
		case INSTR_AND:
		{
			EXEC_INSTR_LOGIC_OP(&);
			//ExecInstr_And();
			break;
		}
		case INSTR_OR_TO:
		{
			EXEC_INSTR_LOGIC_OP_TO(| );
			//ExecInstr_ORTO();
		}
		break;
		case INSTR_OR:
		{
			EXEC_INSTR_LOGIC_OP(| );
			//ExecInstr_Or();
			break;
		}
		case INSTR_XOR_TO:
		{
			EXEC_INSTR_LOGIC_OP_TO(^);
			//ExecInstr_XORTO();
		}
		break;
		case INSTR_XOR:
		{
			EXEC_INSTR_LOGIC_OP(^);
			//ExecInstr_Xor();
			break;
		}
		case INSTR_SHL_TO:
		{
			EXEC_INSTR_BIT_OP_TO(<< );
			//ExecInstr_SHLTO();
		}
		break;
		case INSTR_SHL:
		{
			EXEC_INSTR_BIT_OP(<< );
			//ExecInstr_SHL();
			break;
		}
		case INSTR_SHR_TO:
		{
			EXEC_INSTR_BIT_OP_TO(>> );
			//ExecInstr_SHRTO();
		}
		break;
		case INSTR_SHR:
		{
			EXEC_INSTR_BIT_OP(>> );
			//ExecInstr_SHR();
			break;
		}
		case INSTR_JMP:
		{
			ResolveOpPointer(0, destValue);
			mCurXScriptState->mInstrIndex = destValue.iInstrIndex;
			break;
		}
		case INSTR_JE:
		{
			EXEC_INSTR_JE(==);

			//ExecInstr_JE();
			break;
		}
		case INSTR_JNE:
		{
			EXEC_INSTR_JE(!=);
			//ExecInstr_JNE();
			break;
		}
		case INSTR_JG:
		{
			EXEC_INSTR_J(>, NMT_Great);
			//ExecInstr_JG();
			break;
		}
		case INSTR_JGE:
		{
			EXEC_INSTR_J(>=, NMT_GreatEqual);
			//ExecInstr_JGE();
			break;
		}
		case INSTR_JL:
		{
			EXEC_INSTR_J(<, NMT_Less);
			break;
		}
		case INSTR_JLE:
		{
			EXEC_INSTR_J(<=, NMT_LessEqual);
			//ExecInstr_JLE();
			break;
		}
		case INSTR_PUSH:
		{
			//ResolveOpPointer(0, firstValue);
			mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex] = resolveOpValue(0);
			mCurXScriptState->mTopIndex++;
			break;
		}
		case INSTR_CONCAT_TO:
		{
			ExecInstr_Concat_To();
		}
		break;
		case INSTR_CONCAT:
		{
			ExecInstr_Concat();
			break;
		}
		case INSTR_POP:
		{
			//ResolveOpPointer(0, firstValue);
			mCurXScriptState->mTopIndex--;
			SetOpValue(0, mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex]);
			break;
		}
		case INSTR_CALL:
		{
			ResolveOpPointer(0, firstValue);
			ResolveOpPointer(1, secondValue);
			int numParam = (int)secondValue.iIntValue;

			Function* func = NULL;
			if (firstValue.type != OP_TYPE_FUNC)
			{
				Value callTagMethod = GetMetaMethod(&firstValue, MMT_Call);
				if (IsValueFunction(&callTagMethod))
					func = callTagMethod.func;
				else
					ExecError("attempt to call function on %s", GetOperatorName(0).c_str());
			}
			else
			{
				func = firstValue.func;
				
			}
			
			CallFunctionInLua(func, numParam);

			if (!func->isCFunc)
			{
				numLuaCallCount++;
			}

			if (mCurXScriptState->mStatus == CS_Suspend)
			{
				mCurXScriptState->mInstrIndex++;
				return;
			}
		}
		break;
		case INSTR_RET:
		{
			ExecInstr_RET();
			numLuaCallCount--;
			if (numLuaCallCount < 0)
			{
				return;
			}
			else
			{
				mCurXScriptState->mInstrIndex++;
			}
			break;
		}
		case INSTR_LOGIC_NOT:
		{
			ExecInstr_Logic_Not();
		}
		break;
		case INSTR_TEST_E:
		{
			EXEC_INSTR_TEST_E(== );
			//ExecInstr_Test_E();
		}
		break;
		case INSTR_TEST_NE:
		{
			EXEC_INSTR_TEST_E(!= );
			//ExecInstr_Test_NE();
		}
		break;
		case INSTR_TEST_G:
		{
			EXEC_INSTR_TEST(>, NMT_Great);
			//ExecInstr_Test_G();
		}
		break;
		case INSTR_TEST_L:
		{
			EXEC_INSTR_TEST(<, NMT_Less);
			//ExecInstr_Test_L();
		}
		break;
		case INSTR_TEST_GE:
		{
			EXEC_INSTR_TEST(>=, NMT_GreatEqual);
		}
		break;
		case INSTR_TEST_LE:
		{
			EXEC_INSTR_TEST(<=, NMT_LessEqual);
		}
		break;
		case INSTR_FUNC:
		{
			ExecInstr_Func();
		}
		break;
		case INSTR_LOGIC_AND:
		{
			ExecInstr_Logic_And();
		}
		break;
		case INSTR_LOGIC_OR:
		{
			ExecInstr_Logic_Or();
		}
		break;
		}

		if (lastInstrIndex == mCurXScriptState->mInstrIndex && mCurXScriptState->mCurCallIndex == lastCallIndex)
			mCurXScriptState->mInstrIndex++;
	}
}


void XScriptVM::ExecInstr_Func()
{
	//ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);

	if (firstValue.type == OP_TYPE_INT)
	{
		FuncState* proto = mCurXScriptState->mCurFunctionState->m_subFuncVec[(int)firstValue.iIntValue];
		Function* func = CreateFunction();
		func->isCFunc = false;
		func->funcUnion.luaFunc.proto = proto;
		func->funcUnion.luaFunc.mNumUpVals = proto->m_upValueVec.size();
		if (func->funcUnion.luaFunc.mNumUpVals > 0)
		{
			func->funcUnion.luaFunc.mUpVals = new UpValue*[func->funcUnion.luaFunc.mNumUpVals];

			for (int i = 0; i < (int)proto->m_upValueVec.size(); i++)
			{
				if (proto->m_upValueVec[i].type == VUPVALUE)
				{
					func->funcUnion.luaFunc.mUpVals[i] = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[proto->m_upValueVec[i].index];
				}
				else
				{
					Value* pReferValue = &mCurXScriptState->mStackElements[mCurXScriptState->mFrameIndex + proto->m_upValueVec[i].index];
					bool hasFound = false;
					UpValue* nextUpValue = mCurXScriptState->mNextRefUpVals;
					while (nextUpValue != NULL)
					{
						if (nextUpValue->pValue == pReferValue)
						{
							func->funcUnion.luaFunc.mUpVals[i] = nextUpValue;
							hasFound = true;
							break;
						}

						nextUpValue = nextUpValue->nextValue;
					}

					if (!hasFound)
					{
						UpValue* newUpVal = CreateUpVal();
						newUpVal->pValue = pReferValue;
						newUpVal->nextValue = mCurXScriptState->mNextRefUpVals;
						mCurXScriptState->mNextRefUpVals = newUpVal;
						func->funcUnion.luaFunc.mUpVals[i] = newUpVal;
					}
				}
			}
		}

		Value destValue;
		destValue.type = OP_TYPE_FUNC;
		destValue.func = func;
		SetOpValue(0, destValue);

	}
	else
	{
		ExecError("Error Operation");
	}
}

void XScriptVM::ExecInstr_Concat_To()
{
	Value destValue;
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);

	if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{

			int strLength = stringRawLen(&firstValue) + stringRawLen(&secondValue);

			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
			memcpy(mStrBuffer + stringRawLen(&firstValue), stringRawValue(&secondValue), stringRawLen(&secondValue));

			destValue.type = OP_TYPE_STRING;
			destValue.stringValue = NewXString(mStrBuffer, strLength);
		}
		else if (IsPNumberType(secondValue))
		{
			char numberBuffer[64] = { 0 };

			if (IsValueInt(&secondValue))
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XIntConFmt, secondValue.iIntValue);
			}
			else
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, XFloatConFmt, secondValue.fFloatValue);
			}

			int strLength = stringRawLen(&firstValue) + strlen(numberBuffer);
			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
			memcpy(mStrBuffer + stringRawLen(&firstValue), numberBuffer, strlen(numberBuffer));

			destValue.type = OP_TYPE_STRING;
			destValue.stringValue = NewXString(mStrBuffer, strLength);

		}
		else
		{
			EXEC_OP_ERROR($, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR($, 1, 2);
	}

	SetOpValue(0, destValue);
}

void XScriptVM::CallFunction(Function* funcValue, int numParam)
{
	if (!funcValue->isCFunc)
	{
		FuncState* func = funcValue->funcUnion.luaFunc.proto;
		if (func->hasVarArgs)
		{
			int numVarArgs = numParam + 1 - func->localParamNum;

			if (numVarArgs < 0)
			{
				ExecError("call function %s with error params, expect %d params, but %d params", func->funcName.c_str(), func->localParamNum, numParam);
			}
			else
			{
				TABLE t = newTable();
				setTableValue(t, "n", (XInt)numVarArgs);
				for (XInt i = numVarArgs; i > 0; i--)
				{
					Value value = pop();
					setTableValue(t, ConstructValue(i - 1), value);
				}
				push(ConstructValue(t));
				ExecLuaFunction(funcValue);
			}

		}
		else
		{
			if (func->localParamNum != numParam)
			{
				ExecError("call function %s with error params, expect %d params, but %d params", func->funcName.c_str(), func->localParamNum, numParam);
			}
			else
			{
				ExecLuaFunction(funcValue);
			}
		}
	}
	else
	{
		CallHostFunc(funcValue, funcValue->funcUnion.cFunc.pfnAddr, numParam);
	}
}

int	XScriptVM::ProtectResume(std::string & errorDesc)
{
	int ccallIndex = mCurXScriptState->mCCallIndex;
	int frameIndex = mCurXScriptState->mFrameIndex;
	int lastCallInfoIndex = mCurXScriptState->mCurCallIndex;
	int lastInstrIndex = -1;
	if (mCurXScriptState->mCurFunctionState != NULL)
		lastInstrIndex = mCurXScriptState->mInstrIndex;

	FuncState* lastFuncState = mCurXScriptState->mCurFunctionState;
	Function* lastFunc = mCurXScriptState->mCurFunction;
	int lastTopIndex = mCurXScriptState->mTopIndex;

	XScript_LongJmp jmp;
	jmp.previous = mLongJmp;
	jmp.errorCode = 0;
	jmp.errorFunc = -1;
	mLongJmp = &jmp;

	if (setjmp(jmp.j) == 0)
	{
		ExecuteFunction(mCurXScriptState->mCurCallIndex);
	}

	mLongJmp = jmp.previous;

	if (jmp.errorCode != 0)
	{
		mCurXScriptState->mCurCallIndex = lastCallInfoIndex;
		mCurXScriptState->mCurFunction = lastFunc;
		mCurXScriptState->mCurFunctionState = lastFuncState;
		if (mCurXScriptState->mCurFunctionState != NULL)
			mCurXScriptState->mInstrIndex = lastInstrIndex;
		mCurXScriptState->mFrameIndex = frameIndex;
		mCurXScriptState->mTopIndex = lastTopIndex;
		mCurXScriptState->mCCallIndex = ccallIndex;
		RemoveStackUpVals(lastTopIndex);

		errorDesc = jmp.mErrorDesc;
	}

	return jmp.errorCode;
}

int	XScriptVM::ProtectCallFunction(Function* firstValue, int numParam, std::string& errorDesc, int errorFunc)
{
	int ccallIndex = mCurXScriptState->mCCallIndex;
	int frameIndex = mCurXScriptState->mFrameIndex;
	int lastCallInfoIndex = mCurXScriptState->mCurCallIndex;
	int lastInstrIndex = -1;
	if (mCurXScriptState->mCurFunctionState != NULL)
		lastInstrIndex = mCurXScriptState->mInstrIndex;

	FuncState* lastFuncState = mCurXScriptState->mCurFunctionState;
	Function* lastFunc = mCurXScriptState->mCurFunction;
	int lastTopIndex = mCurXScriptState->mTopIndex - numParam;

	XScript_LongJmp jmp;
	jmp.previous = mLongJmp;
	jmp.errorCode = 0;
	jmp.errorFunc = errorFunc;
	mLongJmp = &jmp;

	if (setjmp(jmp.j) == 0)
	{
		CallFunction(firstValue, numParam);
	}

	mLongJmp = jmp.previous;

	if (jmp.errorCode != 0)
	{
		mCurXScriptState->mCurCallIndex = lastCallInfoIndex;
		mCurXScriptState->mCurFunction = lastFunc;
		mCurXScriptState->mCurFunctionState = lastFuncState;
		if (mCurXScriptState->mCurFunctionState != NULL)
			mCurXScriptState->mInstrIndex = lastInstrIndex;
		mCurXScriptState->mFrameIndex = frameIndex;
		mCurXScriptState->mTopIndex = lastTopIndex;
		mCurXScriptState->mCCallIndex = ccallIndex;
		errorDesc = jmp.mErrorDesc;

		RemoveStackUpVals(lastTopIndex);
	}

	return jmp.errorCode;
}

void XScriptVM::RemoveStackUpVals(int topIndex)
{
	Value* top = &mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex];
	//清除该堆栈上的upvalues
	UpValue* nextRefUpVals = mCurXScriptState->mNextRefUpVals;
	UpValue* lastUpVals = NULL;
	while (nextRefUpVals != NULL)
	{
		if (nextRefUpVals->pValue >= top)
		{
			CopyValue(&nextRefUpVals->value, *nextRefUpVals->pValue);
			nextRefUpVals->pValue = &nextRefUpVals->value;

			UpValue* oldValue = nextRefUpVals;
			if (lastUpVals != NULL)
			{
				lastUpVals->nextValue = nextRefUpVals->nextValue;
			}
			else
			{
				mCurXScriptState->mNextRefUpVals = nextRefUpVals->nextValue;
			}

			nextRefUpVals = nextRefUpVals->nextValue;
			oldValue->nextValue = NULL;

		}
		else
		{
			lastUpVals = nextRefUpVals;
			nextRefUpVals = nextRefUpVals->nextValue;
		}
	}
}

void XScriptVM::CallFunctionInLua(Function* funcValue, int numParam)
{
	if (!funcValue->isCFunc)
	{
		FuncState* func = funcValue->funcUnion.luaFunc.proto;
		if (func->hasVarArgs)
		{
			int numVarArgs = numParam + 1 - func->localParamNum;

			if (numVarArgs < 0)
			{
				ExecError("call function %s with error params, expect %d params, but %d params", func->funcName.c_str(), func->localParamNum, numParam);
			}
			else
			{
				TABLE t = newTable();
				setTableValue(t, "n", (XInt)numVarArgs);
				for (XInt i = numVarArgs; i > 0; i--)
				{
					Value value = pop();
					setTableValue(t, ConstructValue(i - 1), value);
				}
				push(ConstructValue(t));
				CallLuaFunction(funcValue);
			}

		}
		else
		{
			if (func->localParamNum != numParam)
			{
				ExecError("call function %s with error params, expect %d params, but %d params", func->funcName.c_str(), func->localParamNum, numParam);
			}
			else
			{
				CallLuaFunction(funcValue);
			}
		}
	}
	else
	{
		CallHostFunc(funcValue, funcValue->funcUnion.cFunc.pfnAddr, numParam);
	}
}

void  XScriptVM::ExecError(const char* errorStr, ...)
{
	char  buffer[512] = { 0 };
	va_list  args;
	va_start(args, errorStr);
	int len = vsnprintf(buffer, 512, errorStr, args);
	va_end(args);
	buffer[len] = '\0';
	char buffer2[512 + 64] = { 0 };
	snprintf(buffer2, 576, "Run Error: %s \n%s", buffer, stackBackTrace().c_str());

	if (mLongJmp != NULL)
	{
		mLongJmp->mErrorDesc = buffer2;
		mLongJmp->errorCode = -1;

		if (mLongJmp->errorFunc >= 0)
		{
			Value errValue = getStackValue(mLongJmp->errorFunc);
			if (errValue.type == OP_TYPE_FUNC)
			{
				push(ConstructValue((XInt)mLongJmp->errorCode));
				push(ConstructValue(buffer2));
				std::string errprDesc;
				ProtectCallFunction(errValue.func, 2, errprDesc);
			}
		}

		longjmp(mLongJmp->j, 1);
	}
}

void	XScriptVM::ExecLuaFunction(Function* func)
{
	CallLuaFunction(func);
	ExecuteFunction(0);
}

void	XScriptVM::CallLuaFunction(Function* func)
{
	if (mCurXScriptState->mCurCallIndex >= MAX_LUA_CALL_STACK_DEPTH - 1)
	{
		ExecError("stack overflow");
	}

	int lastInstrIndex = -1;
	if (mCurXScriptState->mCurFunctionState != NULL)
		lastInstrIndex = mCurXScriptState->mInstrIndex;

	mCurXScriptState->mCurFunction = func;
	mCurXScriptState->mCurFunctionState = func->funcUnion.luaFunc.proto;

	if (mCurXScriptState->mTopIndex + MAX_PARAM_NUM + mCurXScriptState->mCurFunctionState->localDataSize > mCurXScriptState->mStackSize)
	{
		int growSize = mCurXScriptState->mTopIndex + MAX_PARAM_NUM + mCurXScriptState->mCurFunctionState->localDataSize - mCurXScriptState->mStackSize;
		GrowStack(mCurXScriptState, growSize);
	}

	for (int i = 0; i < mCurXScriptState->mCurFunctionState->localDataSize; i++)
	{
		mCurXScriptState->mStackElements[mCurXScriptState->mTopIndex + i].type = OP_TYPE_NIL;
	}

	resetReturnValue();

	mCurXScriptState->mTopIndex += mCurXScriptState->mCurFunctionState->localDataSize;
	mCurXScriptState->mFrameIndex = mCurXScriptState->mTopIndex;
	mCurXScriptState->mInstrIndex = 0;
	mCurXScriptState->mCurCallIndex++;

	mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mCurFunction = mCurXScriptState->mCurFunction;
	mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mCurFunctionState = mCurXScriptState->mCurFunctionState;
	if (mCurInstr != NULL)
	{
		mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mInstrIndex = lastInstrIndex;
		mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mCurLine = mCurInstr->lineIndex;
	}
	mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mFrameIndex = mCurXScriptState->mFrameIndex;
}

void  XScriptVM::CallHostFunc(Function* func, HOST_FUNC pfnAddr, int numParam)
{
	resetReturnValue();
	int lastParamNum = mNumHostFuncParam;
	mNumHostFuncParam = numParam;
	mCurXScriptState->mCCallIndex++;

	Function* lastCFunction = mCurXScriptState->mCurrentCFunction;
	mCurXScriptState->mCurrentCFunction = func;

	if (pfnAddr != NULL)
	{
		(*pfnAddr)(this);
	}

	mCurXScriptState->mCurrentCFunction = lastCFunction;

	mCurXScriptState->mCCallIndex--;
	mNumHostFuncParam = lastParamNum;
	popFrame(numParam);
}

int   XScriptVM::getParamType(int paramIndex)
{
	Value value = getParamValue(paramIndex);

	int type = value.type;
	if (IsValueLightUserData(&value))
		type = OP_LIGHTUSERDATA;
	return type;
}


Value XScriptVM::getParamValue(int paramIndex)
{
	if (paramIndex < mNumHostFuncParam)
	{
		int index = mCurXScriptState->mTopIndex - (mNumHostFuncParam - paramIndex);
		return mCurXScriptState->mStackElements[index];
	}
	else
	{
		return Value();
	}
}

bool  XScriptVM::getParamAsInt(int paramIndex, XInt& value)
{
	Value stackValue = getParamValue(paramIndex);
	if (IsValueNumber(&stackValue))
	{
		value = (XInt)PNumberValue(stackValue);
		return true;
	}
	return false;
}

bool  XScriptVM::getParamAsFloat(int paramIndex, XFloat& value)
{
	Value stackValue = getParamValue(paramIndex);
	if (IsValueNumber(&stackValue))
	{
		value = PNumberValue(stackValue);
		return true;
	}
	return false;
}

bool  XScriptVM::getParamAsString(int paramIndex, char* &value)
{
	Value stackValue = getParamValue(paramIndex);
	if (IsValueString(&stackValue))
	{
		value = (char*)stringRawValue(&stackValue);
		return true;
	}

	return false;
}


void*  XScriptVM::getParamAsObj(int paramIndex, char* userType)
{
	Value stackValue = getParamValue(paramIndex);
	if (stackValue.type == OP_TYPE_USERDATA)
	{
		TABLE metaTable = NULL;
		if (getTableValue(mMetaTable, userType, metaTable))
		{
			bool isValid = false;
			TABLE valueMeta = stackValue.userData->mMetaTable;
			while (valueMeta != NULL)
			{
				if (valueMeta == metaTable)
				{
					isValid = true;
					break;
				}
				else
				{
					valueMeta = valueMeta->mMetaTable;
				}
			}

			if (isValid)
			{
				void* pThis = NULL;
				memcpy(&pThis, stackValue.userData->value, sizeof(void*));
				return pThis;
			}
			
		}
	}

	return NULL;
}

bool  XScriptVM::getParamAsTable(int paramIndex, TABLE& table)
{
	int index = mCurXScriptState->mTopIndex - (mNumHostFuncParam - paramIndex);
	Value& value = mCurXScriptState->mStackElements[index];
	if (value.type == OP_TYPE_TABLE)
	{
		table = value.tableData;
		return true;
	}

	return false;
}

void  XScriptVM::setReturnAsNil(int regIndex)
{
	mRegValue[regIndex].type = OP_TYPE_NIL;
	mRegValue[regIndex].iIntValue = 0;
}


void  XScriptVM::setReturnAsValue(const Value& table, int regIndex)
{
	CopyValue(&mRegValue[regIndex], table);
}

void  XScriptVM::setReturnAsTable(const TABLE& table, int regIndex)
{
	CopyValue(&mRegValue[regIndex], ConstructValue(table));
}

void  XScriptVM::setReturnAsInt(XInt iResult, int regIndex)
{
	Value value;
	value.type = OP_TYPE_INT;
	value.iIntValue = iResult;
	CopyValue(&mRegValue[regIndex], value);
}

void  XScriptVM::resetReturnValue()
{
	for (int i = 0; i < MAX_FUNC_REG; i++)
	{
		mRegValue[i].type = OP_TYPE_NIL;
	}

}

void  XScriptVM::setReturnAsUserData(const char* className, void* pThis, int regIndex)
{
	if (pThis != NULL)
	{
		UserData* userData = CreateUserData(sizeof(void*));
		TABLE metaTable;
		if (getTableValue(mMetaTable, (char*)className, metaTable))
		{
			userData->mMetaTable = metaTable;
		}

		memcpy(userData->value, &pThis, sizeof(void*));
		SetUserDataValue(&mRegValue[regIndex], userData);
	}
	else
	{
		setReturnAsNil(regIndex);
	}
}

void  XScriptVM::setReturnAsfloat(XFloat fResult, int regIndex)
{
	Value value;
	value.type = OP_TYPE_FLOAT;
	value.fFloatValue = fResult;
	CopyValue(&mRegValue[regIndex], value);
}


void  XScriptVM::setReturnAsStr(const char* strResult, int regIndex)
{
	Value value = ConstructValue((char*)strResult);
	CopyValue(&mRegValue[regIndex], value);
}


void  XScriptVM::RegisterHostApi(const char* apiName, HOST_FUNC pfnAddr)
{
	int index = RegisterGlobalValue(apiName);
	mGlobalStackElements[index].type = OP_TYPE_FUNC;

	Function* f = CreateFunction();
	f->isCFunc = true;
	f->funcUnion.cFunc.pfnAddr = pfnAddr;
	mGlobalStackElements[index].func = f;
}

void	XScriptVM::RegisterUserClass(const char* className, const char* bassClassName, const std::vector<HostFunction>& funcVec)
{
	TableValue* metaTable;
	if (!getTableValue(mMetaTable, (char*)className, metaTable))
	{
		metaTable = CreateTable();
		setTableValue(mMetaTable, (char*)className, metaTable);
		setTableValue(metaTable, (char*)MetaMetodString(MMT_Index), metaTable);
	}

	
	CreateFunctionTable(funcVec, metaTable);
	
	if (bassClassName != NULL)
	{
		Value baseTable;
		if (getTableValue(mMetaTable, ConstructValue(bassClassName), baseTable) 
			&& IsValueTable(&baseTable))
		{
			metaTable->mMetaTable = baseTable.tableData;
		}
		else
		{
			TableValue*	baseMetaTable = CreateTable();
			setTableValue(mMetaTable, (char*)bassClassName, baseMetaTable);
			setTableValue(baseMetaTable, (char*)MetaMetodString(MMT_Index), baseMetaTable);
			metaTable->mMetaTable = baseMetaTable;
		}
	}
	
	RegisterGlobalValue(className);
	Value* pValue = GetGlobalValue(className);
	UserData* userData = CreateUserData(1);
	userData->mMetaTable = metaTable;
	SetUserDataValue(pValue, userData);
	
}


bool    isValueEqual(const Value& value1, const Value& value2)
{
	if (value1.type == value2.type)
	{
		switch (value1.type)
		{
		case OP_TYPE_FLOAT:
		{
			return value1.fFloatValue == value2.fFloatValue;
		}
		case OP_TYPE_INT:
		{
			return  value1.iIntValue == value2.iIntValue;
		}
		case OP_TYPE_STRING:
		{
			return  value1.stringValue == value2.stringValue;
		}
		case OP_TYPE_TABLE:
		{
			return value1.tableData != NULL && value2.tableData == value1.tableData;
		}
		case OP_LIGHTUSERDATA:
		{
			return value1.lightUserData == value2.lightUserData;
		}
		case OP_TYPE_USERDATA:
		{
			return value1.userData == value2.userData;
		}
		case OP_TYPE_FUNC:
		{
			return value1.func == value2.func;
		}
		}
		return false;
	}
	else
		return false;
}

Value			XScriptVM::GetMetaMethod(Value* value, MetaMethodType type)
{
	TableValue* metaTable = NULL;
	if (IsValueUserData(value))
	{
		metaTable = value->userData->mMetaTable;
	}
	else if (IsValueTable(value))
	{
		metaTable = value->tableData->mMetaTable;
	}

	if (metaTable != NULL)
	{
		Value result;
		getTableValue(metaTable, ConstructValue(MetaMetodString(type)), result);
		return result;
	}
	else
		return *mNilValue;
}

const char*		XScriptVM::MetaMetodString(MetaMethodType type)
{
	switch (type)
	{
	case MMT_Index:
		return "__index";
		break;
	case MMT_NewIndex:
		return "__newindex";
		break;
	case MMT_Equal:
		return "__equal";
		break;
	case MMT_Add:
		return "__add";
		break;
	case MMT_Sub:
		return "__sub";
		break;
	case MMT_Mul:
		return "__mul";
		break;
	case MMT_Div:
		return "__div";
		break;
	case MMT_Mod:
		return "__mod";
		break;
	case MMT_Pow:
		return "__pow";
		break;
	case MMT_Neg:
		return "__neg";
		break;
	case MMT_Len:
		return "__len";
		break;
	case NMT_Less:
		return "__less";
		break;
	case NMT_LessEqual:
		return "__lessequal";
		break;
	case MMT_Concat:
		return "__concat";
		break;
	case MMT_Call:
		return "__call";
		break;
	default:
		break;
	}
	return "";
}

const char* getTypeName(int type)
{
	char* str = "nil";
	if (type == OP_TYPE_INT)
	{
		str = "int";
	}
	else if (type == OP_TYPE_FLOAT)
	{
		str = "float";
	}
	else if (type == OP_TYPE_STRING)
	{
		str = "string";
	}
	else if (type == OP_TYPE_TABLE)
	{
		str = "table";
	}
	else if (type == OP_LIGHTUSERDATA)
	{
		str = "lightuserdata";
	}
	else if (type == OP_TYPE_FUNC)
	{
		str = "function";
	}
	else 	if (type == OP_TYPE_THREAD)
	{
		str = "thread";
	}
	else if (type == OP_TYPE_USERDATA)
	{
		str = "userdata";
	}
	return str;
}

bool	XScriptVM::GetNextKey(TableValue* tableData, const Value &keyValue, Value& nextKey, Value& nextValue)
{
	XInt index = FindKeyIndex(tableData, keyValue);
	if (index < -1)
		return false;
	index++;

	if (index < tableData->mArraySize)
	{
		XSetIntValue(&nextKey, index);
		nextValue = tableData->mArrayData[index];
		return true;
	}
	else
	{
		index -= tableData->mArraySize;
		while (index < tableData->mNodeCapacity)
		{
			if (!IsValueNil(&tableData->mNodeData[index].key.keyVal))
			{
				nextKey = tableData->mNodeData[index].key.keyVal;
				nextValue = tableData->mNodeData[index].value;
				return true;
			}
			else
			{
				index++;
			}
		}

		return false;
	
	}
}

XInt	XScriptVM::FindKeyIndex(TableValue* tableData, const Value &keyValue)
{
	if (IsValueNil(&keyValue))
		return -1;

	if (keyValue.type == OP_TYPE_INT && keyValue.iIntValue >= 0 && keyValue.iIntValue < tableData->mArraySize)
	{
		return keyValue.iIntValue;
	}
	else
	{
		bool found = false;
		if (tableData->mNodeCapacity > 0)
		{
			int hashPos = 0;
			GetHashPos(hashPos, tableData, &keyValue);
			TableNode* tableNode = &tableData->mNodeData[hashPos];
			while (tableNode != NULL)
			{
				if (isValueEqual(tableNode->key.keyVal, keyValue))
				{
					return tableNode - tableData->mNodeData + tableData->mArraySize;
				}
				tableNode = tableNode->key.next;
			}

		}
	}

	ExecError("invalid key for next");
	return -2;

}

void	XScriptVM::SetTableValue(TableValue* tableData, const Value &keyValue, const Value& value)
{
	if (IsValueNil(&keyValue))
		return;

	if (keyValue.type == OP_TYPE_INT && keyValue.iIntValue >= 0 && keyValue.iIntValue < tableData->mArraySize)
	{
		tableData->mArrayData[keyValue.iIntValue] = value;
	}
	else
	{
		bool found = false;
		if (tableData->mNodeCapacity > 0)
		{
			int hashPos = 0;
			GetHashPos(hashPos, tableData, &keyValue);
			TableNode* tableNode = &tableData->mNodeData[hashPos];
			while (tableNode != NULL)
			{
				if (isValueEqual(tableNode->key.keyVal, keyValue))
				{
					tableNode->value = value;
					found = true;
					break;
				}
				tableNode = tableNode->key.next;
			}

		}

		if (!found)
			NewTableKey(tableData, &keyValue)->value = value;
	}
}

Value*	XScriptVM::GetTableValueRef(TableValue* tableData, const Value &keyValue)
{
	if (keyValue.type == OP_TYPE_INT && keyValue.iIntValue >= 0 && keyValue.iIntValue < tableData->mArraySize)
	{
		return &tableData->mArrayData[keyValue.iIntValue];
	}
	else if (keyValue.type == OP_TYPE_NIL)
	{
		return NULL;
	}
	else
	{
		if (tableData->mNodeCapacity > 0)
		{
			int hashPos = 0;
			GetHashPos(hashPos, tableData, &keyValue);
			TableNode* tableNode = &tableData->mNodeData[hashPos];
			while (tableNode != NULL)
			{
				if (isValueEqual(tableNode->key.keyVal, keyValue))
				{
					return &tableNode->value;
				}
				tableNode = tableNode->key.next;
			}

		}

		return NULL;
	}
}

bool XScriptVM::getTableValue(TableValue* tableData, const Value &keyValue, Value& resultValue)
{
	Value* ref = GetTableValueRef(tableData, keyValue);
	if (ref != NULL)
	{
		resultValue = *ref;
		return true;
	}

	return false;
}

TableValue*		XScriptVM::CreateTable()
{
	TableValue* table = new TableValue();
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_TABLE;
	GC(table)->marked = MS_White;
	return table;
}

Function*		XScriptVM::CreateCFunction(int numUpvals)
{
	Function* table = new Function();
	memset(&table->funcUnion, 0, sizeof(FuncUnion));
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_FUNC;
	GC(table)->marked = MS_White;

	table->isCFunc = true;
	table->funcUnion.cFunc.mNumUpVal = numUpvals;
	table->funcUnion.cFunc.mUpVal = new Value[numUpvals];
	return table;
}


UserData*		XScriptVM::CreateUserData(int size)
{
	if (size < 1)
		size = 1;
	int realSize = sizeof(UserData) + size - 1;
	UserData* userData = (UserData*)malloc(realSize);
	userData->mSize = size;
	userData->mMetaTable = NULL;
	GC(userData)->next = mRootCG;
	mRootCG = GC(userData);
	GC(userData)->type = OP_TYPE_USERDATA;
	GC(userData)->marked = MS_White;
	return userData;
}

Function*		XScriptVM::CreateFunction()
{
	Function* table = new Function();
	memset(&table->funcUnion, 0, sizeof(FuncUnion));
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_FUNC;
	GC(table)->marked = MS_White;
	return table;
}

FuncState*		XScriptVM::CreateFunctionState()
{
	FuncState* table = new FuncState();
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_PROTO;
	GC(table)->marked = MS_Fixed;
	return table;
}

UpValue*		XScriptVM::CreateUpVal()
{
	UpValue* table = new UpValue();
	GC(table)->next = mRootCG;
	mRootCG = GC(table);
	GC(table)->type = OP_TYPE_UPVAL;
	GC(table)->marked = MS_Fixed;
	return table;
}

void	XScriptVM::RegisterHostLib(const char* libName, std::vector<HostFunction>& hostFuncVec)
{
	int index = RegisterGlobalValue(libName);
	TABLE table = newTable();
	CreateFunctionTable(hostFuncVec, table);


	setGloablStackValue(index, ConstructValue(table));
}

void XScriptVM::CreateFunctionTable(const std::vector<HostFunction> &hostFuncVec, TABLE table)
{
	for (int i = 0; i < (int)hostFuncVec.size(); i++)
	{
		HostFunction hostFunc = hostFuncVec[i];
		Function* f = CreateFunction();
		f->isCFunc = true;
		f->funcUnion.cFunc.pfnAddr = hostFuncVec[i].pfnAddr;
		Value fValue;
		fValue.type = OP_TYPE_FUNC;
		fValue.func = f;

		setTableValue(table, ConstructValue(hostFunc.funcName.c_str()), fValue);
	}
}

void	XScriptVM::MarkProto(FuncState* func)
{
	GC_SetBlack(func);
	for (int i = 0; i < (int)func->m_subFuncVec.size(); i++)
	{
		MarkProto(func->m_subFuncVec[i]);
	}
}

void	XScriptVM::MarkValue(Value* value)
{
	switch (value->type)
	{
	case OP_TYPE_TABLE:
	{
		MarkTable(value->tableData);
	}
	break;
	case OP_TYPE_FUNC:
	{
		GC_SetBlack(value->func);
		if (!value->func->isCFunc)
		{
			MarkProto(value->func->funcUnion.luaFunc.proto);

			for (int i = 0; i < value->func->funcUnion.luaFunc.mNumUpVals; i++)
			{
				GC_SetBlack(value->func->funcUnion.luaFunc.mUpVals[i]);
			}
		}
		else
		{
			for (int i = 0; i < value->func->funcUnion.cFunc.mNumUpVal; i++)
			{
				MarkValue(&value->func->funcUnion.cFunc.mUpVal[i]);
			}
		}

		break;
	}
	case OP_TYPE_THREAD:
	{
		GC_SetBlack(value->threadData);
		for (int i = MAX_GLOBAL_DATASIZE; i < value->threadData->mTopIndex; i++)
		{
			MarkValue(&value->threadData->mStackElements[i]);
		}

		GC_SetBlack(value->threadData->mStartFunction);
	}
	break;
	}
}

void XScriptVM::MarkTable(TABLE table)
{
	GC_SetBlack(table);
	for (int i = 0; i < table->mArraySize; i++)
	{
		MarkValue(&table->mArrayData[i]);
	}

	for (int i = 0; i < table->mNodeCapacity; i++)
	{
		if (!IsValueNil(&table->mNodeData[i].value))
		{
			MarkValue(&table->mNodeData[i].value);
		}

		if (!IsValueNil(&table->mNodeData[i].key.keyVal))
		{
			MarkValue(&table->mNodeData[i].key.keyVal);
		}
	}
}

void	XScriptVM::MarkObjects()
{
	std::map<std::string, int>::iterator it = mGloablVarMap.begin();
	for (; it != mGloablVarMap.end(); it++)
	{
		MarkValue(&mGlobalStackElements[it->second]);
	}

	for (int i = MAX_GLOBAL_DATASIZE; i < mMainXScriptState.mTopIndex; i++)
	{
		MarkValue(&mMainXScriptState.mStackElements[i]);
	}

	MarkTable(mEnvTable);
	MarkTable(mModuleTable);
}

void	XScriptVM::FreeObject(CGObject* obj)
{
	switch (obj->type)
	{
	case OP_TYPE_TABLE:
	{
		TableValue* table = (TableValue*)obj;;
		if (table->mArrayData != NULL)
			delete[] table->mArrayData;

		if (table->mNodeData != NULL)
			delete[]table->mNodeData;
	}
	break;
	case OP_TYPE_UPVAL:
	{
		UpValue* p = (UpValue*)obj;
		delete p;
	}
	break;
	case OP_TYPE_PROTO:
	{
		FuncState* funcState = (FuncState*)obj;
		delete[] funcState->mInstrStream.instrs;
		delete funcState;
	}
	break;
	case OP_TYPE_FUNC:
	{
		Function* func = (Function*)obj;
		if (func->isCFunc)
		{
			delete[]func->funcUnion.cFunc.mUpVal;
		}
		else
		{
			delete[]func->funcUnion.luaFunc.mUpVals;
		}
		delete func;
		func = NULL;
	}
	break;
	case OP_TYPE_THREAD:
	{
		XScriptState* xscriptState = (XScriptState*)obj;

		delete[]xscriptState->mStackElements;
		delete[]xscriptState->mCallInfoBase;

	}
	break;
	default:
		break;
	}
}

bool	IsUpValOpen(CGObject* obj)
{
	UpValue* p = (UpValue*)obj;
	return (p->pValue != &p->value);
}

void	XScriptVM::SweepObjects()
{
	CGObject* lastObj = NULL;
	CGObject*	gcObj = mRootCG;
	while (gcObj != NULL)
	{
		if (gcObj->marked != MS_White || (gcObj->type == OP_TYPE_UPVAL && IsUpValOpen(gcObj)))
		{
			GC_SetWhite(gcObj);

			lastObj = gcObj;
			gcObj = gcObj->next;
		}
		else
		{
			CGObject* p = gcObj->next;
			FreeObject(gcObj);
			if (lastObj != NULL)
			{
				lastObj->next = p;
			}
			else
			{
				mRootCG = p;
			}

			gcObj = p;
		}
	}
}

void	XScriptVM::GarbageCollect()
{
	MarkObjects();
	SweepObjects();
}

bool	XScriptVM::CastStrToFloat(const char *s, XFloat *result) {
	char *endptr;
	*result = (XFloat)strtod(s, &endptr);
	if (endptr == s) return 0;  /* conversion failed */
	if (*endptr == 'x' || *endptr == 'X')  /* maybe an hexadecimal constant? */
		*result = (XFloat)(strtoul(s, &endptr, 16));
	if (*endptr == '\0')
		return 1;  /* most common case */
	while (isspace(*endptr))
		endptr++;
	if (*endptr != '\0') return 0;  /* invalid trailing characters? */
	return 1;
}

void	XScriptVM::RequireMoudle(const char* moudleName)
{
	std::string moudle = moudleName;
	if (moudle.find(".xs", 0) == std::string::npos)
	{
		moudle += ".xs";
	}
	Value value;
	if (!getTableValue(mModuleTable, ConstructValue(moudle.c_str()), value))
	{
		doFile(moudle);
	}
}

Value*	XScriptVM::GetStackValueByName(int stackIndex, const std::string& name)
{
	int callIndex = mCurXScriptState->mCurCallIndex - stackIndex;
	if (callIndex >= 0 && callIndex <= mCurXScriptState->mCurCallIndex)
	{
		bool hasFound = false;
		FuncState* funcState = mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState;
		if (funcState != NULL)
		{
			int varIndex = -1;
			for (int i = 0; i < (int)funcState->m_localVarVec.size(); i++)
			{
				if (funcState->m_localVarVec[i] == name)
				{
					varIndex = i;
					break;
				}
			}

			if (varIndex >= 0 && varIndex < funcState->stackFrameSize)
			{
				int stackPos = mCurXScriptState->mCallInfoBase[callIndex].mFrameIndex + varIndex - funcState->stackFrameSize;
				return getStackValueRef(stackPos);
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}

	}
	else
	{
		return NULL;
	}
}
Value*	XScriptVM::GetStackValueByIndex(int stackIndex, int varIndex, std::string& name)
{
	int callIndex = mCurXScriptState->mCurCallIndex - stackIndex;
	if (callIndex >= 0 && callIndex <= mCurXScriptState->mCurCallIndex)
	{
		bool hasFound = false;
		FuncState* funcState = mCurXScriptState->mCallInfoBase[callIndex].mCurFunctionState;
		if (funcState->hasVarArgs && varIndex >= funcState->localParamNum - 1)
		{
			int varArgStackIndex = mCurXScriptState->mCallInfoBase[callIndex].mFrameIndex + funcState->localParamNum - 1 - funcState->stackFrameSize;

			Value value = getStackValue(varArgStackIndex);
			if (value.type == OP_TYPE_TABLE)
			{
				XInt numVarArgs = -1;
				if (getTableValue(value.tableData, "n", numVarArgs))
				{
					if (varIndex < funcState->localParamNum - 1 + numVarArgs)
					{
						hasFound = true;
						XInt argsIndex = varIndex + 1 - funcState->localParamNum;

						name = funcState->m_localVarVec[funcState->localParamNum - 1];

						return GetTableValueRef(value.tableData, ConstructValue(argsIndex));
					}
					else
					{
						varIndex = varIndex - (int)numVarArgs + 1;
					}
				}
			}
		}

		if (!hasFound)
		{
			if (varIndex >= 0 && varIndex < funcState->stackFrameSize)
			{
				int stackPos = mCurXScriptState->mCallInfoBase[callIndex].mFrameIndex + varIndex - funcState->stackFrameSize;
				name = funcState->m_localVarVec[varIndex];
				return getStackValueRef(stackPos);
			}
			else
			{
				return false;
			}

		}
	}

	
	return NULL;
	
}

void	XScriptVM::CallHookFunction(int event, int curLine)
{
	Value* pValue = GetGlobalValue("_hook");
	if (pValue != NULL && pValue->type == OP_TYPE_FUNC)
	{
		push(ConstructValue((XInt)event));
		push(ConstructValue((XInt)curLine));
		push(ConstructValue(mCurXScriptState->mCurFunctionState->sourceFileName.c_str()));

		Instr* savedInstr = mCurInstr;
		mAllowHook = false;
		CallFunction(pValue->func, 3);
		mAllowHook = true;
		mCurInstr = savedInstr;
	}
}

void	XScriptVM::RehashTable(TABLE table)
{
	int nodeCapacity = table->mNodeCapacity;
	TableNode* oldTableNode = table->mNodeData;

	table->mNodeCapacity *= 2;
	table->mNodeData = new TableNode[table->mNodeCapacity];
	table->lastFreePos = table->mNodeCapacity - 1;

	//memcpy(table->mNodeData, oldTableNode, sizeof(TableNode) * nodeCapacity);

	for (int i = 0; i < nodeCapacity; i++)
	{
		if (!IsValueNil(&oldTableNode[i].value))
		{
			TableNode* node = NewTableKey(table, &oldTableNode[i].key.keyVal);
			node->value = oldTableNode[i].value;
		}
	}

	delete[]oldTableNode;
}

TableNode*	XScriptVM::NewTableKey(TABLE table, const Value* key)
{
	if (table->mNodeCapacity == 0)
	{
		table->mNodeCapacity = 2;
		table->mNodeData = new TableNode[table->mNodeCapacity];

		table->lastFreePos = table->mNodeCapacity - 1;
	}

	int hashPos = 0;
	GetHashPos(hashPos, table, key);
	TableNode* mainPos = &table->mNodeData[hashPos];
	if (IsValueNil(&mainPos->value))
	{
		CopyValue(&table->mNodeData[hashPos].key.keyVal, *key);
		return &table->mNodeData[hashPos];
	}
	else
	{
		TableNode* freeNode = NULL;

		while (table->lastFreePos >= 0)
		{
			if (IsValueNil(&table->mNodeData[table->lastFreePos].value))
			{
				freeNode = &table->mNodeData[table->lastFreePos];
				break;
			}

			table->lastFreePos--;
		}

		if (freeNode == NULL)
		{
			RehashTable(table);

			return NewTableKey(table, key);
		}
		else
		{
			int shouldHashPos = 0;
			GetHashPos(shouldHashPos, table, &table->mNodeData[hashPos].key.keyVal);
			TableNode* otherMainPos = &table->mNodeData[shouldHashPos];
			if (shouldHashPos == hashPos)
			{
				CopyValue(&freeNode->key.keyVal, *key);
				freeNode->key.next = table->mNodeData[hashPos].key.next;
				table->mNodeData[hashPos].key.next = freeNode;
				return freeNode;
			}
			else
			{
				while (otherMainPos->key.next != mainPos)
				{
					otherMainPos = otherMainPos->key.next;
				}

				otherMainPos->key.next = freeNode;

				CopyValue(&freeNode->value, mainPos->value);
				CopyValue(&freeNode->key.keyVal, mainPos->key.keyVal);
				freeNode->key.next = mainPos->key.next;

				mainPos->key.next = NULL;
				CopyValue(&mainPos->key.keyVal, *key);

				return mainPos;

			}

		}
	}


}

Value	XScriptVM::GetEnvValue(int index)
{
	int globalIndex = GloablVarStackIndex(index);
	Value key = ConstructValue((char*)m_stringVec[GloablVarNameIndex(index)].c_str());
	Value pValue ;
	if (getTableValue(mEnvTable, key, pValue))
		return pValue;

	return mGlobalStackElements[globalIndex];

}

unsigned int APHash(const char *str, int len)
{
	unsigned int hash = 0;
	int i;

	for (i = 0; i < len && i < 32; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}

	return (hash & 0x7FFFFFFF);
}

XString*		XScriptVM::NewXString(const char* str, int len)
{
	unsigned int hash = APHash(str, len);

	int hashIndex = hash % mStringHashSize;
	XString* nextStr = mStringHashTable[hashIndex];
	while (nextStr != NULL)
	{
		if (nextStr->len == len && memcmp(&nextStr->value, str, len) == 0)
		{
			return nextStr;
		}

		nextStr = (XString*)nextStr->next;
	}

	int strSize = sizeof(XString) + (len + 1) * sizeof(char);
	char* buffer = new char[strSize];
	XString* newStr = (XString*)buffer;

	newStr->hash = hash;
	newStr->len = len;
	memcpy(&newStr->value, str, len);
	((char*)(&newStr->value))[len] = 0;
	((char*)(&newStr->value))[len + 1] = 0;
	GC_SetWhite(newStr);
	newStr->type = OP_TYPE_STRING;

	newStr->next = (CGObject*)mStringHashTable[hashIndex];
	mStringHashTable[hashIndex] = newStr;
	mStringHashUsedSize++;

	if (mStringHashUsedSize > (mStringHashSize / 2))
	{
		ResizeHashTable();
	}

	return newStr;
}



XString*	XScriptVM::NewXString(const char* str)
{
	int len = strlen(str);
	return NewXString(str, len);
}

void XScriptVM::ResizeHashTable()
{
	int newHashSize = mStringHashSize * 2;
	XString** newHashTable = new XString*[newHashSize];
	memset(newHashTable, 0, sizeof(XString*) * newHashSize);

	for (int i = 0; i < mStringHashSize; i++)
	{
		XString* nextStr = mStringHashTable[i];
		while (nextStr != NULL)
		{
			CGObject* saved = nextStr->next;
			int newHashIndex = nextStr->hash % newHashSize;
			nextStr->next = (CGObject*)newHashTable[newHashIndex];
			newHashTable[newHashIndex] = nextStr;

			nextStr = (XString*)saved;
		}
	}

	delete[]mStringHashTable;

	mStringHashTable = newHashTable;
	mStringHashSize = newHashSize;
}


XScriptState*	XScriptVM::CreateCoroutie(Function* func)
{
	XScriptState* state = new XScriptState();
	resetScriptState(state);
	state->mStartFunction = func;
	return state;
}


void	XScriptVM::ResumeCoroutie(XScriptState* threadState, int offset)
{
	if (GetCoroutieStatus(threadState) != CS_Suspend)
	{
		setReturnAsInt(0, 0);
		char errorDesc[128] = { 0 };
		snprintf(errorDesc, 128, "cannot resume %s coroutine", GetCoroutieStatusName(threadState));
		setReturnAsStr(errorDesc, 1);
	}
	else
	{
		if (threadState->mStatus == CS_Normal)
		{
			for (int i = offset; i < mNumHostFuncParam; i++)
			{
				threadState->mStackElements[threadState->mTopIndex] = getParamValue(i);
				threadState->mTopIndex++;
			}

			XScriptState* lastState = mCurXScriptState;

			mCurXScriptState = threadState;

			std::string errorDesc;
			int status = ProtectCallFunction(threadState->mStartFunction, mNumHostFuncParam - offset, errorDesc);

			if (status == 0)
			{
				if (threadState->mStatus == CS_Normal)
				{
					for (int i = MAX_FUNC_REG - 1; i > 0; i--)
					{
						mRegValue[i] = mRegValue[i - 1];
					}

					threadState->mStatus = CS_Dead;
				}

				setReturnAsInt(1, 0);
			}
			else
			{
				mCurXScriptState->mStatus = CS_Dead;
				resetReturnValue();
				setReturnAsInt(0, 0);
				setReturnAsStr(errorDesc.c_str(), 1);
			}

			mCurXScriptState = lastState;
		}
		else if (threadState->mStatus == CS_Suspend)
		{
			for (int i = offset; i < mNumHostFuncParam; i++)
			{
				mRegValue[i - offset] = getParamValue(i);
			}

			XScriptState* lastState = mCurXScriptState;
			mCurXScriptState = threadState;
			mCurXScriptState->mStatus = CS_Normal;

			std::string errorDesc;
			int status = ProtectResume(errorDesc);
			if (status == 0)
			{
				if (threadState->mStatus == CS_Normal)
				{
					for (int i = MAX_FUNC_REG - 1; i > 0; i--)
					{
						mRegValue[i] = mRegValue[i - 1];
					}

					threadState->mStatus = CS_Dead;
				}

				setReturnAsInt(1, 0);
			}
			else
			{
				mCurXScriptState->mStatus = CS_Dead;
				resetReturnValue();

				setReturnAsInt(0, 0);
				setReturnAsStr(errorDesc.c_str(), 1);
			}

			mCurXScriptState = lastState;
		}
	}

}

void	XScriptVM::YieldCoroutie()
{
	if (mCurXScriptState->mCCallIndex > 1 || mCurXScriptState == &mMainXScriptState)
	{
		ExecError("can't yield coroutine across C calls");
	}

	for (int i = 0; i < mNumHostFuncParam; i++)
	{
		mRegValue[i + 1] = getParamValue(i);
	}

	mCurXScriptState->mStatus = CS_Suspend;

}

const char*		XScriptVM::GetCoroutieStatusName(XScriptState* xsState)
{
	static const char *const statNames[] =
	{ "normal", "running", "suspend", "dead" };

	return statNames[GetCoroutieStatus(xsState)];
}

CoroutineStatus	XScriptVM::GetCoroutieStatus(XScriptState* xsState)
{
	if (xsState->mStatus == CS_Normal)
	{
		if (xsState == mCurXScriptState)
		{
			return CS_Running;
		}
		else
			return CS_Suspend;
	}
	else
		return (CoroutineStatus)xsState->mStatus;
}


void	XScriptVM::GrowStack(XScriptState* xsState, int growSize)
{
	int newSize = xsState->mStackSize * 2;

	if (growSize > xsState->mStackSize)
	{
		newSize = xsState->mStackSize + growSize;
	}

	Value* newStackElem = new Value[newSize];

	memcpy(newStackElem, xsState->mStackElements, sizeof(Value) * xsState->mStackSize);

	for (int i = xsState->mStackSize; i < newSize; i++)
	{
		newStackElem[i].type = OP_TYPE_NIL;
		newStackElem[i].iIntValue = 0;
	}

	UpValue* nextUpVals = xsState->mNextRefUpVals;
	while (nextUpVals != NULL)
	{
		nextUpVals->pValue = newStackElem - xsState->mStackElements + nextUpVals->pValue;
		nextUpVals = nextUpVals->nextValue;
	}
	delete xsState->mStackElements;

	xsState->mStackSize = newSize;
	xsState->mStackElements = newStackElem;
}

bool	XScriptVM::CalByTagMethod(Value* result, Value* value1, Value* value2, MetaMethodType mmtType)
{
	int numParam = 2;
	Value tagMethod;
	if (mmtType == MMT_Add || mmtType == MMT_Sub || mmtType == MMT_Mul || mmtType == MMT_Div 
		|| mmtType == MMT_Mod || mmtType == MMT_Pow || mmtType == MMT_Concat)
	{
		tagMethod = GetMetaMethod(value1, mmtType);
	}
	else if (mmtType == NMT_Less || mmtType == NMT_LessEqual ||
		mmtType == NMT_Great || mmtType == NMT_GreatEqual ||
		mmtType == MMT_Equal)
	{
		if (mmtType == NMT_Great || mmtType == NMT_GreatEqual)
		{
			Value* tmp = value1;
			value1 = value2;
			value2 = tmp;
			if (mmtType == NMT_Great)
				mmtType = NMT_Less;
			else
				mmtType = NMT_LessEqual;
		}

		Value tagMethod1 = GetMetaMethod(value1, mmtType);
		Value tagMethod2 = GetMetaMethod(value2, mmtType);

		if (!IsValueNil(&tagMethod1) && !IsValueNil(&tagMethod2) && isValueEqual(tagMethod1, tagMethod2))
		{
			tagMethod = tagMethod1;
		}
	}
	else if (mmtType == MMT_Neg || mmtType == MMT_Len)
	{
		tagMethod = GetMetaMethod(value1, mmtType);
		numParam = 1;
	}
	

	if (!IsValueFunction(&tagMethod))
		return false;

	push(*value1);
	if (numParam > 1)
		push(*value2);

	Instr* savedInstr = mCurInstr;
	CallFunction(tagMethod.func, numParam);
	mCurInstr = savedInstr;
	*result = mRegValue[0];
	return true;
}


