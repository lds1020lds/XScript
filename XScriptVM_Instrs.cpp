#include "XSriptVM.h"




void XScriptVM::ExecInstr_JMP()
{
	ResolveOpPointer(0, destValue);
	if (destValue->type == ROT_Instr_Index)
	{
		mCurXScriptState->mInstrIndex = destValue->iInstrIndex;
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

	if (firstValue->type == OP_TYPE_STRING)
	{
		if (secondValue->type == OP_TYPE_STRING)
		{

			int strLength = stringRawLen(firstValue) + stringRawLen(secondValue) + 1;

			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(firstValue), stringRawLen(firstValue));
			memcpy(mStrBuffer + stringRawLen(firstValue), stringRawValue(secondValue), stringRawLen(secondValue));
			mStrBuffer[stringRawLen(firstValue) + stringRawLen(secondValue)] = '\0';

			firstValue->type = OP_TYPE_STRING;
			firstValue->stringValue = NewXString(mStrBuffer);
		}
		else if (IsPNumberType(secondValue))
		{
			char numberBuffer[64] = { 0 };

			if (IsValueInt(secondValue))
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, "%d", secondValue->iIntValue);
			}
			else
			{
				snprintf(numberBuffer, MAX_NUMBER_STRING_SIZE, "%f", secondValue->fFloatValue);
			}

			int strLength = stringRawLen(firstValue) + strlen(numberBuffer) + 1;
			CheckStrBuffer(strLength);

			memcpy(mStrBuffer, stringRawValue(firstValue), stringRawLen(firstValue));
			memcpy(mStrBuffer + stringRawLen(firstValue), numberBuffer, strlen(numberBuffer));
			mStrBuffer[stringRawLen(firstValue) + strlen(numberBuffer)] = '\0';

			firstValue->type = OP_TYPE_STRING;
			firstValue->stringValue = NewXString(mStrBuffer);

		}
	}
	else
	{
		EXEC_OP_ERROR($, 0, 1);
	}
}

bool XScriptVM::ExecInstr_CallClassFunc()
{
	bool isLua = false;
	const Value& value = resolveOpValue(0);
	std::string funcName = GetString(value.iFunctionValue >> 16);
	int numParam = (value.iFunctionValue & 0xffff);
	int index = mCurXScriptState->mTopIndex - numParam;
	const Value& thisValue = getStackValue(index);
	if (IsUserType(thisValue.type))
	{
		std::string className = GetString(USERDATA_TYPE(thisValue.type));
		HostFunction* hostFunc = getClassFuncByName(className, funcName);

		if (hostFunc != NULL)
		{
			if (hostFunc->numParams >= 0 && hostFunc->numParams != numParam)
			{
				ExecError("call class function %s with error params, expect %d params, but %d params", funcName.c_str(), hostFunc->numParams, numParam);
			}
			else
			{
				CallHostFunc(hostFunc, numParam);
			}

		}
		else
		{
			ExecError("can't find function %s in class %s", funcName.c_str(), className.c_str());
		}
	}
	else if (thisValue.type == OP_TYPE_TABLE)
	{
		Value funcValue;
		if (getTableValue((TABLE)thisValue.tableData, ConstructValue(funcName.c_str()), funcValue))
		{
			if (funcValue.type == OP_TYPE_FUNC)
			{
				CallFunctionInLua(funcValue.func, numParam);
				isLua = !funcValue.func->isCFunc;
			}
			else
			{
				ExecError("attempt to call function on %s", getTypeName(funcValue.type));
			}
		}
		else
		{
			ExecError("call function on nil");
		}
	}
	else
	{
		ExecError("attempt to call class function on %s", getTypeName(thisValue.type));
	}
	
	return isLua;
}

void XScriptVM::ExecInstr_CallStaticClassFunc()
{
	const Value& value = resolveOpValue(0);

	std::string funcName = GetString(value.iFunctionValue >> 16);
	int numParam = (value.iFunctionValue & 0xffff);

	const Value& secondValue = resolveOpValue(1);
	if (secondValue.type != OP_TYPE_STRING)
	{
		ExecError("invalid op type when call function, expect string but %s", getTypeName(secondValue.type));
	}

	HostFunction* hostFunc = getClassFuncByName( stringRawValue(&secondValue), funcName);
	if (hostFunc != NULL)
	{
		if (hostFunc->numParams >= 0 && hostFunc->numParams != numParam)
		{
			ExecError("call function %s with error params, expect %d params, but %d params", funcName.c_str(), hostFunc->numParams, numParam);
		}
		else
		{
			CallHostFunc(hostFunc, numParam);
		}

	}
	else
	{
		ExecError("can't find class function: %s", funcName.c_str());
	}
}

