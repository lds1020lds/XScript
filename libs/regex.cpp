#include "regex.h"


RegData::~RegData()
{
	if (subRegData != NULL)
	{
		delete subRegData;
	}
}

Regex::Regex(char* regexExpr)
{
	m_groupIncID = 0;
	m_rootRegexFactor = ParseExpr(regexExpr, false);
}

Regex::~Regex()
{
	if (m_rootRegexFactor != NULL)
		ClearRegFactor(m_rootRegexFactor);
}

void	Regex::ClearRegFactor(RegexFactor* factor)
{
	for (int i = 0; i < factor->subFactorSize; i++)
	{
		ClearRegFactor(factor->subFactor[i]);
	}

	delete factor;
}


RegexFactor*	Regex::AddRegFactor(RegexFactor* lastExpr)
{
	RegexFactor* factor = new RegexFactor();
	factor->type = RFT_Normal;
	factor->lastFactor = lastExpr;
	factor->minRepeat = factor->maxRepeat = 1;

	AddSubFactor(lastExpr, factor);

	return factor;
}

void Regex::AddSubFactor(RegexFactor* lastExpr, RegexFactor* factor)
{
	lastExpr->subFactorSize++;
	RegexFactor** subFactor = new RegexFactor*[lastExpr->subFactorSize];
	if (lastExpr->subFactor != NULL)
		memcpy(subFactor, lastExpr->subFactor, sizeof(RegexFactor*) * (lastExpr->subFactorSize - 1));

	lastExpr->subFactor = subFactor;
	lastExpr->subFactor[lastExpr->subFactorSize - 1] = factor;
}

bool	Regex::ParseRange(char* &expr, int& start, int& end)
{
	char* saved = expr;
	expr++;
	start = 0;
	while (true)
	{
		if (*expr >= '0' && *expr <= '9')
		{
			start = start * 10 + (*expr - '0');
		}
		else if (*expr == ',')
		{
			expr++;
			break;
		}
		else if (*expr == '}')
		{
			end = start;
			return true;
		}
		else
		{
			expr = saved;
			return false;
		}

		expr++;
	}


	if (*expr == '}')
	{
		end = -1;
		return true;
	}
	else
	{
		end = 0;
		while (true)
		{
			if (*expr >= '0' && *expr <= '9')
			{
				end = end * 10 + (*expr - '0');
			}
			else if (*expr == '}')
			{
				return true;
			}
			else
			{
				expr = saved;
				return false;
			}

			expr++;
		}
	}

}

