#include "sourcefile.h"
#include "Commonfunc.h"
#include "lexer.h"

#define       MAX_LINE_SIZE         (256)
using namespace std;

CSourceFile::CSourceFile(void)
{
	Reset();
}

CSourceFile::~CSourceFile(void)
{
	for (int i = 0; i < (int)mSourceFileDataList.size(); i++)
	{
		if(mSourceFileDataList[i].pSourceCode != NULL)
			delete mSourceFileDataList[i].pSourceCode;
	}
}
void CSourceFile::Reset()
{
	m_iCurChar = 0;
	m_iCurLine = 0;
}
bool   CSourceFile::LoadSourceFile(const char* pFileName)
{
	m_iMaxLine = 0;
	bool bSucc = LinkSourceFile(pFileName);
	return bSucc;
}

std::vector<std::string> Split(const std::string& src, char key)
{
	std::vector<std::string> result;

	std::string temp(src);
	while (true)
	{
		int pos = temp.find(key);
		if (pos == -1)
			break;

		result.push_back(temp.substr(0, pos));
		temp.erase(0, pos + 1);
	}

	if (!temp.empty())
		result.push_back(temp);

	return result;
}

void  CSourceFile::LoadFromString(const std::string& str)
{
	m_iMaxLine = 0;
	std::vector<std::string> lineVec = Split(str, '\n');
	for (int i = 0; i < (int)lineVec.size(); i++)
	{
		LineSourceCode lineCode;
		int iLen = lineVec[i].length();
		lineCode.pSourceCode = new char[iLen + 1];
		strncpy(lineCode.pSourceCode, lineVec[i].c_str(), iLen + 1);
		lineCode.iLength = iLen;
		mSourceFileDataList.push_back(lineCode);
		m_iMaxLine++;
	}
}

bool  CSourceFile::LinkSourceFile(const char* pFileName)
{
	FILE *fp = fopen(pFileName, "r");
	if (fp == NULL)
		return false;

	bool isFirst = true;
	while(!feof(fp))
	{
		char lineData[MAX_LINE_SIZE] = {0};
		fgets(lineData, MAX_LINE_SIZE, fp);
		char *pStart = lineData;
		if (isFirst && (unsigned char)lineData[0] == 0xef 
			&& (unsigned char)lineData[1] == 0xbb
			&& (unsigned char)lineData[2] == 0xbf)
		{
			continue;
		}

		int iLineLength = (int)strlen(pStart);
		char *pData = new char[iLineLength + 1];
		memcpy(pData, pStart, iLineLength + 1);

		if ( *pData != '\0')
		{
			LineSourceCode lineCode;
			lineCode.pSourceCode = pData;
			lineCode.iLength = strlen(pData);
			mSourceFileDataList.push_back(lineCode);
			m_iMaxLine ++;
		}

		isFirst = false;
	}
	
	fclose(fp);

	return true;
}


char  CSourceFile::GetNextChar()
{
	if (m_iCurLine < mSourceFileDataList.size())
	{
		LineSourceCode line = mSourceFileDataList[m_iCurLine];
		if (m_iCurChar >= line.iLength)
		{
			if (m_iCurLine == m_iMaxLine - 1)
			{
				return '\0';
			}
			else
			{
				m_iCurChar = 0;
				m_iCurLine++;
			}
		}
		return mSourceFileDataList[m_iCurLine].pSourceCode[m_iCurChar++];
	}
	else
	{
		return '\0';
	}
}

char  CSourceFile::LookNextChar()
{
	int curCharIndex =  m_iCurChar;
	int curLineIndex = m_iCurLine;
	LineSourceCode line = mSourceFileDataList[m_iCurLine];
	if (curCharIndex >= line.iLength)
	{
		if(curLineIndex == m_iMaxLine - 1)
		{
			return '\0';
		}
		else
		{
			curCharIndex = 0;
			curLineIndex ++;
		}
	}
	return mSourceFileDataList[curLineIndex].pSourceCode[curCharIndex];
}

void  CSourceFile::ReWindChar()
{
	if (m_iCurChar == 0)
	{
		m_iCurLine --;
		m_iCurChar = mSourceFileDataList[m_iCurLine].iLength -1;
	}
	else
		m_iCurChar--;
}
void  CSourceFile::SetCurChar(char c)
{
	mSourceFileDataList[m_iCurLine].pSourceCode[m_iCurChar] = c;
}


inline const char * CSourceFile::GetNextLine()
{
	if (m_iCurLine >= m_iMaxLine)
		return NULL;
	return mSourceFileDataList[m_iCurLine++].pSourceCode;
}

