

import os
import sys
import xbmc
import xbmcplugin
import xbmcgui
from traceback import print_exc

__plugin__       = sys.modules[ "__main__" ].__plugin__
__author__       = sys.modules[ "__main__" ].__author__
__platform__     = sys.modules[ "__main__" ].__platform__
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
    from globalvars import DIR_ADDON_MODULE, DIR_ADDON_REPO, DIR_CACHE, DIR_CACHE_ADDONS, VALUE_LIST_LOCAL_REPOS, VALUE_LIST_WIKI_REPOS, VALUE_LIST_MANAGE_ADDONS, PARAM_INSTALL_FROM_ZIP, PARAM_LISTTYPE, VERSION_FILE
    from FileManager import fileMgr
    from PluginMgr import PluginMgr
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):
        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()

        # if 1st start create missing directories
        self.fileMgr = fileMgr()
        self.fileMgr.verifrep( DIR_ADDON_MODULE )
        self.fileMgr.verifrep( DIR_ADDON_REPO )
        self.fileMgr.verifrep( DIR_CACHE )
        self.fileMgr.verifrep( DIR_CACHE_ADDONS )

        old_version = ''
        if os.path.isfile(VERSION_FILE):
            fh = open( VERSION_FILE, "r" )
            try:
                old_version = fh.read()
            finally:
                fh.close()
        if old_version != __version__ or not os.path.exists( DIR_ADDON_REPO ):
            fh = open( VERSION_FILE, "wb" )
            try:
                fh.write( __version__ )
            finally:
                fh.close()
            if self._installRepos():
                self._createRootDir()
        else:
            self._createRootDir()

        self.pluginMgr.add_sort_methods( False )
        self.pluginMgr.end_of_directory( True, update=False )

    def _installRepos(self):
        """
        Install default repositories in the plugin data directory
        """
        ok = 0
        repo_source = os.path.join( ROOTDIR, "resources", "repositories" )
        try:
            if os.path.exists( repo_source ):
                self.fileMgr.copyDir( repo_source, DIR_ADDON_REPO )
                print "SUCCESS: Repositories copied %s" % DIR_ADDON_REPO
                ok = 1
            else:
                dialog = xbmcgui.Dialog()
                dialog.ok( __language__(30000), __language__(30006), __language__(30007) )
                print "ERROR: impossible to copy repositories"
                print "Repositories are missing in the plugin structure"
        except:
            dialog = xbmcgui.Dialog()
            dialog.ok( __language__(30000), __language__(30006), __language__(30007) )
            print "ERROR: impossible to copy repositories to %s"%DIR_ADDON_REPO
            print_exc()
        return ok


    def _createRootDir ( self ):
        """
        Creates root list of the plugin
        """
        paramsDicRepo = {}
        paramsDicRepo[PARAM_LISTTYPE] = VALUE_LIST_LOCAL_REPOS
        urlRepo = self.pluginMgr.create_param_url( paramsDicRepo )
        if urlRepo:
            self.pluginMgr.addDir( __language__( 30202 ), urlRepo )

        paramsDicZip = {}
        paramsDicZip[PARAM_INSTALL_FROM_ZIP] = "true"
        urlZip = self.pluginMgr.create_param_url( paramsDicZip )
        if urlZip:
            self.pluginMgr.addLink( __language__( 30203 ), urlZip )

        paramsDicRepo = {}
        paramsDicRepo[PARAM_LISTTYPE] = VALUE_LIST_WIKI_REPOS
        urlRepo = self.pluginMgr.create_param_url( paramsDicRepo )
        if urlRepo:
            self.pluginMgr.addDir( __language__( 30204 ), urlRepo )

        paramsDicRepo = {}
        paramsDicRepo[PARAM_LISTTYPE] = VALUE_LIST_MANAGE_ADDONS
        urlRepo = self.pluginMgr.create_param_url( paramsDicRepo )
        if urlRepo:
            self.pluginMgr.addDir( __language__( 30205 ), urlRepo )