void XScriptVM::ExecInstr_RET()
{
	if (mAllowHook && (mHookMask & MASK_HOOKCALL))
	{
		CallHookFunction(HE_HookRet, -1);
	}

	PopFrame(mCurXScriptState->mCurFunctionState->stackFrameSize);
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

void XScriptVM::ExecInstr_CALL()
{
	ResolveOpPointer(0, firstValue);
	ResolveOpPointer(1, secondValue);
	int numParam = secondValue->iIntValue;

	if (firstValue->type != OP_TYPE_FUNC)
	{
		ExecError("attempt to call function on %s", GetOperatorName(0).c_str());
	}


	CallFunctionInLua(firstValue->func, numParam);

}

void XScriptVM::ExecInstr_JLE()
{
	const Value& firstValue = resolveOpValue(0);
	const Value& secondValue = resolveOpValue(1);
	int thirdValue = resolveOpAsInstrIndex(2);
	if (firstValue.type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.iIntValue <= castValueToInt(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(<= , 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.fFloatValue <= castValueTofloat(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(<= , 1, 2);
		}

	}
	else if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			if (strcmp( stringRawValue(&firstValue), stringRawValue(&secondValue)) != 1)
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(<= , 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(<= , 1, 2);
	}
}

void XScriptVM::ExecInstr_JL()
{
	const Value& firstValue = resolveOpValue(0);
	const Value& secondValue = resolveOpValue(1);
	int thirdValue = resolveOpAsInstrIndex(2);
	if (firstValue.type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.iIntValue < castValueToInt(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(< , 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.fFloatValue < castValueTofloat(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(<, 1, 2);
		}

	}
	else if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			if (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) == -1)
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(<, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(<, 1, 2);
	}
}

void XScriptVM::ExecInstr_JGE()
{
	const Value& firstValue = resolveOpValue(0);
	const Value& secondValue = resolveOpValue(1);
	int thirdValue = resolveOpAsInstrIndex(2);
	if (firstValue.type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.iIntValue >= castValueToInt(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(>=, 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.fFloatValue >= castValueTofloat(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(>= , 1, 2);
		}

	}
	else if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			if (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) != -1)
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(>= , 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(>= , 1, 2);
	}
}

void XScriptVM::ExecInstr_JG()
{
	const Value& firstValue = resolveOpValue(0);
	const Value& secondValue = resolveOpValue(1);
	int thirdValue = resolveOpAsInstrIndex(2);
	if (firstValue.type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.iIntValue > castValueToInt(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(> , 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			if (firstValue.fFloatValue > castValueTofloat(secondValue))
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(>, 1, 2);
		}

	}
	else if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			if (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) == 1)
				mCurXScriptState->mInstrIndex = thirdValue;
		}
		else
		{
			EXEC_OP_ERROR(>, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(>, 1, 2);
	}
}

void XScriptVM::ExecInstr_JNE()
{
	ResolveOpPointer(0, firstValue);
	ResolveOpPointer(1, secondValue);
	ResolveOpPointer(2, thirdValue);

	int ret;
	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))
	{
		ret = (PNumberValue(firstValue) != PNumberValue(secondValue)) ? 1 : 0;
	}
	else if (firstValue->type == secondValue->type)
	{
		switch (firstValue->type)
		{
		case OP_TYPE_STRING:
		{
			ret = (firstValue->stringValue != secondValue->stringValue) ? 1 : 0;
		}
		break;
		case OP_TYPE_TABLE:
		{
			ret = (firstValue->tableData != secondValue->tableData) ? 1 : 0;
		}
		break;
		case OP_USERTYPE:
		{
			ret = (firstValue->func != secondValue->func) ? 1 : 0;
		}
		break;
		case OP_TYPE_NIL:
		{
			ret = 0;
		}
		break;
		default:
			ret = 0;
			break;
		}
	}
	else
	{
		ret = 1;
	}
	if (ret)
	{
		mCurXScriptState->mInstrIndex = thirdValue->iInstrIndex;
	}
}



void XScriptVM::ExecInstr_SHR()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT && second.type == OP_TYPE_INT)
	{
		destValue->iIntValue = (destValue->iIntValue >>= second.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(>>, 0, 1);
	}
}

void XScriptVM::ExecInstr_SHL()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT && second.type == OP_TYPE_INT)
	{
		destValue->iIntValue = (destValue->iIntValue << second.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(<< , 0, 1);
	}
}

void XScriptVM::ExecInstr_Not()
{
	Value* destValue = resolveOpPointer(0);
	if (destValue->type == OP_TYPE_INT)
	{
		destValue->iIntValue = ~destValue->iIntValue;
	}
	else
	{
		ExecError("attempt to perform ~ operator on %s", GetOperatorName(0).c_str());
	}
}

void XScriptVM::ExecInstr_Xor()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT && second.type == OP_TYPE_INT)
	{
		destValue->iIntValue = (destValue->iIntValue ^ second.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(# , 0, 1);
	}
}

void XScriptVM::ExecInstr_Or()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT && second.type == OP_TYPE_INT)
	{
		destValue->iIntValue = (destValue->iIntValue | second.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR( |, 0, 1);
	}
}

