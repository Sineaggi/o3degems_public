#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>

namespace AngelScript
{

    class AngelScriptPreprocessor
    {
    public:

        void AddFile(const AZ::IO::Path& /*fileToPreprocess*/);
        bool Preprocess();


        [[nodiscard]] void DetectClasses(const AZStd::string& content);

        void Preprocess(const AZStd::string& filePath);

    private:



        AZStd:: vector<AZStd::string> m_generatedCode;
        AZStd::vector<AZ::IO::Path> m_filesToPreprocess;


        AZ::IO::HandleType m_openFileHandle;
    };

}
