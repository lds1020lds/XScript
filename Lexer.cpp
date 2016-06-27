#include "Lexer.h"
#include "sourcefile.h"
#include "Commonfunc.h"

static int sOpTypes[] = 
{
	OP_TYPE_ADD,
	OP_TYPE_SUB,
	OP_TYPE_MUL ,
	OP_TYPE_DIV,
	OP_TYPE_MOD ,
	OP_TYPE_EXP ,
	OP_TYPE_CONCAT ,

	OP_TYPE_INC,
	OP_TYPE_DEC,

	OP_TYPE_ASSIGN,
	OP_TYPE_ASSIGN_ADD,
	OP_TYPE_ASSIGN_SUB,
	OP_TYPE_ASSIGN_MUL,
	OP_TYPE_ASSIGN_DIV,
	OP_TYPE_ASSIGN_MOD ,
	OP_TYPE_ASSIGN_EXP,
	OP_TYPE_ASSIGN_CONCAT,

	// ---- Bitwise

	OP_TYPE_BITWISE_AND,
	OP_TYPE_BITWISE_OR ,
	OP_TYPE_BITWISE_XOR ,
	OP_TYPE_BITWISE_NOT  ,
	OP_TYPE_BITWISE_SHIFT_LEFT,
	OP_TYPE_BITWISE_SHIFT_RIGHT,

	OP_TYPE_ASSIGN_AND,
	OP_TYPE_ASSIGN_OR     ,
	OP_TYPE_ASSIGN_XOR  ,
	OP_TYPE_ASSIGN_SHIFT_LEFT,
	OP_TYPE_ASSIGN_SHIFT_RIGHT,


	OP_TYPE_LOGICAL_AND,
	OP_TYPE_LOGICAL_OR,
	OP_TYPE_LOGICAL_NOT ,

	// ---- Relational

	OP_TYPE_EQUAL ,
	OP_TYPE_NOT_EQUAL ,
	OP_TYPE_LESS  ,
	OP_TYPE_GREATER,
	OP_TYPE_LESS_EQUAL ,
	OP_TYPE_GREATER_EQUAL,
};

static struct ProrityData {
	int op;  /* left priority for each binary operator */
	int priority; /* right priority */
} priorityMap[] = {  /* ORDER OPR */
	{ OP_TYPE_ADD, 4 },{ OP_TYPE_SUB, 4 },{ OP_TYPE_MUL, 3 },{ OP_TYPE_DIV, 3 },{ OP_TYPE_MOD, 3 },  /* `+' `-' `/' `%' */
	{ OP_TYPE_EXP, 3 },{ OP_TYPE_CONCAT, 4 },                 /* power and concat (right associative) */
	{ OP_TYPE_BITWISE_AND, 8 },{ OP_TYPE_BITWISE_OR, 10 },                  /* equality and inequality */
	{ OP_TYPE_BITWISE_XOR, 9 },{ OP_TYPE_BITWISE_SHIFT_LEFT, 5 },{ OP_TYPE_BITWISE_SHIFT_RIGHT, 5 },{ OP_TYPE_LOGICAL_AND, 11 },  /* order */
	{ OP_TYPE_LOGICAL_OR, 12 },{ OP_TYPE_EQUAL, 7 },{ OP_TYPE_NOT_EQUAL, 7 },{ OP_TYPE_LESS, 6 },{ OP_TYPE_GREATER, 6 },{ OP_TYPE_LESS_EQUAL, 6 },{ OP_TYPE_GREATER_EQUAL, 6 },                  /* logical (and/or) */
};
static char* sOpNames[] = 
{
	"+",
	"-",
	"*",
	"/",
	"%",
	"^",
	"$",
	"++",
	"--",
	"=",
	"+=",
	"-=",
	"*=",
	"/=",
	"%=",
	"^=",
	"$=",
	"&",
	"|",
	"#",
	"~",
	"<<",
	">>",
	"&=",
	"|=",
	"#=",
	"<<=",
	">>=",
	"&&",
	"||",
	"!",
	"==",
	"!=",
	"<",
	">",
	"<=",
	">="
};



// ---- Delimiters ------------------------------------------------------------------------


