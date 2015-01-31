// Modifier.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <windows.h>
#include <sstream>
#include <algorithm>

using namespace std;

const string kPlayerFile = "test.txt";//"ismplayer.html";
const string kConfigFile = "Modifier.cfg";

typedef map<string, string> templateParams;

bool FileExists(const string& fname)
{
	return ifstream(fname).is_open();
}

string ExpandTemplate(string& sourceStr, templateParams& tmplParams)
{
	string resultString, tmpStr;
	bool substrExit = false;

	bool exit = false;
	size_t sourceLength = sourceStr.length() - 1;
	for (size_t range = 0; range < sourceStr.length(); range++)
	{
		bool endOfStr = (sourceLength == range) ? true : false;
		
		size_t tmpRange = range;
		tmpStr += sourceStr[range];
		
		if (!endOfStr)
			++tmpRange;

		for each(const auto& pair in tmplParams)
		{
			size_t indexOfPos = pair.first.find(tmpStr);
			if (indexOfPos != string::npos)
			{
				substrExit = true;

				if (!endOfStr)
				{
					string tempNextStr = tmpStr + sourceStr.substr(tmpRange, 1);
					size_t indexOfPosTmp = pair.first.find(tempNextStr);
					
					if (indexOfPosTmp != string::npos)
					{
						exit = true;
						break;
					}
				}
			}
		}

		if (exit)
		{
			exit = false;
			continue;
		}

		if (substrExit)
		{
			substrExit = false;
			templateParams::iterator it = tmplParams.find(tmpStr);

			if (it != tmplParams.end())
			{
				resultString += it->second;
			}
			else
			{
				resultString += tmpStr;
			}
		}
		else
		{
			resultString += sourceStr[range];
		}

		tmpStr.clear();
	}

	return resultString;
}

string FindFileFromDir(string inputFolder)
{
	WIN32_FIND_DATA findFileData;
	inputFolder.append("\\");
	HANDLE hFind = FindFirstFile((inputFolder + "*").c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return "";
	}

	string filePath;
	do{
		const bool is_directory = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		const string fileName = (char*)findFileData.cFileName;

		if (is_directory && (fileName.compare(".") != 0) && (fileName.compare("..") != 0))
		{
			filePath = FindFileFromDir(inputFolder + fileName);

			if (FileExists(filePath))
			{
				break;
			}
		}
		else 
		{
			if (fileName == kPlayerFile)
			{
				
				filePath =  inputFolder + fileName;

				break;
			}
		}

	} while (FindNextFile(hFind, &findFileData));

	FindClose(hFind);

	return filePath;
}

templateParams ReadConfigFile()
{
	ifstream inFile(kConfigFile);
	if (!inFile)
	{
		throw exception("Can't open file.");
	}

	string valueStr, keyStr;
	bool isNowValue = false;
	bool canReedValue = false;
	templateParams configMap;

	char ch;
	while (inFile.get(ch))
	{
		if ((ch == ';') || (ch == '\n') || (ch == '\r'))
		{
			continue;
		}

		if (!isNowValue && (ch == ' '))
		{
			continue;
		}

		if (!isNowValue && (ch == '='))
		{
			isNowValue = true;
			continue;
		}

		if (isNowValue && (ch == '"'))
		{
			if (canReedValue)
			{
				canReedValue = false;
				isNowValue = false;

				configMap[keyStr] = valueStr;

				keyStr.clear();
				valueStr.clear();
			}
			else
			{
				canReedValue = true;
			}

			continue;
		}

		if (isNowValue)
		{
			if (canReedValue)
				valueStr += ch;
		}
		else
		{
			keyStr += ch;
		}
	};



	return configMap;
}

string GetWord(string line, size_t pos)
{
	string temp = line.substr(pos);

	string tempKey;
	for each(char ch in temp)
	{
		if ((ch == ' ') || (ch == '='))
		{
			break;
		}

		tempKey += ch;
	}

	return tempKey;
}

templateParams ReplaceKeys(const string& filePath,templateParams& configMap)
{
	ifstream inFile(filePath);
	if (!inFile)
	{
		throw exception("Can't open file.");
	}

	string valueStr, keyStr;
	bool isNowValue = false;
	bool canReedValue = false;

	templateParams newParams;
	while (!inFile.eof())
	{
		string line;
		getline(inFile, line);

		for each(auto& pair in configMap)
		{
			size_t indexOfPos = line.find(pair.first);
			if (indexOfPos != string::npos)
			{
				string strKey = GetWord(line, indexOfPos);

				if (strKey == pair.first)
				{
					size_t quoteOpen = line.find("\"");
					if (quoteOpen != string::npos)
					{
						size_t quoteClose = line.find("\"", quoteOpen + 1);
						if (quoteClose != string::npos)
						{
							string tempKey = line.substr(quoteOpen + 1, quoteClose - quoteOpen - 1);
							string value = pair.second;

							newParams.insert(std::pair<string,string>(tempKey, value));
						}
					}
				}
			}
		}
	};

	return newParams;
}

void Modifi(const TCHAR *inputFolder)
{
	templateParams config = ReadConfigFile();
	string filePath = FindFileFromDir(inputFolder);

	config = ReplaceKeys(filePath, config);

	ifstream inFile(filePath, ifstream::binary);
	if (!inFile)
	{
		throw exception("Can't open file.");
	}

	string sourceFile;
	char ch;
	while (inFile.get(ch))
	{
		sourceFile += ch;
	}

	string df = ExpandTemplate(sourceFile, config);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2)
	{
		cout << "Example: modifier.exe <folder path>";
		return 1;
	}

	try
	{
		Modifi(argv[1]);
	}
	catch (exception e)
	{
		cout << e.what();
	}

	return 0;
}

