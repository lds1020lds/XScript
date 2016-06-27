#include "Parser.h"
#include "SourceFile.h"
#include "Lexer.h"
#include "MidCode.h"
#include "SymbolTable.h"
#include <math.h>

#include <setjmp.h>
#include "XsriptVM.h"

jmp_buf setjmp_buffer;


CParser::CParser(void)
	: mLastFreeIndex(0)
{
}

CParser::~CParser(void)
{
}

bool  CParser::IsRegVar(int varIndex)
{
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].mVarIndex == varIndex)
		{
			return true;
		}
	}

	return false;
}

int    CParser::GetFreeReg()
{
	int lastIndex = -1;
	int lasFreeIndex = -1;
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].bUsed == false)
		{
			if (mReg[i].mLastIndex > lasFreeIndex)
			{
				lastIndex = i;
				lasFreeIndex = mReg[i].mLastIndex;
			}
			
		}
	}
	
	if (lastIndex >= 0)
	{
		mReg[lastIndex].bUsed = true;
		return mReg[lastIndex].mVarIndex;
	}
	else
	{
		Error("Cant't get free reg");
	}
	return lastIndex;
}

void		CParser::FreeFactorReg(const Factor& factor)
{
	if (factor.type == FACTOR_VAR)
		FreeReg(factor.varIndex);
}

void CParser::FreeReg(int reg)
{
	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		if (mReg[i].mVarIndex == reg)
		{
			mReg[i].bUsed = false;
			mReg[i].mLastIndex = mLastFreeIndex;
			mLastFreeIndex++;
		}
	}
}

CParser::Factor CParser::ComputeFactors(int computeWay, Factor const& factor1, Factor const & factor2)
{
	Factor ret;
	if (factor1.type == FACTOR_STRING)
	{
		_ASSERT(factor2.type == FACTOR_STRING);
		int newLength = strlen(mSymbolTable->getString(factor1.strIndex)) + strlen(mSymbolTable->getString(factor2.strIndex)) + 1;
		char* newStr = new char[newLength]; 
		strcpy(newStr, mSymbolTable->getString(factor1.strIndex));
		strcat(newStr, mSymbolTable->getString(factor2.strIndex));
		newStr[newLength - 1] = '\0';
		ret.type = FACTOR_STRING;
		ret.strIndex = mSymbolTable->AddString(newStr);
		delete newStr;
	}
	else
	{
		float fValue = 0.0f;
		if (factor1.type == FACTOR_FLOAT)
		{
			fValue = factor1.floatValue;
		}
		else
		{
			fValue = (float)factor1.intValue;
		}
		int intValue = 0;
		if (factor2.type == FACTOR_FLOAT)
		{
			if (computeWay == OP_TYPE_ADD)
				fValue += factor2.floatValue;
			else if (computeWay == OP_TYPE_SUB)
				fValue -= factor2.floatValue;
			else if (computeWay == OP_TYPE_MUL)
				fValue *= factor2.floatValue;
			else if (computeWay == OP_TYPE_DIV)
				fValue /= factor2.floatValue;
			else if (computeWay == OP_TYPE_MOD)
				fValue = fmodf(fValue, (float)factor2.floatValue);
			else if (computeWay == OP_TYPE_EXP)
				fValue = pow(fValue, (float)factor2.floatValue);
			else if (computeWay == OP_TYPE_BITWISE_AND)
				intValue = ((int)fValue) & ((int)factor2.floatValue);
			else if (computeWay == OP_TYPE_BITWISE_OR)
				intValue = ((int)fValue) | ((int)factor2.floatValue);
			else if (computeWay == OP_TYPE_BITWISE_XOR)
				intValue = ((int)fValue) ^ ((int)factor2.floatValue);
			else if (computeWay == OP_TYPE_BITWISE_SHIFT_LEFT)
				intValue = (((int)fValue) << ((int)factor2.floatValue));
			else if (computeWay == OP_TYPE_BITWISE_SHIFT_RIGHT)
				intValue = ((int)fValue) >> ((int)factor2.floatValue);
		}
		else
		{
			if (computeWay == OP_TYPE_ADD)
				fValue += (float)factor2.intValue;
			else if (computeWay == OP_TYPE_SUB)
				fValue -= (float)factor2.intValue;
			else if (computeWay == OP_TYPE_MUL)
				fValue *= (float)factor2.intValue;
			else if (computeWay == OP_TYPE_DIV)
				fValue /= (float)factor2.intValue;
			else if(computeWay == OP_TYPE_MOD)
				fValue = fmodf(fValue, (float)factor2.intValue);
			else if (computeWay == OP_TYPE_EXP)
				fValue = pow(fValue, (float)factor2.intValue);
			else if (computeWay == OP_TYPE_BITWISE_AND)
				intValue = ((int)fValue) & ((int)factor2.intValue);
			else if (computeWay == OP_TYPE_BITWISE_OR)
				intValue = ((int)fValue) | ((int)factor2.intValue);
			else if (computeWay == OP_TYPE_BITWISE_XOR)
				intValue = ((int)fValue) ^ ((int)factor2.intValue);
			else if (computeWay == OP_TYPE_BITWISE_SHIFT_LEFT)
				intValue = (((int)fValue) << ((int)factor2.intValue));
			else if (computeWay == OP_TYPE_BITWISE_SHIFT_RIGHT)
				intValue = ((int)fValue) >> ((int)factor2.intValue);
		}

		if (computeWay == OP_TYPE_BITWISE_AND
			|| computeWay == OP_TYPE_BITWISE_OR
			|| computeWay == OP_TYPE_BITWISE_XOR
			|| computeWay == OP_TYPE_BITWISE_SHIFT_LEFT
			|| computeWay == OP_TYPE_BITWISE_SHIFT_RIGHT)
		{
			ret.type = FACTOR_INT;
			ret.intValue = (int)intValue;
		}
		else
		{
			if (fValue == (int)fValue)
			{
				ret.type = FACTOR_INT;
				ret.intValue = (int)fValue;
			}
			else
			{
				ret.type = FACTOR_FLOAT;
				ret.floatValue = fValue;
			}
		}
	}


	return ret;
}

bool  CParser::ParseFile(CSourceFile* sourefile, XScriptVM* vm, CMidCode* midCode, CSymbolTable* symbol)
{
	CLexer lexer(sourefile);
	mMidCode= midCode;
	mLexer = &lexer;
	mSymbolTable = symbol;
	mVM = vm;
	midCode->SetLex(mLexer);

	mCurFuncIndex = 0;

	mNilSymbolIndex = mSymbolTable->AddVariant("nil", -1, 0, IDENT_TYPE_INTERNAL_VAR);

	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		mReg[i].bUsed = false;
		mReg[i].mLastIndex = 0;

		char regName[6];
		sprintf(regName, "_R%d", i);
		mReg[i].mVarIndex = mSymbolTable->AddVariant(regName, -1, 0, IDENT_TYPE_INTERNAL_VAR);
	}


	if (setjmp(setjmp_buffer))
	{
		return false;
	}
	
	while (lexer.LookNextToken() != TOKEN_TYPE_END_OF_STREAM)
	{
		ParseStateMent();
	}
	
	mMidCode->setCurFuncIndex(0);
	mMidCode->AddInstr(INSTR_RET);

	mSymbolTable->computeParmamStackIndex();
	optimizeCode();
	insertSaveRegCode();
	return true;
}
bool  CParser::ParseStateMent()
{
	TOKEN token = mLexer->LookNextToken();
	switch(token)
	{
	case TOKEN_TYPE_DELIM_SEMICOLON:      //; 空语句
		{
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
			break;
		}
	case TOKEN_TYPE_END_OF_STREAM:
		{
			Error("Unexpected end of file");
			break;
		}
	case TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE:
		{
			ParseBlock();
			break;
		}
	case TOKEN_TYPE_RSRVD_FUNC:
		{
			Factor factor;
			ParseFunction(factor);
			break;
		}
	case TOKEN_TYPE_RSRVD_VAR:
		{
			ParseVar(false);
			break;
		}
	case TOKEN_TYPE_RSRVD_IF:
		{
			ParseIf();
			break;
		}
	case TOKEN_TYPE_RSRVD_WHILE:
		ParseWhile ();
		break;
	case TOKEN_TYPE_RSRVD_FOR:
		ParseFor ();
		break;
	case TOKEN_TYPE_RSRVD_FOREACH:
		ParseForeach();
		break;
	case TOKEN_TYPE_RSRVD_BREAK:
		ParseBreak ();
		break;

		// continue

	case TOKEN_TYPE_RSRVD_CONTINUE:
		ParseContinue ();
		break;

	case TOKEN_TYPE_RSRVD_RETURN:
		ParseReturn ();
		break;
	case TOKEN_TYPE_IDENT:
		{
			ParserIdent();
			break;
		}
	default:
		Error("error statement, should be ident, continue, return, break...");
		break;
	}

	for (int i = 0; i < MAX_REG_NUM; i++)
	{
		mReg[i].bUsed = false;
	}

	return true;
}

void CParser::ParserIdent()
{
	Factor firstRet, secondRet;
	ParserVariableAndFunction(firstRet, secondRet);

	if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COMMA)
	{
		std::vector<VarData> varVec;
		varVec.push_back(VarData(firstRet, secondRet));

		while (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COMMA)
		{
			mLexer->GetNextToken();

			Factor first, second;
			ParserVariableAndFunction(first, second);
			varVec.push_back(VarData(first, second));
		}

		mLexer->ExpectToken(TOKEN_TYPE_OP, OP_TYPE_ASSIGN);

		std::vector<Factor> retVec;

		while (true)
		{
			Factor ret = ParseExpr(false);
			retVec.push_back(ret);
			if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
				break;
			mLexer->GetNextToken();
		}

		MultiAssignment(retVec, varVec);

		//mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
	}
	else if (mLexer->LookNextToken() == TOKEN_TYPE_OP)
	{
		ParserAssignRight(firstRet, secondRet, true);
	}
	else if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_SEMICOLON)
	{
		mLexer->GetNextToken();
	}
	else 
	{
		if (firstRet.type != FACTOR_FUNC)
		{
			Error("invalid ident use, operator expected");
		}
	}
}

