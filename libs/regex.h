#pragma once
#ifndef regex_h__
#define regex_h__
#include <vector>
#include <string>
#include <map>
#define MaxRegRepeat			0x7fffffff
#define MaxContentSize			64
#define MaxGroupNameSize		64
#define MaxCharRangeSize		8

enum MetaCharType
{
	MCT_Number = -1,
	MCT_Not_Number = -2,
	MCT_Space = -3,
	MCT_Not_Space = -4,
	MCT_Char = - 5,
	MCT_Not_Char = -6,
	MCT_Point = -7,
};

enum RegFactorType
{
	RFT_Normal,
	RFT_Start,
	RFT_Group,
};

class CharRange
{
public:
	CharRange(char _start = 0, char _end = 0)
		: start(_start), end(_end)
	{
	}

	char start;
	char end;
};

class RegexFactor
{
public:
	RegFactorType					type;

	RegexFactor**					subFactor;
	int								subFactorSize;

	RegexFactor*					lastFactor;
	RegexFactor*					startFactor;
	int								maxRepeat;
	int								minRepeat;
	bool							isNot;
	bool							isGreedy;
	char							mContent[MaxContentSize];
	int								mContentSize;

	CharRange						mCharRangeVec[MaxCharRangeSize];
	int								mCharRangeVecSize;

	int								groupIndex;
	std::string						groupName;

	int								refGroupIndex;
	char							refGroupName[MaxGroupNameSize];

	RegexFactor()
	{
		mContentSize = 0;
		mCharRangeVecSize = 0;
		memset(refGroupName, 0, MaxGroupNameSize);
		memset(mContent, 0, MaxContentSize);
		subFactor = NULL;
		subFactorSize = 0;
		groupIndex = -1;
		refGroupIndex = -1;
		type = RFT_Normal;
		isNot = false;
		isGreedy = true;
		maxRepeat = minRepeat = 1;
		lastFactor = NULL;
	}
};


class RegDataVec;

class RegData
{
public:
	RegexFactor*				factor;
	int							startIndex;
	int							numTimes;
	int							numLastTryTimes;
	int							numLastTryTimesSave;
	bool						isRollBack;
	RegDataVec*					subRegData;

	RegData(RegexFactor* _f = NULL)
	{
		numLastTryTimesSave = 0;
		startIndex = 0;
		factor = _f;
		numTimes = 0;
		numLastTryTimes = 0;
		isRollBack = false;
		subRegData = NULL;
	}

	~RegData();
};

class RegDataVec
{
public:
	RegData*		mRegData;
	int				mLastIndex;
	int				mMaxSize;

	~RegDataVec()
	{
		for (int i = 0; i <= mLastIndex; i++)	
			(&mRegData[i])->~RegData();	
		free(mRegData);
	}
};


#define	 InitRegData(T, size)		\
	(T).mMaxSize = size;	\
	(T).mLastIndex = -1;	\
	(T).mRegData = (RegData*)malloc(sizeof(RegData) * size);


#define	 Back(T)		(T).mRegData[(T).mLastIndex]
#define	 BackRegDataPointer(T)		(T)->mRegData[(T)->mLastIndex]


#define  PushRegData(T, t)		\
	if ((T).mLastIndex < (T).mMaxSize - 1)	\
	{	\
		(T).mLastIndex++;	\
		memcpy(&(T).mRegData[(T).mLastIndex], &t, sizeof(RegData));	\
	}	\
	else \
	{	\
		int newSize = (T).mMaxSize * 2;	\
		RegData* regData = (RegData*)malloc(sizeof(RegData) * newSize);	\
		memcpy(regData, (T).mRegData, sizeof(RegData) * (T).mMaxSize);	\
		free((T).mRegData);	\
		(T).mRegData = regData;	\
		(T).mMaxSize = newSize;	\
		(T).mLastIndex++;	\
		memcpy(&(T).mRegData[(T).mLastIndex], &t, sizeof(RegData));	\
	}

#define PopRegData(T)		\
	(&(T).mRegData[(T).mLastIndex])->~RegData();	\
	(T).mLastIndex--;


class MatchData
{
public:
	std::map<std::string, std::string>		groupNameMap;
	std::map<int, std::string>				groupIDMap;
	std::string								matchedValue;
};

class MatchGroupData
{
public:
	int startPos;
	int	endPos;

	MatchGroupData()
	{
		startPos = 0;
		endPos = 0;
	}
};

class Regex
{
public:
	Regex(char* regexExpr);
	~Regex();

	const std::vector<MatchData>&				Search(char* content);
	bool										Match(char* content);
	
protected:
	void										ClearRegFactor(RegexFactor* factor);
	RegexFactor*								AddRegFactor(RegexFactor* lastExpr);

	void										AddSubFactor(RegexFactor* lastExpr, RegexFactor* factor);

	bool										ParseRange(char* &expr, int& start, int& end);
	RegexFactor*								ParseExpr(char* &p, bool hasParent);
	inline bool									CanMatch(const RegexFactor* factor, char* c, int index, int& num);

	bool										MatchGroupRegExprGreedy(RegData& regData, char* content, int& index);
	bool										MatchGroupRegExprNoGreedy(RegData& regData, char* content, int& index);
	bool										MatchGroupRegExpr(RegData& regData, char* content, int& index);

	void										RefreshMatchGroupData(RegData &regData, int startIndex, int endIndex);

	bool										MatchRegExpr(RegDataVec& regDataVec, char* content, int& index);

protected:
	int											m_groupIncID;
	std::vector<std::string>					m_groupNameVec;
	RegexFactor*								m_rootRegexFactor;

	std::map<int, MatchGroupData>				m_matchGroupIDMap;
	std::map<std::string, MatchGroupData>		m_matchGroupNameMap;

	std::vector<MatchData>						m_matchResultVec;
};



#define  IsCharMatched(ret, c, charType)	\
	switch (charType)	\
	{	\
	case MCT_Number:	\
		ret = (c >= '0' && c <= '9');	\
		break;	\
	case MCT_Not_Number:	\
		ret = !(c >= '0' && c <= '9');	\
		break;	\
	case MCT_Space:	\
		ret = (c == '\r' || c == '\n' || c == '\t' || c == ' ');	\
		break;	\
	case MCT_Not_Space:	\
		ret = !(c == '\r' || c == '\n' || c == '\t' || c == ' ');	\
		break;	\
	case MCT_Char:	\
		ret = ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));	\
		break;	\
	case MCT_Not_Char:	\
		ret = !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));	\
		break;	\
	case MCT_Point:	\
		ret = (c != '\r' && c != '\n');	\
	}	


#define  GetNumMatchChar(numChar, factor)	\
	if (factor->refGroupIndex >= 0)	\
	{	\
		numChar = m_matchGroupIDMap[factor->refGroupIndex].endPos - m_matchGroupIDMap[factor->refGroupIndex].startPos;	\
	}	\
	else if (factor->refGroupName[0] != 0)	\
	{	\
		numChar = m_matchGroupNameMap[factor->refGroupName].endPos - m_matchGroupNameMap[factor->refGroupName].startPos;	\
	}	\
	numChar = factor->mContentSize;	\
	if (numChar < 1)	\
	{	\
		if (factor->mCharRangeVecSize > 0)	\
			numChar = 1;	\
	}


#endif // regex_h__