RegexFactor*	Regex::ParseExpr(char* &p, bool hasParent)
{
	RegexFactor* startExpr = new RegexFactor();
	startExpr->type = RFT_Start;

	RegexFactor* lastExpr = startExpr;

	while (true)
	{
		switch (*p)
		{
		case '\\':
		{
			if (*(p + 1) != 0)
			{
				switch (*(p + 1))
				{
				case 'd':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Number, MCT_Number));

					break;
				case 'D':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Not_Number, MCT_Not_Number));
					break;
				case 's':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Space, MCT_Space));
					break;
				case 'S':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Not_Space, MCT_Not_Space));
					break;
				case 'w':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Char, MCT_Char));
					break;
				case 'W':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Not_Char, MCT_Not_Char));
					break;
				case 'n':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange('\n', '\n'));
					break;
				case 'r':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange('\r', '\r'));
					break;
				case 't':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange('\t', '\t'));
					break;
				default:
					{
						lastExpr = AddRegFactor(lastExpr);
						if (*(p + 1) >= '0' && *(p + 1) <= '9')
						{
							lastExpr->refGroupIndex = (*(p + 1) - '0');
							if (lastExpr->refGroupIndex >= m_groupIncID)
							{
								goto failure;
							}
						}
						else if (*(p + 1) == 'k' && *(p + 2) == '<')
						{
							p += 3;

							std::string groupName;
							while (1)
							{
								if (*p == '>')
								{
									p++;
									break;
								}
								else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '-' || *p == '_')
								{
									groupName.push_back(*p);
								}
								else
									goto failure;

								p++;
							}

							if (groupName.empty())
							{
								goto failure;
							}

							bool hasFound = false;
							for (int i = 0; i < (int)m_groupNameVec.size(); i++)
							{
								if (m_groupNameVec[i] == groupName)
								{
									hasFound = true;
									break;
								}
							}
							
							if (!hasFound)
							{
								goto failure;
							}

							p -= 2;

							if (groupName.size() < MaxGroupNameSize)
							{
								strncpy(lastExpr->refGroupName, groupName.c_str(), MaxGroupNameSize);
							}
							else
							{
								goto failure;
							}
						
						}
						else
						{
							lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(*(p + 1), *(p + 1)));
						}
					}
				break;
					
					
				}

				p += 2;
			}
			else
			{
				goto failure;
			}
		}
		break;
		case '.':
		{
			lastExpr = AddRegFactor(lastExpr);
			lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Point, MCT_Point));
			p++;
		}
		break;
		case '{':
		{
			int start, end;
			bool isRange = ParseRange(p, start, end);
			if (isRange)
			{
				if (lastExpr == startExpr)
				{
					goto failure;
				}
				else
				{
					lastExpr->maxRepeat = end;
					lastExpr->minRepeat = start;

					if (*(p + 1) == '?')
					{
						lastExpr->isGreedy = false;
						p++;
					}
				}
			}
			else
			{
				lastExpr = AddRegFactor(lastExpr);
				lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(*p, *p));
			}

			p++;
		}
		break;
		case '[':
		{
			p++;
			if (*p == 0)
				goto failure;

			RegexFactor* factor = AddRegFactor(lastExpr);
			bool isNot = false;
			if (*p == '^')
			{
				factor->isNot = true;
				p++;
			}

			while (true)
			{
				if (*p == 0)
				{
					goto failure;
				}
				else if (*p == ']')
				{
					if (factor->mCharRangeVecSize == 0)
					{
						goto failure;
					}
					else
					{
						p++;
						break;
					}
				}
				else
				{
					if (factor->mCharRangeVecSize >= MaxCharRangeSize)
					{
						goto failure;
					}

					if (*p == '\\')
					{
						if (*p == '\0')
							goto failure;

						switch (*(p + 1))
						{
						case 'd':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Number, MCT_Number));
							break;
						case 'D':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Not_Number, MCT_Not_Number));
							break;
						case 's':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Space, MCT_Space));
							break;
						case 'S':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Not_Space, MCT_Not_Space));
							break;
						case 'w':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Char, MCT_Char));
							break;
						case 'W':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Not_Char, MCT_Not_Char));
							break;
						case 'n':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange('\n', '\n'));
							break;
						case 'r':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange('\r', '\r'));
							break;
						case 't':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange('\t', '\t'));
							break;
						default:
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(*(p + 1), *(p + 1)));
							break;
						}

						p += 2;
					}
					else  if (*(p + 1) == '-' && *(p + 2) != ']' && *(p + 2) != 0)
					{
						factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(*p, *(p + 2)));
						p += 3;
					}
					else
					{
						factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(*p, *p));
						p++;
					}
				}
			}

			lastExpr = factor;
		}
		break;
		case '|':
		{
			if (lastExpr == startExpr)
			{
				goto failure;
			}
			else
			{
				lastExpr = startExpr;
			}
			p++;
		}
		break;
		case '(':
		{
			p++;

			std::string groupName;
			if (*p == '?')
			{
				p++;
				if (*p == '<')
				{
					p++;
					while (1)
					{
						if (*p == '>')
						{
							p++;
							break;
						}
						else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '-' || *p == '_')
						{
							groupName.push_back(*p);
						}
						else
							goto failure;

						p++;
					}

					if (groupName.size() == 0)
					{
						goto failure;
					}
				}
			}

			int groupID = -1;
			if (groupName.empty())
			{
				groupID = m_groupIncID++;
			}

			
			RegexFactor*  subFactor = ParseExpr(p, true);
			if (subFactor == NULL)
				goto failure;

			RegexFactor* groupFactor = new RegexFactor();
			groupFactor->type = RFT_Group;
			groupFactor->startFactor = subFactor;

			groupFactor->groupName = groupName;
			if (groupName.empty())
			{
				groupFactor->groupIndex = groupID;
			}
			else
			{
				m_groupNameVec.push_back(groupName);
			}

			AddSubFactor(lastExpr, groupFactor);
			lastExpr = groupFactor;
		}
		break;
		case ')':
		{
			if (hasParent)
			{
				p++;
				return startExpr;
			}
			else
			{
				goto failure;
			}
		}
		break;
		case '+':
		{
			if (lastExpr == startExpr)
				goto failure;
			else
			{
				lastExpr->minRepeat = 1;
				lastExpr->maxRepeat = MaxRegRepeat;
				if (*(p + 1) == '?')
				{
					lastExpr->isGreedy = false;
					p++;
				}
			}

			p++;
		}
		break;
		case '*':
		{
			if (lastExpr == startExpr)
				goto failure;
			else
			{
				lastExpr->minRepeat = 0;
				lastExpr->maxRepeat = MaxRegRepeat;
				if (*(p + 1) == '?')
				{
					lastExpr->isGreedy = false;
					p++;
				}
			}

			p++;
		}
		break;
		case '?':
		{
			if (lastExpr == startExpr)
				goto failure;
			else
			{
				lastExpr->minRepeat = 0;
				lastExpr->maxRepeat = 1;
				if (*(p + 1) == '?')
				{
					lastExpr->isGreedy = false;
					p++;
				}
			}

			p++;
		}
		break;
		case '\0':
		{
			if (hasParent)
			{
				goto failure;
			}
			else
			{
				return startExpr;
			}
		}
		default:

			bool hasLimit = *(p + 1) == '{' || *(p + 1) == '?' || *(p + 1) == '+' || *(p + 1) == '*';

			if (lastExpr->mContentSize > 0 && lastExpr->mContentSize < MaxContentSize -1 && !hasLimit && (lastExpr->minRepeat == lastExpr->maxRepeat) && lastExpr->minRepeat == 1)
			{
				lastExpr->mContent[lastExpr->mContentSize] = *p;
				lastExpr->mContentSize++;
			}
			else
			{
				lastExpr = AddRegFactor(lastExpr);
				lastExpr->mContent[0] = (*p);
				lastExpr->mContentSize = 1;
			}

			p++;
			break;
		}
	}