void CParser::MultiAssignment(std::vector<Factor> &retVec, std::vector<VarData> &varVec)
{
	if (retVec.size() >= varVec.size())
	{
		for (int i = 0; i < varVec.size(); i++)
		{
			AddAssignCode(varVec[i], retVec[i]);
		}
	}
	else
	{
		if (retVec.back().type == FACTOR_FUNC)
		{
			for (int i = 0; i < retVec.size() - 1; i++)
				AddAssignCode(varVec[i], retVec[i]);

			for (int i = retVec.size() - 1; i < varVec.size() && i < MAX_REG_NUM + retVec.size() - 1; i++)
			{
				int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				if (varVec[i].second.type != FACTOR_INVALID)
					AddTableFactor(varVec[i].second, iInstrIndex, varVec[i].first.varIndex);
				else
					AddOperandByFactor(iInstrIndex, varVec[i].first);

				mMidCode->AddRegOperand(iInstrIndex, i + 1 - retVec.size());
			}
		}
		else
		{
			for (int i = 0; i < varVec.size(); i++)
			{
				if (i < retVec.size())
				{
					AddAssignCode(varVec[i], retVec[i]);
				}
				else
				{
					int iInstrIndex = mMidCode->AddInstr(INSTR_LOADNIL);
					if (varVec[i].second.type != FACTOR_INVALID)
						AddTableFactor(varVec[i].second, iInstrIndex, varVec[i].first.varIndex);
					else
						AddOperandByFactor(iInstrIndex, varVec[i].first);
				}
			}
		}
	}
}

void  CParser::AddAssignCode(VarData& var, Factor& expr)
{
	int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
	if (var.second.type != FACTOR_INVALID)
		AddTableFactor(var.second, iInstrIndex, var.first.varIndex);
	else
		AddOperandByFactor(iInstrIndex, var.first);

	AddOperandByFactor(iInstrIndex, expr);
}

bool  CParser::ParseBlock()
{
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE);
	while (mLexer->LookNextToken() 
		!= TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
	{
		ParseStateMent();
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE);
	return true;
}
void  CParser::Error(const char *errInfo, ...)
{
	char  buffer[512] = { 0 };
	va_list  args;
	va_start(args, errInfo);
	int len = vsnprintf(buffer, 512, errInfo, args);
	va_end(args);
	buffer[len] = '\0';

	mLexer->RewindToken();
	ExitOnError(buffer, mLexer->GetLine(), mLexer->GetChar());
}

void  CParser::ErrorIdentRedefine(const char* ident)
{
	char errInfo[MAX_ERRORINFO_SIZE];
	sprintf(errInfo, "the identifier %s is redefined", ident);
	Error(errInfo);
}

bool  CParser::ParseFunction(Factor assignFactor, bool isLocal)
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FUNC);
	std::string funcName;
	static int sNumAnonymousFunc = 0;
	char szFuncName[128] = { 0 };
	sprintf(szFuncName, "anonymous_func_%d", sNumAnonymousFunc);
	funcName = szFuncName;
	sNumAnonymousFunc++;
	Factor tableFactor;

	bool hasSelf = false;
	int symbIndex = -1;
	if (assignFactor.type == FACTOR_INVALID)
	{
		if (isLocal)
		{
			mLexer->ExpectToken(TOKEN_TYPE_IDENT);
			funcName = mLexer->GetCurLexeme();

			int index = mSymbolTable->AddVariant(funcName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR);
			assignFactor.type = FACTOR_VAR;
			assignFactor.varIndex = index;
		}
		else
		{
			mLexer->ExpectToken(TOKEN_TYPE_IDENT);
			std::string identName = mLexer->GetCurLexeme();
			symbIndex = mSymbolTable->SearchValue(mLexer->GetCurLexeme(), mCurFuncIndex);
			bool isArray = ParserVariable(symbIndex, tableFactor);
			if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COLON)
			{
				mLexer->GetNextToken();
				mLexer->ExpectToken(TOKEN_TYPE_IDENT);

				if (isArray)
				{
					int req = GetFreeReg();
					int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
					mMidCode->AddVarOperand(iInstrIndex, req);

					AddTableFactor(tableFactor, iInstrIndex, symbIndex);
					FreeFactorReg(tableFactor);

					FreeReg(symbIndex);
					symbIndex = req;
				}
				

				std::string name = mLexer->GetCurLexeme();
				int strIndex = mSymbolTable->AddString(name.c_str());

				tableFactor.type = FACTOR_STRING;
				tableFactor.strIndex = strIndex;

				hasSelf = true;
				funcName = name;
			}
			else
			{
				if (!isArray)
				{
					funcName = identName;
				}
			}
			
		}
	}

	int iFuncIndex = mSymbolTable->AddFunction(funcName.c_str(), 0, mCurFuncIndex);
	mSymbolTable->AddSubFunction(mCurFuncIndex, iFuncIndex);
	
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);
	int iParamNum = 0;

	if (hasSelf)
	{
		iParamNum++;
		mSymbolTable->AddVariant("self", iFuncIndex, 0, IDENT_TYPE_PARAM);
	}

	while(mLexer->LookNextToken() != TOKEN_TYPE_DELIM_CLOSE_PAREN)
	{
		TOKEN token = mLexer->GetNextToken( );
		if (token == TOKEN_TYPE_IDENT)
		{
			if (mSymbolTable->GetVarByName(mLexer->GetCurLexeme(), iFuncIndex, false) != NULL)
			{
				ErrorIdentRedefine(mLexer->GetCurLexeme());
			}
			mSymbolTable->AddVariant(mLexer->GetCurLexeme(), iFuncIndex, 0, IDENT_TYPE_PARAM);
		}
		else if (token == TOKEN_TYPE_DELIM_THREE_POINT)
		{
			mSymbolTable->AddVariant(ARGS, iFuncIndex, 0, IDENT_TYPE_PARAM);
			mSymbolTable->SetHasVarArgs(iFuncIndex);
		}
		
		iParamNum++;

		if (token == TOKEN_TYPE_DELIM_THREE_POINT)
			break;	

		if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_CLOSE_PAREN)
			break;

		mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA);
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

	mSymbolTable->SetFunctionParamNum(iFuncIndex, iParamNum);
	int lastFuncIndex = mCurFuncIndex;
	mCurFuncIndex = iFuncIndex;
	mMidCode->setCurFuncIndex(iFuncIndex);
	int iCodeStart, iCodeEnd;
	
	ParseBlock();
 	if (mMidCode->getLastInstrOp() != INSTR_RET)
 		mMidCode->AddInstr(INSTR_RET);

	mCurFuncIndex = lastFuncIndex;
	mMidCode->setCurFuncIndex(mCurFuncIndex);
	int iInstrIndex = mMidCode->AddInstr(INSTR_FUNC);
	int funcIndex = mSymbolTable->GetFunctionByIndex(mCurFuncIndex)->subIndexVec.size() - 1;

	if (assignFactor.type == FACTOR_INVALID && !isLocal)
	{
		if (tableFactor.type == FACTOR_INVALID)
		{
			mMidCode->AddVarOperand(iInstrIndex, symbIndex);
		}
		else
		{
			AddTableFactor(tableFactor, iInstrIndex, symbIndex);
		}
		
	}
	else
	{
		AddOperandByFactor(iInstrIndex, assignFactor);
		
	}
	
	mMidCode->AddIntOperand(iInstrIndex, funcIndex);
	return true;
}


bool  CParser::ParseVar(bool isGlobal)
{
	if (!isGlobal)
		mLexer->ExpectToken(TOKEN_TYPE_RSRVD_VAR);

	if (!isGlobal && mLexer->LookNextToken() == TOKEN_TYPE_RSRVD_FUNC)
	{
		ParseFunction(Factor(), true);
	}

	std::vector<VarData> varVec;

	do
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT);
		int iVarIndex = mSymbolTable->AddVariant(mLexer->GetCurLexeme(), mCurFuncIndex, 0, IDENT_TYPE_VAR);
		
		Factor factor;
		factor.type = FACTOR_VAR;
		factor.varIndex = iVarIndex;
		varVec.push_back(VarData(factor, Factor()));

		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
		{
			break;
		}
		mLexer->GetNextToken();
	} while (true);


	if (mLexer->LookNextToken() == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OP_TYPE_ASSIGN)
	{
		mLexer->GetNextToken();
		std::vector<Factor> retVec;
		while (true)
		{
			Factor ret = ParseExpr(false);
			retVec.push_back(ret);
			if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
				break;
			mLexer->GetNextToken();
		}

		MultiAssignment(retVec, varVec);
	}
	mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);
	return true;
}

void  CParser::AddOperandByFactor(int iInstrIndex, Factor ret)
{
	switch(ret.type)
	{
	case FACTOR_INT:
		{
			mMidCode->AddIntOperand(iInstrIndex, ret.intValue);
			break;
		}
	case FACTOR_NIL:
		{
			mMidCode->AddVarOperand(iInstrIndex, mNilSymbolIndex);
			break;
		}
	case FACTOR_FLOAT:
		{
			mMidCode->AddFloatOperand(iInstrIndex, ret.floatValue);
			break;
		}
	case FACTOR_VAR:
		{
			mMidCode->AddVarOperand(iInstrIndex, ret.varIndex);
			break;
		}
	case FACTOR_TABLE:
		{
			switch (ret.iOffset)
			{
			case ROT_Int:
				mMidCode->AddTableIntOperand(iInstrIndex, ret.varIndex, ret.intTableValue);
				break;
			case ROT_String:
				mMidCode->AddTableStringOperand(iInstrIndex, ret.varIndex, ret.intTableValue);
				break;
			case ROT_Float:
				mMidCode->AddTableFloatOperand(iInstrIndex, ret.varIndex, ret.fTableValue);
				break;
			case ROT_Stack_Index:
				mMidCode->AddTableIndexOperand(iInstrIndex, ret.varIndex, ret.intTableValue);
				break;
			case ROT_Reg:
				mMidCode->AddTableRegOperand(iInstrIndex, ret.varIndex, 0);
				break;
			}
			break;
		}
	case FACTOR_FUNC:
		{
			mMidCode->AddRegOperand(iInstrIndex);
			break;
		}
	case FACTOR_STRING:
		{
			mMidCode->AddStringIndexOperand(iInstrIndex, ret.strIndex);
			break;
		}
	default:;
	}
}