void XScriptVM::ExecInstr_And()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT && second.type == OP_TYPE_INT)
	{
		destValue->iIntValue = (destValue->iIntValue & second.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(&, 0, 1);
	}
}

void XScriptVM::ExecInstr_Dec()
{
	Value* destValue = resolveOpPointer(0);
	if (destValue->type == OP_TYPE_INT)
	{
		destValue->iIntValue = destValue->iIntValue - 1;
		if (destValue->iIntValue < 0)
		{
			destValue->iIntValue = destValue->iIntValue;
		}
	}
	else if (destValue->type == OP_TYPE_FLOAT)
	{
		destValue->fFloatValue = destValue->fFloatValue - 1.0f;
	}
	else
		ExecError("attempt to perform -- operator on %s", GetOperatorName(0).c_str());
}

void XScriptVM::ExecInstr_Inc()
{
	ResolveOpPointer(0, destValue);
	if (destValue->type == OP_TYPE_INT)
		destValue->iIntValue++;
	else if (destValue->type == OP_TYPE_FLOAT)
	{
		destValue->fFloatValue = destValue->fFloatValue + 1.0f;
	}
	else
		ExecError("attempt to perform ++ operator on %s", GetOperatorName(0).c_str());
}

void XScriptVM::ExecInstr_Neg()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	if (secondValue.type == OP_TYPE_INT)
	{
		destValue->type = OP_TYPE_INT;
		destValue->iIntValue = -secondValue.iIntValue;
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		destValue->fFloatValue = -secondValue.fFloatValue;
		destValue->type = OP_TYPE_FLOAT;
	}
	else
	{
		ExecError("attempt to perform - operator on  %s", GetOperatorName(1).c_str());
	}
}

void XScriptVM::ExecInstr_Pow()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->iIntValue = (int)pow(destValue->iIntValue, secondValue.iIntValue);
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			float fValue = pow(((float)destValue->iIntValue), secondValue.fFloatValue);
			destValue->fFloatValue = fValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(^, 0, 1);
		}
	}

	else if (destValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->fFloatValue = pow(destValue->fFloatValue, (float)secondValue.iIntValue);
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = pow(destValue->fFloatValue, (float)secondValue.fFloatValue);
		}
		else
		{
			EXEC_OP_ERROR(^, 0, 1);
		}
	}
	else
	{
		EXEC_OP_ERROR(^, 0, 1);
	}
}

void XScriptVM::ExecInstr_Mod()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->iIntValue %= secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			float fValue = fmod(((float)destValue->iIntValue), secondValue.fFloatValue);
			destValue->fFloatValue = fValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(%, 0, 1);
		}
	}

	else if (destValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->fFloatValue = fmod(destValue->fFloatValue, (float)secondValue.iIntValue);
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = fmod(destValue->fFloatValue, (float)secondValue.fFloatValue);
		}
		else
		{
			EXEC_OP_ERROR(% , 0, 1);
		}
	}
	else
	{
		EXEC_OP_ERROR(%, 0, 1);
	}
}

void XScriptVM::ExecInstr_Div()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->iIntValue /= secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			float fValue = ((float)destValue->iIntValue) / secondValue.fFloatValue;
			destValue->fFloatValue = fValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(/, 0, 1);
		}
	}

	else if (destValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->fFloatValue /= (float)secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = destValue->fFloatValue / secondValue.fFloatValue;
		}
		else
		{
			EXEC_OP_ERROR(/ , 0, 1);
		}
	}
	else
	{
		EXEC_OP_ERROR(/ , 0, 1);
	}
}

