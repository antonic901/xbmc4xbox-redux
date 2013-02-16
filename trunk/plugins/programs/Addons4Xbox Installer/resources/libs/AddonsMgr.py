"""
    Provides funcitons ofr managing the install of addons
"""

__all__ = [
    # public names
    "AddonsMgr",
    "getInstalledAddonInfo",
    "isLibInstalled",
    "addMissingModules2DB",
    "saveLocalAddonInfo",
    "removeMissingModule2DB"
    ]


import os
import sys
import pickle

#import urllib
import xbmc
import xbmcgui
#import xbmcplugin
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
PERSIT_REPO_LIST   = sys.modules[ "__main__" ].PERSIT_REPO_LIST



__platform__ = "xbmc media center, [%s]" % xbmc.__platform__
__language__ = xbmc.Language( ROOTDIR ).getLocalizedString


# Custom modules
try:
    #from globalvars import SPECIAL_HOME_DIR, DIR_ADDON_MODULE, DIR_ADDON_REPO, DIR_CACHE, VALUE_LIST_LOCAL_REPOS, PARAM_INSTALL_FROM_ZIP, PARAM_LISTTYPE
    #from globalvars import SPECIAL_HOME_DIR
    #from FileManager import fileMgr
    from Item import get_install_path, MISSING_MODULES_PATH, TYPE_ADDON_REPO, TYPE_ADDON_MODULE, TYPE_ADDON_VIDEO, TYPE_ADDON_MUSIC, TYPE_ADDON_PROGRAMS, TYPE_ADDON_PICTURES, TYPE_ADDON_SCRIPT
    #from utilities import RecursiveDialogProgress
    #from XmlParser import ListItemFromXML, parseAddonXml
    from XmlParser import parseAddonXml
    from utilities import versionsCmp, readURL, PersistentDataCreator, PersistentDataRetriever
except:
    print_exc()


def getInstalledAddonInfo( addonpath ):
    """
    Get metadata from addon.xml of an installed addon
    """
    itemInfo = {}

    # Open addon.xml
    print "getInstalledAddonInfo: Addon path: %s"%addonpath
    xmlInfofPath = os.path.join( addonpath, "addon.xml")
    if os.path.exists( xmlInfofPath ):
        try:
            xmlData = open( os.path.join( xmlInfofPath ), "r" )
            statusGetInfo = parseAddonXml( xmlData, itemInfo )
            xmlData.close()
        except:
            print_exc()
        if statusGetInfo == "OK":
            iconPath = os.path.join( xmlInfofPath, "icon.png")
            if os.path.exists( iconPath ):
                itemInfo [ "icon" ] = iconPath
            else:
                #TODO: move default image selection in the caller code????
                itemInfo [ "icon" ]="DefaultFolder.png"
    return itemInfo


def isLibInstalled(id):
    """
    Check if a lib/module is already install and return the version of the current installed version
    """
    libVersion = None
    installPath = get_install_path( TYPE_ADDON_MODULE )
    libpath = os.path.join( installPath, os.path.basename( id ) )
    #TODO: add check on version,  for now we just check a module with the right id is installed or not
    if os.path.exists( libpath ):
        # Get version
        libInfo = getInstalledAddonInfo( os.path.join( libpath ) )
        libVersion = libInfo[ "version" ]

    print "isLibInstalled: %s installed version: %s"%( id, libVersion )
    return libVersion


def addMissingModules2DB(addonIdList):
    """
    Add the list of modules (which failed to be installed) to the list of missing module
    """
    # Update list of module which fail to install
    missingModules = []
    if os.path.exists(MISSING_MODULES_PATH):
        pdr = PersistentDataRetriever( MISSING_MODULES_PATH )
        missingModules = pdr.get_data()

    print 'missingModules:'
    print missingModules
    for lib in addonIdList:
        print lib
        # add missing module and check if already in the list
        if len(missingModules) > 0:
            for module in missingModules:
                if module['id'] == lib['id']:
                    print "Module already in missing module list - checking version ..."
                    if versionsCmp( module['version'], lib["version"] ) < 0:
                        print "module version in list older than lib required - updating version ..."
                        module['version'] = lib["version"]
                    break
                else:
                    print "module not found in missing module list - adding it"
                    missingModules.append(lib)
        else:
            print "module not found in missing module list - adding it"
            missingModules.append(lib)

    PersistentDataCreator( missingModules, MISSING_MODULES_PATH )


def removeMissingModule2DB(item):
    """
    Check if a module is part of the missing modules list and remove it if it is the case
    Remove only module with version identical or more recent than the one in the list
    """
    # Update list of module which fail to install
    missingModules = []
    if os.path.exists(MISSING_MODULES_PATH):
        pdr = PersistentDataRetriever( MISSING_MODULES_PATH )
        missingModules = pdr.get_data()

    print 'missingModules:'
    print missingModules
    if len(missingModules) > 0:
        for module in missingModules:
            if module['id'] == item['id']:
                print "Module already in missing module list - checking version ..."
                if module['version'] == item["version"] or versionsCmp( module['version'], item["version"] ) < 0:
                    print "module version in list identical or older than installed module - removing it form list ..."
                    missingModules.remove(item)
                break
            else:
                print "module not found in missing module list"
    else:
        print "No missing modules"
    PersistentDataCreator( missingModules, MISSING_MODULES_PATH )


