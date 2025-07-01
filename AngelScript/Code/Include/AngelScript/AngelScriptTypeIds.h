
#pragma once

namespace AngelScript
{
    // System Component TypeIds
    inline constexpr const char* AngelScriptSystemComponentTypeId = "{807EE226-37A5-4F30-9429-F73D09CA7E3F}";
    inline constexpr const char* AngelScriptEditorSystemComponentTypeId = "{2D1EAC14-CF11-4CCF-9CE3-A18DCBDA4DBF}";

    // Module derived classes TypeIds
    inline constexpr const char* AngelScriptModuleInterfaceTypeId = "{0F068B98-FD9F-4CF8-8BFE-6E90F7DF7975}";
    inline constexpr const char* AngelScriptModuleTypeId = "{31A35788-C98C-40E5-9F1A-D1848390833A}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* AngelScriptEditorModuleTypeId = AngelScriptModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* AngelScriptRequestsTypeId = "{3F907FE0-DC48-4872-93BB-4DAEF514875D}";
} // namespace AngelScript