void XScriptVM::ExecInstr_Mul()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	if (destValue->type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->iIntValue *= secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			float fValue = ((float)destValue->iIntValue) * secondValue.fFloatValue;
			destValue->fFloatValue = fValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(*, 0, 1);
		}
	}

	else if (destValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT)
			destValue->fFloatValue *= (float)secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = destValue->fFloatValue * secondValue.fFloatValue;
		}
		else
		{
			EXEC_OP_ERROR(*, 0, 1);
		}
	}
	else
	{
		EXEC_OP_ERROR(*, 0, 1);
	}
}

void XScriptVM::ExecInstr_Sub()
{
	Value* destValue = resolveOpPointer(0);
	if (destValue->type == OP_TYPE_INT)
	{
		const Value& secondValue = resolveOpValue(1);
		if (secondValue.type == OP_TYPE_INT)
			destValue->iIntValue -= secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			float fValue = destValue->iIntValue - secondValue.fFloatValue;
			destValue->fFloatValue = fValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(-, 0, 1);
		}
	}

	else if (destValue->type == OP_TYPE_FLOAT)
	{
		const Value& secondValue = resolveOpValue(1);
		if (secondValue.type == OP_TYPE_INT)
			destValue->fFloatValue -= secondValue.iIntValue;
		else if (secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = destValue->fFloatValue - secondValue.fFloatValue;
		}
		else
		{
			EXEC_OP_ERROR(-, 0, 1);
		}
	}
	else
	{
		EXEC_OP_ERROR(-, 0, 1);
	}
}

void XScriptVM::ExecInstr_Add()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, secondValue);
	
	if (destValue->type == OP_TYPE_INT)
	{
		if (secondValue->type == OP_TYPE_INT)
			destValue->iIntValue += secondValue->iIntValue;
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = (float)destValue->iIntValue + secondValue->fFloatValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else if (secondValue->type == OP_TYPE_STRING)
		{
			float value;
			if (CastStrToFloat(stringRawValue(secondValue), &value))
			{
				destValue->fFloatValue = (float)destValue->iIntValue + value;
				destValue->type = OP_TYPE_FLOAT;
			}
			else
			{
				EXEC_OP_ERROR(+, 0, 1);
			}
		}
		else
		{
			EXEC_OP_ERROR(+, 0, 1);
		}
	}
	else if (destValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue->type == OP_TYPE_INT)
			destValue->fFloatValue += secondValue->iIntValue;
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue += secondValue->fFloatValue;
		}
		else if (secondValue->type == OP_TYPE_STRING)
		{
			float value;
			if (CastStrToFloat(stringRawValue(secondValue), &value))
			{
				destValue->fFloatValue += value;
			}
			else
			{
				EXEC_OP_ERROR(+, 0, 1);
			}
		}
		else
		{
			EXEC_OP_ERROR(+, 0, 1);
		}
	}
	else if (destValue->type == OP_TYPE_STRING)
	{
		float destFloatValue;
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))
		{
			destValue->type = OP_TYPE_FLOAT;
			if (secondValue->type == OP_TYPE_INT)
				destValue->fFloatValue = destFloatValue + (float)secondValue->iIntValue;
			else if (secondValue->type == OP_TYPE_FLOAT)
			{
				destValue->fFloatValue = destFloatValue + secondValue->fFloatValue;
			}
			else if (secondValue->type == OP_TYPE_STRING)
			{
				float value;
				if (CastStrToFloat(stringRawValue(secondValue), &value))
				{
					destValue->fFloatValue = destFloatValue + value;
				}
				else
				{
					EXEC_OP_ERROR(+, 0, 1);
				}
			}
			else
			{
				EXEC_OP_ERROR(+, 0, 1); 
			}
		}
		else
		{
			EXEC_OP_ERROR(+, 0, 1);
		}
		
	}
	else
	{
		EXEC_OP_ERROR(+, 0, 1);
	}
}

void XScriptVM::ExecInstr_SHRTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	const Value& third = resolveOpValue(2);
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)
	{
		destValue->type = OP_TYPE_INT;
		destValue->iIntValue = (second.iIntValue >> third.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(>> , 1, 2);
	}
}

void XScriptVM::ExecInstr_SHLTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	const Value& third = resolveOpValue(2);
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)
	{
		destValue->type = OP_TYPE_INT;
		destValue->iIntValue = (second.iIntValue << third.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(<< , 1, 2);
	}
}