def saveLocalAddonInfo( repoId, destination, addonInstaller ):
    """
    Persit data about addon we have just install
    """
    from datetime import datetime

    #Install OK so save information for future update
    addonInstallName = addonInstaller.getItemInstallName()
    addonId          = addonInstaller.getItemId()
    addonVersion     = addonInstaller.getItemVersion()
    addonType        = addonInstaller.getItemType()

    addonInfo         = {}
    addonInfo['id']   = addonId
    addonInfo['name'] = addonInstallName
    addonInfo['date'] = datetime.now()
    addonInfo['version'] = addonVersion
    if addonType == TYPE_ADDON_REPO:
        addonInfo['repository'] = addonId
    else:
        addonInfo['repository'] = repoId
    addonInfo['installer_version'] = __version__
    print addonInfo
    PersistentDataCreator( addonInfo, os.path.join( destination, "a4x.psdt" ) )


class AddonsMgr:
    """
    Manage install of Addons
    """

    def __init__( self, *args, **kwargs ):
        pass

    def getInstalledAddInfo( self, addonpath ):
        #TODO: move to InstallMgr?
        """
        Get metadata from addon.xml of an installed addon
        """
        itemInfo = {}

        # Open addon.xml
        print "Addon path: %s"%addonpath
        xmlInfofPath = os.path.join( addonpath, "addon.xml")
        if os.path.exists( xmlInfofPath ):
            try:
                xmlData = open( os.path.join( xmlInfofPath ), "r" )
                statusGetInfo = parseAddonXml( xmlData, itemInfo )
                xmlData.close()
            except:
                print_exc()
            if statusGetInfo == "OK":
                iconPath = os.path.join( xmlInfofPath, "icon.png")
                if os.path.exists( iconPath ):
                    itemInfo [ "icon" ] = iconPath
                else:
                    #TODO: move default image selection in the caller code????
                    itemInfo [ "icon" ]="DefaultFolder.png"
        return itemInfo

    def _run_addon( self, type, addon_basename ):
        """
        """
        print "_run_addon"
        print type
        print addon_basename
        result = True
        if ( type == TYPE_ADDON_VIDEO ):
            #command = "XBMC.ActivateWindow(10025,plugin://addons/%s/)" % ( addon_basename, )
            #command = "RunPlugin(%s)" % ( os.path.join( get_install_path( type ), addon_basename ))
            command = "XBMC.ActivateWindow(10025,%s/)" % ( os.path.join( get_install_path( type ), addon_basename) )

        elif ( type == TYPE_ADDON_MUSIC ):
            command = "XBMC.ActivateWindow(10502,plugin://addons/%s/)" % ( addon_basename, )
        elif ( type == TYPE_ADDON_PROGRAMS ):
            command = "XBMC.ActivateWindow(10001,plugin://addons/%s/)" % ( addon_basename, )
        elif ( type == TYPE_ADDON_PICTURES ):
            command = "XBMC.ActivateWindow(10002,plugin://addons/%s/)" % ( addon_basename, )
        elif ( type == TYPE_ADDON_SCRIPT ):
            command = "XBMC.RunScript(%s)" % ( os.path.join( get_install_path( type ), addon_basename, "default.py" ), )

        try:
            xbmc.executebuiltin( command )
        except:
            print_exc()
            result = False
        return result

    def message_cb(self, msgType, title, message1, message2="", message3=""):
        """
        Callback function for sending a message to the UI
        @param msgType: Type of the message
        @param title: Title of the message
        @param message1: Message part 1
        @param message2: Message part 2
        @param message3: Message part 3
        """
        #print("message_cb with %s STARTS"%msgType)
        result = None

        # Display the correct dialogBox according the type
        if msgType == "OK" or msgType == "Error":
            dialogInfo = xbmcgui.Dialog()
            result = dialogInfo.ok(title, message1, message2,message3)
        elif msgType == "YESNO":
            dialogYesNo = xbmcgui.Dialog()
            result = dialogYesNo.yesno(title, message1, message2, message3)
        return result

#class AddonLocalInfo:
#    import md5
#    def _load_downloaded_property( self ):
#        self.downloaded_property = set()
#        try:
#            file_path = os.path.join( SPECIAL_SCRIPT_DATA, "downloaded.txt" )
#            if os.path.exists( file_path ):
#                self.downloaded_property = eval( file( file_path, "r" ).read() )
#        except:
#            print_exc()
#
#    def _save_downloaded_property( self ):
#        try:
#            self._load_downloaded_property()
#            selected_label = self.getListItem( self.getCurrentListPosition() ).getLabel()
#            self.downloaded_property.update( [ md5.new( selected_label ).hexdigest() ] )
#            file_path = os.path.join( SPECIAL_SCRIPT_DATA, "downloaded.txt" )
#            file( file_path, "w" ).write( repr( self.downloaded_property ) )
#        except:
#            print_exc()
#        else:
#            self.getListItem( self.getCurrentListPosition() ).setProperty( "Downloaded", "isDownloaded" )


