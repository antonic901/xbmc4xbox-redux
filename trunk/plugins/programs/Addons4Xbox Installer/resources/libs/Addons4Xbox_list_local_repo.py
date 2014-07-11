

import os
from traceback import print_exc

# Custom modules
try:
    from globalvars import DIR_ADDON_REPO, PARAM_REPO_ID, PARAM_LISTTYPE, VALUE_LIST_CATEGORY
    from PluginMgr import PluginMgr
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

            itemInfo = getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, repoId) )
            if itemInfo:
                # Dic Not empty
                paramsRepo = {}
                paramsRepo[PARAM_REPO_ID]  = repoId
                paramsRepo[PARAM_LISTTYPE] = VALUE_LIST_CATEGORY
                urlRepo = self.pluginMgr.create_param_url( paramsRepo )
                if urlRepo:
                    self.pluginMgr.addDir( itemInfo [ "name" ], urlRepo, iconimage=itemInfo [ "icon" ] )