void XScriptVM::ExecInstr_ExpTo()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	const Value& thirdValue = resolveOpValue(2);

	if (secondValue.type == OP_TYPE_INT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_INT;
			destValue->iIntValue = (int)pow(secondValue.iIntValue, thirdValue.iIntValue);
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = pow((float)secondValue.iIntValue, thirdValue.fFloatValue);
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(pow , 1, 2);
		}
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = pow((float)secondValue.fFloatValue, (float)thirdValue.iIntValue);
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = pow((float)secondValue.fFloatValue, (float)thirdValue.fFloatValue);
		}
		else
		{
			EXEC_OP_ERROR(pow, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(pow, 1, 2);
	}
}

void XScriptVM::ExecInstr_MODTO()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, secondValue);
	ResolveOpPointer(2, thirdValue);
	if (secondValue->type == OP_TYPE_INT)
	{
		if (thirdValue->type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_INT;
			destValue->iIntValue = secondValue->iIntValue % thirdValue->iIntValue;
		}
		else if (thirdValue->type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = fmodf((float)secondValue->iIntValue, thirdValue->fFloatValue);
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(%, 1, 2);
		}
	}
	else if (secondValue->type == OP_TYPE_FLOAT)
	{
		if (thirdValue->type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = fmodf((float)secondValue->fFloatValue, (float)thirdValue->iIntValue);
		}
		else if (thirdValue->type == OP_TYPE_FLOAT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = fmodf((float)secondValue->fFloatValue, (float)thirdValue->fFloatValue);
		}
		else
		{
			EXEC_OP_ERROR(%, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(%, 1, 2);
	}
}

void XScriptVM::ExecInstr_XORTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	const Value& third = resolveOpValue(2);
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)
	{
		destValue->type = OP_TYPE_INT;
		destValue->iIntValue = (second.iIntValue ^ third.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(^, 1, 2);
	}
}

void XScriptVM::ExecInstr_ORTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	const Value& third = resolveOpValue(2);
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)
	{
		destValue->type = OP_TYPE_INT;
		destValue->iIntValue = (second.iIntValue | third.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(|, 1, 2);
	}
}

void XScriptVM::ExecInstr_AndTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& second = resolveOpValue(1);
	const Value& third = resolveOpValue(2);
	if (second.type == OP_TYPE_INT && third.type == OP_TYPE_INT)
	{
		destValue->type = OP_TYPE_INT;
		destValue->iIntValue = (second.iIntValue & third.iIntValue);
	}
	else
	{
		EXEC_OP_ERROR(& , 1, 2);
	}
}

void XScriptVM::ExecInstr_DIVTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	const Value& thirdValue = resolveOpValue(2);

	if (secondValue.type == OP_TYPE_INT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_INT;
			destValue->iIntValue = secondValue.iIntValue / thirdValue.iIntValue;
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = (float)secondValue.iIntValue / thirdValue.fFloatValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(/, 1, 2); 
		}
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = (float)secondValue.fFloatValue / (float)thirdValue.iIntValue;
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = (float)secondValue.fFloatValue / (float)thirdValue.fFloatValue;
		}
		else
		{
			EXEC_OP_ERROR(/ , 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(/ , 1, 2);
	}
}

void XScriptVM::ExecInstr_MULTO()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	const Value& thirdValue = resolveOpValue(2);

	if (secondValue.type == OP_TYPE_INT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_INT;
			destValue->iIntValue = secondValue.iIntValue * thirdValue.iIntValue;
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = (float)secondValue.iIntValue * thirdValue.fFloatValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(*, 1, 2);
		}
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = (float)secondValue.fFloatValue * (float)thirdValue.iIntValue;
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = (float)secondValue.fFloatValue * (float)thirdValue.fFloatValue;
		}
		else
		{
			EXEC_OP_ERROR(* , 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(* , 1, 2);
	}
}

