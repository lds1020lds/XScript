#ifndef  COMMON_FUNC_H_
#define  COMMON_FUNC_H_

#include <vector>
#include <string>

#ifndef TRUE
#define  TRUE	1
#endif
#ifndef FALSE
#define  FALSE	0
#endif

#define ICODE_NODE_INSTR        0               // An I-code instruction
#define ICODE_NODE_SOURCE_LINE  1               // Source-code annotation
#define ICODE_NODE_JUMP_TARGET  2               // A jump target

// ---- I-Code Instruction Opcodes --------------------------------------------------------

enum InstrCode
{
	INSTR_MOV,
	INSTR_ADD,
	INSTR_SUB,
	INSTR_MUL,
	INSTR_DIV,
	INSTR_MOD,
	INSTR_EXP,
	INSTR_NEG,
	INSTR_INC,
	INSTR_DEC,
	INSTR_AND,
	INSTR_OR,
	INSTR_XOR,
	INSTR_SHL,
	INSTR_SHR,
	INSTR_CONCAT,
	INSTR_JMP,
	INSTR_JE,
	INSTR_JNE,
	INSTR_JG,
	INSTR_JL,
	INSTR_JGE,
	INSTR_JLE,
	INSTR_PUSH,
	INSTR_POP,
	INSTR_CALL,
	INSTR_RET,
	INSTR_TYPE,
	INSTR_ADD_TO,
	INSTR_SUB_TO,
	INSTR_MUL_TO,
	INSTR_DIV_TO,
	INSTR_MOD_TO,
	INSTR_EXP_TO,
	INSTR_AND_TO,
	INSTR_OR_TO,
	INSTR_XOR_TO,
	INSTR_NOT_TO,
	INSTR_SHL_TO,
	INSTR_SHR_TO,
	INSTR_CONCAT_TO,
	INSTR_LOGIC_NOT,
	INSTR_TEST_E,
	INSTR_TEST_NE,
	INSTR_TEST_G,
	INSTR_TEST_L,
	INSTR_TEST_GE,
	INSTR_TEST_LE,
	INSTR_LOGIC_AND,
	INSTR_LOGIC_OR,
	INSTR_FUNC,
	INSTR_LOADNIL,
};

enum TokenType
{
	TOKEN_TYPE_END_OF_STREAM,
	TOKEN_TYPE_INVALID,
	TOKEN_TYPE_INT,
	TOKEN_TYPE_FLOAT,
	TOKEN_TYPE_IDENT,
	TOKEN_TYPE_RSRVD_VAR,
	TOKEN_TYPE_RSRVD_TRUE,
	TOKEN_TYPE_RSRVD_FALSE,
	TOKEN_TYPE_RSRVD_IF,
	TOKEN_TYPE_RSRVD_ELSE,
	TOKEN_TYPE_RSRVD_BREAK,
	TOKEN_TYPE_RSRVD_CONTINUE,
	TOKEN_TYPE_RSRVD_FOR,
	TOKEN_TYPE_RSRVD_WHILE,
	TOKEN_TYPE_RSRVD_FUNC,
	TOKEN_TYPE_RSRVD_RETURN,
	TOKEN_TYPE_RSRVD_NIL,
	TOKEN_TYPE_OP,
	TOKEN_TYPE_DELIM_COMMA,
	TOKEN_TYPE_DELIM_OPEN_PAREN,
	TOKEN_TYPE_DELIM_CLOSE_PAREN,
	TOKEN_TYPE_DELIM_OPEN_BRACE,
	TOKEN_TYPE_DELIM_CLOSE_BRACE,
	TOKEN_TYPE_DELIM_OPEN_CURLY_BRACE,
	TOKEN_TYPE_DELIM_CLOSE_CURLY_BRACE,
	TOKEN_TYPE_DELIM_SEMICOLON,
	TOKEN_TYPE_DELIM_JINGHAO,
	TOKEN_TYPE_DELIM_COLON,
	TOKEN_TYPE_DELIM_INTERROGATION,
	TOKEN_TYPE_DELIM_POINT,
	TOKEN_TYPE_DELIM_THREE_POINT,
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_RSRVD_FOREACH,
	TOKEN_TYPE_RSRVD_IN,
};



