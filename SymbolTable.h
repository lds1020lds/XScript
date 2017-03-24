#pragma once

#include "Commonfunc.h"
#include <vector>

class CMidCode;
class CParser;
using namespace std;
class CSymbolTable
{
friend class XScriptVM;
friend class CParser;
public:
	CSymbolTable(void);
	~CSymbolTable(void);
	static CSymbolTable * GetInstance()
	{
		static CSymbolTable instance;
		return &instance;
	}

	

	void			SetHasVarArgs(int funcIndex);
	int				AddFunction(const char* pFuncName, int iParamNum = 0, int parentFuncIndex = 0);
	void			AddSubFunction(int funcIndex, int subFuncIndex);

	int				AddVariant(const char* pVarName, int iScope = 0, int iSize = 1, int iType = 0);
	int				AddString(const char* str);
	char*			getString(int index);

	void			 SetFunctionParamNum(int iIndex, int iParamNum);
	FunctionST*		 GetFunctionByName(const char* pFuncName);

	VariantST*		 GetVarByName(const char* pVarName, int iScope, bool global = false);
	FunctionST*		 GetFunctionByIndex(int iIndex);

	int				 SearchValue(const char* pVarName, int iScope);
	int				 SearchUpValue(const char* pVarName, int iScope, int& index);

	void			 GetFunctionVars(int scope, std::vector<std::string>& varVec);



	VariantST*		GetVarByIndex(int iIndex);
	void			computeParmamStackIndex();
private:

	vector<VariantST>   mVarTable;
	vector<FunctionST>  mFuncTable;
	vector<StringST>    mStringTable;
	int                 mGlobalDataSize;

	int					mStringIndex;
	int					mVarIndex;
	int					mFuncIndex;
	friend class CParser;
};