void XScriptVM::ExecInstr_SubTo()
{
	Value* destValue = resolveOpPointer(0);
	const Value& secondValue = resolveOpValue(1);
	const Value& thirdValue = resolveOpValue(2);

	if (secondValue.type == OP_TYPE_INT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_INT;
			destValue->iIntValue = secondValue.iIntValue - thirdValue.iIntValue;
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->fFloatValue = (float)secondValue.iIntValue - thirdValue.fFloatValue;
			destValue->type = OP_TYPE_FLOAT;
		}
		else
		{
			EXEC_OP_ERROR(-, 1, 2);
		}
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		if (thirdValue.type == OP_TYPE_INT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = (float)secondValue.fFloatValue - (float)thirdValue.iIntValue;
		}
		else if (thirdValue.type == OP_TYPE_FLOAT)
		{
			destValue->type = OP_TYPE_FLOAT;
			destValue->fFloatValue = (float)secondValue.fFloatValue - (float)thirdValue.fFloatValue;
		}
		else
		{
			EXEC_OP_ERROR(-, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(-, 1, 2);
	}
}

void XScriptVM::ExecInstr_AddTo()
{
	ResolveOpPointer(0, firstValue);
	ResolveOpPointer(1, destValue);
	ResolveOpPointer(2, secondValue);

	if (destValue->type == OP_TYPE_INT)
	{
		if (secondValue->type == OP_TYPE_INT)
		{
			firstValue->type = OP_TYPE_INT;
			firstValue->iIntValue = destValue->iIntValue + secondValue->iIntValue;
		}
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			firstValue->fFloatValue = (float)destValue->iIntValue + secondValue->fFloatValue;
			firstValue->type = OP_TYPE_FLOAT;
		}
		else if (secondValue->type == OP_TYPE_STRING)
		{
			float value;
			if (CastStrToFloat(stringRawValue(secondValue), &value))
			{
				firstValue->fFloatValue = (float)destValue->iIntValue + value;
				firstValue->type = OP_TYPE_FLOAT;
			}
			else
			{
				EXEC_OP_ERROR(+, 1, 2);
			}
		}
		else
		{
			EXEC_OP_ERROR(+, 1, 2);
		}
	}
	else if (destValue->type == OP_TYPE_FLOAT)
	{
		firstValue->type = OP_TYPE_FLOAT;
		if (secondValue->type == OP_TYPE_INT)
			firstValue->fFloatValue = destValue->fFloatValue + secondValue->iIntValue;
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			firstValue->fFloatValue = destValue->fFloatValue + secondValue->fFloatValue;
		}
		else if (secondValue->type == OP_TYPE_STRING)
		{
			float value;
			if (CastStrToFloat(stringRawValue(secondValue), &value))
			{
				firstValue->fFloatValue = (float)destValue->fFloatValue + value;
			}
			else
			{
				EXEC_OP_ERROR(+, 1, 2);
			}
		}
		else
		{
			EXEC_OP_ERROR(+, 1, 2);
		}
	}
	else if (destValue->type == OP_TYPE_STRING)
	{
		float destFloatValue;
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))
		{
			firstValue->type = OP_TYPE_FLOAT;
			if (secondValue->type == OP_TYPE_INT)
				firstValue->fFloatValue = destFloatValue + (float)secondValue->iIntValue;
			else if (secondValue->type == OP_TYPE_FLOAT)
			{
				firstValue->fFloatValue = destFloatValue + secondValue->fFloatValue;
			}
			else if (secondValue->type == OP_TYPE_STRING)
			{
				float value;
				if (CastStrToFloat(stringRawValue(secondValue), &value))
				{
					firstValue->fFloatValue = destFloatValue + value;
				}
				else
				{
					EXEC_OP_ERROR(+, 1, 2);
				}
			}
			else
			{
				EXEC_OP_ERROR(+, 1, 2);
			}
		}
		else
		{
			EXEC_OP_ERROR(+, 1, 2);
		}

	}
	else
	{
		EXEC_OP_ERROR(+, 1, 2);
	}
}