failure:
	ClearRegFactor(startExpr);
	return NULL;
}



inline bool	Regex::CanMatch(const RegexFactor* factor, char* c, int index, int& num)
{
	if (factor->refGroupIndex >= 0)
	{
		std::map<int, MatchGroupData>::iterator it = m_matchGroupIDMap.find(factor->refGroupIndex);
		if (it != m_matchGroupIDMap.end())
		{
			num = it->second.endPos - it->second.startPos;
			if (num == 0)
				return true;
			return strncmp(c + it->second.startPos, c + index, num) == 0;
		}
		else
		{
			num = 0;
			return true;
		}
	}
	else if (factor->refGroupName[0] != 0)
	{
		std::map<std::string, MatchGroupData>::iterator it = m_matchGroupNameMap.find(factor->refGroupName);
		if (it != m_matchGroupNameMap.end())
		{
			num = it->second.endPos - it->second.startPos;
			if (num == 0)
				return true;
			return strncmp(c + it->second.startPos, c + index, num) == 0;
		}
		else
		{
			return false;
		}
	}
	if (factor->mContent[0] != 0)
	{
		char* p = c + index;
		num = factor->mContentSize;
		bool isEqual = true;
		for (int i = 0; i < num; i++)
		{
			if (p[i] != factor->mContent[i] || p[i] == '\0')
			{
				isEqual = false;
				break;
			}
		}
		return isEqual;
	}
	else
	{
		num = 1;
		if (factor->isNot)
		{
			for (int i = 0; i < factor->mCharRangeVecSize; i++)
			{
				if (factor->mCharRangeVec[i].start < 0)
				{
					bool ret = false;
					IsCharMatched(ret, *(c + index), factor->mCharRangeVec[i].start)
					if (ret)
						return false;
				}
				else
				{
					if (*(c + index) >= factor->mCharRangeVec[i].start && *(c + index) <= factor->mCharRangeVec[i].end)
						return	false;
				}
				
			}

			return true;
		}
		else
		{
			for (int i = 0; i < factor->mCharRangeVecSize; i++)
			{
				if (factor->mCharRangeVec[i].start < 0)
				{
					bool ret = false;
					IsCharMatched(ret, *(c + index), factor->mCharRangeVec[i].start)
					if (ret)
						return true;
				}
				else if (*(c + index) >= factor->mCharRangeVec[i].start && *(c + index) <= factor->mCharRangeVec[i].end)
					return true;
			}

			return false;
		}

	}




}