int	GetOpInstr(int op)
{
	int iInstr = -1;
	switch (op)
	{
	case OP_TYPE_ADD:
		iInstr = INSTR_ADD;
		break;
	case OP_TYPE_SUB:
		iInstr = INSTR_SUB;
		break;
	case OP_TYPE_CONCAT:
		iInstr = INSTR_CONCAT;
		break;
	case OP_TYPE_BITWISE_AND:
		iInstr = INSTR_AND;
		break;
	case OP_TYPE_BITWISE_OR:
		iInstr = INSTR_OR;
		break;
	case OP_TYPE_BITWISE_XOR:
		iInstr = INSTR_XOR;
		break;
	case OP_TYPE_MUL:
		iInstr = INSTR_MUL;
		break;
	case OP_TYPE_DIV:
		iInstr = INSTR_DIV;
		break;
	case OP_TYPE_MOD:
		iInstr = INSTR_MOD;
		break;
	case OP_TYPE_EXP:
		iInstr = INSTR_EXP;
		break;
	case OP_TYPE_BITWISE_SHIFT_LEFT:
		iInstr = INSTR_SHL;
		break;
	case OP_TYPE_BITWISE_SHIFT_RIGHT:
		iInstr = INSTR_SHR;
		break;
	default:
		break;
	}

	return iInstr;
						
}

int	GetOpInstrTo(int op)
{
	int iInstr = -1;
	switch (op)
	{
	case OP_TYPE_ADD:
		iInstr = INSTR_ADD_TO;
		break;
	case OP_TYPE_SUB:
		iInstr = INSTR_SUB_TO;
		break;
	case OP_TYPE_CONCAT:
		iInstr = INSTR_CONCAT_TO;
		break;
	case OP_TYPE_BITWISE_AND:
		iInstr = INSTR_AND_TO;
		break;
	case OP_TYPE_BITWISE_OR:
		iInstr = INSTR_OR_TO;
		break;
	case OP_TYPE_BITWISE_XOR:
		iInstr = INSTR_XOR_TO;
		break;
	case OP_TYPE_MUL:
		iInstr = INSTR_MUL_TO;
		break;
	case OP_TYPE_DIV:
		iInstr = INSTR_DIV_TO;
		break;
	case OP_TYPE_MOD:
		iInstr = INSTR_MOD_TO;
		break;
	case OP_TYPE_EXP:
		iInstr = INSTR_EXP_TO;
		break;
	case OP_TYPE_BITWISE_SHIFT_LEFT:
		iInstr = INSTR_SHL_TO;
		break;
	case OP_TYPE_BITWISE_SHIFT_RIGHT:
		iInstr = INSTR_SHR_TO;
		break;
	case OP_TYPE_EQUAL:
		iInstr = INSTR_TEST_E;
		break;
	case OP_TYPE_NOT_EQUAL:
		iInstr = INSTR_TEST_NE;
		break;
	case OP_TYPE_LESS:
		iInstr = INSTR_TEST_L;
		break;
	case OP_TYPE_GREATER:
		iInstr = INSTR_TEST_G;
		break;
	case OP_TYPE_LESS_EQUAL:
		iInstr = INSTR_TEST_LE;
		break;
	case OP_TYPE_GREATER_EQUAL:
		iInstr = INSTR_TEST_GE;
		break;
	case OP_TYPE_LOGICAL_AND:
		iInstr = INSTR_LOGIC_AND;
		break;
	case OP_TYPE_LOGICAL_OR:
		iInstr = INSTR_LOGIC_OR;
		break;
	default:
		break;
	}

	return iInstr;
}


bool  CParser::ParseFactor(Factor& factor)
{
	bool bNegative = false;
	bool isNot = false;

	int token = mLexer->GetNextToken();
	if (token == TOKEN_TYPE_OP)
	{
		if ((mLexer->GetCurOprType() == OP_TYPE_ADD || mLexer->GetCurOprType() == OP_TYPE_SUB))
		{
			if (mLexer->GetCurOprType() == OP_TYPE_SUB)
				bNegative = true;

			token = mLexer->GetNextToken();
		}
		else if (mLexer->GetCurOprType() == OP_TYPE_LOGICAL_NOT)
		{
			isNot = true;
			token = mLexer->GetNextToken();
		}
	}

	factor.type = FACTOR_INVALID;
	switch(token)
	{
	case TOKEN_TYPE_RSRVD_NIL:
		{
			factor.type = FACTOR_NIL;
		}
		break;
	case TOKEN_TYPE_INT:
		{
			factor.type = FACTOR_INT;
			factor.intValue = atoi(mLexer->GetCurLexeme());
			break;
		}
	case TOKEN_TYPE_FLOAT:
		{
			factor.type = FACTOR_FLOAT;
			factor.floatValue = atof(mLexer->GetCurLexeme());
			break;
		}
	case TOKEN_TYPE_RSRVD_TRUE:
		{
			factor.type  = FACTOR_INT;
			factor.intValue = 1;
			break;
		}
	case TOKEN_TYPE_RSRVD_FALSE:
		{
			factor.type = FACTOR_INT;
			factor.intValue = 0;
			break;
		}
	case TOKEN_TYPE_STRING:
		{
			factor.type = FACTOR_STRING;
			factor.strIndex = mSymbolTable->AddString(mLexer->GetCurLexeme());
			break;
		}
	case TOKEN_TYPE_IDENT:
		{
			mLexer->RewindToken();
			Factor firstRet, secondRet;
			ParserVariableAndFunction(firstRet, secondRet);
			
			if (secondRet.type != FACTOR_INVALID)
			{
				factor.type = FACTOR_TABLE;
				factor.varIndex = firstRet.varIndex;

				if (secondRet.type == FACTOR_INT)
				{
					factor.iOffset = ROT_Int;
					factor.intTableValue = secondRet.intValue;
				}
				else if (secondRet.type == FACTOR_STRING)
				{
					factor.iOffset = ROT_String;
					factor.intTableValue = secondRet.strIndex;
				}
				else if (secondRet.type == FACTOR_FLOAT)
				{
					factor.iOffset = ROT_Float;
					factor.floatValue = secondRet.floatValue;
				}
				else if (secondRet.type == FACTOR_VAR)
				{
					factor.iOffset = ROT_Stack_Index;
					factor.intTableValue = secondRet.varIndex;
				}
				else if (secondRet.type == FACTOR_FUNC)
				{
					factor.iOffset = ROT_Reg;
					factor.intTableValue = secondRet.varIndex;
				}
				else
				{
					Error("invalid factor");
				}
			}
			else
			{
				factor = firstRet;
			}

			break;
		}
	case TOKEN_TYPE_DELIM_OPEN_PAREN:
		{
			factor = ParseExpr(false, true);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

			break;
		}
	case TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE:
		{
			mLexer->RewindToken();
			int tableReg = ParseTableInit();
			factor.type = FACTOR_VAR;
			factor.varIndex = tableReg;
		}
		break;
	case TOKEN_TYPE_RSRVD_FUNC:
		{
			mLexer->RewindToken();
			int freeReg = GetFreeReg();
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
			ParseFunction(factor);
		}
		break;
	break;
	default:
		Error("Error expresion");
		break;
	}

	if (bNegative)
	{
		if (factor.type == FACTOR_NIL)
		{

		}
		if (factor.type == FACTOR_INT)
		{
			factor.intValue = -factor.intValue;
		}
		else if (factor.type == FACTOR_FLOAT)
		{
			factor.floatValue = -factor.floatValue;
		}
		else if (factor.type == FACTOR_STRING)
		{
			Error("error use - on string");
		}
		else
		{
			int freeReg = GetFreeReg();
			int instr = mMidCode->AddInstr(INSTR_NEG);
			mMidCode->AddVarOperand(instr, freeReg);
			AddOperandByFactor(instr, factor);
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
		}
	}
	else if (isNot)
	{
		if (factor.type == FACTOR_INT)
		{
			factor.intValue = factor.intValue == 0;
		}
		else if (factor.type == FACTOR_NIL)
		{
			factor.intValue = 1;
			factor.type = FACTOR_INT;
		}
		else if (factor.type == FACTOR_FLOAT)
		{
			factor.intValue = factor.floatValue == 0.0f;
			factor.type = FACTOR_INT;
		}
		else if (factor.type == FACTOR_STRING)
		{
			Error("error use ! on string");
		}
		else
		{
			int freeReg = GetFreeReg();
			int iInstrIndex = mMidCode->AddInstr(INSTR_LOGIC_NOT);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			AddOperandByFactor(iInstrIndex, factor);
			factor.type = FACTOR_VAR;
			factor.varIndex = freeReg;
		}


	}

	return true;
}

bool  CParser::ParseAssign(bool bEnd)
{
	mLexer->ExpectToken(TOKEN_TYPE_IDENT);
	int symbIndex = mSymbolTable->SearchValue(mLexer->GetCurLexeme(), mCurFuncIndex);

	Factor ret;
	bool isArray = ParserVariable(symbIndex, ret);
	Factor firstRet;
	firstRet.type = FACTOR_VAR;
	firstRet.varIndex = symbIndex;
	ParserAssignRight(firstRet, ret, bEnd);

	return true;
}