void XScriptVM::ExecInstr_Test_G()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);
	destValue->type = OP_TYPE_INT;
	if (firstValue->type == OP_TYPE_INT)
	{
		if (secondValue->type == OP_TYPE_INT)
		{
			destValue->iIntValue = (firstValue->iIntValue > secondValue->iIntValue) ? 1 : 0;
		}
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue->iIntValue > secondValue->fFloatValue) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(>, 1, 2);
		}
	}
	else if (firstValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue->type == OP_TYPE_INT)
		{
			destValue->iIntValue = (firstValue->fFloatValue > secondValue->iIntValue) ? 1 : 0;
		}
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue->fFloatValue > secondValue->fFloatValue) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(>, 1, 2);
		}


	}
	else if (firstValue->type == OP_TYPE_STRING)
	{
		if (secondValue->type == OP_TYPE_STRING)
		{
			destValue->iIntValue = (strcmp(stringRawValue(firstValue), stringRawValue(secondValue)) == 1) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(>, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(>, 1, 2);
	}
}

void XScriptVM::ExecInstr_Test_NE()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);

	destValue->type = OP_TYPE_INT;

	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))
	{
		destValue->iIntValue = (PNumberValue(firstValue) != PNumberValue(secondValue)) ? 1 : 0;
	}
	else if (firstValue->type == secondValue->type)
	{
		switch (firstValue->type)
		{
		case OP_TYPE_STRING:
		{
			destValue->iIntValue = (firstValue->stringValue != secondValue->stringValue) ? 1 : 0;
		}
		break;
		case OP_TYPE_TABLE:
		{
			destValue->iIntValue = (firstValue->tableData != secondValue->tableData) ? 1 : 0;
		}
		break;
		case OP_USERTYPE:
		{
			destValue->iIntValue = (firstValue->func != secondValue->func) ? 1 : 0;
		}
		break;
		case OP_TYPE_NIL:
		{
			destValue->iIntValue = 0;
		}
		break;
		default:
			destValue->iIntValue = 0;
			break;
		}
	}
	else
	{
		destValue->iIntValue = 1;
	}
}

void XScriptVM::ExecInstr_Test_E()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);

	destValue->type = OP_TYPE_INT;

	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))
	{
		destValue->iIntValue = (PNumberValue(firstValue) == PNumberValue(secondValue)) ? 1 : 0;
	}
	else if (firstValue->type == secondValue->type)
	{
		switch (firstValue->type)
		{
			case OP_TYPE_STRING:
			{
				destValue->iIntValue = (firstValue->stringValue == secondValue->stringValue) ? 1 : 0;
			}
			break;
			case OP_TYPE_TABLE:
			{
				destValue->iIntValue = (firstValue->tableData == secondValue->tableData) ? 1 : 0;
			}
			break;
			case OP_USERTYPE:
			{
				destValue->iIntValue = (firstValue->func == secondValue->func) ? 1 : 0;
			}
			break;
			case OP_TYPE_NIL:
			{
				destValue->iIntValue = (1 == 1);
			}
			break;
		default:
			destValue->iIntValue = 0;
			break;
		}
	}
	else
	{
		destValue->iIntValue = (1 == 0);
	}
}

void XScriptVM::ExecInstr_Logic_Not()
{
	Value* destValue = resolveOpPointer(0);
	destValue->type = OP_TYPE_INT;
	const Value& secondValue = resolveOpValue(1);
	if (secondValue.type == OP_TYPE_INT)
	{
		destValue->iIntValue = (secondValue.iIntValue == 0) ? 1 : 0;
	}
	else if (secondValue.type == OP_TYPE_FLOAT)
	{
		destValue->iIntValue = (secondValue.fFloatValue == 0.0f) ? 1 : 0;
	}
	else
	{
		destValue->iIntValue = (secondValue.type == OP_TYPE_NIL) ? 1 : 0;
	}
}

void XScriptVM::ExecInstr_Test_L()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);

	destValue->type = OP_TYPE_INT;
	if (firstValue->type == OP_TYPE_INT)
	{
		if (secondValue->type == OP_TYPE_INT)
		{
			destValue->iIntValue = (firstValue->iIntValue < secondValue->iIntValue) ? 1 : 0;
		}
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue->iIntValue < secondValue->fFloatValue) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(< , 1, 2);
		}
	}
	else if (firstValue->type == OP_TYPE_FLOAT)
	{
		if (secondValue->type == OP_TYPE_INT)
		{
			destValue->iIntValue = (firstValue->fFloatValue < secondValue->iIntValue) ? 1 : 0;
		}
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue->fFloatValue < secondValue->fFloatValue) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(<, 1, 2);
		}
	}
	else if (firstValue->type == OP_TYPE_STRING)
	{
		if (secondValue->type == OP_TYPE_STRING)
		{
			destValue->iIntValue = (strcmp(stringRawValue(firstValue), stringRawValue(secondValue)) == -1) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(<, 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(<, 1, 2);
	}
}

void XScriptVM::ExecInstr_Test_GE()
{
	Value* destValue = resolveOpPointer(0);
	const Value& firstValue = resolveOpValue(1);
	const Value& secondValue = resolveOpValue(2);
	destValue->type = OP_TYPE_INT;
	if (firstValue.type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue.iIntValue >= castValueToInt(secondValue)) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(>=, 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue.iIntValue >= castValueToInt(secondValue)) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(>= , 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			destValue->iIntValue = (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) != -1) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(>= , 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(>= , 1, 2);
	}
}

void XScriptVM::ExecInstr_Test_LE()
{
	Value* destValue = resolveOpPointer(0);
	const Value& firstValue = resolveOpValue(1);
	const Value& secondValue = resolveOpValue(2);
	destValue->type = OP_TYPE_INT;
	if (firstValue.type == OP_TYPE_INT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue.iIntValue <= castValueToInt(secondValue)) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(<= , 1, 2);
		}
	}
	else if (firstValue.type == OP_TYPE_FLOAT)
	{
		if (secondValue.type == OP_TYPE_INT || secondValue.type == OP_TYPE_FLOAT)
		{
			destValue->iIntValue = (firstValue.iIntValue <= castValueToInt(secondValue)) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(<= , 1, 2);
		}


	}
	else if (firstValue.type == OP_TYPE_STRING)
	{
		if (secondValue.type == OP_TYPE_STRING)
		{
			destValue->iIntValue = (strcmp(stringRawValue(&firstValue), stringRawValue(&secondValue)) != 1) ? 1 : 0;
		}
		else
		{
			EXEC_OP_ERROR(<= , 1, 2);
		}
	}
	else
	{
		EXEC_OP_ERROR(<= , 1, 2);
	}
}

