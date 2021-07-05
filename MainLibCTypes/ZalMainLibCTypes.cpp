/*
        Ctypes wrapper for Zal MainLib
        Exports some interfaces to be consumed by Python 3
*/

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <memory>
#include <cstring>
#include <iomanip>

#include "Logging.h"
#include "IDictionary.h"
#include "ILexeme.h"
#include "IWordForm.h"
#include "Gramhasher.h"
#include "IParser.h"
#include "IAspectPair.h"
#include "EString.h"
#include "Enums.h"

using namespace Hlib;

const int MAX_SIZE = 1000;

namespace MainLibForPython {

    struct StWordForm {

        long long llLexemeId;
        wchar_t szWordForm[MAX_SIZE] = { L'\0' };
        wchar_t szStem[MAX_SIZE] = { L'\0' };
        ET_PartOfSpeech ePos;
        ET_Case eCase;
        ET_Number eNumber;
        ET_Subparadigm eSubparadigm;
        ET_Gender eGender;
        ET_Person ePerson;
        ET_Animacy eAnimacy;
        ET_Reflexivity eReflexivity;
        ET_Aspect eAspect;
        ET_Status eStatus;
        bool bIrregular;      // came from the DB as opposed to being generated by the app
        wchar_t szLeadComment[MAX_SIZE] = { L'\0' };
        wchar_t szTrailingComment[MAX_SIZE] = { L'\0' };
        bool bIsEdited;
        bool bIsVariant;
//        wchar_t arrStress[MAX_SIZE] = { 0 };
        wchar_t szStress[MAX_SIZE] = { L'\0' };
        wchar_t szGrammHash[MAX_SIZE] = { L'\0' };
    };

    extern "C"
    {
        __declspec(dllexport) ET_ReturnCode GetDictionary(IDictionary*&);
        __declspec(dllexport) bool Init(const wchar_t* szDbPath);
        __declspec(dllexport) bool bParseWord(const wchar_t* szWord);
        __declspec(dllexport) int iNumOfParses();
        __declspec(dllexport) bool GetParsedWordForm(int iNum, StWordForm* pstParsedForm);
        __declspec(dllexport) bool GenerateAllForms();
        __declspec(dllexport) void ReportProgress (int iPercentDone, bool bDone, int recordNumber, double dDuration);
        __declspec(dllexport) bool AddLexemeHashes();
        __declspec(dllexport) long long llParseText(const wchar_t* szTextName, const wchar_t* szMetadata, const wchar_t* szText);
    }

    static IDictionary* g_pDictionary = nullptr;
    static IParser* g_pParser = nullptr;
    static ILexeme* g_pLexeme = nullptr;
    static IAnalytics* g_pAnalytics = nullptr;

    static vector<IWordForm *> vecWordForms;

    bool Init(const wchar_t* szDbPath)
    {
        ET_ReturnCode rc = GetDictionary(g_pDictionary);

        if (nullptr == g_pDictionary)
        {
            cout << L"Unable to intialize the dictionary." << endl;
            exit(-1);
        }

        rc = g_pDictionary->eSetDbPath(szDbPath);

        return true;
    }

    void CopyStressData(IWordForm* pWf, wchar_t* szData) 
    {    
        try {

            int iPos = 0;
            ET_StressType eType = ET_StressType::STRESS_TYPE_UNDEFINED;
            CEString sReturn (L"Stress position(s): ");

            ET_ReturnCode eRet = pWf->eGetFirstStressPos(iPos, eType);
            do {

                if (eRet != H_NO_ERROR && eRet != H_FALSE)
                {
                    ERROR_LOG(L"Unable to read stress position.");
                    continue;
                }

                if (H_NO_ERROR == eRet)
                {
                    sReturn += CEString::sToString(iPos);
                    sReturn += L", type = ";
                    if (STRESS_PRIMARY == eType)
                    {
                        sReturn += L"primary";
                    }
                    else if (STRESS_SECONDARY == eType)
                    {
                        sReturn += L"secondary";
                    }
                    else
                    {
                        ERROR_LOG(L"Illegal stress type.");
                        continue;
                    }
                    eRet = pWf->eGetNextStressPos(iPos, eType);
                }

            } while (H_NO_ERROR == eRet);

            memcpy(szData, sReturn, sReturn.uiLength() * sizeof(wchar_t));


        }
        catch (...)
        {
            ERROR_LOG(L"Exception!");
            return;
        }
    
    }       //  CopyStressData(...)

    void FillWordFormData(IWordForm* pWf, StWordForm* pstTarget) {

        pstTarget->llLexemeId = pWf->llLexemeId();
        std::memcpy(pstTarget->szWordForm, pWf->sWordForm(), pWf->sWordForm().uiLength() * sizeof(wchar_t));
        std::memcpy(pstTarget->szStem, pWf->sStem(), pWf->sStem().uiLength() * sizeof(wchar_t));
        pstTarget->ePos = pWf->ePos();
        pstTarget->eCase = pWf->eCase();
        pstTarget->eNumber = pWf->eNumber();
        pstTarget->eSubparadigm = pWf->eSubparadigm();
        pstTarget->eGender = pWf->eGender();
        pstTarget->ePerson = pWf->ePerson();
        pstTarget->eAnimacy = pWf->eAnimacy();
        pstTarget->eReflexivity = pWf->eReflexive();
        pstTarget->eAspect = pWf->eAspect();
        pstTarget->eStatus = pWf->eStatus();
        pstTarget->bIrregular = pWf->bIrregular();      // came from the DB as opposed to being generated by the app
        memcpy(pstTarget->szLeadComment, pWf->sLeadComment(), pWf->sLeadComment().uiLength() * sizeof(wchar_t));
        memcpy(pstTarget->szTrailingComment, pWf->sTrailingComment(), pWf->sTrailingComment().uiLength() * sizeof(wchar_t));
        pstTarget->bIsEdited = pWf->bIsEdited();
        pstTarget->bIsVariant = pWf->bIsVariant();
        memcpy(pstTarget->szGrammHash, pWf->sGramHash(), pWf->sGramHash().uiLength() * sizeof(wchar_t));
        CopyStressData(pWf, pstTarget->szStress);

    }       //  FillWordFormData()

