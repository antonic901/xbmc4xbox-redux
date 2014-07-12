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

import xbmc
import xbmcgui
from traceback import print_exc

__language__     = sys.modules[ "__main__" ].__language__
__version__      = sys.modules[ "__main__" ].__version__

# Custom modules
try:
    from Item import get_install_path, MISSING_MODULES_PATH, TYPE_ADDON_REPO, TYPE_ADDON_MODULE, TYPE_ADDON_VIDEO, TYPE_ADDON_MUSIC, TYPE_ADDON_PROGRAMS, TYPE_ADDON_PICTURES, TYPE_ADDON_SCRIPT
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
    xbmc.log("getInstalledAddonInfo: Addon path: %s"%addonpath, xbmc.LOGDEBUG)
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


def isLibInstalled(id, type = TYPE_ADDON_MODULE, name = ""):
    """
    Check if a lib/module is already install and return the version of the current installed version
    """
    libVersion = None
    installPath = get_install_path( type )
    if type == TYPE_ADDON_MODULE:
        name = id
    libpath = os.path.join( installPath, name )
    if os.path.exists( libpath ):
        # Get version
        libInfo = getInstalledAddonInfo( os.path.join( libpath ) )
        libVersion = libInfo[ "version" ]

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
        xbmc.log("Addon path: %s"%addonpath, xbmc.LOGDEBUG)
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
        xbmc.log("_run_addon %s", xbmc.LOGDEBUG)
        result = True
        if ( type == TYPE_ADDON_VIDEO ):
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
        result = None

        # Display the correct dialogBox according the type
        if msgType == "OK" or msgType == "Error":
            dialogInfo = xbmcgui.Dialog()
            result = dialogInfo.ok(title, message1, message2,message3)
        elif msgType == "YESNO":
            dialogYesNo = xbmcgui.Dialog()
            result = dialogYesNo.yesno(title, message1, message2, message3)
        return result