bool Regex::MatchGroupRegExprGreedy(RegData& regData, char* content, int& index)
{
	if (regData.isRollBack)
	{

		bool hasIncrease = false;
		bool hasFound = false;
		while (!hasFound)
		{
			if (regData.numLastTryTimes == 0)
			{
				if (regData.numLastTryTimesSave >= 0 && regData.numLastTryTimesSave < regData.factor->maxRepeat)
				{
					regData.numLastTryTimes = regData.numLastTryTimesSave + 1;
					regData.numLastTryTimesSave = -1;

					RegData t(regData.factor->startFactor);
					RegDataVec& vec = *regData.subRegData;
					PushRegData(vec, t)
				}
				else
				{
					return false;
				}
			}

			if (regData.numTimes > 0)
			{
				regData.numTimes--;
				Back(*regData.subRegData).isRollBack = true;
			}

			while (regData.numTimes < regData.numLastTryTimes)
			{
				int oldIndex = index;
				if (MatchRegExpr(*regData.subRegData, content, index))
				{
					if (index > oldIndex)
						RefreshMatchGroupData(regData, oldIndex, index);
					else if (regData.numLastTryTimesSave < 0 )
					{
						return false;
					}

					regData.numTimes++;
					if (regData.numTimes < regData.numLastTryTimes)
					{
						RegData t(regData.factor->startFactor);
						RegDataVec& vec = *regData.subRegData;
						PushRegData(vec, t)
					}
					else
					{
						regData.isRollBack = false;
						return true;
					}
				}
				else
				{
					if (regData.numTimes == 0)
					{
						if (regData.numLastTryTimesSave >= 0)
						{
							if (regData.numLastTryTimes > regData.factor->minRepeat)
							{
								regData.numLastTryTimes--;

								if (regData.numLastTryTimes == 0)
								{
									regData.isRollBack = false;
									return true;
								}

								RegData t(regData.factor->startFactor);
								RegDataVec& vec = *regData.subRegData;
								PushRegData(vec, t)
									break;
							}
							else
							{
								if (regData.numLastTryTimesSave < regData.factor->maxRepeat)
								{
									regData.numLastTryTimes = regData.numLastTryTimesSave + 1;
									regData.numLastTryTimesSave = -1;

									RegData t(regData.factor->startFactor);
									RegDataVec& vec = *regData.subRegData;
									PushRegData(vec, t)
								}
								else
								{
									regData.isRollBack = false;
									return false;
								}

							}
						}
						else
						{
							if (regData.numLastTryTimes < regData.factor->maxRepeat && !hasIncrease)
							{
								hasIncrease = true;
								regData.numLastTryTimes++;
								RegData t(regData.factor->startFactor);
								RegDataVec& vec = *regData.subRegData;
								PushRegData(vec, t)
									break;
							}
							else
							{
								regData.isRollBack = false;
								return false;
							}
						}

					}
					else
					{
						regData.numTimes--;
						Back(*regData.subRegData).isRollBack = true;
					}
				}
			}


		}
	}
	else
	{
		regData.startIndex = index;
		while (regData.numTimes < regData.factor->maxRepeat)
		{
			int oldIndex = index;
			if (MatchRegExpr(*regData.subRegData, content, index))
			{
				if (index > oldIndex)
					RefreshMatchGroupData(regData, oldIndex, index);

				regData.numTimes++;
				if (index == oldIndex && regData.numTimes >= regData.factor->minRepeat)
				{
					break;
				}
				else if (regData.numTimes < regData.factor->maxRepeat)
				{
					RegData t(regData.factor->startFactor);
					RegDataVec& vec = *regData.subRegData;
					PushRegData(vec, t)
				}
			}
			else
			{
				if (regData.numTimes >= regData.factor->minRepeat)
				{
					break;
				}
				else
				{
					if (regData.numTimes == 0)
					{
						return false;
					}
					else
					{
						regData.numTimes--;
						Back(*regData.subRegData).isRollBack = true;
					}
				}

			}
		}
		regData.numLastTryTimesSave = regData.numTimes;
		regData.numLastTryTimes = regData.numTimes;
		return true;
	}
}

