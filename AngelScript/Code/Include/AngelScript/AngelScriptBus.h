
#pragma once

#include <AngelScript/AngelScriptTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AngelScript
{
    class AngelScriptRequests
    {
    public:
        AZ_RTTI(AngelScriptRequests, AngelScriptRequestsTypeId);
        virtual ~AngelScriptRequests() = default;
        // Put your public methods here
    };

    class AngelScriptBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using AngelScriptRequestBus = AZ::EBus<AngelScriptRequests, AngelScriptBusTraits>;
    using AngelScriptInterface = AZ::Interface<AngelScriptRequests>;

} // namespace AngelScript
