

import os
#import urllib
import sys
import xbmc
#import xbmcplugin
import xbmcgui
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
__language__     = sys.modules[ "__main__" ].__language__
#ROOTDIR            = sys.modules[ "__main__" ].ROOTDIR
#BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
#LIBS_PATH          = sys.modules[ "__main__" ].LIBS_PATH
#MEDIA_PATH         = sys.modules[ "__main__" ].MEDIA_PATH


# Custom modules
try:
    from InstallMgr import InstallMgr
    from PluginMgr import PluginMgr
    from globalvars import DIR_CACHE, PARAM_REPO_ID, PARAM_TYPE, PARAM_URL, PARAM_ADDON_NAME, PARAM_ADDON_ID, PARAM_DATADIR
    from utilities import PersistentDataRetriever
    from Item import TYPE_ADDON_MODULE
    #from XmlParser import ListItemFromXML, parseAddonXml
    from AddonsMgr import removeMissingModule2DB, saveLocalAddonInfo
    from FileManager import fileMgr
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):

        self.fileMgr   = fileMgr()
        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()

        # Install from remote server
        status = self._install_addon_remote()

        self.pluginMgr.end_of_directory( True, update=False )


    def _install_addon_remote( self ):
        """
        Install an addon from a remote/web repository
        """
        xbmc.log("_install_addon_remote", xbmc.LOGDEBUG)
        status = "OK"
        installMgr = InstallMgr()

        #TODO: solve encoding pb on name

        addonName = unicode( self.parameters[ PARAM_ADDON_NAME ] , 'ISO 8859-1', errors='ignore' )
        addonId = '%s'%self.parameters[ PARAM_ADDON_ID ]
        addonUrl = self.parameters[ PARAM_URL ].replace( ' ', '%20' )
        addonFormat = self.parameters[ PARAM_TYPE ]
        repoId = self.parameters[ PARAM_REPO_ID ]
        #repoId = self.parameters[ self.PARAM_REPO_ID ]
        dataDir = self.parameters[ PARAM_DATADIR ].replace( ' ', '%20' ) #self.repoList[repoId]['datadir']

        if ( xbmcgui.Dialog().yesno( addonName, __language__( 30050 ), "", "" ) ):

            # Check if we install repo
            if "None" != repoId:
                # Retrieve addon info from persitence
                pdr = PersistentDataRetriever( os.path.join( DIR_CACHE, repoId + ".txt" ) )
                addonDic = pdr.get_data()
                requiredLibs = addonDic[addonId]['required_lib']
                status = installMgr._getAddonRequiredLibs( requiredLibs, repoId )

            if status == "OK":
                # TODO: check repo ID
                status, itemName, destination, addonInstaller = installMgr.install_from_repo( addonName, addonUrl, addonFormat, dataDir )
            else:
                # The install of required addons was not full
                itemName       = addonDic[addonId]["name"]
                destination    = None
                addonInstaller = None

            # Check if install went well
            status, destination = installMgr.check_install(status, itemName, destination, addonInstaller)

            if status == "OK": # and "None" != repoId:
                #self.saveLocalAddonInfo(repoId, destination, addonInstaller)
                saveLocalAddonInfo(repoId, destination, addonInstaller)

                # Check is addon is a module and if it was part of the missing modules list
                #addonType    = addonInstaller.getItemType()
                #addonVersion = addonInstaller.getItemVersion()
                if "None" != repoId:
                    installedModuleItem = addonDic[addonId]
                    if TYPE_ADDON_MODULE == installedModuleItem["type"]:
                        # We just installed successfully a module
                        # Check if it was part of the missing modules list and remove it if it is the case
                        removeMissingModule2DB(installedModuleItem)


        return status

