

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
    from globalvars import DIR_ADDON_MODULE, DIR_ADDON_REPO, DIR_CACHE, DIR_CACHE_ADDONS, VALUE_LIST_LOCAL_REPOS, VALUE_LIST_WIKI_REPOS, VALUE_LIST_MANAGE_ADDONS, PARAM_INSTALL_FROM_ZIP, PARAM_LISTTYPE
    from FileManager import fileMgr
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

            # if 1st start create missing directories
            self.fileMgr = fileMgr()
            self.fileMgr.verifrep( DIR_ADDON_MODULE )
            self.fileMgr.verifrep( DIR_ADDON_REPO )
            self.fileMgr.verifrep( DIR_CACHE )
            self.fileMgr.verifrep( DIR_CACHE_ADDONS )

            # Check settings
            if xbmcplugin.getSetting('first_run') == 'true':
                # Check (only the 1st time) is xbmcaddon module is available
                print( "     **First run")
                if self._check_addon_lib():
                    print( "         XBMC Addon 4 XBOX Addon Library already installed")
                    print( "         Installing default repositories")
                    if ( self._installRepos() ):
                        xbmcplugin.setSetting('first_run','false')
                        self._createRootDir()
                else:
                    print( "         ERROR - XBMC Addon 4 XBOX Addon Library MISSING")
                    dialog = xbmcgui.Dialog()
                    dialog.ok( __language__(30000), __language__(30091) ,__language__(30092))
            else:
                self._createRootDir()

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


    def _check_compatible(self):
        """
        Check if XBMC version is compatible with the plugin
        """
        xbmcgui = None
        try:
            # spam plugin statistics to log
            print( "[PLUGIN] '%s: Version - %s-r%s' initialized!" % ( __plugin__, __version__, __svn_revision__.replace( "$", "" ).replace( "Revision", "" ).strip( ": " ) ) )
            # get xbmc revision
            xbmc_version = xbmc.getInfoLabel( "System.BuildVersion" )
            xbmc_rev = int( xbmc_version.split( " " )[ 1 ].replace( "r", "" ) )
            # compatible?
            ok = xbmc_rev >= int( __XBMC_Revision__ )
            print xbmc_rev
            print __XBMC_Revision__
        except:
            # error, so unknown, allow to run
            print_exc()
            xbmc_rev = 0
            ok = 2
        # spam revision info
        print( "     ** Required XBMC Revision: r%s **" % ( __XBMC_Revision__, ) )
        print( "     ** Found XBMC Revision: r%d [%s] **" % ( xbmc_rev, ( "Not Compatible", "Compatible", "Unknown", )[ ok ], ) )
        # if not compatible, inform user
        if ( not ok ):
            xbmcgui.Dialog().ok( "%s - %s: %s" % ( __plugin__, __language__( 30900 ), __version__, ), __language__( 30901 ) % ( __plugin__, ), __language__( 30902 ) % ( __XBMC_Revision__, ), __language__( 30903 ) )
        #if not xbmc run under xbox, inform user
        # get xbmc run under?
        platform = os.environ.get( "OS", "xbox" )
        if ( platform.upper() not in __platform__ ):
            ok = 0
            print( "system::os.environ [%s], This plugin run under %s only." % ( platform, __platform__, ) )
            if xbmcgui == None:
                xbmcgui.Dialog().ok( __plugin__, "%s: system::os.environ [[COLOR=ffe2ff43]%s[/COLOR]]" % ( __language__( 30904 ), platform, ), __language__( 30905 ) % __platform__ )
        return ok

    def _installRepos(self):
        """
        Install default repositories in the plugin data directory
        """
        ok = 0
        repo_source = os.path.join( ROOTDIR, "resources", "repositories" )
        try:
            if os.path.exists( repo_source ):
                self.fileMgr.copyDir( repo_source, DIR_ADDON_REPO )
                print "SUCCESS: Repositories copied %s"%DIR_ADDON_REPO
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


