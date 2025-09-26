#include <AngelScript/AngelScriptAsset.h>

namespace AngelScript
{
    void AngelScriptData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<AngelScriptData>()
                ->Version(1)
                ->Field("m_debugName", &AngelScriptData::m_debugName)
                ->Field("m_dependencies", &AngelScriptData::m_dependencies)
                ->Field("m_script", &AngelScriptData::m_script)
                ;
        }
    }

    AZ::IO::MemoryStream AngelScriptData::CreateScriptReadStream()
    {
        return AZ::IO::MemoryStream(m_script.data(), m_script.size());
    }

    AZ::IO::ByteContainerStream<AZStd::vector<char>> AngelScriptData::CreateScriptWriteStream()
    {
        return AZ::IO::ByteContainerStream<AZStd::vector<char>>(&m_script, m_script.size());
    }

    const char* AngelScriptData::GetDebugName()
    {
        return m_debugName.empty() ? nullptr : m_debugName.c_str();
    }

    const AZStd::vector<char>& AngelScriptData::GetScriptBuffer()
    {
        return m_script;
    }
}