#define OP_TYPE_ADD                     0       // +
#define OP_TYPE_SUB                     1       // -
#define OP_TYPE_MUL                     2       // *
#define OP_TYPE_DIV                     3       // /
#define OP_TYPE_MOD                     4       // %
#define OP_TYPE_EXP                     5       // ^
#define OP_TYPE_CONCAT                  35       // $

#define OP_TYPE_INC                     15      // ++
#define OP_TYPE_DEC                     17      // --

#define OP_TYPE_ASSIGN                  11      // =
#define OP_TYPE_ASSIGN_ADD              14      // +=
#define OP_TYPE_ASSIGN_SUB              16      // -=
#define OP_TYPE_ASSIGN_MUL              18      // *=
#define OP_TYPE_ASSIGN_DIV              19      // /=
#define OP_TYPE_ASSIGN_MOD              20      // %=
#define OP_TYPE_ASSIGN_EXP              21      // ^=
#define OP_TYPE_ASSIGN_CONCAT           36      // $=

// ---- Bitwise

#define OP_TYPE_BITWISE_AND             6       // &
#define OP_TYPE_BITWISE_OR              7       // |
#define OP_TYPE_BITWISE_XOR             8       // #
#define OP_TYPE_BITWISE_NOT             9       // ~
#define OP_TYPE_BITWISE_SHIFT_LEFT      30      // <<
#define OP_TYPE_BITWISE_SHIFT_RIGHT     32      // >>

#define OP_TYPE_ASSIGN_AND              22      // &=
#define OP_TYPE_ASSIGN_OR               24      // |=
#define OP_TYPE_ASSIGN_XOR              26      // #=
#define OP_TYPE_ASSIGN_SHIFT_LEFT       33      // <<=
#define OP_TYPE_ASSIGN_SHIFT_RIGHT      34      // >>=


#define OP_TYPE_LOGICAL_AND             23      // &&
#define OP_TYPE_LOGICAL_OR              25      // ||
#define OP_TYPE_LOGICAL_NOT             10      // !

// ---- Relational

#define OP_TYPE_EQUAL                   28      // ==
#define OP_TYPE_NOT_EQUAL               27      // !=
#define OP_TYPE_LESS                    12      // <
#define OP_TYPE_GREATER                 13      // >
#define OP_TYPE_LESS_EQUAL              29      // <=
#define OP_TYPE_GREATER_EQUAL           31      // >=


#define OP_TYPE_POINTER					40      // :
#define OP_TYPE_DOUBLECOLON				41      // ::

// 
#define		MAX_FUNC_REG			8

enum RuntimeOperatorType
{
	ROT_Int,
	ROT_Float,
	ROT_String,
	ROT_Table,
	ROT_UpValue_Table,
	ROT_UpVal_Index,
	ROT_Stack_Index,
	ROT_Instr_Index,
	ROT_FuncValue,
	ROT_Reg,
};

enum ParseOperandType
{
	PST_Int,
	PST_Float,
	PST_String_Index,
	PST_Var_Index,
	PST_JumpTarget_Index,
	PST_FuncIndex,
	PST_Reg,
	PST_Table,
};

enum ValueType
{
	OP_TYPE_NIL = -1,
	OP_TYPE_INT = 0,
	OP_TYPE_FLOAT,
	OP_TYPE_STRING,
	OP_TYPE_TABLE,
	OP_LIGHTUSERDATA,
	OP_TYPE_FUNC,
	OP_TYPE_PROTO,
	OP_TYPE_UPVAL,
	OP_TYPE_THREAD,
	OP_TYPE_USERDATA,
};

enum MetaMethodType
{
	MMT_Index,
	MMT_NewIndex,
	MMT_Equal,
	MMT_Add,
	MMT_Sub,
	MMT_Mul,
	MMT_Div,
	MMT_Mod,
	MMT_Pow,
	MMT_Neg,
	MMT_Len,
	NMT_Less,
	NMT_LessEqual,
	MMT_Concat,
	MMT_Call,
	NMT_Great,
	NMT_GreatEqual,
	MTT_Count,
};



#define INSTR_TYPE_CODE             1
#define INSTR_TYPE_JUMPTARGET       2

#define MAX_ERRORINFO_SIZE          100

