

import os
#import urllib
#import sys
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
#__version__      = sys.modules[ "__main__" ].__version__
#__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__
#__XBMC_Revision__= sys.modules[ "__main__" ].__XBMC_Revision__
#__language__     = sys.modules[ "__main__" ].__language__
#ROOTDIR            = sys.modules[ "__main__" ].ROOTDIR
#BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
#LIBS_PATH          = sys.modules[ "__main__" ].LIBS_PATH
#MEDIA_PATH         = sys.modules[ "__main__" ].MEDIA_PATH


# Custom modules
try:
    from globalvars import DIR_ADDON_REPO, PARAM_REPO_ID, PARAM_LISTTYPE, VALUE_LIST_CATEGORY
    from PluginMgr import PluginMgr
    #from utilities import readURL,RecursiveDialogProgress, checkURL
    from XmlParser import parseAddonXml
    from AddonsMgr import getInstalledAddonInfo
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):

        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()

        self._createLocalRepoDir()

        self.pluginMgr.add_sort_methods( True )
        self.pluginMgr.end_of_directory( True, update=False )


    def _createLocalRepoDir ( self ):
        """
        List of installed repositories
        """
        for repoId in os.listdir( DIR_ADDON_REPO ):

            # Retrieve info from  addon.xml
            #itemInfo = self._getInstalledAddInfo( os.path.join( DIR_ADDON_REPO, repoId ) )
            itemInfo = getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, repoId) )
            if itemInfo:
                # Dic Not empty
                paramsRepo = {}
                paramsRepo[PARAM_REPO_ID]  = repoId
                paramsRepo[PARAM_LISTTYPE] = VALUE_LIST_CATEGORY
                urlRepo = self.pluginMgr.create_param_url( paramsRepo )
                if urlRepo:
                    self.pluginMgr.addDir( itemInfo [ "name" ], urlRepo, iconimage=itemInfo [ "icon" ] )