bool	Regex::MatchGroupRegExprNoGreedy(RegData& regData, char* content, int& index)
{
	if (regData.isRollBack)
	{

		bool hasFound = false;
		bool hasIncrease = false;
		while (!hasFound)
		{
			if (regData.numLastTryTimes == 0)
				regData.numLastTryTimes = 1;

			if (regData.numTimes > 0)
			{
				regData.numTimes--;
				Back(*regData.subRegData).isRollBack = true;
			}

			while (regData.numTimes < regData.numLastTryTimes)
			{
				int oldIndex = index;
				if (MatchRegExpr(*regData.subRegData, content, index))
				{
					if (index > oldIndex)
						RefreshMatchGroupData(regData, oldIndex, index);
					regData.numTimes++;
					if (regData.numTimes < regData.numLastTryTimes)
					{
						RegData t(regData.factor->startFactor);
						RegDataVec& vec = *regData.subRegData;
						PushRegData(vec, t)
					}
					else
					{
						hasFound = true;
						break;
					}
				}
				else
				{
					if (regData.numTimes == 0)
					{
						if (regData.numLastTryTimes < regData.factor->maxRepeat && !hasIncrease)
						{
							hasIncrease = true;
							regData.numLastTryTimes++;
							RegData t(regData.factor->startFactor);
							RegDataVec& vec = *regData.subRegData;
							PushRegData(vec, t)
							break;
						}
						else
						{
							regData.isRollBack = false;
							return false;
						}
					}
					else
					{
						regData.numTimes--;
						Back(*regData.subRegData).isRollBack = true;
					}
				}
			}


		}

		regData.isRollBack = false;
		return true;
	}
	else
	{
		regData.startIndex = index;
		while (regData.numTimes < regData.factor->minRepeat)
		{
			int oldIndex = index;
			if (MatchRegExpr(*regData.subRegData, content, index))
			{
				if (index > oldIndex)
					RefreshMatchGroupData(regData, oldIndex, index);
				regData.numTimes++;
				if (regData.numTimes < regData.factor->minRepeat)
				{
					RegData t(regData.factor->startFactor);
					RegDataVec& vec = *regData.subRegData;
					PushRegData(vec, t)
				}
			}
			else
			{
				if (regData.numTimes == 0)
				{
					return false;
				}
				else
				{
					regData.numTimes--;
					Back(*regData.subRegData).isRollBack = true;
				}
			}
		}

		regData.numLastTryTimes = regData.numTimes;
	}


	return true;
}

bool	Regex::MatchGroupRegExpr(RegData& regData, char* content, int& index)
{
	if (regData.subRegData == NULL)
	{
		regData.subRegData = new RegDataVec();
		InitRegData(*(regData.subRegData), 4);
		RegData re(regData.factor->startFactor);
		RegDataVec& t = *(regData.subRegData);
		PushRegData(t, re);
	}
	bool succ = false;


	if (regData.factor->isGreedy)
		succ = MatchGroupRegExprGreedy(regData, content, index);
	else
		succ = MatchGroupRegExprNoGreedy(regData, content, index);

	return succ;
}

