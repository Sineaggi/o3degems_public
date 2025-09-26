#include "AngelScriptPreprocessor.h"
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/regex_impl.h>

#include <AzCore/Utils/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AngelScript/Descriptions/AngelScriptClassDescription.h>

namespace AngelScript
{
    bool AngelScriptPreprocessor::Preprocess()
    {
        //for (auto& file : m_filesToPreprocess)
        //{
        //   // Preprocess(file.c_str());
        //}

        return false;
    }

    void AngelScriptPreprocessor::Preprocess(const AZStd::string& filePath)
    {
        auto readFileResult = AZ::Utils::ReadFile(filePath.c_str());
        if (readFileResult.IsSuccess())
        {
            AZStd::string fileContents = readFileResult.TakeValue();
            DetectClasses(fileContents);
        }
    }


    void AngelScriptPreprocessor::DetectClasses(const AZStd::string& content)
    {
        static const AZStd::regex classPattern("(class|struct)\\s+([A-Za-z0-9_]+)(\\s*:\\s*([A-Za-z0-9_]+\\s*::\\s*)*([A-Za-z0-9_]+))?");

        AZStd::smatch matches;
        //if (AZStd::regex_match(content, matches, classPattern))
        if (AZStd::regex_search(content.begin(), content.end(), matches, classPattern))
        {
            for (size_t i = 1; i < matches.size(); ++i)
            {
                auto matchedString = matches[i].str();

                AZStd::shared_ptr<ClassDescription> classDesc = AZStd::make_shared<ClassDescription>();
                classDesc->m_className = matchedString;
            }
        }

    }

    void AngelScriptPreprocessor::AddFile(const AZ::IO::Path& fileToPreprocess)
    {
        m_filesToPreprocess.push_back(fileToPreprocess);
    }
}
