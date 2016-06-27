#pragma once


typedef    int    TOKEN;
#define    MAX_LEXEME_SIZE        (256)


#define LEX_STATE_UNKNOWN               0       // Unknown lexeme type
#define LEX_STATE_START                 1       // Start state
#define LEX_STATE_INT                   2       // Integer
#define LEX_STATE_FLOAT                 3       // Float
#define LEX_STATE_IDENT                 4       // Identifier
#define LEX_STATE_OP                    5       // Operator
#define LEX_STATE_DELIM                 6       // Delimiter    
#define LEX_STATE_STRING                7       // String value
#define LEX_STATE_STRING_ESCAPE         8       // Escape sequence
#define LEX_STATE_STRING_CLOSE_QUOTE    9       // String closing quote

#define LEX_STATE_LINE_COMMENT          10       // comment "//"
#define LEX_STATE_SEGMENT_COMMENT       11       // comment "/* */"

#define LEX_STATE_POINT					12       // comment "/* */"
#define LEX_STATE_TWO_POINT					13       // comment "/* */"
#define LEX_STATE_THREE_POINT			14       // comment "/* */"

#define  MAX_RECORD_DEPTH               7
#define  MAX_NUM_RECORD_TOKEN           200
class CSourceFile;
#include <string>
class CLexer
{
public:
	CLexer(CSourceFile *pSourceFile);
	~CLexer(void);
	TOKEN   GetNextToken();
	TOKEN   LookNextToken();
	void    RewindToken();
	void	BackToken();
	bool	TestToken(int token);

	TOKEN    GetCurToken();
	const char* GetCurLexeme();
	char    GetAheadChar();
	bool    ExpectToken(int token, int op = -1);
	int     GetLine(){return mLastLineIndex;}
	int     GetChar(){return mLastLineIndex;}
	void    CopyLexme(char* buffer);
	int     GetCurOprType();
	int		GetOpPrority(int op);

	void    beginRecordToken();
	void    endRecordToken();

	void    beginReadTokenFromRecord();
	void    endReadTokenFromRecord();
private:
	int      GetReserveType(const char *ident);
	CSourceFile*  mSourceFile;
	TOKEN		 mCurToken;
	int          mOprType;
	char	     mCurLexeme[MAX_LEXEME_SIZE];
	int		     mLastCharIndex, mLastLineIndex;

	int          mRecordTokens[MAX_RECORD_DEPTH][MAX_NUM_RECORD_TOKEN];
	std::string  mTokensLexeme[MAX_RECORD_DEPTH][MAX_NUM_RECORD_TOKEN];
	bool         mIsRecordToken;
	bool         mIsReadTokenFromRecord;
	int          mRecordTokenIndex;
	int          mReadTokeIndex;
	int          mNumRecordTokens[MAX_RECORD_DEPTH];
	bool         mLastLookToken;
	bool         mLastLookTokenInRead;

	int          mCurRecordDepth;
};