int		CLexer::GetOpPrority(int op)
{
	int count = sizeof(priorityMap) / sizeof(ProrityData);
	for (int i = 0; i < count; i++)
	{
		if (priorityMap[i].op == op)
		{
			return priorityMap[i].priority;
		}
	}

	return -1;
}

CLexer::CLexer(CSourceFile *pSourceFile)
{
	mLastCharIndex = 0;
	mLastLineIndex = 0;
	mSourceFile = pSourceFile;

	mIsRecordToken = false;
	mIsReadTokenFromRecord = false;

	mRecordTokenIndex = 0;
	mReadTokeIndex = 0;

	mLastLookToken = false;
	mLastLookTokenInRead = false;

	mCurRecordDepth = 0;
}

CLexer::~CLexer()
{
	
}
TOKEN   CLexer::LookNextToken()
{

	if (mIsReadTokenFromRecord)
		mLastLookTokenInRead = true;

	if (mIsReadTokenFromRecord)
	{
		if (mReadTokeIndex >= mNumRecordTokens[mCurRecordDepth] - 1)
		{
			return TOKEN_TYPE_INVALID;
		}
		return mRecordTokens[mCurRecordDepth][mReadTokeIndex];
	}

	int iLastLineIndex = mLastLineIndex;
	int iLastCharIndex = mLastCharIndex;
	TOKEN token = GetNextToken();

	if (mIsRecordToken)
	{	
		mLastLookToken = true;
	}


	RewindToken();
	mLastLineIndex = iLastLineIndex;
	mLastCharIndex = iLastCharIndex;
	return token;
}


