

import os
#import urllib
import sys
#import xbmc
#import xbmcplugin
#import xbmcgui
from traceback import print_exc

#__script__       = sys.modules[ "__main__" ].__script__
#__plugin__       = sys.modules[ "__main__" ].__plugin__
#__author__       = sys.modules[ "__main__" ].__author__
#__url__          = sys.modules[ "__main__" ].__url__
#__svn_url__      = sys.modules[ "__main__" ].__svn_url__
#__credits__      = sys.modules[ "__main__" ].__credits__
#__platform__     = sys.modules[ "__main__" ].__platform__
#__date__         = sys.modules[ "__main__" ].__date__
__version__      = sys.modules[ "__main__" ].__version__
#__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__
#__XBMC_Revision__= sys.modules[ "__main__" ].__XBMC_Revision__
#__language__     = sys.modules[ "__main__" ].__language__
#ROOTDIR            = sys.modules[ "__main__" ].ROOTDIR
#BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
#LIBS_PATH          = sys.modules[ "__main__" ].LIBS_PATH
#MEDIA_PATH         = sys.modules[ "__main__" ].MEDIA_PATH


# Custom modules
try:
    from PluginMgr import PluginMgr
    from InstallMgr import InstallMgr
    from utilities import PersistentDataCreator
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

        #TODO: addd install of required lib
#        requiredLibs = addonDic[addonId]['required_lib']
#        print requiredLibs
#        status = installMgr._getAddonRequiredLibs( requiredLibs, repoId )

        #Check if install went well
        status, destination = installMgr.check_install(status, itemName, destination, addonInstaller)

        if status == "OK":
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