void XScriptVM::ExecInstr_Logic_And()
{
	ResolveOpPointer(0, destValue);
	ResolveOpPointer(1, firstValue);
	ResolveOpPointer(2, secondValue);
	destValue->type = OP_TYPE_INT;
	int value = 0;
	if (firstValue->type == OP_TYPE_INT)
	{
		value = ((firstValue->iIntValue != 0) ? 1 : 0);
	}
	else if (firstValue->type == OP_TYPE_FLOAT)
	{
		value = ((firstValue->fFloatValue != 0) ? 1 : 0);
	}
	else if (IsUserType(firstValue->type))
	{
		value = ((firstValue->userData != NULL) ? 1 : 0);
	}
	else
	{
		ExecError("attempt to perform && operator on", GetOperatorName(1).c_str());
	}

	if (value == 1)
	{
		int value = 0;
		if (secondValue->type == OP_TYPE_INT)
		{
			value = ((secondValue->iIntValue != 0) ? 1 : 0);
		}
		else if (secondValue->type == OP_TYPE_FLOAT)
		{
			value = ((secondValue->fFloatValue != 0) ? 1 : 0);
		}
		else if (IsUserType(secondValue->type))
		{
			value = ((secondValue->userData != NULL) ? 1 : 0);
		}
		else
		{
			ExecError("attempt to perform && operator on", GetOperatorName(2).c_str());
		}
	}

	destValue->iIntValue = value;
}

void XScriptVM::ExecInstr_Logic_Or()
{
	Value* destValue = resolveOpPointer(0);
	const Value& firstValue = resolveOpValue(1);
	const Value& secondValue = resolveOpValue(2);
	destValue->type = OP_TYPE_INT;
	int value = 0;
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
		value = ((firstValue.userData != NULL) ? 1 : 0);
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
			value = ((secondValue.userData != NULL) ? 1 : 0);
		}
		else
		{
			ExecError("attempt to perform || operator on", GetOperatorName(2).c_str());
		}
	}

	destValue->iIntValue = value;
}

void XScriptVM::ExecInstr_JE()
{
	ResolveOpPointer(0, firstValue);
	ResolveOpPointer(1, secondValue);
	ResolveOpPointer(2, thirdValue);

	int ret;
	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))
	{
		ret = (PNumberValue(firstValue) == PNumberValue(secondValue)) ? 1 : 0;
	}
	else if (firstValue->type == secondValue->type)
	{
		switch (firstValue->type)
		{
		case OP_TYPE_STRING:
		{
			ret = (firstValue->stringValue == secondValue->stringValue) ? 1 : 0;
		}
		break;
		case OP_TYPE_TABLE:
		{
			ret = (firstValue->tableData == secondValue->tableData) ? 1 : 0;
		}
		break;
		case OP_USERTYPE:
		{
			ret = (firstValue->func == secondValue->func) ? 1 : 0;
		}
		break;
		case OP_TYPE_NIL:
		{
			ret = 1 == 1;
		}
		break;
		default:
			ret = 0;
			break;
		}
	}
	else
	{
		ret = 1 == 0;
	}

	if (ret)
	{
		mCurXScriptState->mInstrIndex = thirdValue->iInstrIndex;
	}
}