TOKEN   CLexer::GetNextToken()
{
	if (mIsReadTokenFromRecord)
	{
		mLastLookTokenInRead = false;
		if (mReadTokeIndex >= mNumRecordTokens[mCurRecordDepth])
		{
			return TOKEN_TYPE_INVALID;
		}
		mReadTokeIndex++;
		return mRecordTokens[mCurRecordDepth][mReadTokeIndex - 1];
	}
	mLastLineIndex = mSourceFile->GetCurLineIndex();
	mLastCharIndex = mSourceFile->GetCurCharIndex();

	bool bAdd = true;
	bool bIsDone = false;
	bool bRewindChar = true;
	int  state = LEX_STATE_START;
	
	char lexeme[MAX_LEXEME_SIZE] = {0};
	int  iLexemeIndex = 0;
	int  iOprLevel = 0;
	int  iLastOprIndex = 0;
	while(!bIsDone)
	{
		bAdd = true;
		char curChar = mSourceFile->GetNextChar();
		if (curChar == '\0')
		{
			bAdd = false;
			bRewindChar = false;
			break;
		}
		switch(state)
		{
		case LEX_STATE_START:
			{
				if (IsCharWhitespace(curChar))
				{
					bAdd = false;
					break;
				} else if (IsCharNumeric(curChar))
				{
					state = LEX_STATE_INT;
				}
				else if (IsCharIdent(curChar))
				{
					state = LEX_STATE_IDENT;
				}
				else if (IsCharDelim(curChar))
				{
					state = LEX_STATE_DELIM;
				}
				else if (curChar == '\"')
				{
					bAdd = false;
					state = LEX_STATE_STRING;
				}
				else if (curChar == '.')
				{
					state = LEX_STATE_POINT;
				}

				if (curChar == '/' 
					&& (mSourceFile->LookNextChar() == '/'
					|| mSourceFile->LookNextChar() == '*'))
				{
					if (mSourceFile->GetNextChar() == '/')
					{
						state = LEX_STATE_LINE_COMMENT;
					}
					else
					{
						state = LEX_STATE_SEGMENT_COMMENT;
					}
					bAdd = false;
					
				}
				else if (IsCharOpChar(curChar, iLastOprIndex, iOprLevel) >= 0)
				{
					iLastOprIndex = IsCharOpChar(curChar, iLastOprIndex, iOprLevel);
					iOprLevel++;
					state = LEX_STATE_OP;
				}
				break;
			}
		case LEX_STATE_POINT:
			{
				if (curChar == '.')
				{
					state = LEX_STATE_TWO_POINT;
				}
				else
				{
					bIsDone = true;
					bAdd = false;
				}
			}
			break;
		case LEX_STATE_TWO_POINT:
		{
			if (curChar == '.')
			{
				state = LEX_STATE_THREE_POINT;
			}
			else
			{
				bAdd = false;
				state = LEX_STATE_UNKNOWN;
			}
		}
		break;
		case LEX_STATE_THREE_POINT:
		{
			bIsDone = true;
			bAdd = false;
		}
		break;
		case LEX_STATE_DELIM:
			bIsDone = true;
			bAdd = false;
			break;
		case LEX_STATE_INT:
			{
				if(IsCharNumeric(curChar))
				{
					state = LEX_STATE_INT;
				}
				else if (curChar == '.')
				{
					state = LEX_STATE_FLOAT; 
				}
				else if (IsCharIdent(curChar))
				{
					bAdd = false;
					state = LEX_STATE_UNKNOWN;
				}
				else 
				{
					bAdd = false;
					bIsDone = true;
				}
				break;
			}
		case LEX_STATE_FLOAT:
			{
				if(IsCharNumeric(curChar))
				{
					state = LEX_STATE_FLOAT;
				}
				else if (!IsCharIdent(curChar))
				{
					bAdd = false;
					bIsDone = true;
				}
				else
				{
					state = LEX_STATE_UNKNOWN;
				}
				break;
			}
		case LEX_STATE_IDENT:
			{
				if (!IsCharIdent(curChar))
				{
					bAdd = false;
					bIsDone = true;
				}
				break;
			}
		case LEX_STATE_STRING:
			{
				if (curChar == '"')
				{
					bAdd = false;
					bRewindChar = false;
					bIsDone = true;
				}
				else if (curChar == '\\')
				{
					bAdd = false;
					state = LEX_STATE_STRING_CLOSE_QUOTE;
				}
				break;
			}
		case LEX_STATE_STRING_CLOSE_QUOTE:
			{
				if (curChar == 't')
				{
					curChar = '\t';
				}
				else if (curChar == 'r')
				{
					curChar = '\r';
				}
				else if (curChar == 'n')
				{
					curChar = '\n';
				}
				state = LEX_STATE_STRING;
				break;
			}
		case LEX_STATE_OP:
			{
				if(IsCharOpChar(curChar, iLastOprIndex, iOprLevel) >= 0)
				{
					state = LEX_STATE_OP;
					iLastOprIndex = IsCharOpChar(curChar, iLastOprIndex, iOprLevel);
					iOprLevel++;
				}
				else
				{
					mOprType = GetOprType(iOprLevel - 1, iLastOprIndex);
					bIsDone = true;
					bAdd    = false;
				}
				break;
			}
		case LEX_STATE_LINE_COMMENT:
			{
				bAdd = false;
				if (curChar == 10)
				{
					state = LEX_STATE_START; 
				}
				break;
			}
		case LEX_STATE_SEGMENT_COMMENT:
			{
				bAdd = false;
				if (curChar == '*' && mSourceFile->GetNextChar() == '/')
				{
					state = LEX_STATE_START;
				}
				break;
			}
		default:
			break;
		}

		if (LEX_STATE_UNKNOWN == state)
		{
			_ASSERT(FALSE);
		}
		if (bAdd)
		{
			if (iLexemeIndex < MAX_LEXEME_SIZE)
				lexeme[iLexemeIndex++] = curChar;
			else
			{
				char errInfo[128] = { 0 };
				sprintf(errInfo, "lexeme too long");
				ExitOnError(errInfo, mLastLineIndex, mLastCharIndex);
			}
		}
	}
	if (bRewindChar)
		mSourceFile->ReWindChar();
	lexeme[iLexemeIndex] = '\0';


	TOKEN  token;
	switch(state)
	{
	case LEX_STATE_START:
		token = TOKEN_TYPE_END_OF_STREAM;
		break;
	case LEX_STATE_INT:
		token = TOKEN_TYPE_INT;
		break;
	case LEX_STATE_FLOAT:
		token = TOKEN_TYPE_FLOAT;
		break;
	case LEX_STATE_STRING:
		token = TOKEN_TYPE_STRING;
		break;
	case LEX_STATE_POINT:
		token = TOKEN_TYPE_DELIM_POINT;
		break;
	case LEX_STATE_THREE_POINT:
		token = TOKEN_TYPE_DELIM_THREE_POINT;
		break;
	case LEX_STATE_DELIM:
		{
			if (lexeme[0] == ',')
				token = TOKEN_TYPE_DELIM_COMMA;
			else if(lexeme[0] == '(')
				token = TOKEN_TYPE_DELIM_OPEN_PAREN;
			else if(lexeme[0] == ')')
				token = TOKEN_TYPE_DELIM_CLOSE_PAREN;
			else if(lexeme[0] == '[')
				token = TOKEN_TYPE_DELIM_OPEN_BRACE;
			else if(lexeme[0] == ']')
				token = TOKEN_TYPE_DELIM_CLOSE_BRACE;
			else if(lexeme[0] == '{')
				token = TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE;
			else if(lexeme[0] == '}')
				token = TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE;
			else if(lexeme[0] == ';')
				token = TOKEN_TYPE_DELIM_SEMICOLON;
			else if(lexeme[0] == '#')
				token = TOKEN_TYPE_DELIM_JINGHAO;
			else if(lexeme[0] == '?')
				token = TOKEN_TYPE_DELIM_INTERROGATION;
			else if(lexeme[0] == ':')
				token = TOKEN_TYPE_DELIM_COLON;
			break;
		}
	case LEX_STATE_IDENT:
		{
			token = GetReserveType(lexeme);
			if (token == TOKEN_TYPE_INVALID)
			{
				token = TOKEN_TYPE_IDENT;
			}
			break;
		}
	case LEX_STATE_OP:
		{
			token = TOKEN_TYPE_OP;
			break;
		}
	case LEX_STATE_LINE_COMMENT:
		{
			token = TOKEN_TYPE_END_OF_STREAM;
			break;
		}
	case LEX_STATE_SEGMENT_COMMENT:
		{
			token = TOKEN_TYPE_END_OF_STREAM;
			break;
		}
	default:
		token = TOKEN_TYPE_INVALID;
		break;
	}
	strncpy_s(mCurLexeme, lexeme, MAX_LEXEME_SIZE);
	mCurToken = token;

	if (mIsRecordToken)
	{
		mRecordTokens[mCurRecordDepth][mRecordTokenIndex] = token;
		mTokensLexeme[mCurRecordDepth][mRecordTokenIndex] = mCurLexeme;
		mRecordTokenIndex++;
		mLastLookToken = false;
	}

	return token;
}