void CParser::ParserAssignRight(Factor& firstRet, Factor& tableIndexRet, bool bEnd)
{
	mLexer->ExpectToken(TOKEN_TYPE_OP);
	int oprType = mLexer->GetCurOprType();
	bool isTable = tableIndexRet.type != FACTOR_INVALID;
	bool bIsString = false;
	Factor ret;

	if (firstRet.type == FACTOR_NIL && oprType == OP_TYPE_ASSIGN)
	{
		//nil 常量不能用来赋值
		Error("error use nil");
	}

	bool isTableAssign = false;
	if (oprType != OP_TYPE_INC && oprType != OP_TYPE_DEC)
	{
		if (oprType == OP_TYPE_ASSIGN && mLexer->LookNextToken() == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
		{
			isTableAssign = true;
			if (isTable || firstRet.type != FACTOR_VAR)
			{
				int tableReg = ParseTableInit();
				int instr = mMidCode->AddInstr(INSTR_MOV);
				AddTableFactor(tableIndexRet, instr, firstRet.varIndex);
				FreeFactorReg(tableIndexRet);
				mMidCode->AddVarOperand(instr, tableReg);
				FreeReg(tableReg);
			}
			else
			{
				ParseTableInit(firstRet.varIndex);
			}
			
		}
		else if (oprType == OP_TYPE_ASSIGN && mLexer->LookNextToken() == TOKEN_TYPE_RSRVD_FUNC)
		{
			isTableAssign = true;
			if (isTable || firstRet.type != FACTOR_VAR)
			{
				int freeReg = GetFreeReg();
				Factor factor;
				factor.type = FACTOR_VAR;
				factor.varIndex = freeReg;
				ParseFunction(factor);
				int instr = mMidCode->AddInstr(INSTR_MOV);
				AddTableFactor(tableIndexRet, instr, firstRet.varIndex);
				
				mMidCode->AddVarOperand(instr, freeReg);
				FreeReg(freeReg);
				FreeFactorReg(tableIndexRet);
			}
			else
			{
				ParseFunction(firstRet);
			}

		}
		else
		{
			ret = ParseExpr(false);
		}
	}

	if (!isTableAssign)
	{
		int assignInstrIndex;
		if (oprType == OP_TYPE_ASSIGN)
			assignInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		else if (OP_TYPE_ASSIGN_ADD == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_ADD);
		else if (OP_TYPE_ASSIGN_SUB == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_SUB);
		else if (OP_TYPE_ASSIGN_MUL == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_MUL);
		else if (OP_TYPE_ASSIGN_DIV == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_DIV);
		else if (OP_TYPE_ASSIGN_MOD == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_MOD);
		else if (OP_TYPE_ASSIGN_EXP == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_EXP);
		else if (OP_TYPE_ASSIGN_CONCAT == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_CONCAT);
		else if (OP_TYPE_ASSIGN_AND == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_AND);
		else if (OP_TYPE_ASSIGN_OR == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_OR);
		else if (OP_TYPE_ASSIGN_XOR == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_XOR);
		else if (OP_TYPE_ASSIGN_SHIFT_LEFT == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_SHL);
		else if (OP_TYPE_ASSIGN_SHIFT_RIGHT == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_SHR);
		else if (OP_TYPE_INC == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_INC);
		else if (OP_TYPE_DEC == oprType)
			assignInstrIndex = mMidCode->AddInstr(INSTR_DEC);
		else
			Error("illegal operator");

		if (isTable)
		{
			AddTableFactor(tableIndexRet, assignInstrIndex, firstRet.varIndex);
			FreeFactorReg(tableIndexRet);
		}
		else
			AddOperandByFactor(assignInstrIndex, firstRet);

		if (OP_TYPE_INC != oprType  && OP_TYPE_DEC != oprType)
		{
			AddOperandByFactor(assignInstrIndex, ret);
		}

		FreeFactorReg(ret);
	}

	if (bEnd)
		mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);

	FreeFactorReg(firstRet);
	FreeFactorReg(tableIndexRet);
}

bool IsLogicOp(int op)
{
	return OP_TYPE_LOGICAL_AND == op
		|| OP_TYPE_LOGICAL_OR == op
		|| OP_TYPE_EQUAL == op
		|| OP_TYPE_NOT_EQUAL == op
		|| OP_TYPE_LESS == op
		|| OP_TYPE_GREATER == op
		|| OP_TYPE_LESS_EQUAL == op
		|| OP_TYPE_GREATER_EQUAL == op;
}

CParser::Factor  CParser::ParseExpr(bool push, bool noTable)
{
	std::vector<Factor> retVec;
	std::vector<int>	opVec;
	Factor ret2;
	ParseFactor(ret2);

	bool isFirst = true;

	int freeReg = -1;

	retVec.push_back(ret2);
	while (mLexer->LookNextToken() == TOKEN_TYPE_OP && GetOpInstrTo(mLexer->GetCurOprType()) >= 0)
	{
		mLexer->ExpectToken(TOKEN_TYPE_OP);
		int op = mLexer->GetCurOprType();

		while (opVec.size() > 0 && mLexer->GetOpPrority(op) >= mLexer->GetOpPrority(opVec.back()))
		{
			DoFactorOperation(retVec, opVec);
		}

		if (retVec.size() > 0 && retVec.back().type == OP_TYPE_FUNC)
		{

			int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			int freeReg = GetFreeReg();
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			mMidCode->AddRegOperand(iInstrIndex);

			retVec.back().type = FACTOR_VAR;
			retVec.back().varIndex = freeReg;
		}

		Factor ret2;
		ParseFactor(ret2);

		retVec.push_back(ret2);
		opVec.push_back(op);
	}

	while (opVec.size() > 0)
	{
		DoFactorOperation(retVec, opVec);
	}

	Factor resultRet = retVec[0];

	if (push)
	{
		int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
		AddOperandByFactor(iInstrIndex, resultRet);
		FreeFactorReg(resultRet);
	}
	else
	{
		if (noTable)
		{
			if (resultRet.type == FACTOR_TABLE)
			{
				int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				int freeReg = GetFreeReg();
				mMidCode->AddVarOperand(iInstrIndex, freeReg);
				AddOperandByFactor(iInstrIndex, resultRet);
				FreeFactorReg(resultRet);
				resultRet.type = FACTOR_VAR;
				resultRet.varIndex = freeReg;
			}
		}
	}
	
	return resultRet;
}

void CParser::DoFactorOperation(std::vector<Factor> &retVec, std::vector<int> &opVec)
{
	Factor f2 = retVec.back();
	retVec.pop_back();
	Factor f1 = retVec.back();
	retVec.pop_back();

// 	if (f1.type == f2.type == FACTOR_FUNC)
// 	{
// 		int freeReg = GetFreeReg();
// 		int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
// 		mMidCode->AddVarOperand(iInstrIndex, freeReg);
// 		mMidCode->AddRegOperand(iInstrIndex);
// 
// 		f1.re
// 	}

	int lastOp = opVec.back();
	opVec.pop_back();

	bool isReg = f1.type == FACTOR_VAR && IsRegVar(f1.varIndex);
	Factor ret = f1;
	if (!IsLogicOp(lastOp) && isReg)
	{
		int instr = GetOpInstr(lastOp);
		int iInstrIndex = mMidCode->AddInstr(instr);
		AddOperandByFactor(iInstrIndex, f1);
		AddOperandByFactor(iInstrIndex, f2);
		FreeFactorReg(f2);

	}
	else
	{
		if ((f1.type == FACTOR_FLOAT || f1.type == FACTOR_INT || f1.type == FACTOR_STRING)
			&& (f2.type == FACTOR_FLOAT || f2.type == FACTOR_INT || f2.type == FACTOR_STRING))
		{
			ret = ComputeFactors(lastOp, f1, f2);
		}
		else
		{
			int freeReg = GetFreeReg();
			int instr = GetOpInstrTo(lastOp);
			int iInstrIndex = mMidCode->AddInstr(instr);
			mMidCode->AddVarOperand(iInstrIndex, freeReg);
			AddOperandByFactor(iInstrIndex, f1);
			AddOperandByFactor(iInstrIndex, f2);
			ret.type = FACTOR_VAR;
			ret.varIndex = freeReg;
			FreeFactorReg(f1);
			FreeFactorReg(f2);
		}
	
	}

	retVec.push_back(ret);
}

void  CParser::ParserVariableAndFunction(Factor& firstRet, Factor& secondRet)
{
	mLexer->ExpectToken(TOKEN_TYPE_IDENT);
	std::string firstIdentName = mLexer->GetCurLexeme();

	firstRet.type = FACTOR_VAR;
	firstRet.varIndex = mSymbolTable->SearchValue(firstIdentName.c_str(), mCurFuncIndex);

	bool first = true;

	while (true)
	{
		
		TOKEN nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_OPEN_PAREN)
		{
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);
			int iParamNum = 0;
			int token = mLexer->LookNextToken();
			while (token != TOKEN_TYPE_DELIM_CLOSE_PAREN)
			{
				iParamNum++;
				ParseExpr();
				token = mLexer->LookNextToken();
				if (token == TOKEN_TYPE_DELIM_CLOSE_PAREN)
					break;
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA);
			}

			int instrIndex = mMidCode->AddInstr(INSTR_CALL);

			if (secondRet.type != FACTOR_INVALID)
			{
				AddTableFactor(secondRet, instrIndex, firstRet.varIndex);
			}
			else
			{
				AddOperandByFactor(instrIndex, firstRet);
			}

			mMidCode->AddIntOperand(instrIndex, iParamNum);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

			firstRet.type = FACTOR_FUNC;

			secondRet.type = FACTOR_INVALID;
		}
		else if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE || nextToken == TOKEN_TYPE_DELIM_POINT)
		{ 
			if (firstRet.type == FACTOR_FUNC)
			{
				int instrIndex = mMidCode->AddInstr(INSTR_MOV);
				int freeReg = GetFreeReg();
				mMidCode->AddVarOperand(instrIndex, freeReg);
				mMidCode->AddRegOperand(instrIndex);
				firstRet.type = FACTOR_VAR;
				firstRet.varIndex = freeReg;
			}

			if (secondRet.type != FACTOR_INVALID)
			{
				int iInstrIndex = -1;
				int req = GetFreeReg();
				iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddVarOperand(iInstrIndex, req);
				AddTableFactor(secondRet, iInstrIndex, firstRet.varIndex);
				FreeFactorReg(secondRet);
				FreeFactorReg(firstRet);
				firstRet.type = FACTOR_VAR;
				firstRet.varIndex = req;
			}

			if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
			{
				mLexer->GetNextToken();
				secondRet = ParseExpr(false, true);
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE);
			}
			else
			{
				mLexer->GetNextToken();
				mLexer->ExpectToken(TOKEN_TYPE_IDENT);
				std::string name = mLexer->GetCurLexeme();
				int strIndex = mSymbolTable->AddString(name.c_str());

				secondRet.type = FACTOR_STRING;
				secondRet.strIndex = strIndex;
			}

		}
		else if (nextToken == TOKEN_TYPE_DELIM_COLON)
		{
			bool isStatic = false;
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_COLON);
			if (mLexer->LookNextToken() == TOKEN_TYPE_DELIM_COLON)
			{
				if (!first)
				{
					Error("Invalid call static function");
				}
				mLexer->GetNextToken();
				isStatic = true;
			}
			
			if (!isStatic)
			{
				int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
				if (secondRet.type != FACTOR_INVALID)
				{
					AddTableFactor(secondRet, iInstrIndex, firstRet.varIndex);
				}
				else
				{
					AddOperandByFactor(iInstrIndex, firstRet);
				}
			}

			
			mLexer->ExpectToken(TOKEN_TYPE_IDENT);
			std::string classFuncName = mLexer->GetCurLexeme();

			mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);

			int iParamNum = 0;
			int token = mLexer->LookNextToken();
			while (token != TOKEN_TYPE_DELIM_CLOSE_PAREN)
			{
				iParamNum++;
				ParseExpr();
				token = mLexer->LookNextToken();
				if (token == TOKEN_TYPE_DELIM_CLOSE_PAREN)
					break;
				mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA);
			}
			int nameIndex = mVM->AddString(classFuncName);
			if (isStatic)
			{
				int instrIndex = mMidCode->AddInstr(INSTR_CALL_STATIC_CLASS_FUNC);
				mMidCode->AddFuncOperand(instrIndex, nameIndex, iParamNum);
				int stringIndex = mSymbolTable->AddString(firstIdentName.c_str());
				mMidCode->AddStringIndexOperand(instrIndex, stringIndex);
			}
			else
			{
				iParamNum++;
				int instrIndex = mMidCode->AddInstr(INSTR_CALL_CLASS_FUNC);
				mMidCode->AddFuncOperand(instrIndex, nameIndex, iParamNum);
			}

			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

			firstRet.type = FACTOR_FUNC;
			secondRet.type = FACTOR_INVALID;
		}
		else
		{
			break;
		}

		first = false;
	}
}