    bool bParseWord(const wchar_t* szWord)
    {
        if (nullptr == g_pDictionary)
        {
            ERROR_LOG(L"Dictionary is not available.");
            exit(-1);
        }

        IParser* pParser = nullptr;
        ET_ReturnCode eRet = g_pDictionary->eGetParser(pParser);
        if (eRet != H_NO_ERROR)
        {
            ERROR_LOG(L"Unable to get parser object, exiting.");
            exit(-1);
        }

        eRet = pParser->eParseWord(szWord);
        if (eRet != H_NO_ERROR && eRet != H_FALSE)
        {
            cout << "Unable to parse word " << szWord << endl;
            return false;
        }

        if (H_FALSE == eRet)
        {
            cout << "Word " << szWord << "not found." << endl;
            return false;
        }

        IWordForm* pWf = nullptr;
        eRet = pParser->eGetFirstWordForm(pWf);
        if (eRet != H_NO_ERROR || nullptr == pWf)
        {
            cout << "Word " << szWord << ": unable to get first word form data." << endl;
            return false;
        }

        vecWordForms.push_back(pWf);

        while (H_NO_ERROR == eRet)
        {
            eRet = pParser->eGetNextWordForm(pWf);
            if (H_NO_ERROR == eRet)
            {
                if (nullptr == pWf)
                {
                    cout << "Word " << szWord << ": unable to get next word form data." << endl;
                    return false;
                }
            }

            vecWordForms.push_back(pWf);
        }

        return true;

    }       //  bParseWord()

    int iNumOfParses() 
    {
        return (int)vecWordForms.size();
    }

    bool GetParsedWordForm(int iNum, StWordForm* pstParsedForm)
    {
        if (iNum < 0 || iNum >= vecWordForms.size())
        {
            ERROR_LOG(L"Illegal parse index.");
            return false;
        }

        auto num = vecWordForms.size();
        auto koko = vecWordForms[iNum];
        FillWordFormData(koko, pstParsedForm);

//        cout << pstParsedForm->szWordForm;

        return true;

    }

    bool GenerateAllForms()
    {
        if (nullptr == g_pDictionary)
        {
            ERROR_LOG(L"Dictionary object not initialized.");
            return false;
        }

        ET_ReturnCode eRet = g_pDictionary->eGenerateAllForms();
        if (eRet != H_NO_ERROR)
        {
            cout << "Failed to generate word forms." << endl;
            return false;
        }

        return true;
    }

/*
    void ReportProgress(int iPercentDone, bool bOperationComplete, int iRecord, double dDuration)
    {
        cout << "Record " << iRecord << ", " << dDuration << " seconds." << endl;
        if (bOperationComplete)
        {
            cout << endl << "Done!" << endl << endl;
        }
    }
*/

    void ReportProgress(int iPercentDone, bool bOperationComplete, int iRecord, double dDuration)
    {
        cout << "Record " << right << setw(7) << iRecord << ", " << fixed << setprecision(3) << dDuration << " seconds, " << setw(4) << iPercentDone << "%" << endl;
        if (bOperationComplete)
        {
            cout << endl << "****    Done!" << endl << endl;
        }
    }

    bool AddLexemeHashes()
    {
        if (nullptr == g_pDictionary)
        {
            ERROR_LOG(L"Dictionary is not available.");
            exit(-1);
        }

        cout << endl << "****      ADDING LEXEME HASHES" << endl << endl;
        ET_ReturnCode eRet = g_pDictionary->ePopulateHashToDescriptorTable(nullptr, &ReportProgress);

        return true;
    }


    //
    //  Analytics
    //

    long long llParseText(const wchar_t* szTextName, const wchar_t* szMetadata, const wchar_t* szText)
    {
        ET_ReturnCode eRet = H_NO_ERROR;

        if (nullptr == g_pDictionary)
        {
            ERROR_LOG(L"Dictionary is not available.");
            return - 1;
        }

        if (nullptr == g_pAnalytics)
        {
            eRet = g_pDictionary->eGetAnalytics(g_pAnalytics);
            if (eRet != H_NO_ERROR)
            {
                ERROR_LOG(L"Unable to get analytics object, exiting.");
                return -1;
            }
        }

        long long llParsedTextId = 0;
        eRet = g_pAnalytics->eParseText(szTextName, szMetadata, szText, llParsedTextId);
        if (H_NO_ERROR != eRet)
        {
            llParsedTextId = -1;
            wcout << endl << L"Failed to parse text " << szTextName << endl << endl;
        }

        return llParsedTextId;
    }


}       //  namespace MainLibForPython 