void    CLexer::RewindToken()
{
	if (mIsReadTokenFromRecord)
	{
		mReadTokeIndex--;
	} 
	else
	{
		mSourceFile->SetCurLineIndex(mLastLineIndex);
		mSourceFile->SetCurCharIndex(mLastCharIndex);
		if (mIsRecordToken)
		{
			mRecordTokenIndex--;
		}
	}

}
TOKEN    CLexer::GetCurToken()
{
	return mCurToken;
}
const char*  CLexer::GetCurLexeme()
{
	if (mIsReadTokenFromRecord)
		return mTokensLexeme[mCurRecordDepth][mReadTokeIndex - (mLastLookTokenInRead?0:1)].c_str();
	return mCurLexeme;
}
char    CLexer::GetAheadChar()
{
	GetNextToken();
	char c = mCurLexeme[0];
	RewindToken();
	return c;
}


int      CLexer::GetReserveType(const char *ident)
{
	if (_stricmp(ident, "var") == 0)
		return TOKEN_TYPE_RSRVD_VAR;
	else if(_stricmp(ident, "true") == 0)
		return TOKEN_TYPE_RSRVD_TRUE;
	else if(_stricmp(ident, "false") == 0)
		return TOKEN_TYPE_RSRVD_FALSE;
	else if(_stricmp(ident, "if") == 0)
		return TOKEN_TYPE_RSRVD_IF;
	else if(_stricmp(ident, "else") == 0)
		return TOKEN_TYPE_RSRVD_ELSE;
	else if(_stricmp(ident, "break") == 0)
		return TOKEN_TYPE_RSRVD_BREAK;
	else if(_stricmp(ident, "continue") == 0)
		return TOKEN_TYPE_RSRVD_CONTINUE;
	else if(_stricmp(ident, "for") == 0)
		return TOKEN_TYPE_RSRVD_FOR;
	else if(_stricmp(ident, "while") == 0)
		return TOKEN_TYPE_RSRVD_WHILE;
	else if(_stricmp(ident, "function") == 0)
		return TOKEN_TYPE_RSRVD_FUNC;
	else if(_stricmp(ident, "return") == 0)
		return TOKEN_TYPE_RSRVD_RETURN;
	else if (_stricmp(ident, "nil") == 0)
		return TOKEN_TYPE_RSRVD_NIL;
	else if (_stricmp(ident, "foreach") == 0)
		return TOKEN_TYPE_RSRVD_FOREACH;
	else if (_stricmp(ident, "in") == 0)
		return TOKEN_TYPE_RSRVD_IN;
	else
		return TOKEN_TYPE_INVALID;
}