bool CParser::ParserVariable(int& symbIndex, Factor& ret)
{
	bool isArray = false;
	int numRefDepth = 0;
	TOKEN token = mLexer->LookNextToken();
	Factor lastRet;
	while (token == TOKEN_TYPE_DELIM_OPEN_BRACE || token == TOKEN_TYPE_DELIM_POINT)
	{
		isArray = true;

		if (numRefDepth > 0)
		{
			int iInstrIndex = -1;
			
			int req = GetFreeReg();
			iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddVarOperand(iInstrIndex, req);

			AddTableFactor(lastRet, iInstrIndex, symbIndex);
			FreeFactorReg(lastRet);

			FreeReg(symbIndex);
			symbIndex = req;
		}

		if (token == TOKEN_TYPE_DELIM_OPEN_BRACE)
		{
			mLexer->GetNextToken();
			lastRet = ParseExpr(false, true);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE);
		}
		else
		{
			mLexer->GetNextToken();
			mLexer->ExpectToken(TOKEN_TYPE_IDENT);
			std::string name = mLexer->GetCurLexeme();
			//int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
			int strIndex = mSymbolTable->AddString(name.c_str());

			lastRet.type = FACTOR_STRING;
			lastRet.strIndex = strIndex;
			//mMidCode->AddStringIndexOperand(iInstrIndex, strIndex);
		}

		token = mLexer->LookNextToken();
		numRefDepth++;
	}

	ret = lastRet;

	return isArray;
}

void CParser::AddTableFactor(Factor &lastRet, int iInstrIndex, int& symbIndex)
{
	if (lastRet.type == FACTOR_INT)
	{
		mMidCode->AddTableIntOperand(iInstrIndex, symbIndex, lastRet.intValue);
	}
	else if (lastRet.type == FACTOR_FLOAT)
	{
		mMidCode->AddTableFloatOperand(iInstrIndex, symbIndex, lastRet.floatValue);
	}
	else if (lastRet.type == FACTOR_STRING)
	{
		mMidCode->AddTableStringOperand(iInstrIndex, symbIndex, lastRet.strIndex);
	}
	else if (lastRet.type == FACTOR_VAR)
	{
		mMidCode->AddTableIndexOperand(iInstrIndex, symbIndex, lastRet.varIndex);
	}
	else if (lastRet.type == FACTOR_FUNC)
	{
		mMidCode->AddTableRegOperand(iInstrIndex, symbIndex, 0);
	}
}

bool  CParser::ParseIf()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_IF);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);
	Factor ret = ParseExpr(false);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2;
	//JE _T0, 0, JumpFALSE
	int iInstrIndex = mMidCode->AddInstr(INSTR_JE);
	AddOperandByFactor(iInstrIndex, ret);
	mMidCode->AddIntOperand(iInstrIndex, 0);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	ParseStateMent();

	bool hasReturnEarly = false;
	bool bHasElse = false;
	if (mLexer->LookNextToken() == TOKEN_TYPE_RSRVD_ELSE)
	{
		int lastOp = mMidCode->getLastInstrOp();
		if (lastOp == INSTR_RET)
			hasReturnEarly = true;

		if (!hasReturnEarly)
		{
			iJumpIndex2 = mMidCode->GetNextJumpIndex();
			iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
			mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);
		}
		bHasElse = true;

	}
	mMidCode->AddJumpTarget(iJumpIndex1);
	if (bHasElse)
	{
		mLexer->ExpectToken(TOKEN_TYPE_RSRVD_ELSE);
		ParseStateMent();
		if (!hasReturnEarly)
		   mMidCode->AddJumpTarget(iJumpIndex2);
	}
	return true;
}
bool  CParser::ParseReturn()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_RETURN);
	int token = mLexer->LookNextToken();
	if (token == TOKEN_TYPE_DELIM_SEMICOLON)
	{
		mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
		int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddRegOperand(iInstrIndex);
		mMidCode->AddIntOperand(iInstrIndex, 0);
	}
	else
	{
		std::vector<Factor> retVec;
		while (true)
		{
			Factor ret = ParseExpr(false);
			retVec.push_back(ret);
			if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
				break;

			mLexer->GetNextToken();
		}
		
		if (retVec.size() >= MAX_FUNC_REG)
		{
			Error("too much return values, the maximum of return value is %d, but now %d", MAX_FUNC_REG, (int)retVec.size());
		}

		mLexer->TestToken(TOKEN_TYPE_DELIM_SEMICOLON);
		
		for (int i = 0; i < retVec.size(); i++)
		{
			int iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
			mMidCode->AddRegOperand(iInstrIndex, i);
			AddOperandByFactor(iInstrIndex, retVec[i]);
		}
		
		mMidCode->AddInstr(INSTR_RET);
	}
	return true;
}

bool  CParser::ParseWhile()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_WHILE);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);
	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();
	mMidCode->AddJumpTarget(iJumpIndex1);
	Factor ret = ParseExpr(false);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

	//int iInstrIndex = mMidCode->AddInstr(INSTR_POP);
	//mMidCode->AddVarOperand(iInstrIndex, mSymbolIndexT0);

	int iInstrIndex = mMidCode->AddInstr(INSTR_JE);
	AddOperandByFactor(iInstrIndex, ret);
	mMidCode->AddIntOperand(iInstrIndex, 0);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);
	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2));
	ParseStateMent();
	mLoopStack.pop();

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);
	return true;
}

bool CParser::ParseBreak()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_BREAK);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
	if (mLoopStack.empty())
		Error("error use \"break\"(it must be used in a loop)");
	int iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, mLoopStack.top().iEndJumpIndex);
	return true;
}

bool CParser::ParseContinue()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_CONTINUE);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
	if (mLoopStack.empty())
		Error("error use \"break\"(it must be used in a loop)");
	int iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, mLoopStack.top().iStartJumpIndex);
	return true;
}

bool  CParser::ParseForeach()
{
	/*
	foreach (var_1, ..., var_n in <explist>) { <block> } -- 就等价于以下代码：
		do
			local _f, _s, _var = <explist>    --返回迭代器函数、恒定状态和控制变量的初值
			while true do
				local var_1, ..., var_n = _f(_s, _var)
				_var = var_1
				if _var == nil then break end
					<block>
					end
					end
					end

	*/
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FOREACH);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);

	std::vector<int>	localValVec;
	while (true)
	{
		mLexer->ExpectToken(TOKEN_TYPE_IDENT);
		std::string identName = mLexer->GetCurLexeme();
		localValVec.push_back(mSymbolTable->AddVariant(identName.c_str(), mCurFuncIndex, 1, IDENT_TYPE_VAR));
		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			break;
		mLexer->GetNextToken();
	}

	if (localValVec.size() > MAX_FUNC_REG)
	{
		Error("too many foreach vars, the maximum is %d, but now %d", MAX_FUNC_REG, (int)localValVec.size());
	}

	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_IN);

	std::vector<Factor> retVec;
	while (true)
	{
		Factor ret = ParseExpr(false);
		retVec.push_back(ret);
		if (mLexer->LookNextToken() != TOKEN_TYPE_DELIM_COMMA)
			break;
		mLexer->GetNextToken();
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);

	char funcVarName[128] = { 0 }, stateVarName[128] = { 0 }, initVarName[128] = { 0 };
	sprintf(funcVarName, "foreach_func_%d", (int)mLoopStack.size());
	sprintf(stateVarName, "foreach_state_%d", (int)mLoopStack.size());
	sprintf(initVarName, "foreach_init_%d", (int)mLoopStack.size());

		int funcVarIndex = mSymbolTable->AddVariant(funcVarName, mCurFuncIndex, 1, IDENT_TYPE_VAR);
	int stateVarIndex = mSymbolTable->AddVariant(stateVarName, mCurFuncIndex, 1, IDENT_TYPE_VAR);
	int initVarIndex = mSymbolTable->AddVariant(initVarName, mCurFuncIndex, 1, IDENT_TYPE_VAR);
	Factor factor1, factor2, factor3;
	factor1.type = FACTOR_VAR;
	factor1.varIndex = funcVarIndex;
	factor2.type = FACTOR_VAR;
	factor2.varIndex = stateVarIndex;
	factor3.type = FACTOR_VAR;
	factor3.varIndex = initVarIndex;

	std::vector<VarData> varVec;
	varVec.push_back(VarData(factor1, Factor()));
	varVec.push_back(VarData(factor2, Factor()));
	varVec.push_back(VarData(factor3, Factor()));
	MultiAssignment(retVec, varVec);

	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();

	mMidCode->AddJumpTarget(iJumpIndex1);

	int iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(iInstrIndex, stateVarIndex);

	iInstrIndex = mMidCode->AddInstr(INSTR_PUSH);
	mMidCode->AddVarOperand(iInstrIndex, initVarIndex);


	iInstrIndex = mMidCode->AddInstr(INSTR_CALL);
	mMidCode->AddVarOperand(iInstrIndex, funcVarIndex);
	mMidCode->AddIntOperand(iInstrIndex, 2);

	for (int i = 0; i < localValVec.size(); i++)
	{
		iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
		mMidCode->AddVarOperand(iInstrIndex, localValVec[i]);
		mMidCode->AddRegOperand(iInstrIndex, i);
	}

	iInstrIndex = mMidCode->AddInstr(INSTR_MOV);
	mMidCode->AddVarOperand(iInstrIndex, initVarIndex);
	mMidCode->AddVarOperand(iInstrIndex, localValVec[0]);

	iInstrIndex = mMidCode->AddInstr(INSTR_JE);
	mMidCode->AddVarOperand(iInstrIndex, initVarIndex);
	mMidCode->AddVarOperand(iInstrIndex, mNilSymbolIndex);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);


	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2));
	ParseStateMent();
	mLoopStack.pop();

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);

	return true;
}

