

import os
import sys
from traceback import print_exc

# Custom modules
try:
    from PluginMgr import PluginMgr
    from InstallMgr import InstallMgr
    from utilities import PersistentDataCreator, PersistentDataRetriever
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):
        self.pluginMgr = PluginMgr()

        # Install from zip file
        status = self._install_addon_zip()

        self.pluginMgr.end_of_directory( True, update=False )


    def _install_addon_zip( self ):
        """
        Install an addon from a local zip file
        """
        installMgr = InstallMgr()

        # Install from zip file
        status, itemName, destination, addonInstaller = installMgr.install_from_zip()

        #Check if install went well
        status, destination = installMgr.check_install(status, itemName, destination, addonInstaller)

        if status == "OK":
            requiredLibs = addonInstaller.itemInfo[ "required_lib" ]
            status = installMgr._getAddonRequiredLibs( requiredLibs )
            
            from datetime import datetime

            #Install OK so save information for future update
            addonInstallName = addonInstaller.getItemInstallName()
            addonVersion = addonInstaller.getItemVersion()
            addonInfo = {}
            addonInfo['name'] = addonInstallName
            addonInfo['date'] = datetime.now()
            addonInfo['version'] = addonVersion
            addonInfo['repository'] = None
            addonInfo['installer_version'] = __version__

            PersistentDataCreator( addonInfo, os.path.join( destination, "a4x.psdt" ) )

        return status


