#pragma once
#ifndef MacroDefs_h__
#define MacroDefs_h__

#define	 IsPNumberType(t)	((t)->type == OP_TYPE_INT || (t)->type == OP_TYPE_FLOAT)
#define  PNumberValue(t)		((t)->type == OP_TYPE_INT ? (t)->iIntValue : (t)->fFloatValue)


#define		 ResolveOpPointer(index, T)			\
	Value* T = (Value*)&mCurInstr->mOpList[index];		\
	switch (T->type)	\
	{	\
	case ROT_Stack_Index:	\
	{							\
		int stackIndex = mCurInstr->mOpList[index].iStackIndex;	\
		T = ResoveStackIndexWithEnv(stackIndex);\
		break;	\
	}\
	case ROT_Table:\
	case ROT_UpValue_Table:\
	{	\
		T = resolveTableValue(&mCurInstr->mOpList[index], true);	\
	}	\
	break;	\
	case ROT_UpVal_Index: \
	{	\
			int stackIndex = mCurInstr->mOpList[index].iStackIndex;	\
			T = mCurXScriptState->mCurFunction->funcUnion.luaFunc.mUpVals[stackIndex]->pValue;	\
	}	\
	break;	\
	case ROT_Reg:	\
	{	\
		T = &mRegValue[T->iRegIndex];	\
	}	\
	}	

#define CopyValue(destValue, srcValue)		*(destValue) = (srcValue);

#define		EXEC_INSTR_TEST_E(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, firstValue);	\
	ResolveOpPointer(2, secondValue);	\
	destValue->type = OP_TYPE_INT;	\
	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))	\
	{	\
		destValue->iIntValue = (PNumberValue(firstValue) OP PNumberValue(secondValue)) ? 1 : 0;	\
	}	\
	else if (firstValue->type == secondValue->type)	\
	{	\
		switch (firstValue->type)	\
		{	\
		case OP_TYPE_STRING:	\
		{	\
			destValue->iIntValue = (firstValue->stringValue OP secondValue->stringValue) ? 1 : 0;	\
		}	\
		break;	\
		case OP_TYPE_TABLE:	\
		{	\
			destValue->iIntValue = (firstValue->tableData OP secondValue->tableData) ? 1 : 0;	\
		}	\
		break;	\
		case OP_USERTYPE:	\
		{	\
			destValue->iIntValue = (firstValue->func OP secondValue->func) ? 1 : 0;	\
		}	\
		break;	\
		case OP_TYPE_NIL:	\
		{	\
			destValue->iIntValue = (1 OP 1) ? 1 : 0;	\
		}	\
		break;	\
		default:	\
			destValue->iIntValue = 0;	\
			break;	\
		}	\
	}	\
	else \
	{	\
		destValue->iIntValue = (1 OP 0) ? 1 : 0;	\
	}


