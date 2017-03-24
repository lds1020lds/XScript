#pragma once
class CLexer;
class CSymbolTable;
class CMidCode;
class CSourceFile;
class XScriptVM;
struct ICode;
#include <stack>
#include "Commonfunc.h"
class CParser
{
private:
	friend class XScriptVM;

	struct  LoopStruct
	{
		int iStartJumpIndex;
		int iEndJumpIndex;
		LoopStruct(int iStartIndex, int iEndIndex)
		{
			iStartJumpIndex = iStartIndex;
			iEndJumpIndex   = iEndIndex;
		}
		LoopStruct(){}
	};

	enum 
	{
		MAX_REG_NUM = 16
	};

	enum EFactorType
	{
		FACTOR_INVALID,
		FACTOR_INT, 
		FACTOR_FLOAT,
		FACTOR_VAR,
		FACTOR_TABLE,
		FACTOR_FUNC,
		FACTOR_STRING,
		FACTOR_NIL,
	};
	struct Factor
	{
		EFactorType type;
		union
		{
			XInt		intValue;
			XFloat		floatValue;
			int			varIndex;
			int			strIndex;
		};
		
		int iOffset;
		union
		{
			XInt		intTableValue;
			XFloat		fTableValue;
		};

		Factor()
		{
			type = FACTOR_INVALID;
		}
	};

	struct FactorComputeResult
	{
		int valueType;
		union
		{
			float floatValue;
			int   intValue;
		};
	};

	struct REG
	{
		int mVarIndex;
		bool bUsed;
		int mLastIndex;
	};

	struct VarData
	{
		Factor first;
		Factor second;

		VarData(const Factor& _first, const Factor& _second)
			: first(_first), second(_second)
		{
		}
	};

	XScriptVM*		mVM;
	CLexer*			mLexer;
	CSymbolTable*	mSymbolTable;
	CMidCode*		mMidCode;
	int				mCurFuncIndex;
	int				mNilSymbolIndex;
	REG				mReg[MAX_REG_NUM];
	bool			mIsRegInUse;
	Factor*			mRegFactor;
	std::stack<LoopStruct>  mLoopStack;

	int			  mLastFreeIndex;

	Factor ComputeFactors(int computeWay, Factor const& factor1, Factor const & factor2);
	void   AddOperandByFactor(int iInstrIndex, Factor ret);
	int    GetFreeReg();
	void   FreeReg(int reg);
	void   insertSaveRegCode();
	void   updateSymbolsOffset(int startIndex, int offset, int funcIndex);
public:
	CParser(void);
	~CParser(void); 

	void  Error(const char *errInfo, ...);
	void  ErrorIdentRedefine(const char* ident);

	bool  ParseFile(CSourceFile* sourefile, XScriptVM* vm, CMidCode* midCode, CSymbolTable* symbol);
	bool  ParseStateMent();

	void ParserIdent();

	void MultiAssignment(std::vector<Factor> &retVec, std::vector<VarData> &varVec);

	bool  ParseBlock();
	bool  ParseVar( );
	bool  ParseFunction(Factor assignFactor, bool isLocal = false);
	Factor  ParseExpr(bool push = true, bool noTable = false);

	void DoFactorOperation(std::vector<Factor> &retVec, std::vector<int> &opVec);
	bool  ParseFactor(Factor & factor);


	void  ParserVariableAndFunction(Factor& firstRet, Factor& secondRet);

	bool  ParseAssign(bool bEnd = true);

	void ParserAssignRight(Factor& firstRet, Factor& tableIndexRet, bool bEnd);

	bool	ParserVariable(int& symbIndex, Factor& ret);

	void AddTableFactor(Factor &lastRet, int iInstrIndex, int& symbIndex);

	bool  ParseIf();
	bool  ParseReturn();
	bool  ParseWhile();
	bool  ParseBreak();
	bool  ParseContinue();
	bool  ParseFor();
	int   ParseTableInit( int initReg = -1 );
	bool  ParseForeach();

	void  beginRecordTokens();
	void  endRecordTokens();
	void  beginParseFromBuffer();
	void  endParseFromBuffer();

	bool  isJumpTarget(int instrIndex, int funcIndex);
	bool  hasOperandBeenUsed(const Operand& op, int fromIndex, int endIndex, int funcIndex);
	bool  isOperandRelated(const Operand& op1, const Operand& op2);
	bool  IsRegVar(int varIndex);
	void  AddAssignCode(VarData& var, Factor& expr);

public:
	static char* FindVarNameBySymbolIndex(CSymbolTable& symbolTable,  int symbolIndex, int funcIndex);

	void  OutPutCode(CMidCode *, CSymbolTable*, char*);
	char* GetCodeText(const ICode  &code, int funcIndex);
	void  optimizeCode();
	void		FreeFactorReg(const Factor& factor);

};
