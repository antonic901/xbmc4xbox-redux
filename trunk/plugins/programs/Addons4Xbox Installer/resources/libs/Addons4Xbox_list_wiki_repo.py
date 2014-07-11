

import sys
import xbmcplugin
import xbmcgui
from traceback import print_exc

__language__     = sys.modules[ "__main__" ].__language__
REPO_LIST_URL_LIST = sys.modules[ "__main__" ].REPO_LIST_URL_LIST

__settings__ = xbmcplugin.getSetting

# Custom modules
try:
    from globalvars import PARAM_REPO_ID, PARAM_ADDON_ID, PARAM_TYPE, PARAM_INSTALL_FROM_REPO, PARAM_ADDON_NAME, PARAM_URL, PARAM_DATADIR
    from PluginMgr import PluginMgr
    from wikiparser import ListItemFromWiki
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):

        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()

        print "List of repositories from XBMC wiki page"
        self._createRepo2InstallListDir()

        self.pluginMgr.add_sort_methods( True )
        self.pluginMgr.end_of_directory( True )

    def _createRepo2InstallListDir( self ):
        """
        Creates list for install of the Unofficial repositories available on XBMC wiki
        """

        print "createRepo2InstallListDir"

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