void Regex::RefreshMatchGroupData(RegData &regData, int startIndex, int endIndex)
{
	if (!regData.factor->groupName.empty())
	{
		m_matchGroupNameMap[regData.factor->groupName].startPos = startIndex;
		m_matchGroupNameMap[regData.factor->groupName].endPos = endIndex;
	}
	else
	{
		m_matchGroupIDMap[regData.factor->groupIndex].startPos = startIndex;
		m_matchGroupIDMap[regData.factor->groupIndex].endPos = endIndex;
	}
}

bool	Regex::MatchRegExpr(RegDataVec& regDataVec, char* content, int& index)
{
	int savedIndex = index;
	while (true)
	{
		if (*(content + index) == 0)
		{
			bool isSucc = (Back(regDataVec).numTimes >= Back(regDataVec).factor->minRepeat
				&& Back(regDataVec).factor->subFactorSize == 0);

			if (isSucc && !Back(regDataVec).isRollBack)
				return isSucc;
		}

		if (Back(regDataVec).isRollBack)
		{
			if (Back(regDataVec).factor->type == RFT_Start)
			{
				if (Back(regDataVec).numTimes < Back(regDataVec).factor->subFactorSize - 1)
				{
					Back(regDataVec).numTimes++;
					Back(regDataVec).isRollBack = false;
				}
				else
				{
					PopRegData(regDataVec)
					return false;
				}

			}
			else if (Back(regDataVec).factor->type == RFT_Normal)
			{
				int numChar = 1;
				GetNumMatchChar(numChar, Back(regDataVec).factor);

				if (Back(regDataVec).factor->isGreedy)
				{
					if (numChar > 0 && Back(regDataVec).numTimes > Back(regDataVec).factor->minRepeat)
					{
						Back(regDataVec).numTimes--;
						Back(regDataVec).isRollBack = false;

						index -= numChar;

						if (Back(regDataVec).factor->subFactorSize > 0)
						{
							RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[0];
							RegData t(nextFactor);
							PushRegData(regDataVec, t);
						}
						else
						{
							return true;
						}
					}
					else
					{
						index -= Back(regDataVec).numTimes * numChar;
						PopRegData(regDataVec)
						Back(regDataVec).isRollBack = true;
					}
				}
				else
				{
					int maxRepeat = Back(regDataVec).factor->maxRepeat;
					int num = 0;
					if (numChar > 0 && (Back(regDataVec).numTimes < maxRepeat)
						&& CanMatch(Back(regDataVec).factor, content, index, num))
					{
						Back(regDataVec).numTimes++;
						Back(regDataVec).isRollBack = false;
						index += numChar;
					}
					else
					{
						index -= Back(regDataVec).numTimes * numChar;
						PopRegData(regDataVec)
						Back(regDataVec).isRollBack = true;

					}
				}

			}
			else if (Back(regDataVec).factor->type == RFT_Group)
			{
				if (MatchGroupRegExpr(Back(regDataVec), content, index))
				{
					if (Back(regDataVec).factor->subFactorSize > 0)
					{
						RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[0];
						RegData t(nextFactor);
						PushRegData(regDataVec, t);
					}
					else
					{
						return true;
					}
				}
				else
				{
					PopRegData(regDataVec)
					Back(regDataVec).isRollBack = true;
				}
			}
		}
		else
		{
			if (Back(regDataVec).factor->type == RFT_Start)
			{
				if (Back(regDataVec).numTimes < Back(regDataVec).factor->subFactorSize)
				{
					RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[Back(regDataVec).numTimes];
					RegData t(nextFactor);
					PushRegData(regDataVec, t);
				}
				else
				{
					PopRegData(regDataVec)
					return false;
				}
			}
			else if (Back(regDataVec).factor->type == RFT_Normal)
			{
				if (!Back(regDataVec).factor->isGreedy)
				{
					if (Back(regDataVec).numTimes < Back(regDataVec).factor->minRepeat)
					{
						int num = 1;
						if (CanMatch(Back(regDataVec).factor, content, index, num))
						{
							Back(regDataVec).numTimes++;
							index += num;
						}
						else
						{
							int numChar = 1;
							GetNumMatchChar(numChar, Back(regDataVec).factor);

							index -= Back(regDataVec).numTimes * numChar;
							PopRegData(regDataVec)
							Back(regDataVec).isRollBack = true;
						}
					}
					else
					{
						if (Back(regDataVec).factor->subFactorSize > 0)
						{
							RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[0];
							RegData t(nextFactor);
							PushRegData(regDataVec, t);
						}
						else
						{
							return true;
						}

					}
				}
				else
				{
					while (Back(regDataVec).numTimes < Back(regDataVec).factor->maxRepeat)
					{
						int num = 1;
						if (CanMatch(Back(regDataVec).factor, content, index, num))
						{
							Back(regDataVec).numTimes++;
							index += num;
						}
						else
						{
							break;
						}
					}

					if (Back(regDataVec).numTimes >= Back(regDataVec).factor->minRepeat)
					{
						if (Back(regDataVec).factor->subFactorSize > 0)
						{
							RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[0];
							RegData t(nextFactor);
							PushRegData(regDataVec, t);
						}
						else
						{
							return true;
						}
					}
					else
					{
						int numChar = 1;
						 GetNumMatchChar(numChar, Back(regDataVec).factor);
						index -= Back(regDataVec).numTimes * numChar;
						PopRegData(regDataVec)
						Back(regDataVec).isRollBack = true;
					}

				}

			}
			else if (Back(regDataVec).factor->type == RFT_Group)
			{
				if (MatchGroupRegExpr(Back(regDataVec), content, index))
				{
					if (Back(regDataVec).factor->subFactorSize > 0)
					{
						RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[0];
						RegData t(nextFactor);
						PushRegData(regDataVec, t);
					}
					else
					{
						return true;
					}
				}
				else
				{
					PopRegData(regDataVec)
					Back(regDataVec).isRollBack = true;
				}
			}
		}
	}
}


