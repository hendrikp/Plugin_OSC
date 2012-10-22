/* OSC_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CPluginOSC.h>

namespace OSCPlugin
{
    CPluginOSC* gPlugin = NULL;

    CPluginOSC::CPluginOSC()
    {
        gPlugin = this;
    }

    CPluginOSC::~CPluginOSC()
    {
        Release( true );

        gPlugin = NULL;
    }

    bool CPluginOSC::Release( bool bForce )
    {
        bool bRet = true;

        if ( !m_bCanUnload )
        {
            // Should be called while Game is still active otherwise there might be leaks/problems
            bRet = CPluginBase::Release( bForce );

            if ( bRet )
            {
                // Depending on your plugin you might not want to unregister anything
                // if the System is quitting.
                // if(gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting()) {

                // Cleanup like this always (since the class is static its cleaned up when the dll is unloaded)
                gPluginManager->UnloadPlugin( GetName() );

                // Allow Plugin Manager garbage collector to unload this plugin
                AllowDllUnload();
            }
        }

        return bRet;
    };

    bool CPluginOSC::Init( SSystemGlobalEnvironment& env, SSystemInitParams& startupParams, IPluginBase* pPluginManager, const char* sPluginDirectory )
    {
        gPluginManager = ( PluginManager::IPluginManager* )pPluginManager->GetConcreteInterface( NULL );
        CPluginBase::Init( env, startupParams, pPluginManager, sPluginDirectory );

        // Note: Autoregister Flownodes will be automatically registered

        return true;
    }

    const char* CPluginOSC::ListCVars() const
    {
        return "";
    }

    const char* CPluginOSC::GetStatus() const
    {
        return "OK";
    }

    // TODO: Add your plugin concrete interface implementation
}