bool  CParser::ParseFor()
{
	mLexer->ExpectToken(TOKEN_TYPE_RSRVD_FOR);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_PAREN);
	ParseAssign();
	int iJumpIndex1 = mMidCode->GetNextJumpIndex();
	int iJumpIndex2 = mMidCode->GetNextJumpIndex();
	mMidCode->AddJumpTarget(iJumpIndex1);
	Factor ret = ParseExpr(false);
	mLexer->ExpectToken(TOKEN_TYPE_DELIM_SEMICOLON);
	//int iInstrIndex = mMidCode->AddInstr(INSTR_POP);
	//mMidCode->AddVarOperand(iInstrIndex, mSymbolIndexT0);

	int iInstrIndex = mMidCode->AddInstr(INSTR_JE);
	AddOperandByFactor(iInstrIndex, ret);
	mMidCode->AddIntOperand(iInstrIndex, 0);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex2);
	mLoopStack.push(LoopStruct(iJumpIndex1, iJumpIndex2));


	beginRecordTokens();
	ParseAssign(false);
	endRecordTokens();

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_PAREN);
	ParseStateMent();
	mLoopStack.pop();

	beginParseFromBuffer();
	ParseAssign(false);
	endParseFromBuffer();

	iInstrIndex = mMidCode->AddInstr(INSTR_JMP);
	mMidCode->AddJumpIndexOperand(iInstrIndex, iJumpIndex1);
	mMidCode->AddJumpTarget(iJumpIndex2);
	return true;
}


int  CParser::ParseTableInit(int initReg)
{
	int tableReg = initReg;
	int instrIndex = -1;
	if (tableReg < 0)
	{
		tableReg = GetFreeReg();
	}

	instrIndex = mMidCode->AddInstr(INSTR_TYPE);
	mMidCode->AddVarOperand(instrIndex, tableReg);
	mMidCode->AddIntOperand(instrIndex, OP_TYPE_TABLE);

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE);

	int tableIndex = 1;

	while (true)
	{
		TOKEN nextToken = mLexer->LookNextToken();

		if (nextToken == TOKEN_TYPE_DELIM_OPEN_BRACE)
		{
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_OPEN_BRACE);
			Factor ret;
			ret = ParseExpr(false, true);
			mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_BRACE);
			mLexer->ExpectToken(TOKEN_TYPE_OP, OP_TYPE_ASSIGN);

			Factor ret2;
			ret2 = ParseExpr(false);
			instrIndex = mMidCode->AddInstr(INSTR_MOV);
			AddTableFactor(ret, instrIndex, tableReg);
			AddOperandByFactor(instrIndex, ret2);

			FreeFactorReg(ret);
			FreeFactorReg(ret2);
		}
		else
		{
			bool needParseExpr = true;
			if (nextToken == TOKEN_TYPE_IDENT)
			{
				std::string identName = mLexer->GetCurLexeme();
				mLexer->GetNextToken();
				nextToken = mLexer->LookNextToken();
				if (nextToken == TOKEN_TYPE_OP && mLexer->GetCurOprType() == OP_TYPE_ASSIGN)
				{
					mLexer->GetNextToken();

					needParseExpr = false;
					Factor ret2;
					ret2 = ParseExpr(false);
					instrIndex = mMidCode->AddInstr(INSTR_MOV);
					int strIndex = mSymbolTable->AddString(identName.c_str());
					mMidCode->AddTableStringOperand(instrIndex, tableReg, strIndex);
					AddOperandByFactor(instrIndex, ret2);
					FreeFactorReg(ret2);
				}
				else
				{
					mLexer->RewindToken();
					nextToken = mLexer->LookNextToken();
				}
			}
			else if (nextToken == TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE)
			{
				needParseExpr = false;

				int subTableReg = ParseTableInit();
				instrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddTableIntOperand(instrIndex, tableReg, tableIndex);
				mMidCode->AddVarOperand(instrIndex, subTableReg);
				tableIndex++;

			}
			 
			if (nextToken != TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE && needParseExpr)
			{
				Factor ret;
				ret = ParseExpr(false);
				instrIndex = mMidCode->AddInstr(INSTR_MOV);
				mMidCode->AddTableIntOperand(instrIndex, tableReg, tableIndex);
				AddOperandByFactor(instrIndex, ret);
				FreeFactorReg(ret);
				tableIndex++;
			}
		}

		nextToken = mLexer->LookNextToken();
		if (nextToken == TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE)
		{
			break;
		}

		mLexer->ExpectToken(TOKEN_TYPE_DELIM_COMMA);
	}

	mLexer->ExpectToken(TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE);
	return tableReg;

}


void CParser::OutPutCode(CMidCode *midCode, CSymbolTable* symbolTable, char* outputName)
{
	FILE *fp = fopen(outputName, "w");
    char  codeText[100];
	for (int iFuncIndex = 0; iFuncIndex < mSymbolTable->mFuncTable.size(); iFuncIndex++)
	{
		int curFuncIndex = mSymbolTable->mFuncTable[iFuncIndex].iIndex;

		sprintf(codeText, "FUNC	%s\r\n", mSymbolTable->mFuncTable[iFuncIndex].funcName);
		fwrite(codeText, 1, strlen(codeText), fp);
		if (iFuncIndex > 0)
		{
			std::string code;
			bool isFirst = true;
			for (int iVarIndex = 0; iVarIndex < symbolTable->mVarTable.size(); iVarIndex++)
			{
				if (symbolTable->mVarTable[iVarIndex].iScope == mSymbolTable->mFuncTable[iFuncIndex].iIndex)
				{
					if (symbolTable->mVarTable[iVarIndex].iType == IDENT_TYPE_PARAM)
					{
						if (isFirst)
							code += "PARAM	";
						else
							code += ",	";
						isFirst = false;
						code += symbolTable->mVarTable[iVarIndex].varName;
					}
					
				}
			}
			code += "\r\n";
			fwrite(code.c_str(), 1, code.size(), fp);
			code = "";
			isFirst = true;
			for (int iVarIndex = 0; iVarIndex < symbolTable->mVarTable.size(); iVarIndex++)
			{
				if (symbolTable->mVarTable[iVarIndex].iScope == mSymbolTable->mFuncTable[iFuncIndex].iIndex)
				{
					if (symbolTable->mVarTable[iVarIndex].iType == IDENT_TYPE_VAR)
					{
						if (isFirst)
							code += "VAR	";
						else
							code += ",	";
						isFirst = false;
						code += symbolTable->mVarTable[iVarIndex].varName;
					}
				}
			}
			code += "\r\n";
			fwrite(code.c_str(), 1, code.size(), fp);
		}
		

		int numCodeSize = mMidCode->mCodeList[curFuncIndex].size();
		//int iCodeStart = mSymbolTable->mFuncTable[iFuncIndex].iCodeStartIndex;
		//int iCodeEnd  = mSymbolTable->mFuncTable[iFuncIndex].iCodeEndIndex;
		for (int iInstrIndex = 0; iInstrIndex < numCodeSize; iInstrIndex++)
		{
			for (int i = 0; i < mMidCode->mJumpList[curFuncIndex].size(); i++)
			{
				if (mMidCode->mJumpList[curFuncIndex][i].codeIndex == iInstrIndex)
				{
					sprintf(codeText, "  Label%d:\r\n", mMidCode->mJumpList[curFuncIndex][i].jumpIndex);
					fwrite(codeText, 1, strlen(codeText), fp);
				}
			}

			

			ICode code = mMidCode->mCodeList[curFuncIndex][iInstrIndex];
			sprintf(codeText, "%d: 	%s", iInstrIndex, GetCodeText(code, curFuncIndex));


			fwrite(codeText, 1, strlen(codeText), fp);
		}
		strcpy(codeText, "END  FUNC\r\n");
		fwrite(codeText, 1, strlen(codeText), fp);
	}
	fclose(fp);
}

