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
#include "src_gumbo\gumbo.h"

using namespace std;

const string kPlayerFile = "ismplayer.html";
const string kTypePresentation = "presentation";
const string kTypeQuiz = "quiz";
const string kTypeInteraction = "interaction";
const string kConfigFile = "Modifier.cfg";

typedef map<string, string> templateParams;

bool FileExists(const string& fname)
{
	return ifstream(fname).is_open();
}

string ExpandTemplate(const string& sourceStr, templateParams& tmplParams)
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
		throw exception("Can't open file of configuration.");
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

GumboNode *
gumbo_get_element_by_id(const char *id, GumboNode *document) {

	if (GUMBO_NODE_DOCUMENT != document->type
		&& GUMBO_NODE_ELEMENT != document->type) {
		return NULL;
	}

	GumboAttribute *node_id =
		gumbo_get_attribute(&document->v.element.attributes, "id");
	if (node_id && 0 == strcmp(id, node_id->value)) {
		return document;
	}

	// iterate all children
	GumboVector *children = &document->v.element.children;
	for (unsigned int i = 0; i < children->length; i++) {
		GumboNode *node = gumbo_get_element_by_id(id, (GumboNode *)children->data[i]);
		if (node) return node;
	}

	return NULL;
}

string FindValueFromSrcFile(const string &sourceFile, string findKey)
{
	size_t indexOfPos = sourceFile.find(findKey);
	if (indexOfPos != string::npos)
	{
		string strKey = GetWord(sourceFile, indexOfPos);

		if (strKey == findKey)
		{
			size_t quoteOpen = sourceFile.find("\"", indexOfPos);
			if (quoteOpen != string::npos)
			{
				size_t quoteClose = sourceFile.find("\"", quoteOpen + 1);
				if (quoteClose != string::npos)
				{
					return sourceFile.substr(quoteOpen + 1, quoteClose - quoteOpen - 1);
				}
			}
		}
	}

	return "";
}

templateParams ReplaceKeys(const string& srcFile,templateParams& configMap)
{
	string valueStr, keyStr;
	bool isNowValue = false;
	bool canReedValue = false;

	templateParams newParams;

	GumboOutput *output = gumbo_parse(srcFile.c_str());
	GumboNode *element = NULL;

	for each(auto& pair in configMap)
	{
		string keyStr;
		element = gumbo_get_element_by_id(pair.first.c_str(), output->root);
		if (!element)
		{
			keyStr = FindValueFromSrcFile(srcFile, pair.first);
			if (keyStr.empty())
				continue;
		}
		else
		{
			GumboNode* title_text = static_cast<GumboNode*>(element->v.element.children.data[0]);
			if (title_text->type == GUMBO_NODE_TEXT)
			{
				keyStr = title_text->v.text.text;
			}
		}

		newParams.insert(std::pair<string, string>(keyStr, pair.second));
	}

	return newParams;
}

void ReplaceContentType(templateParams& config)
{
	auto copyConfig = config;
	for each(auto& pair in copyConfig)
	{
		string type;
		templateParams tempMap;
		if (pair.first.find(kTypePresentation) != string::npos)
		{
			type = kTypePresentation;
		}
		else if (pair.first.find(kTypeQuiz) != string::npos)
		{
			type = kTypeQuiz;
		}
		else if (pair.first.find(kTypeInteraction) != string::npos)
		{
			type = kTypeInteraction;
		}

		if (!type.empty())
		{
			tempMap.insert(std::pair<string, string>("{%CONTENT%}", type));
			string key = pair.first;
			string value = pair.second;
			config.erase(pair.first);
			config.insert(std::pair<string, string>(key, ExpandTemplate(value, tempMap)));
		}
	}
}

void Modifi(const TCHAR *inputFolder)
{
	templateParams config = ReadConfigFile();
	string filePath = FindFileFromDir(inputFolder);

	ifstream inFile(filePath, ifstream::binary);
	if (!inFile)
	{
		throw exception("Can't open input file.");
	}

	inFile.seekg(0, std::ios::end);
	size_t size = (size_t)inFile.tellg();

	string strFileBuf(size, ' ');

	inFile.seekg(0);
	inFile.read(&strFileBuf[0], size);

	inFile.close();

	config = ReplaceKeys(strFileBuf, config);

	ReplaceContentType(config);

	ofstream outFile(filePath, ifstream::binary);
	if (!outFile)
	{
		throw exception("Can't open output file.");
	}

	outFile << ExpandTemplate(strFileBuf, config);
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

		cout << "The operation completed successfully" << endl;
	}
	catch (exception e)
	{
		cout << e.what();
	}

	return 0;
}

