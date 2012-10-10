/* OSC_Plugin - for licensing and copyright see license.txt */

#include <IPluginBase.h>

#pragma once

/**
* @brief OSC Plugin Namespace
*/
namespace OSCPlugin
{
    /**
    * @brief plugin OSC concrete interface
    */
    struct IPluginOSC
    {
        /**
        * @brief Get Plugin base interface
        */
        virtual PluginManager::IPluginBase* GetBase() = 0;

        // TODO: Add your concrete interface declaration here
    };
};