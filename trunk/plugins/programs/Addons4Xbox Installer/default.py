"""
   Installing XBMC addons on XBMC4XBOX using xbmcaddon Nuka1195 library
   Special thanks to Frost for his library, Nuka1195 for his library too and Maxoo for the logo
   Please read changelog
"""

REMOTE_DBG       = False


# Plugin constants
__script__       = "Unknown"
__plugin__       = "Addons4Xbox Installer"
__author__       = "Temhil (http://passion-xbmc.org)"
__url__          = "http://passion-xbmc.org/index.php"
__svn_url__      = "http://passion-xbmc.googlecode.com/svn/trunk/plugins/programs/Addons4xbox/"
__credits__      = "Team XBMC Passion"
__platform__     = "xbmc media center [XBOX]"
__date__         = "2012-02-11"
__version__      = "0.10"
__svn_revision__ = 0
__XBMC_Revision__= 30805


import os
import urllib
import xbmc
import xbmcplugin
import xbmcgui
from traceback import print_exc


# Remote debugger using Eclipse and Pydev
if REMOTE_DBG:
    # Note pydevd module need to be copied in XBMC\system\python\Lib\pysrc
    try:
        import pysrc.pydevd as pydevd
        pydevd.settrace('localhost', stdoutToServer=True, stderrToServer=True)
    except ImportError:
        sys.stderr.write("Error: " +
            "You must add org.python.pydev.debug.pysrc to XBMC\system\python\Lib\pysrc")
        sys.exit(1)



ROOTDIR            = os.getcwd()
BASE_RESOURCE_PATH = os.path.join( ROOTDIR, "resources" )
LIBS_PATH          = os.path.join( BASE_RESOURCE_PATH, "libs" )
MEDIA_PATH         = os.path.join( BASE_RESOURCE_PATH, "media" )
PERSIT_REPO_LIST   = "repo_list.txt"

# URLs
#REPO_LIST_URL = "http://wiki.xbmc.org/index.php?title=Unofficial_Add-on_Repositories"
REPO_LIST_URL_LIST = ["http://wiki.xbmc.org/index.php?title=Unofficial_Add-on_Repositories",
                      "http://home.brantje.com/xbmcrepositories/Unofficial-add-on-repositories-XBMC.htm" ]

__platform__ = "xbmc media center, [%s]" % xbmc.__platform__
__language__ = xbmc.Language( ROOTDIR ).getLocalizedString


# Custom modules
try:
    from resources.libs.globalvars import PARAM_INSTALL_FROM_REPO, PARAM_INSTALL_FROM_ZIP, PARAM_LISTTYPE, PARAM_ACTION, VALUE_LIST_LOCAL_REPOS, VALUE_LIST_WIKI_REPOS, VALUE_LIST_CATEGORY, VALUE_LIST_ADDONS, VALUE_DISPLAY_INFO, VALUE_LIST_MANAGE_ADDONS, VALUE_LIST_MISSING_MODULES, SPECIAL_SCRIPT_DATA
    from resources.libs.PluginMgr import PluginMgr
except:
    print_exc()

# get xbmc run under?
#platform = os.environ.get( "OS", "xbox" )


class Addons4xboxInstallerPlugin:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):
        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()
        print "Parameters"
        print self.parameters
        self.select()



    def select( self ):
        """
        Decides what to do based on the plugin URL
        """
        try:
            print "select"
            print self.parameters

            #
            # Create root list
            #
            if len(self.parameters) < 1:
                import resources.libs.Addons4Xbox_list_root as plugin

            #
            # Install from remote repository
            #
            elif PARAM_INSTALL_FROM_REPO in self.parameters.keys():
                import resources.libs.Addons4Xbox_install_remote as plugin

            #
            # Install from zip
            #
            elif PARAM_INSTALL_FROM_ZIP in self.parameters.keys():
                import resources.libs.Addons4Xbox_install_zip as plugin

            #
            # List of available Add-ons repositories installed
            #
            elif PARAM_LISTTYPE in self.parameters.keys() and VALUE_LIST_LOCAL_REPOS == self.parameters[PARAM_LISTTYPE]:
                import resources.libs.Addons4Xbox_list_local_repo as plugin


            #
            # List of available repositories on XBMC wiki page
            #
            elif PARAM_LISTTYPE in self.parameters.keys() and VALUE_LIST_WIKI_REPOS == self.parameters[PARAM_LISTTYPE]:
                import resources.libs.Addons4Xbox_list_wiki_repo as plugin

            #
            # List of Add-ons categories for a repository
            #
            elif PARAM_LISTTYPE in self.parameters.keys() and VALUE_LIST_CATEGORY == self.parameters[PARAM_LISTTYPE]:
                import resources.libs.Addons4Xbox_list_repo_cat as plugin

            #
            # List of Add-ons of a categories for a specific repository
            #
            elif PARAM_LISTTYPE in self.parameters.keys() and VALUE_LIST_ADDONS == self.parameters[PARAM_LISTTYPE]:
                import resources.libs.Addons4Xbox_list_addons as plugin


            #
            # List of options for managing installed Add-ons
            #
            elif PARAM_LISTTYPE in self.parameters.keys() and VALUE_LIST_MANAGE_ADDONS == self.parameters[PARAM_LISTTYPE]:
                import resources.libs.Addons4Xbox_list_manage_addons as plugin


            #
            # List of missing modules (usually not found during the installed of an addon)
            #
            elif PARAM_LISTTYPE in self.parameters.keys() and VALUE_LIST_MISSING_MODULES == self.parameters[PARAM_LISTTYPE]:
                import resources.libs.Addons4Xbox_list_missing_libs as plugin


            #
            # Display addon info Window
            #
            elif PARAM_ACTION in self.parameters.keys() and VALUE_DISPLAY_INFO == self.parameters[PARAM_ACTION]:
                import resources.libs.DialogRepoInfo as plugin

            # Run
            plugin.Main()
        except:
            print_exc()
            self.pluginMgr.end_of_directory( False )




#######################################################################################################################
# BEGIN !
#######################################################################################################################

if __name__ == "__main__":
    try:
        Addons4xboxInstallerPlugin()
    except:
        print_exc()