#define		EXEC_INSTR_J(OP)	\
ResolveOpPointer(0, firstValue);	\
ResolveOpPointer(1, secondValue);	\
ResolveOpPointer(2, instrValue);	\
int thirdValue = instrValue->iInstrIndex;	\
if (firstValue->type == OP_TYPE_INT)		\
{	\
	if (secondValue->type == OP_TYPE_INT)	\
	{	\
		if (firstValue->iIntValue OP secondValue->iIntValue)	\
			mCurXScriptState->mInstrIndex = thirdValue;	\
	}	\
	else if (secondValue->type == OP_TYPE_FLOAT)	\
	{	\
		if (firstValue->iIntValue OP secondValue->fFloatValue)	\
			mCurXScriptState->mInstrIndex = thirdValue;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	\
}	\
else if (firstValue->type == OP_TYPE_FLOAT)	\
{	\
	if (secondValue->type == OP_TYPE_INT)	\
	{	\
		if (firstValue->fFloatValue OP secondValue->iIntValue)	\
			mCurXScriptState->mInstrIndex = thirdValue;	\
	}	\
	else if (secondValue->type == OP_TYPE_FLOAT)	\
	{	\
		if (firstValue->fFloatValue OP secondValue->fFloatValue)	\
			mCurXScriptState->mInstrIndex = thirdValue;	\
	}	\
	else \
	{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	\
}	\
else if (firstValue->type == OP_TYPE_STRING)	\
{	\
	if (secondValue->type == OP_TYPE_STRING)	\
	{	\
		if (strcmp(stringRawValue(firstValue), stringRawValue(secondValue)) OP 0 )	\
			mCurXScriptState->mInstrIndex = thirdValue;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP,  GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	\
}	\
else \
{	\
	ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
}


#define  EXEC_INSTR_JE(OP)	\
	ResolveOpPointer(0, firstValue);	\
	ResolveOpPointer(1, secondValue);	\
	ResolveOpPointer(2, thirdValue);	\
	int ret;	\
	if (IsPNumberType(firstValue) && IsPNumberType(secondValue))	\
	{	\
		ret = (PNumberValue(firstValue) OP PNumberValue(secondValue)) ? 1 : 0;	\
	}	\
	else if (firstValue->type == secondValue->type)	\
	{	\
	switch (firstValue->type)	\
	{	\
	case OP_TYPE_STRING:	\
	{	\
		ret = (firstValue->stringValue OP secondValue->stringValue) ? 1 : 0;	\
	}	\
	break;	\
	case OP_TYPE_TABLE:	\
	{	\
		ret = (firstValue->tableData OP secondValue->tableData) ? 1 : 0;	\
	}	\
	break;	\
	case OP_USERTYPE:	\
	{	\
		ret = (firstValue->func OP secondValue->func) ? 1 : 0;	\
	}	\
	break;	\
	case OP_TYPE_NIL:	\
	{	\
		ret = (1 OP 1) ? 1 : 0;	\
	}	\
	break;	\
	default:	\
		ret = 0;	\
		break;	\
	}	\
	}	\
	else \
	{	\
		ret = 1 OP 0 ? 1 : 0;	\
	}	\
	if (ret)	\
	{	\
		mCurXScriptState->mInstrIndex = thirdValue->iInstrIndex;	\
	}	

#define  EXEC_INSTR_TEST(OP)	\
ResolveOpPointer(0, destValue);	\
ResolveOpPointer(1, firstValue);	\
ResolveOpPointer(2, secondValue);	\
destValue->type = OP_TYPE_INT;	\
if (firstValue->type == OP_TYPE_INT)	\
{	\
	if (secondValue->type == OP_TYPE_INT)	\
	{	\
		destValue->iIntValue = (firstValue->iIntValue OP secondValue->iIntValue) ? 1 : 0;	\
	}	\
	else if (secondValue->type == OP_TYPE_FLOAT)	\
	{	\
		destValue->iIntValue = (firstValue->iIntValue OP secondValue->fFloatValue) ? 1 : 0;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}	\
}	\
else if (firstValue->type == OP_TYPE_FLOAT)	\
{	\
	if (secondValue->type == OP_TYPE_INT)	\
	{	\
		destValue->iIntValue = (firstValue->fFloatValue OP secondValue->iIntValue) ? 1 : 0;	\
	}	\
	else if (secondValue->type == OP_TYPE_FLOAT)	\
	{	\
		destValue->iIntValue = (firstValue->fFloatValue OP secondValue->fFloatValue) ? 1 : 0;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}	\
}	\
else if (firstValue->type == OP_TYPE_STRING)	\
{	\
	if (secondValue->type == OP_TYPE_STRING)	\
	{	\
		destValue->iIntValue = (strcmp(stringRawValue(firstValue), stringRawValue(secondValue)) OP 0) ? 1 : 0;	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}	\
}	\
else \
{	\
	ExecError("attempt to perform %s operator on %s and %s",  #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
}



#define EXEC_INSTR_MATH(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, secondValue);	\
	if (destValue->type == OP_TYPE_INT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
			destValue->iIntValue OP##= secondValue->iIntValue;	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			destValue->fFloatValue = (float)destValue->iIntValue OP secondValue->fFloatValue;	\
			destValue->type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				destValue->fFloatValue = (float)destValue->iIntValue OP value;	\
				destValue->type = OP_TYPE_FLOAT;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
			destValue->fFloatValue OP##= secondValue->iIntValue;	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			destValue->fFloatValue OP##= secondValue->fFloatValue;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				destValue->fFloatValue = destValue->fFloatValue OP value;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_STRING)	\
	{	\
		float destFloatValue;	\
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))	\
		{	\
			destValue->type = OP_TYPE_FLOAT;	\
			if (secondValue->type == OP_TYPE_INT)	\
				destValue->fFloatValue = destFloatValue OP (float)secondValue->iIntValue;	\
			else if (secondValue->type == OP_TYPE_FLOAT)	\
			{	\
				destValue->fFloatValue = destFloatValue OP secondValue->fFloatValue;	\
			}	\
			else if (secondValue->type == OP_TYPE_STRING)	\
			{	\
				float value;	\
				if (CastStrToFloat(stringRawValue(secondValue), &value))	\
				{	\
					destValue->fFloatValue = destFloatValue OP value;	\
				}	\
				else \
				{	\
					ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
				}	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	


#define EXEC_INSTR_MATH_TO(OP)	\
	ResolveOpPointer(0, firstValue);	\
	ResolveOpPointer(1, destValue);	\
	ResolveOpPointer(2, secondValue);	\
	if (destValue->type == OP_TYPE_INT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
		{	\
			firstValue->type = OP_TYPE_INT;	\
			firstValue->iIntValue = destValue->iIntValue OP secondValue->iIntValue;	\
		}	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			firstValue->fFloatValue = (float)destValue->iIntValue OP secondValue->fFloatValue;	\
			firstValue->type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{		\
				firstValue->fFloatValue = (float)destValue->iIntValue OP value;	\
				firstValue->type = OP_TYPE_FLOAT;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		firstValue->type = OP_TYPE_FLOAT;	\
		if (secondValue->type == OP_TYPE_INT)	\
			firstValue->fFloatValue = destValue->fFloatValue OP secondValue->iIntValue;	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			firstValue->fFloatValue = destValue->fFloatValue OP secondValue->fFloatValue;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				firstValue->fFloatValue = destValue->fFloatValue OP value;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		} \
	}	\
	else if (destValue->type == OP_TYPE_STRING)	\
	{	\
		float destFloatValue;	\
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))	\
		{	\
			firstValue->type = OP_TYPE_FLOAT;	\
			if (secondValue->type == OP_TYPE_INT)	\
				firstValue->fFloatValue = destFloatValue OP (float)secondValue->iIntValue;	\
			else if (secondValue->type == OP_TYPE_FLOAT)	\
			{	\
				firstValue->fFloatValue = destFloatValue OP secondValue->fFloatValue;	\
			}	\
			else if (secondValue->type == OP_TYPE_STRING)	\
			{	\
				float value;	\
				if (CastStrToFloat(stringRawValue(secondValue), &value))	\
				{	\
					firstValue->fFloatValue = destFloatValue OP value;	\
				}	\
				else \
				{	\
					ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
				}	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}


#define EXEC_INSTR_MATH_OP_TO(OP)	\
	ResolveOpPointer(0, firstValue);	\
	ResolveOpPointer(1, destValue);	\
	ResolveOpPointer(2, secondValue);	\
	if (destValue->type == OP_TYPE_INT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
		{	\
			firstValue->type = OP_TYPE_INT;	\
			firstValue->iIntValue = OP(destValue->iIntValue, secondValue->iIntValue);	\
		}	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			firstValue->fFloatValue = OP((float)destValue->iIntValue, secondValue->fFloatValue);	\
			firstValue->type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{		\
				firstValue->fFloatValue = OP((float)destValue->iIntValue, value);	\
				firstValue->type = OP_TYPE_FLOAT;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		firstValue->type = OP_TYPE_FLOAT;	\
		if (secondValue->type == OP_TYPE_INT)	\
			firstValue->fFloatValue = OP(destValue->fFloatValue, secondValue->iIntValue);	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			firstValue->fFloatValue = OP(destValue->fFloatValue, secondValue->fFloatValue);	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				firstValue->fFloatValue = OP(destValue->fFloatValue, value);	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		} \
	}	\
	else if (destValue->type == OP_TYPE_STRING)	\
	{	\
		float destFloatValue;	\
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))	\
		{	\
			firstValue->type = OP_TYPE_FLOAT;	\
			if (secondValue->type == OP_TYPE_INT)	\
				firstValue->fFloatValue = OP(destFloatValue, (float)secondValue->iIntValue);	\
			else if (secondValue->type == OP_TYPE_FLOAT)	\
			{	\
				firstValue->fFloatValue = OP(destFloatValue, secondValue->fFloatValue);	\
			}	\
			else if (secondValue->type == OP_TYPE_STRING)	\
			{	\
				float value;	\
				if (CastStrToFloat(stringRawValue(secondValue), &value))	\
				{	\
					firstValue->fFloatValue = OP(destFloatValue, value);	\
				}	\
				else \
				{	\
					ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
				}	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}


#define EXEC_INSTR_MATH_OP(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, secondValue);	\
	if (destValue->type == OP_TYPE_INT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
			destValue->iIntValue = OP(destValue->iIntValue, secondValue->iIntValue);	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			destValue->fFloatValue = OP((float)destValue->iIntValue, secondValue->fFloatValue);	\
			destValue->type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				destValue->fFloatValue = OP((float)destValue->iIntValue, value);	\
				destValue->type = OP_TYPE_FLOAT;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
			destValue->fFloatValue = OP(destValue->fFloatValue,  secondValue->iIntValue);	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			destValue->fFloatValue = OP(destValue->fFloatValue, secondValue->fFloatValue);	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				destValue->fFloatValue = OP(destValue->fFloatValue, value);	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_STRING)	\
	{	\
		float destFloatValue;	\
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))	\
		{	\
			destValue->type = OP_TYPE_FLOAT;	\
			if (secondValue->type == OP_TYPE_INT)	\
				destValue->fFloatValue = OP(destFloatValue, (float)secondValue->iIntValue);	\
			else if (secondValue->type == OP_TYPE_FLOAT)	\
			{	\
				destValue->fFloatValue = OP(destFloatValue, secondValue->fFloatValue);	\
			}	\
			else if (secondValue->type == OP_TYPE_STRING)	\
			{	\
				float value;	\
				if (CastStrToFloat(stringRawValue(secondValue), &value))	\
				{	\
					destValue->fFloatValue = OP(destFloatValue, value);	\
				}	\
				else \
				{	\
					ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
				}	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	



#define EXEC_INSTR_MATH_INTOP_TO(OP, IntOP)	\
	ResolveOpPointer(0, firstValue);	\
	ResolveOpPointer(1, destValue);	\
	ResolveOpPointer(2, secondValue);	\
	if (destValue->type == OP_TYPE_INT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
		{	\
			firstValue->type = OP_TYPE_INT;	\
			firstValue->iIntValue = destValue->iIntValue IntOP secondValue->iIntValue;	\
		}	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			firstValue->fFloatValue = OP((float)destValue->iIntValue, secondValue->fFloatValue);	\
			firstValue->type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{		\
				firstValue->fFloatValue = OP((float)destValue->iIntValue, value);	\
				firstValue->type = OP_TYPE_FLOAT;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		firstValue->type = OP_TYPE_FLOAT;	\
		if (secondValue->type == OP_TYPE_INT)	\
			firstValue->fFloatValue = OP(destValue->fFloatValue, secondValue->iIntValue);	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			firstValue->fFloatValue = OP(destValue->fFloatValue, secondValue->fFloatValue);	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				firstValue->fFloatValue = OP(destValue->fFloatValue, value);	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		} \
	}	\
	else if (destValue->type == OP_TYPE_STRING)	\
	{	\
		float destFloatValue;	\
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))	\
		{	\
			firstValue->type = OP_TYPE_FLOAT;	\
			if (secondValue->type == OP_TYPE_INT)	\
				firstValue->fFloatValue = OP(destFloatValue, (float)secondValue->iIntValue);	\
			else if (secondValue->type == OP_TYPE_FLOAT)	\
			{	\
				firstValue->fFloatValue = OP(destFloatValue, secondValue->fFloatValue);	\
			}	\
			else if (secondValue->type == OP_TYPE_STRING)	\
			{	\
				float value;	\
				if (CastStrToFloat(stringRawValue(secondValue), &value))	\
				{	\
					firstValue->fFloatValue = OP(destFloatValue, value);	\
				}	\
				else \
				{	\
					ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
				}	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
		}	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}


#define EXEC_INSTR_MATH_INTOP(OP, IntOP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, secondValue);	\
	if (destValue->type == OP_TYPE_INT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
			destValue->iIntValue = destValue->iIntValue IntOP secondValue->iIntValue;	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			destValue->fFloatValue = OP((float)destValue->iIntValue, secondValue->fFloatValue);	\
			destValue->type = OP_TYPE_FLOAT;	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				destValue->fFloatValue = OP((float)destValue->iIntValue, value);	\
				destValue->type = OP_TYPE_FLOAT;	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		if (secondValue->type == OP_TYPE_INT)	\
			destValue->fFloatValue = OP(destValue->fFloatValue,  secondValue->iIntValue);	\
		else if (secondValue->type == OP_TYPE_FLOAT)	\
		{	\
			destValue->fFloatValue = OP(destValue->fFloatValue, secondValue->fFloatValue);	\
		}	\
		else if (secondValue->type == OP_TYPE_STRING)	\
		{	\
			float value;	\
			if (CastStrToFloat(stringRawValue(secondValue), &value))	\
			{	\
				destValue->fFloatValue = OP(destValue->fFloatValue, value);	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else if (destValue->type == OP_TYPE_STRING)	\
	{	\
		float destFloatValue;	\
		if (CastStrToFloat(stringRawValue(destValue), &destFloatValue))	\
		{	\
			destValue->type = OP_TYPE_FLOAT;	\
			if (secondValue->type == OP_TYPE_INT)	\
				destValue->fFloatValue = OP(destFloatValue, (float)secondValue->iIntValue);	\
			else if (secondValue->type == OP_TYPE_FLOAT)	\
			{	\
				destValue->fFloatValue = OP(destFloatValue, secondValue->fFloatValue);	\
			}	\
			else if (secondValue->type == OP_TYPE_STRING)	\
			{	\
				float value;	\
				if (CastStrToFloat(stringRawValue(secondValue), &value))	\
				{	\
					destValue->fFloatValue = OP(destFloatValue, value);	\
				}	\
				else \
				{	\
					ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
				}	\
			}	\
			else \
			{	\
				ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
			}	\
		}	\
		else \
		{	\
			ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
		}	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}	

#define  EXEC_INSTR_MATH_INC(OP)	\
	ResolveOpPointer(0, destValue);	\
	if (destValue->type == OP_TYPE_INT)	\
		destValue->iIntValue##OP##OP;	\
	else if (destValue->type == OP_TYPE_FLOAT)	\
	{	\
		destValue->fFloatValue = destValue->fFloatValue OP 1.0f;	\
	}	\
	else \
		ExecError("attempt to perform %s%s operator on %s", #OP, #OP, GetOperatorName(0).c_str());	\


#define EXEC_INSTR_LOGIC_OP_TO(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, second);	\
	ResolveOpPointer(2, third);		\
	if (second->type == OP_TYPE_INT && third->type == OP_TYPE_INT)	\
	{	\
		destValue->type = OP_TYPE_INT;	\
		destValue->iIntValue = ((second->iIntValue OP third->iIntValue));	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}

#define EXEC_INSTR_LOGIC_OP(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, second);	\
	if (destValue->type == OP_TYPE_INT && second->type == OP_TYPE_INT)	\
	{	\
		destValue->iIntValue = (destValue->iIntValue OP second->iIntValue);	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}

#define EXEC_INSTR_BIT_OP(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, second);	\
	if (destValue->type == OP_TYPE_INT && second->type == OP_TYPE_INT)	\
	{	\
		destValue->iIntValue = (destValue->iIntValue OP second->iIntValue);	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(0).c_str(), GetOperatorName(1).c_str());	\
	}

#define EXEC_INSTR_BIT_OP_TO(OP)	\
	ResolveOpPointer(0, destValue);	\
	ResolveOpPointer(1, second);	\
	ResolveOpPointer(2, third);		\
	if (second->type == OP_TYPE_INT && third->type == OP_TYPE_INT)	\
	{	\
		destValue->type = OP_TYPE_INT;	\
		destValue->iIntValue = (second->iIntValue OP third->iIntValue);	\
	}	\
	else \
	{	\
		ExecError("attempt to perform %s operator on %s and %s", #OP, GetOperatorName(1).c_str(), GetOperatorName(2).c_str());	\
	}


#define GetHashPos(hashPos,t, key)	\
	hashPos = 0;	\
	if (IsUserType((key)->type))	\
	{	\
		hashPos = (int)(key)->userData % (t)->mNodeCapacity;	\
	}	\
	else \
	{	\
		switch ((key)->type)	\
		{	\
		case OP_TYPE_INT:	\
		case OP_TYPE_FLOAT:	\
			hashPos = (int)PNumberValue((key)) % (t)->mNodeCapacity;	\
			break;	\
		case OP_TYPE_STRING:	\
			hashPos = (int)(key)->stringValue->hash % ((t)->mNodeCapacity);	\
			break;	\
		case OP_TYPE_TABLE:	\
			hashPos = (int)(key)->tableData % ((t)->mNodeCapacity - 1);	\
		break;	\
		case OP_USERTYPE:	\
		hashPos = (int)(key)->userData % ((t)->mNodeCapacity - 1);	\
		break;	\
		case OP_TYPE_FUNC:	\
		hashPos = (int)(key)->func % ((t)->mNodeCapacity - 1);	\
		break;	\
		default:	\
			break;	\
		}	\
	}	


#endif // MacroDefs_h__