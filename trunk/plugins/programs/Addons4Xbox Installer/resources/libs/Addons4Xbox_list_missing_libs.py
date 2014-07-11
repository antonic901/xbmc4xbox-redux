

import os
import sys
import xbmc
import xbmcplugin
import xbmcgui
from traceback import print_exc

# Custom modules
try:
    from globalvars import MISSING_MODULES_PATH, PARAM_LISTTYPE
    from utilities import PersistentDataRetriever
    from PluginMgr import PluginMgr
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):
        if True: #self._check_compatible():
            self.pluginMgr = PluginMgr()
            self.parameters = self.pluginMgr.parse_params()

            self._createMissingModulesDir()

        self.pluginMgr.add_sort_methods( False )
        self.pluginMgr.end_of_directory( True, update=False )


    def _check_addon_lib(self):
        """
        Check id xbmcaddon module is available
        Returns 1 if success, 0 otherwise
        """
        ok = 1
        try:
            import xbmcaddon
        except ImportError:
            ok = 0
        return ok


    def _createMissingModulesDir ( self ):
        """
        Creates list of missing modules
        For now this is retrieve from local DB (persistent file)
        TODO: implement option in order to check each installed addon
        """

        if os.path.exists(MISSING_MODULES_PATH):
            pdr = PersistentDataRetriever( MISSING_MODULES_PATH )
            missingModules = pdr.get_data()
            print missingModules

            for lib in missingModules:
                self.pluginMgr.addLink( lib["id"], "" )


        else:
            print "No missing modules found"



