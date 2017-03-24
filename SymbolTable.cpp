#include "SymbolTable.h"
#include "Commonfunc.h"
CSymbolTable::CSymbolTable(void)
{
	mGlobalDataSize = 1;
	mVarIndex = 0;
	mStringIndex = 0;
	mFuncIndex = 0;
}

CSymbolTable::~CSymbolTable(void)
{
}


int   CSymbolTable::AddFunction(const char* pFuncName, int iParamNum, int parentFuncIndex)
{
	FunctionST func;
	strncpy_s(func.funcName,  pFuncName, MAX_IDENT_SIZE);
	func.iParamSize = iParamNum;
	func.localDataSize = 0;
	func.curParamIndex = 0;
	func.curVarIndex = 0;
	func.iIndex = mFuncIndex;
	func.hasVarArgs = false;
	func.parentIndex = parentFuncIndex;
	mFuncIndex++;
	mFuncTable.push_back(func);
	return func.iIndex;
}

void  CSymbolTable::SetHasVarArgs(int funcIndex)
{
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		if (mFuncTable[i].iIndex == funcIndex)
			mFuncTable[i].hasVarArgs = true;
	}
}

void	CSymbolTable::GetFunctionVars(int scope, std::vector<std::string>& varVec)
{
	for (int i = 0; i < (int)mVarTable.size(); i++)
	{
		if (mVarTable[i].iScope == scope)
			varVec.push_back(mVarTable[i].varName);
	}
}

int   CSymbolTable::AddVariant(const char* pVarName, int iScope, int iSize, int iType)
{
	if (strlen(pVarName) > 0)
	{
		VariantST* lastVar = GetVarByName(pVarName, iScope);
		if (lastVar != NULL)
		{
			return lastVar->iIndex;
		}
	}
	

	VariantST var;
	var.iScope = iScope;
	var.iSize = iSize;
	var.iType = iType;
	strncpy_s(var.varName,  pVarName, MAX_IDENT_SIZE);

	var.iIndex = mVarIndex;
	mVarIndex++;

	if (iType == IDENT_TYPE_VAR)
	{
		if (iScope >= 0)
		{
			FunctionST* func = GetFunctionByIndex(iScope);
			func->localDataSize += 1;
		}

	}
	mVarTable.push_back(var);
	return var.iIndex;
}

int   CSymbolTable::AddString(const char* str)
{
	for (int i = 0; i < (int)mStringTable.size(); i++)
	{
		if (strcmp(mStringTable[i].str, str) == 0)
			return i;
	}


	StringST strST;
	strncpy_s(strST.str,  str, MAX_STRING_SIZE);
	strST.iIndex = mStringIndex;
	mStringTable.push_back(strST);
	mStringIndex++;
	return strST.iIndex;
}


char* CSymbolTable::getString(int index)
{
	return mStringTable[index].str;
}

FunctionST*   CSymbolTable::GetFunctionByName(const char* pFuncName)
{
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		if (strcmp(mFuncTable[i].funcName, pFuncName) == 0)
			return &mFuncTable[i];
	}
	return NULL;
}


int	 CSymbolTable::SearchValue(const char* pVarName, int iScope)
{
	int index = -1;
	int type = SearchUpValue(pVarName, iScope, index);
	if (type == VUPVALUE)
	{
		return (index | UPVALMASK);
	}

	return index;
}

int	CSymbolTable::SearchUpValue(const char* pVarName, int iScope, int& index)
{
	for (int i = 0; i < (int)mVarTable.size(); i++)
	{
		if (iScope == mVarTable[i].iScope && strcmp(mVarTable[i].varName, pVarName) == 0)
		{
			index = mVarTable[i].iIndex;
			break;
		}
	}

	if (index >= 0)
	{
		return VLOCAL;
	}
	else
	{
		FunctionST* func = GetFunctionByIndex(iScope);
		if (func->parentIndex >= 0)
		{
			int type = SearchUpValue(pVarName, func->parentIndex, index);
			if (type == VGLOBAL)
			{
				return VGLOBAL;
			}
			else
			{
				int newIndex = -1;
				for (int i = 0; i < (int)func->upValueVec.size(); i++)
				{
					if (type == func->upValueVec[i].type
						&& index == func->upValueVec[i].index)
					{
						newIndex = i;
						break;
					}
				}
				if (newIndex < 0)
				{
					UpValueST up(type, index);
					newIndex = func->upValueVec.size();
					func->upValueVec.push_back(up);
				}

				index = newIndex;
				return VUPVALUE;
			}
		}
		else
		{
			index = AddVariant(pVarName, -1);
			return VGLOBAL;
		}
		
	}
}


VariantST*    CSymbolTable::GetVarByName(const char* pVarName, int iScope, bool global)
{
	for (int i = 0; i < (int)mVarTable.size(); i++)
	{
		if (iScope == mVarTable[i].iScope && strcmp(mVarTable[i].varName, pVarName) == 0)
			return &mVarTable[i];
	}

	if (global)
	{
		for (int i = 0; i < (int)mVarTable.size(); i++)
		{
			if (mVarTable[i].iScope < 0 && strcmp(mVarTable[i].varName, pVarName) == 0)
				return &mVarTable[i];
		}
	}

	return NULL;
}

FunctionST*   CSymbolTable::GetFunctionByIndex(int iIndex)
{
	for (int i = 0; i < (int)mFuncTable.size(); i++)
	{
		if (mFuncTable[i].iIndex == iIndex)
			return &mFuncTable[i];
	}
	return NULL;
}

void  CSymbolTable::AddSubFunction(int funcIndex, int subFuncIndex)
{
	FunctionST *func = GetFunctionByIndex(funcIndex);
	if (func != NULL)
		func->subIndexVec.push_back(subFuncIndex);
}



VariantST*    CSymbolTable::GetVarByIndex(int iIndex)
{
	for (int i = 0; i < (int)mVarTable.size(); i++)
	{
		if (mVarTable[i].iIndex == iIndex)
			return &mVarTable[i];
	}
	return NULL;
}
void          CSymbolTable::SetFunctionParamNum(int iIndex, int iParamNum)
{
	FunctionST *func =  GetFunctionByIndex(iIndex);
	if (func != NULL)
		func->iParamSize = iParamNum;
}


void  CSymbolTable::computeParmamStackIndex()
{
	for (int i = 0; i < (int)mVarTable.size(); i++)
	{
		if(mVarTable[i].iType == IDENT_TYPE_PARAM && mVarTable[i].iScope >= 0)
		{
			FunctionST *func =  GetFunctionByIndex(mVarTable[i].iScope);
			mVarTable[i].stackIndex = -(func->localDataSize + func->iParamSize - func->curParamIndex);
			func->curParamIndex++;
		}
		else if (mVarTable[i].iType == IDENT_TYPE_VAR && mVarTable[i].iScope >= 0)
		{
			FunctionST *func = GetFunctionByIndex(mVarTable[i].iScope);
			mVarTable[i].stackIndex = -func->localDataSize + func->curVarIndex;
			func->curVarIndex++;
		}
	}
}


