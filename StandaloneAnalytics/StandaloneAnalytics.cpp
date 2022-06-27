// StandaloneAnalytics.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "Logging.h"
#include "EString.h"
#include "Enums.h"
#include "IDictionary.h"
#include "IAnalytics.h"

using namespace Hlib;
using namespace std;

extern "C"
{
    ET_ReturnCode GetDictionary(Hlib::IDictionary*&);        // the only external function defined in MainLib
}

int main()
{
    CEString sDbPath(L"/home/konstantin/zal/ZalData_demo.db3");
//    CEString sDbPath(L"C:\\git-repos\\zal/ZalData_demo.db3");
    CEString sSourceTextPath(L"/home/konstantin/zal/Pasternak_05_2020_UTF8.txt");
//    CEString sSourceTextPath(L"C:\\git-repos\\zal/Pasternak_05_2020_UTF8.txt");

    IDictionary* pDictionary = nullptr;
    ET_ReturnCode rc = GetDictionary(pDictionary);
    if (nullptr == pDictionary)
    {
        ERROR_LOG(L"Unable to initialize dictionary, exiting.");
        exit(-1);
    }

    rc = pDictionary->eSetDbPath(sDbPath);
    if (rc != H_NO_ERROR)
    {
        ERROR_LOG(L"Unable to set DB path, exiting.");
        exit(-1);
    }
 
    IAnalytics* pAnalytics = nullptr;
    ET_ReturnCode eRet = pDictionary->eGetAnalytics(pAnalytics);
    if (eRet != H_NO_ERROR)
    {
        ERROR_LOG(L"Unable to get analytics object, exiting.");
        exit(-1);
    }

    CEString sAuthor, sCollection, sBook, sTitle, sDate;
    CEString sText, sLastLine;
    long long llLastTextId = -1;
    int iCount = 0;


    string strPath(sSourceTextPath.stl_sToUtf8());
    ifstream Infile(strPath.c_str());
    string strLine;
    while (getline(Infile, strLine))
    {
        CEString sLine(CEString::sFromUtf8(strLine));
        sLine.TrimRight(L"\r ");
        if (sLine.bRegexMatch(L"^\\<(\\w+?)\\s+(.+?)\\/(\\w+)>"))
        {
            if (!sText.bIsEmpty())
            {
                CEString sMetadata;
                sMetadata += L"author = ";
                sMetadata += sAuthor;
                sMetadata += L" | collection = ";
                sMetadata += sCollection;
                sMetadata += L" | book = ";
                sMetadata += sBook;
                sMetadata += L" | title = ";
                sMetadata += sTitle;
                sMetadata += L"";
                sMetadata += L" | date = ";
                sMetadata += sDate;

                MESSAGE_LOG(sMetadata);

                bool bIsProse{false};
                ET_ReturnCode eRet = pAnalytics->eParseText(sTitle, sMetadata, sText, llLastTextId, bIsProse);
                sText.Erase();
                if (eRet != H_NO_ERROR)
                {
                    ERROR_LOG(L"Analytics module returned error.");
                    continue;
                }

//                if (++iCount % 100 == 0)
//                {
                    cout << ++iCount << endl;
//                }
            }

            CEString sStartTag = sLine.sGetRegexMatch(0);
            CEString sValue = sLine.sGetRegexMatch(1);
            CEString sEndTag = sLine.sGetRegexMatch(2);

            if (sStartTag.bIsEmpty() || sValue.bIsEmpty() || sEndTag.bIsEmpty())
            {
                CEString sMsg(L"Empty metadata tag(s) in: ");
                sMsg += sLine;
                ERROR_LOG(sMsg);
                continue;
            }

            if (sStartTag != sEndTag)
            {
                CEString sMsg(L"Tag mismatch in: ");
                sMsg += sLine;
                ERROR_LOG(sMsg);
                continue;
            }

            if (L"author" == sStartTag)
            {
                sAuthor = sValue;
            }
            else if (L"collection" == sStartTag)
            {
                sCollection = sValue;
            }
            else if (L"book" == sStartTag)
            {
                sBook = sValue;
            }
            else if (L"title" == sStartTag)
            {
                sTitle = sValue;
            }
            else if (L"date" == sStartTag)
            {
                sDate = sValue;
            }
            else
            {
                CEString sMsg(L"Unable to parse tag: ");
                sMsg += sStartTag;
                sMsg += L" in ";
                sMsg += sLine;
                continue;
            }
        }       // Match on regex
        else
        {
            if (!sText.bIsEmpty())
            {
                sText += L"\r\n";
            }
            sText += sLine;
        }

        sLastLine = sLine;
    }

    delete pDictionary;
}
