/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>
#include <Builders/AngelScriptBuilderWorker.h>

namespace AngelScript
{
    class AngelScriptBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AngelScriptBuilderComponent, "{4CE84E52-AAF1-4802-B4CE-8685B36B59AA}");
        static void Reflect(AZ::ReflectContext* context);

        AngelScriptBuilderComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:

        //class cannot be copied
        AngelScriptBuilderComponent(const AngelScriptBuilderComponent&) = delete;
        AngelScriptBuilderComponent& operator=(const AngelScriptBuilderComponent&) = delete;

        AngelScriptBuilderWorker m_angelScriptBuilder;
    };
}