#define		MAX_NUMBER_STRING_SIZE			(64)
#define		MAX_INSTR_NAME_SIZE				(16)
#define		MAX_TOKEN_NAME_SIZE				(32)


#define		MAX_IDENT_SIZE    (40)
#define		MAX_STRING_SIZE   (256)
#define		IDENT_TYPE_PARAM  (1)
#define		IDENT_TYPE_VAR    (2)
#define		IDENT_TYPE_INTERNAL_VAR   (3)
#define		MAX_FUNC_NAME_SIZE	64

#define		IsUserType(type)		((type >> 16) == OP_LIGHTUSERDATA)
#define		UserDataType(type)		(type & 0xffff)
#define		MAKE_USERTYPE(index)	((OP_LIGHTUSERDATA << 16) + index)

#define		ARGS					("args")
#define		UPVALMASK				0x1000000
#define		MAX_PARAM_NUM			128
#define		MAX_TAGMETHOD_LOOP		10

#define USE_HIGH_PRECIOUS_NUMBER
#ifdef USE_HIGH_PRECIOUS_NUMBER

typedef	signed long long		XInt;
typedef	double					XFloat;

#define		XIntConFmt			"%lld"
#define		XFloatConFmt		"%lf"
#define		XIntConFmt2			"[%lld]"
#define		XFloatConFmt2		"[%lf]"
#define		StrToXInt			atoll
#define		StrToXFloat			atof
#define		XFMod				fmod
#else

typedef		int				XInt;
typedef		float					XFloat;
#define		XIntConFmt			"%d"
#define		XFloatConFmt		"%f"
#define		XIntConFmt2			"[%d]"
#define		XFloatConFmt2		"[%f]"
#define		StrToXInt			atoi
#define		StrToXFloat			(float)atof
#define		XFMod				fmodf

#endif


enum
{
	VLOCAL,
	VUPVALUE,
	VGLOBAL,
};

struct Operand
{
	ParseOperandType operandType;
	union
	{
		XInt		iIntValue;
		XFloat		fFloatValue;
		int			iSymbolIndex;
		int			iJumppIndex;
		int			iStringIndex;
		int			iRegIndex;
		int			iFunData;
	};

	int tableIndexType;

	union 
	{
		XInt		iIntTableValue;
		XFloat		fFloatTableValue;
	};

	Operand()
	{
		tableIndexType = 0;
	}
};

struct  Code
{
	int iCodeOpr;
	std::vector<Operand> oprList;
};

struct JumpTarget
{
	int jumpIndex;
	int codeIndex;
};

struct ICode
{
	int iCodeType;
	Code code;
	int  JumpIndex;
	int	 lineIndex;
};

class XScriptVM;
typedef void(*HOST_FUNC)(XScriptVM* vm);

struct VariantST
{
	int  iIndex;
	char varName[MAX_IDENT_SIZE];
	int  iScope;
	int  iSize;
	int  iType;
	int  stackIndex;
	std::vector<Operand> initValues;
};

class UpValueST
{
public:
	UpValueST(int _type, int _index)
		: type(_type), index(_index)
	{

	}
	int type;
	int index;
};

struct FunctionST
{
	int					iIndex;
	char				funcName[MAX_IDENT_SIZE];
	int					iParamSize;
	int					localDataSize;
	int					curParamIndex;
	bool				hasVarArgs;
	int					curVarIndex;
	int					parentIndex;
	std::vector<int>	subIndexVec;

	std::vector<UpValueST>	upValueVec;
};


struct HostFunctionST
{
	int		    iIndex;
	char		funcName[MAX_IDENT_SIZE];
	int			iParamSize;
	HOST_FUNC	pfnAddr;
};

struct StringST
{
	int         iIndex;
	char        str[MAX_STRING_SIZE];
};


inline std::string ConvertToString(const int n)
{
	char text[32] = { 0 };
	snprintf(text, 32, "%d", n);
	return text;
}




int IsCharDelim ( char cChar );
int IsCharWhitespace ( char cChar );
int IsCharNumeric ( char cChar );
int IsCharIdent ( char cChar );
int IsCharOpChar (char curChar, int preIndex, int iLevel);

void ExitOnError(const char* info, int iLine, int iChar);
int  GetOprType(int iLevel,  int iIndex);
bool IsRelativeOpr(int opr);
bool IsLogicalOpr(int opr);

#endif