char*  CParser::GetCodeText(const ICode  &code, int funcIndex)
{
	char oprText[32] = {0};
	switch(code.code.iCodeOpr)
	{
	case INSTR_CONCAT_TO:
	case INSTR_CONCAT:
		strcpy(oprText, "CONCAT");
		break;
	case INSTR_FUNC:
		strcpy(oprText, "FUNC");
		break;
	case INSTR_TEST_NE:
		strcpy(oprText, "TNE");
		break;
	case INSTR_TEST_E:
		strcpy(oprText, "TE");
		break;
	case INSTR_TEST_G:
		strcpy(oprText, "TG");
		break;
	case INSTR_TEST_L:
		strcpy(oprText, "TL");
		break;
	case INSTR_TEST_GE:
		strcpy(oprText, "TGE");
		break;
	case INSTR_TEST_LE:
		strcpy(oprText, "TLE");
		break;
	case INSTR_LOGIC_AND:
		strcpy(oprText, "LOGICAND");
		break;
	case INSTR_LOGIC_OR:
		strcpy(oprText, "LOGICOR");
		break;
	case INSTR_MOV:
		strcpy(oprText, "MOV");
		break;
	case INSTR_ADD:
	case INSTR_ADD_TO:
		strcpy(oprText, "ADD");
		break;
	case INSTR_SUB_TO:
	case INSTR_SUB:
		strcpy(oprText, "SUB");
		break;
	case INSTR_MUL_TO:
	case INSTR_MUL:
		strcpy(oprText, "MUL");
		break;
	case INSTR_DIV_TO:
	case INSTR_DIV:
		strcpy(oprText, "DIV");
		break;
	case INSTR_MOD_TO:
	case INSTR_MOD:
		strcpy(oprText, "MOD");
		break;
	case INSTR_EXP_TO:
	case INSTR_EXP:
		strcpy(oprText, "EXP");
		break;
	case INSTR_NEG:
		strcpy(oprText, "NEC");
		break;
	case INSTR_INC:
		strcpy(oprText, "INC");
		break;
	case INSTR_DEC:
		strcpy(oprText, "DEC");
		break;
	case INSTR_AND:
		strcpy(oprText, "AND");
		break;
	case INSTR_OR_TO:
	case INSTR_OR:
		strcpy(oprText, "OR");
		break;
	case INSTR_XOR_TO:
	case INSTR_XOR:
		strcpy(oprText, "XOR");
		break;
	case INSTR_NOT:
		strcpy(oprText, "NOT");
		break;
	case INSTR_SHL_TO:
	case INSTR_SHL:
		strcpy(oprText, "SHL");
		break;
	case INSTR_SHR_TO:
	case INSTR_SHR:
		strcpy(oprText, "SHR");
		break;
	case INSTR_JMP:
		strcpy(oprText, "JMP");
		break;
	case INSTR_JE:
		strcpy(oprText, "JE");
		break;
	case INSTR_JNE:
		strcpy(oprText, "JNE");
		break;
	case INSTR_JG:
		strcpy(oprText, "JG");
		break;
	case INSTR_JL:
		strcpy(oprText, "JL");
		break;
	case INSTR_JGE:
		strcpy(oprText, "JGE");
		break;
	case INSTR_JLE:
		strcpy(oprText, "JLE");
		break;
	case INSTR_PUSH:
		strcpy(oprText, "PUSH");
		break;
	case INSTR_POP:
		strcpy(oprText, "POP");
		break;
	case INSTR_CALL:
		strcpy(oprText, "CALL");
		break;
	case INSTR_RET:
		strcpy(oprText, "RET");
		break;
	case INSTR_CALL_CLASS_FUNC:
		strcpy(oprText, "CALLCLSFUNC");
		break;
	case INSTR_CALL_STATIC_CLASS_FUNC:
		strcpy(oprText, "CALLSTATICCLSFUNC");
		break;
	case INSTR_TYPE:
		strcpy(oprText, "TYPE");
		break;
	}

	static char codeText[100];
	memset(codeText, 0, sizeof(codeText));
	strcat(codeText, oprText);
	for (int paramIndex = 0; paramIndex < code.code.oprList.size(); paramIndex++)
	{
		Operand operand = code.code.oprList[paramIndex];
		char operandText[256];
		if (operand.operandType == PST_Int)
		{
			sprintf(operandText, "	%d", operand.iIntValue);
		}
		else if (operand.operandType == PST_Float)
		{
			sprintf(operandText, "	%lf", operand.fFloatValue);
		}
		else if (operand.operandType == PST_String_Index)
		{
			sprintf(operandText, "	\"%s\"", mSymbolTable->mStringTable[operand.iStringIndex].str);
		}
		else if (operand.operandType == PST_Var_Index)
		{
			const char* varName = FindVarNameBySymbolIndex(*mSymbolTable, operand.iSymbolIndex, funcIndex);
			sprintf(operandText, "	%s", varName);
		}
		else if (operand.operandType == PST_Table)
		{
			const char* varName1 = FindVarNameBySymbolIndex(*mSymbolTable, operand.iSymbolIndex, funcIndex);
			switch (operand.tableIndexType)
			{
			case ROT_Float:
			{
				sprintf(operandText, "	%s[%f]", varName1, operand.fFloatTableValue);
			}
			break;
			case ROT_Int:
			{
				sprintf(operandText, "	%s[%d]", varName1, operand.iIntTableValue);
			}
			break;
			case ROT_String:
			{
				sprintf(operandText, "	%s[\"%s\"]", varName1, mSymbolTable->getString(operand.iIntTableValue));
			}
			break;
			case ROT_Stack_Index:
			{
				const char* varName2 = FindVarNameBySymbolIndex(*mSymbolTable, operand.iIntTableValue, funcIndex);
				sprintf(operandText, "	%s[%s]", varName1, varName2);
			}
			break;
			case ROT_Reg:
			{
				sprintf(operandText, "	%s[REG%d]", varName1, operand.iIntTableValue);
			}
			}
		}
		else if (operand.operandType == PST_JumpTarget_Index)
		{
			sprintf(operandText, "	Label%d", operand.iJumppIndex);
		}
		else if (operand.operandType == PST_FuncIndex)
		{
			sprintf(operandText, "	 %s", mVM->GetString((operand.iFunData >> 16)).c_str());
		}
		else if (operand.operandType == PST_Reg)
		{
			sprintf(operandText, "	 REG%d", operand.iRegIndex);
		}
		if (paramIndex < code.code.oprList.size() - 1)
			strcat(operandText, ",");
		strcat(codeText, operandText);
	}

	if (code.code.iCodeOpr == INSTR_FUNC && code.code.oprList.size() > 1)
	{
		FunctionST* func = mSymbolTable->GetFunctionByIndex(funcIndex);
		int subFuncIndex = func->subIndexVec[code.code.oprList[1].iIntValue];
		FunctionST* subFunc = mSymbolTable->GetFunctionByIndex(subFuncIndex);
		std::string desc = " (" + std::string(subFunc->funcName) + ")";
		strcat(codeText, desc.c_str());
	}

	strcat(codeText, "\r\n");

	
	return codeText;
}



char* CParser::FindVarNameBySymbolIndex(CSymbolTable& symbolTable, int iSymbolIndex, int funcIndex)
{
	if (iSymbolIndex & UPVALMASK)
	{
		int searchFuncIndex = funcIndex;
		int upindex = iSymbolIndex - UPVALMASK;

		FunctionST* func = symbolTable.GetFunctionByIndex(funcIndex);
		while (func->upValueVec[upindex].type != VLOCAL && func->parentIndex >= 0)
		{
			upindex = func->upValueVec[upindex].index;
			func = symbolTable.GetFunctionByIndex(func->parentIndex);
		}

		if (func->parentIndex >= 0 && func->upValueVec[upindex].type == VLOCAL)
		{
			VariantST* var = symbolTable.GetVarByIndex(func->upValueVec[upindex].index);
			return var->varName;
		}
		return "";
	}
	else
	{
		VariantST* var = symbolTable.GetVarByIndex(iSymbolIndex);
		return var->varName;
	}
}

void  CParser::insertSaveRegCode()
{
	for (int i = 0; i < (int)mSymbolTable->mFuncTable.size(); i++)
	{
		const FunctionST& func = mSymbolTable->mFuncTable[i];
		int funcIndex = func.iIndex;
		int saveRegs[MAX_REG_NUM];
		int numNeedSaveReg = 0;
		for (int codeIndex = 0; codeIndex < (int)mMidCode->mCodeList[funcIndex].size(); codeIndex++)
		{
			Code code = mMidCode->mCodeList[funcIndex][codeIndex].code;
			for (int op = 0; op < (int)code.oprList.size(); op++)
			{
				Operand operand = code.oprList[op];

				if(operand.operandType == PST_Var_Index)
				{
					int regIndex = -1;
					for (int i = 0; i < MAX_REG_NUM; i++)
					{
						if (mReg[i].mVarIndex == operand.iSymbolIndex)
						{
							regIndex = i;
							break;
						}
					}

					if (regIndex >= 0)
					{
						bool hasFound = false;
						for (int j = 0; j < numNeedSaveReg; j++)
						{
							if (saveRegs[j] == regIndex)
							{
								hasFound = true;
								break;
							}
						}

						if (!hasFound)
						{
							saveRegs[numNeedSaveReg] = regIndex;
							numNeedSaveReg++;
						}
					}
				}
			}
		}

		int codeSize = mMidCode->mCodeList[funcIndex].size();
		vector<ICode>::iterator beginIt = mMidCode->mCodeList[funcIndex].begin();
		vector<ICode>::iterator nextIt = beginIt;
		int curInstrIndex = 0;
		for (int reg = 0; reg < numNeedSaveReg; reg++)
		{
			ICode code;
			code.iCodeType = INSTR_TYPE_CODE;
			code.code.iCodeOpr = INSTR_PUSH;
			Operand operand;
			operand.operandType = PST_Var_Index;
			operand.iSymbolIndex = mReg[saveRegs[reg]].mVarIndex;
			code.lineIndex = -1;
			code.code.oprList.push_back(operand);
			nextIt = mMidCode->mCodeList[funcIndex].insert(nextIt, code);
		}
		updateSymbolsOffset(curInstrIndex, numNeedSaveReg, funcIndex);
		curInstrIndex += numNeedSaveReg;
		int numOrginalCode = 0;
		nextIt += numNeedSaveReg;

		int numInstrs = codeSize;
		for (int j = 0; j < numInstrs; j++)
		{
			ICode instr = *nextIt;
			if (instr.code.iCodeOpr == INSTR_RET)
			{

				for (int reg = 0; reg < numNeedSaveReg; reg++)
				{
					ICode code;
					code.iCodeType = INSTR_TYPE_CODE;
					code.code.iCodeOpr = INSTR_POP;
					Operand operand;
					operand.operandType = PST_Var_Index;
					code.lineIndex = -1;
					operand.iSymbolIndex = mReg[saveRegs[numNeedSaveReg - reg - 1]].mVarIndex;
					code.code.oprList.push_back(operand);
					nextIt = mMidCode->mCodeList[funcIndex].insert(nextIt, code);
				}

				updateSymbolsOffset(curInstrIndex, numNeedSaveReg, funcIndex);
				curInstrIndex += numNeedSaveReg;

				nextIt += numNeedSaveReg;
			}
			nextIt++;
			curInstrIndex++;
		}
		
	}
}

void   CParser::updateSymbolsOffset(int startIndex, int offset, int funcIndex)
{
	for (int i = 0; i < (int)mMidCode->mJumpList[funcIndex].size(); i++)
	{
		if (mMidCode->mJumpList[funcIndex][i].codeIndex > startIndex)
			mMidCode->mJumpList[funcIndex][i].codeIndex += offset;
	}
}