bool	CLexer::TestToken(int token)
{
	if (token == LookNextToken())
	{
		GetNextToken();
		return true;
	}
	return false;
}

bool    CLexer::ExpectToken(int token, int op)
{
	if(token  != GetNextToken() || (op >= 0 && GetCurOprType() != op ))
	{
		char tokeName[MAX_TOKEN_NAME_SIZE] = {0};
		switch (token)
		{
		case TOKEN_TYPE_INT:
			{
				strncpy_s(tokeName, "int", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_FLOAT:
			{
				strncpy_s(tokeName, "float", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_IDENT:
			{
				strncpy_s(tokeName, "identifier", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_VAR:
			{
				strncpy_s(tokeName, "var", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_IF:
			{
				strncpy_s(tokeName, "if", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_ELSE:
			{
				strncpy_s(tokeName, "else", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_FOR:
			{
				strncpy_s(tokeName, "for", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_RSRVD_RETURN:
			{
				strncpy_s(tokeName, "return", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_COMMA:
			{
				strncpy_s(tokeName, ",", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_OPEN_PAREN:
			{
				strncpy_s(tokeName, "(", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_CLOSE_PAREN:
			{
				strncpy_s(tokeName, ")", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_OPEN_BRACE:
			{
				strncpy_s(tokeName, "[", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE:
			{
				strncpy_s(tokeName, "{", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE:
			{
				strncpy_s(tokeName, "}", MAX_TOKEN_NAME_SIZE);
				break;
			}
		case TOKEN_TYPE_DELIM_SEMICOLON:
			{
				strncpy_s(tokeName, ";", MAX_TOKEN_NAME_SIZE);
				break;
			}
		}
		RewindToken();
		char errInfo[128] = {0};
		sprintf(errInfo, "Expected %s", tokeName);
		ExitOnError(errInfo, mLastLineIndex, mLastCharIndex);
	}
	return false;
}

void	CLexer::BackToken()
{
	RewindToken();
	LookNextToken();
}

void    CLexer::CopyLexme(char* buffer)
{
	_ASSERT(buffer);
	strcpy(buffer, mCurLexeme);
}

int     CLexer::GetCurOprType()
{
	if (mIsReadTokenFromRecord)
	{
		const char* opName = GetCurLexeme();
		for (int i = 0; i < sizeof(sOpTypes) / sizeof(int); i ++)
		{
			if (strcmp(sOpNames[i], opName) == 0)
			{
				return sOpTypes[i];
			}
		}
		return -1;
	}
	return mOprType;
}





void    CLexer::beginRecordToken()
{
	mCurRecordDepth++;
	mIsRecordToken = true;
	mRecordTokenIndex = 0;
	mNumRecordTokens[mCurRecordDepth] = 0;
	mLastLookToken = false;
}


void    CLexer::endRecordToken()
{
	mIsRecordToken = false;
	mNumRecordTokens[mCurRecordDepth] = mRecordTokenIndex;
	if (mLastLookToken)
		mNumRecordTokens[mCurRecordDepth]++;
	mRecordTokenIndex = 0;

	
}

void    CLexer::beginReadTokenFromRecord()
{
	mLastLookTokenInRead = false;
	mIsReadTokenFromRecord = true;
	mReadTokeIndex = 0;
}

void    CLexer::endReadTokenFromRecord()
{
	mIsReadTokenFromRecord = false;
	mReadTokeIndex = 0;
	mCurRecordDepth--;
}