bool	Regex::Match(char* content)
{
	if (m_rootRegexFactor != NULL)
	{
		m_matchGroupIDMap.clear();
		m_matchGroupNameMap.clear();

		RegDataVec	regDataVec;
		InitRegData(regDataVec, 4);
		RegData re(m_rootRegexFactor);
		PushRegData(regDataVec, re);
		int	index = 0;
		bool ret = MatchRegExpr(regDataVec, content, index);
		return ret && content[index] == 0;
	}
	return false;
}

const std::vector<MatchData>&	Regex::Search(char* content)
{
	m_matchResultVec.clear();
	if (m_rootRegexFactor != NULL)
	{

		int	index = 0;
		int save = 0;
		RegDataVec	regDataVec;
		InitRegData(regDataVec, 4);
		RegData re(m_rootRegexFactor);

		while (content[index] != '\0')
		{
			int startIndex = index;
			PushRegData(regDataVec, re);
			if (MatchRegExpr(regDataVec, content, index) && index > startIndex)
			{
				MatchData matchData;
				matchData.matchedValue.assign(content + startIndex, content + index);

				std::map<int, MatchGroupData>::iterator it = m_matchGroupIDMap.begin();
				for (; it != m_matchGroupIDMap.end(); it++)
				{
					std::string& groupValue = matchData.groupIDMap[it->first];
					groupValue.assign(content + it->second.startPos, content + it->second.endPos);
				}

				std::map<std::string, MatchGroupData>::iterator NameIt = m_matchGroupNameMap.begin();
				for (; NameIt != m_matchGroupNameMap.end(); NameIt++)
				{
					std::string& groupValue = matchData.groupNameMap[NameIt->first];
					groupValue.assign(content + NameIt->second.startPos, content + NameIt->second.endPos);
				}

				m_matchResultVec.push_back(matchData);

				m_matchGroupIDMap.clear();
				m_matchGroupNameMap.clear();

				for (int i = 0; i <= regDataVec.mLastIndex; i++)
					regDataVec.mRegData[i].~RegData();

				regDataVec.mLastIndex = -1;
			}
			else
			{
				index++;
			}
		}
		
	}
	return m_matchResultVec;
}