void  CParser::beginRecordTokens()
{
	mLexer->beginRecordToken();
	mMidCode->setIngoreInstr(true);
}


void  CParser::endRecordTokens()
{
	mLexer->endRecordToken();
	mMidCode->setIngoreInstr(false);
}


void  CParser::beginParseFromBuffer()
{
	mLexer->beginReadTokenFromRecord();
}


void  CParser::endParseFromBuffer()
{
	mLexer->endReadTokenFromRecord();
}

bool	IsLogicJmp(int op)
{
	return (op == INSTR_JE || op == INSTR_JNE || op == INSTR_JG || op == INSTR_JL || op == INSTR_JGE || op == INSTR_JLE);
}

bool	IsLogicTest(int op)
{
	return (op == INSTR_TEST_E || op == INSTR_TEST_NE || op == INSTR_TEST_G || op == INSTR_TEST_L || op == INSTR_TEST_GE || op == INSTR_TEST_LE);
}

void  CParser::optimizeCode()
{
	bool hasOptimize = true;

	while(hasOptimize)
	{
		hasOptimize = false;
		for (int i = 0; i < (int)mSymbolTable->mFuncTable.size(); i++)
		{
			FunctionST& func = mSymbolTable->mFuncTable[i];
			int iFuncIndex = func.iIndex;
			int numNeedSaveReg = 0;
			for (int codeIndex = 0; (int)codeIndex < mMidCode->mCodeList[iFuncIndex].size(); codeIndex++)
			{
				Code code = mMidCode->mCodeList[iFuncIndex][codeIndex].code;
				if (code.iCodeOpr == INSTR_PUSH && code.oprList[0].operandType != PST_Reg)
				{

					int nextPopIndex = codeIndex + 1;
					if (nextPopIndex < mMidCode->mCodeList[iFuncIndex].size())
					{
						Code& nextCode = mMidCode->mCodeList[iFuncIndex][nextPopIndex].code;
						if (nextCode.iCodeOpr == INSTR_POP)
						{
							bool hasJumpTarget = false;
							for (int c = codeIndex + 1; c <= nextPopIndex; c++)
							{
								if (isJumpTarget(c, iFuncIndex))
								{
									hasJumpTarget = true;
									break;
								}
							}

							if (!hasJumpTarget && !hasOperandBeenUsed(code.oprList[0], codeIndex + 1, nextPopIndex, iFuncIndex))
							{
								updateSymbolsOffset(codeIndex, -1, iFuncIndex);
								mMidCode->mCodeList[iFuncIndex].erase(mMidCode->mCodeList[iFuncIndex].begin() + codeIndex);
								mMidCode->mCodeList[iFuncIndex][nextPopIndex - 1].code.iCodeOpr = INSTR_MOV;
								mMidCode->mCodeList[iFuncIndex][nextPopIndex - 1].code.oprList.push_back(code.oprList[0]);
								codeIndex--;
								hasOptimize = true;
							}
						}
					}
				
				}
				else if (code.iCodeOpr == INSTR_MOV && code.oprList[1].operandType == PST_Var_Index && IsRegVar(code.oprList[1].iSymbolIndex) && codeIndex > 0)
				{
					Code lastCode = mMidCode->mCodeList[iFuncIndex][codeIndex - 1].code;
					if (lastCode.oprList[0].operandType == PST_Var_Index && code.oprList[1].iSymbolIndex == lastCode.oprList[0].iSymbolIndex
						&& !isJumpTarget(codeIndex, iFuncIndex))
					{
						if (lastCode.iCodeOpr == INSTR_ADD_TO ||
							lastCode.iCodeOpr == INSTR_SUB_TO ||
							lastCode.iCodeOpr == INSTR_MUL_TO ||
							lastCode.iCodeOpr == INSTR_MOD_TO ||
							lastCode.iCodeOpr == INSTR_DIV_TO ||
							lastCode.iCodeOpr == INSTR_EXP_TO ||
							lastCode.iCodeOpr == INSTR_AND_TO ||
							lastCode.iCodeOpr == INSTR_OR_TO ||
							lastCode.iCodeOpr == INSTR_XOR_TO ||
							lastCode.iCodeOpr == INSTR_NOT_TO ||
							lastCode.iCodeOpr == INSTR_SHL_TO ||
							lastCode.iCodeOpr == INSTR_SHR_TO ||
							lastCode.iCodeOpr == INSTR_CONCAT_TO ||
							lastCode.iCodeOpr == INSTR_TEST_E ||
							lastCode.iCodeOpr == INSTR_TEST_NE ||
							lastCode.iCodeOpr == INSTR_TEST_G ||
							lastCode.iCodeOpr == INSTR_TEST_L ||
							lastCode.iCodeOpr == INSTR_TEST_GE ||
							lastCode.iCodeOpr == INSTR_TEST_LE ||
							lastCode.iCodeOpr == INSTR_LOGIC_AND ||
							lastCode.iCodeOpr == INSTR_LOGIC_OR)
						{
							updateSymbolsOffset(codeIndex, -1, iFuncIndex);
							mMidCode->mCodeList[iFuncIndex].erase(mMidCode->mCodeList[iFuncIndex].begin() + codeIndex);
							codeIndex--;
							mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[0] = code.oprList[0];
						}
					}

				}
				else if (!isJumpTarget(codeIndex, iFuncIndex) && codeIndex > 0 && code.iCodeOpr == INSTR_JE && code.oprList[0].operandType == PST_Var_Index && IsRegVar(code.oprList[0].iSymbolIndex) && code.oprList[1].operandType == PST_Int && code.oprList[1].iIntValue == 0)
				{
					Code lastCode = mMidCode->mCodeList[iFuncIndex][codeIndex - 1].code;
					if (IsLogicTest(lastCode.iCodeOpr) && lastCode.oprList[0].operandType == PST_Var_Index && IsRegVar(lastCode.oprList[0].iSymbolIndex) && lastCode.oprList[0].iSymbolIndex == code.oprList[0].iSymbolIndex)
					{

						updateSymbolsOffset(codeIndex, -1, iFuncIndex);

						int instrOp = -1;
						switch (lastCode.iCodeOpr)
						{
							case INSTR_TEST_E:
								instrOp = INSTR_JNE;
								break;
							case INSTR_TEST_NE:
								instrOp = INSTR_JE;
								break;
							case INSTR_TEST_G:
								instrOp = INSTR_JLE;
								break;
							case INSTR_TEST_L:
								instrOp = INSTR_JGE;
								break;
							case INSTR_TEST_GE:
								instrOp = INSTR_JL;
								break;
							case INSTR_TEST_LE:
								instrOp = INSTR_JG;
								break;
						}

						mMidCode->mCodeList[iFuncIndex].erase(mMidCode->mCodeList[iFuncIndex].begin() + codeIndex);
						codeIndex--;

						mMidCode->mCodeList[iFuncIndex][codeIndex].code.iCodeOpr = instrOp;
						mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[0] = lastCode.oprList[1];
						mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[1] = lastCode.oprList[2];
						mMidCode->mCodeList[iFuncIndex][codeIndex].code.oprList[2] = code.oprList[2];
					}
				}
			}
		}
	}

}

bool  CParser::isJumpTarget(int instrIndex, int funcIndex)
{
	int numJumpTarget = mMidCode->mJumpList[funcIndex].size();
	for (int i  = 0; i < numJumpTarget; i++)
	{
		if (mMidCode->mJumpList[funcIndex][i].codeIndex == instrIndex)
			return true;
	}

	return false;
}

bool  CParser::hasOperandBeenUsed(const Operand& op, int fromIndex, int endIndex, int funcIndex)
{

	if (op.operandType == PST_Int
		|| op.operandType == PST_Float
		|| op.operandType == PST_String_Index
		|| op.operandType == PST_FuncIndex
		|| op.operandType == PST_JumpTarget_Index)
	{
		return false;
	}

	for (int codeIndex = fromIndex; codeIndex < endIndex; codeIndex++)
	{
		Code& code = mMidCode->mCodeList[funcIndex][codeIndex].code;
		switch(code.iCodeOpr)
		{
		case INSTR_MOV:
		case INSTR_ADD:
		case INSTR_SUB:
		case INSTR_MUL:
		case INSTR_DIV:
		case INSTR_MOD:
		case INSTR_EXP:
		case INSTR_NEG:
		case INSTR_INC:
		case INSTR_DEC:
		case INSTR_AND:
		case INSTR_OR:
		case INSTR_XOR:
		case INSTR_NOT:
		case INSTR_SHL:
		case INSTR_SHR:
		case INSTR_ADD_TO:
		case INSTR_SUB_TO:
		case INSTR_MUL_TO:
		case INSTR_DIV_TO:
		case INSTR_MOD_TO:
		case INSTR_EXP_TO:
		case INSTR_AND_TO:
		case INSTR_OR_TO:
		case INSTR_XOR_TO:
		case INSTR_NOT_TO:
		case INSTR_SHL_TO:
		case INSTR_SHR_TO:
		case INSTR_CONCAT_TO:
		case INSTR_TEST_E:
		case INSTR_TEST_NE:
		case INSTR_TEST_G:
		case INSTR_TEST_L:
		case INSTR_TEST_GE:
		case INSTR_TEST_LE:
		case INSTR_LOGIC_AND:
		case INSTR_LOGIC_OR:
			{
				if (isOperandRelated(op, code.oprList[0]))
					return true;
			}
		}
	}

	return false;
}

bool  CParser::isOperandRelated(const Operand& op1, const Operand& op2)
{
	_ASSERT(op1.operandType == PST_Table  ||op1.operandType == PST_Var_Index || op1.operandType == PST_Reg);
	_ASSERT(op2.operandType == PST_Table ||op2.operandType == PST_Var_Index || op2.operandType == PST_Reg);

	bool op1IsVar = (op1.operandType == PST_Var_Index || op1.operandType == PST_Table);
	bool op2IsVar = (op2.operandType == PST_Var_Index || op2.operandType == PST_Table);
	if (op1IsVar != op2IsVar)
		return false;
	if (!op1IsVar)
		return true;
	return op1.iSymbolIndex == op2.iSymbolIndex;
}