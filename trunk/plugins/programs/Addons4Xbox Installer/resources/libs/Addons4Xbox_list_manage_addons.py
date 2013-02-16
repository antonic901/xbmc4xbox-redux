

import os
#import urllib
import sys
import xbmc
import xbmcplugin
import xbmcgui
from traceback import print_exc

__script__       = sys.modules[ "__main__" ].__script__
__plugin__       = sys.modules[ "__main__" ].__plugin__
__author__       = sys.modules[ "__main__" ].__author__
__url__          = sys.modules[ "__main__" ].__url__
__svn_url__      = sys.modules[ "__main__" ].__svn_url__
__credits__      = sys.modules[ "__main__" ].__credits__
__platform__     = sys.modules[ "__main__" ].__platform__
__date__         = sys.modules[ "__main__" ].__date__
__version__      = sys.modules[ "__main__" ].__version__
__svn_revision__ = sys.modules[ "__main__" ].__svn_revision__
__XBMC_Revision__= sys.modules[ "__main__" ].__XBMC_Revision__
__language__     = sys.modules[ "__main__" ].__language__
ROOTDIR            = sys.modules[ "__main__" ].ROOTDIR
BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
LIBS_PATH          = sys.modules[ "__main__" ].LIBS_PATH
MEDIA_PATH         = sys.modules[ "__main__" ].MEDIA_PATH


# Custom modules
try:
    from globalvars import VALUE_LIST_MISSING_MODULES, PARAM_LISTTYPE
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

            self._createManageAddonsDir()

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


    def _createManageAddonsDir ( self ):
        """
        Creates manage addons list options
        """
        paramsDicRepo = {}
        paramsDicRepo[PARAM_LISTTYPE] = VALUE_LIST_MISSING_MODULES
        urlRepo = self.pluginMgr.create_param_url( paramsDicRepo )
        if urlRepo:
            self.pluginMgr.addDir( __language__( 30250 ), urlRepo )



