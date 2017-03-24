#include "XSriptVM.h"


void XScriptVM::ExecInstr_JMP()
{
	ResolveOpPointer(0, destValue);
	if (destValue.type == ROT_Instr_Index)
	{
		mCurXScriptState->mInstrIndex = destValue.iInstrIndex;
	}
	else
	{
		ExecError("op type is not INSTR index");
	}
}

void XScriptVM::ExecInstr_Concat()
{
	ResolveOpPointer(0, firstValue);
	ResolveOpPointer(1, secondValue);
	Value resultValue;
	if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			int strLength = stringRawLen(&firstValue) + stringRawLen(&secondValue);
			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(&firstValue), stringRawLen(&firstValue));
			memcpy(mStrBuffer + stringRawLen(&firstValue), stringRawValue(&secondValue), stringRawLen(&secondValue));
			resultValue.type = OP_TYPE_STRING;
			resultValue.stringValue = NewXString(mStrBuffer, strLength);
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
			resultValue.type = OP_TYPE_STRING;
			resultValue.stringValue = NewXString(mStrBuffer, strLength);
		}
	}
	
	if (IsValueNil(&resultValue))
	{
		if (!CalByTagMethod(&resultValue, &firstValue, &secondValue, MMT_Concat))
		{	
			EXEC_OP_ERROR($, 0, 1);
		}	
	}

	SetOpValue(0, resultValue);
}


void XScriptVM::ExecInstr_RET()
{
	if (mAllowHook && (mHookMask & MASK_HOOKCALL))
	{
		CallHookFunction(HE_HookRet, -1);
	}

	mCurXScriptState->mTopIndex -= (mCurXScriptState->mCurFunctionState->stackFrameSize);
	RemoveStackUpVals(mCurXScriptState->mTopIndex);

	int lastInstrIndex = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mInstrIndex;
	mCurXScriptState->mCurCallIndex--;
	if (mCurXScriptState->mCurCallIndex >= 0)
	{
		mCurXScriptState->mCurFunction = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mCurFunction;
		mCurXScriptState->mCurFunctionState = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mCurFunctionState;

		if (mCurXScriptState->mCurFunctionState != NULL)
			mCurXScriptState->mInstrIndex = lastInstrIndex;
		mCurXScriptState->mFrameIndex = mCurXScriptState->mCallInfoBase[mCurXScriptState->mCurCallIndex].mFrameIndex;
	}
	else
	{
		mCurXScriptState->mCurFunction = NULL;
		mCurXScriptState->mCurFunctionState = NULL;
		mCurXScriptState->mFrameIndex = 0;
	}
}



void XScriptVM::ExecInstr_Neg()
{
	Value secondValue = resolveOpValue(1);
	if (secondValue.type == OP_TYPE_INT)
	{
		SetOpValue(0, ConstructValue((XInt)-secondValue.iIntValue));
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		SetOpValue(0, ConstructValue((XFloat)-secondValue.fFloatValue));
	}
	else 
	{
		Value resultValue;	
		if(CalByTagMethod(&resultValue, &secondValue, NULL, MMT_Neg))
		{
			SetOpValue(0, resultValue);
		}
		else
		{
			ExecError("attempt to perform - operator on  %s", GetOperatorName(1).c_str());
		}
	}
}

void XScriptVM::ExecInstr_Logic_Not()
{
	XInt iRet = 0;
	const Value& secondValue = resolveOpValue(1);
	if (secondValue.type == OP_TYPE_INT)
	{
		iRet = (secondValue.iIntValue == 0) ? 1 : 0;
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		iRet = (secondValue.fFloatValue == 0.0f) ? 1 : 0;
	}
	else
	{
		iRet = (secondValue.type == OP_TYPE_NIL) ? 1 : 0;
	}
	SetOpValue(0, ConstructValue(iRet));
}



void XScriptVM::ExecInstr_Logic_And()
{
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);
	XInt value = 0;
	if (firstValue.type == OP_TYPE_INT)
	{
		value = ((firstValue.iIntValue != 0) ? 1 : 0);
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		value = ((firstValue.fFloatValue != 0) ? 1 : 0);
	}
	else if (IsUserType(firstValue.type))
	{
		value = ((firstValue.lightUserData != NULL) ? 1 : 0);
	}
	else
	{
		ExecError("attempt to perform && operator on", getTypeName(firstValue.type));
	}

	if (value == 1)
	{
		if (secondValue.type == OP_TYPE_INT)
		{
			value = ((secondValue.iIntValue != 0) ? 1 : 0);
		}
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			value = ((secondValue.fFloatValue != 0) ? 1 : 0);
		}
		else if (IsUserType(secondValue.type))
		{
			value = ((secondValue.lightUserData != NULL) ? 1 : 0);
		}
		else
		{
			ExecError("attempt to perform && operator on", getTypeName(secondValue.type));
		}
	}
	SetOpValue(0, ConstructValue(value));
}

void XScriptVM::ExecInstr_Logic_Or()
{
	const Value& firstValue = resolveOpValue(1);
	const Value& secondValue = resolveOpValue(2);
	XInt value = 0;
	if (firstValue.type == OP_TYPE_INT)
	{
		value = ((firstValue.iIntValue != 0) ? 1 : 0);
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		value = ((firstValue.fFloatValue != 0) ? 1 : 0);
	}
	else if (IsUserType(firstValue.type))
	{
		value = ((firstValue.lightUserData != NULL) ? 1 : 0);
	}
	else
	{
		ExecError("attempt to perform || operator on", GetOperatorName(1).c_str());
	}

	if (value == 0)
	{
		if (secondValue.type == OP_TYPE_INT)
		{
			value = ((secondValue.iIntValue != 0) ? 1 : 0);
		}
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			value = ((secondValue.fFloatValue != 0) ? 1 : 0);
		}
		else if (IsUserType(secondValue.type))
		{
			value = ((secondValue.lightUserData != NULL) ? 1 : 0);
		}
		else
		{
			ExecError("attempt to perform || operator on", GetOperatorName(2).c_str());
		}
	}

	SetOpValue(0, ConstructValue(value));
}
