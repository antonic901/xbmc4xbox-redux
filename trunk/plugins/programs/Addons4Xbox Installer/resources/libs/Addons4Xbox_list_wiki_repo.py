

#import os
#import urllib
import sys
#import xbmc
import xbmcplugin
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
#__version__      = sys.modules[ "__main__" ].__version__
#__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__
#__XBMC_Revision__= sys.modules[ "__main__" ].__XBMC_Revision__
__language__     = sys.modules[ "__main__" ].__language__
#ROOTDIR            = sys.modules[ "__main__" ].ROOTDIR
#BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
#LIBS_PATH          = sys.modules[ "__main__" ].LIBS_PATH
MEDIA_PATH         = sys.modules[ "__main__" ].MEDIA_PATH
PERSIT_REPO_LIST   = sys.modules[ "__main__" ].PERSIT_REPO_LIST
REPO_LIST_URL_LIST = sys.modules[ "__main__" ].REPO_LIST_URL_LIST

__settings__ = xbmcplugin.getSetting


# Custom modules
try:
    from globalvars import PARAM_REPO_ID, PARAM_ADDON_ID, PARAM_TYPE, PARAM_INSTALL_FROM_REPO, PARAM_ADDON_NAME, PARAM_URL, PARAM_DATADIR
    from PluginMgr import PluginMgr
    #from Item import supportedAddonList #TYPE_ADDON_SCRIPT, TYPE_ADDON_MUSIC, TYPE_ADDON_PICTURES, TYPE_ADDON_PROGRAMS, TYPE_ADDON_VIDEO, TYPE_ADDON_WEATHER, TYPE_ADDON_MODULE
    #from utilities import readURL,RecursiveDialogProgress, checkURL
    #from XmlParser import ListItemFromXML, parseAddonXml
    from wikiparser import ListItemFromWiki
    #from utilities import PersistentDataCreator
except:
    print_exc()

# URLs
#REPO_LIST_URL = "http://wiki.xbmc.org/index.php?title=Unofficial_Add-on_Repositories"

class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):

        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()

        print "List of repositories from XBMC wiki page"
        self._createRepo2InstallListDir()

        #self.pluginMgr.end_of_directory( True, update=False )
        self.pluginMgr.add_sort_methods( True )
        self.pluginMgr.end_of_directory( True )

    def _createRepo2InstallListDir( self ):
        """
        Creates list for install of the Unofficial repositories available on XBMC wiki
        """

        print "createRepo2InstallListDir"
        #xbmcplugin.setPluginCategory( handle=int( sys.argv[ 1 ] ), category=__language__( 30001 ) )

        wiki_server_id = int( __settings__( 'wiki_server' ) )
        print "Loading wiki page: %s"%REPO_LIST_URL_LIST[wiki_server_id]
        listRepoWiki = ListItemFromWiki(REPO_LIST_URL_LIST[wiki_server_id])
        keepParsing = True
        while (keepParsing):
            item = listRepoWiki.getNextItem()
            if item:
                if 'repo_url' in item and item['repo_url']:
                    paramsAddons = {}
                    paramsAddons[PARAM_INSTALL_FROM_REPO]   = 'true'
                    paramsAddons[PARAM_ADDON_ID]            = 'None' # No ID at this stage in case of repo from the wiki
                    paramsAddons[PARAM_ADDON_NAME]          = item['name']
                    paramsAddons[PARAM_URL]                 = item['repo_url']
                    paramsAddons[PARAM_DATADIR]             = 'None'
                    paramsAddons[PARAM_TYPE]                = 'zip'
                    paramsAddons[PARAM_REPO_ID]             = 'None'

                    url = self.pluginMgr.create_param_url( paramsAddons )

                    if ( url ):
                        item['PluginUrl'] = url
                        self.pluginMgr.addItemLink( item )
            else:
                keepParsing = False

        # Save list to a file
        #PersistentDataCreator( list, PERSIT_REPO_LIST )


