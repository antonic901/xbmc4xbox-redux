"""
   Installing XBMC addons on XBMC4XBOX using xbmcaddon Nuka1195 library
   Special thanks to Frost for his library, Nuka1195 for his library too and Maxoo for the logo
   Please read changelog
"""

__all__ = [
    # public names
    "InstallMgr"
    ]


import os
import sys
#import urllib
import xbmc
import xbmcgui
#import xbmcplugin
from traceback import print_exc

__language__     = sys.modules[ "__main__" ].__language__

# URLs
REPO_LIST_URL = "http://wiki.xbmc.org/index.php?title=Unofficial_Add-on_Repositories"

# Custom modules
try:
    from globalvars import SPECIAL_HOME_DIR, REPO_ID_XBMC4XBOX, REPO_ID_HELIX, REPO_ID_XBMC, DIR_CACHE, DIR_ADDON_REPO
    from FileManager import fileMgr
    #from Item import *
    from Item import TYPE_ADDON_MODULE, get_install_path
    import LocalArchiveInstaller
    import RemoteArchiveInstaller
    from utilities import RecursiveDialogProgress, versionsCmp, readURL, fileOlderThan
    from AddonsMgr import getInstalledAddonInfo, isLibInstalled, addMissingModules2DB, saveLocalAddonInfo
    from XmlParser import ListItemFromXML
except:
    print_exc()



class InstallMgr:
    """
    Manage install of Addons
    """

    def __init__( self, *args, **kwargs ):
        pass


    def install_from_repo( self, addonName, addonUrl, addonFormat, repoUrl ):
        """
        Install an addon from a remote/web repository
        """
        status = "CANCELED"
        destination = None
        addonInstaller = None

        if addonFormat == "zip":
            # install from zip file
            addonInstaller = RemoteArchiveInstaller.RemoteArchiveInstaller( addonName, addonUrl )
        else:
            # Remote dir installer
            addonInstaller = RemoteArchiveInstaller.RemoteDirInstaller( addonName, addonUrl, repoUrl )

        dp = RecursiveDialogProgress(__language__( 30137 ), __language__( 30138 ))
        status, destination = addonInstaller.installItem( msgFunc=self.message_cb, progressBar=dp )
        dp.close()
        del dp
        return status, addonName, destination, addonInstaller

    def install_from_zip(self):
        """
        Install an addon from a local zip file
        """
        status = "OK"
        destination = None
        addonInstaller = None

        dialog = xbmcgui.Dialog()
        zipPath = dialog.browse(1, 'XBMC', 'files', '', False, False, SPECIAL_HOME_DIR)
        itemName = os.path.basename(zipPath)
        xbmc.log("_install_from_zip - installing %s"%zipPath, xbmc.LOGDEBUG)

        # install from zip file
        addonInstaller = LocalArchiveInstaller.LocalArchiveInstaller( zipPath )
        #dp = xbmcgui.DialogProgress()
        #dp.create(__language__( 30137 ))
        dp = RecursiveDialogProgress(__language__( 30137 ), __language__( 30138 ))
        status, destination = addonInstaller.installItem( msgFunc=self.message_cb, progressBar=dp )

        dp.close()
        del dp
        return status, itemName, destination, addonInstaller


    def _getAddonRequiredLibs ( self, addonIdList, repoId = None):
        """
        Display the addons to install for a repository
        """
        status = "OK"

        # List of repositories we will look in order to find the required modules
        repoList = [ getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, REPO_ID_XBMC4XBOX) ) ]
        if repoId:
            repoList.extend( [ getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, repoId) ) ] )
        repoList.extend( [ getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, REPO_ID_HELIX) ) ] )
        repoList.extend( [ getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, REPO_ID_XBMC) ) ] )

        # Check if required lib already exist - we do an additional check later for non script modules once we have the module name from the addons repo
        addonIdCheck = []
        addonIdCheck.extend(addonIdList)
        for requiredlib in addonIdCheck:
            localLibVersion = isLibInstalled( requiredlib['id'] )
            if localLibVersion:
                xbmc.log("Requested %s version %s - found version %s" % (requiredlib['id'], requiredlib["version"], localLibVersion), xbmc.LOGDEBUG)
                if versionsCmp( localLibVersion, requiredlib["version"] ) >= 0:
                    addonIdList.remove(requiredlib)

                else:
                    installPath = get_install_path( TYPE_ADDON_MODULE )
                    fileMgr().deleteDir( os.path.join( installPath, requiredlib['id'] ) )

        if len(addonIdList) == 0:
            xbmc.log("No required libs", xbmc.LOGDEBUG)
            return "OK"

        # Parse each repository in the list and try to find in it the required module
        if len(addonIdList) > 0:
            allLibsFound = False
            for repoInfo in repoList:
                # Retrieving addons.xml from remote repository
                xmlInfofPath = os.path.join( DIR_CACHE, repoInfo [ "id" ] + ".xml")
                if fileOlderThan(xmlInfofPath, 60 * 30):
                    data = readURL( repoInfo [ "repo_url" ], save=True, localPath=xmlInfofPath )

                if ( os.path.exists( xmlInfofPath ) ):
                    try:
                        xmlData = open( os.path.join( xmlInfofPath ), "r" )
                        listAddonsXml = ListItemFromXML(xmlData)
                        xmlData.close()
                    except:
                        print_exc()

                    keepParsingCurrentRepo = True
                    while (keepParsingCurrentRepo):
                        item = listAddonsXml.getNextItem()
                        if item:
                            if len(addonIdList) > 0:
                                for lib in addonIdList:
                                    if lib["id"] == item['id']:
                                        localLibVersion = isLibInstalled( item['id'], item['type'], item['name'] )
                                        if localLibVersion:
                                            xbmc.log("Requested %s version %s - found version %s" % (requiredlib['id'], requiredlib["version"], localLibVersion), xbmc.LOGDEBUG)
                                            if versionsCmp( localLibVersion, lib["version"] ) >= 0:
                                                addonIdList.remove(lib)
                                                continue
                                            else:
                                                name = item['name']
                                                if item['type'] == TYPE_ADDON_MODULE:
                                                    name = item['id']
                                                installPath = get_install_path( item['type'] )
                                                fileMgr().deleteDir( os.path.join( installPath, name ) )

                                        endRepoChar = "/"
                                        if repoInfo [ "repo_datadir" ].endswith( "/" ):
                                            endRepoChar = ""

                                        if repoInfo [ "repo_format" ] ==  'zip':
                                            downloadUrl = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + item["id"]  + '-' + item["version"] + ".zip").replace(' ', '%20')
                                            changelog   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "changelog" + '-' + item["version"] + ".txt").replace(' ', '%20')
                                            iconimage   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "icon.png").replace(' ', '%20')
                                        else:
                                            downloadUrl = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/').replace(' ', '%20')
                                            changelog   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "changelog" + ".txt").replace(' ', '%20')
                                            iconimage   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "icon.png").replace(' ', '%20')
                                        item["ImageUrl"] = iconimage
                                        item["changelog"] = changelog

                                        xbmc.log("Download URL: %s" % (downloadUrl), xbmc.LOGDEBUG)

                                        # Install lib
                                        installMgr = InstallMgr()
                                        status, itemName, destination, addonInstaller = installMgr.install_from_repo( item['name'].encode('utf8'), downloadUrl, repoInfo[ "repo_format" ], repoInfo[ "repo_datadir" ] )
                                        if status == "OK":
                                            status, destination = addonInstaller.installItem()
                                            if status == "OK":
                                                saveLocalAddonInfo(repoInfo[ "id" ], destination, addonInstaller)
                                            # recursively install further dependencies
                                            requiredLibs = addonInstaller.itemInfo[ "required_lib" ]
                                            if len(requiredLibs) > 0:
                                                status = self._getAddonRequiredLibs ( addonInstaller.itemInfo[ "required_lib" ], repoInfo[ "id" ] )
                                                if status != "OK":
                                                    return status
                                        else:
                                            # Notify user it is impossible to install the current kib and check if he want to continue
                                            if not xbmcgui.Dialog().yesno( item['name'].encode('utf8'), __language__( 30070 ), __language__( 30071 ), __language__( 30072 ) ):
                                                keepParsingCurrentRepo = False
                                                allLibsFound = True
                                                status = "CANCELED"
                                                xbmc.log("User cancelled due to error of a lib install", xbmc.LOGDEBUG)
                                            else:
                                                # User wants to continue
                                                status = "OK"
                                        # Module installed or already installed - Remove it for the list of libs to install
                                        addonIdList.remove(lib)
                            else:
                                # No lib to find remaining
                                keepParsingCurrentRepo = False
                                allLibsFound = True
                        else:
                            keepParsingCurrentRepo = False
                if not allLibsFound:
                    # all libs found, no need to go to parse next repo
                    continue
            if len(addonIdList) > 0:
                xbmc.log("Not all required lib has been installed", xbmc.LOGDEBUG)

                if not xbmcgui.Dialog().yesno( lib['id'], __language__( 30070 ), __language__( 30071 ), __language__( 30072 ) ):
                    xbmc.log("User cancelled due to error of a lib install", xbmc.LOGDEBUG)
                    allLibsFound = True
                    status = "CANCELED"
                else:
                    # User wants to continue

                    # Update list of module which fail to install
                    #self.addMissingModules(addonIdList)
                    addMissingModules2DB(addonIdList)

                    status = "OK"

        return status#, itemName, destination, addonInstaller


    def check_install(self, status, itemName, destination, itemInstaller):
        """
        Check if install went well and if not ask the user what to do
        """
        # Default message: error
        title = __language__( 30144 )
        msg1  = __language__( 30144 )
        msg2  = ""
        loop = True
        # Get Item install name
        if itemInstaller:
            itemInstallName = itemInstaller.getItemInstallName()

        try:
            while loop:
                if status == "OK":
                    title = __language__( 30141 )
                    msg1  = __language__( 30142 )%itemName # should we manage only unicode instead of string?
                    msg2  = __language__( 30143 )
                    loop = False

                elif status == "CANCELED":
                    title = __language__( 30146 )
                    msg1  = __language__( 30147 )%itemName
                    msg2  = ""
                    loop = False

                elif status == "ALREADYINSTALLED":
                    title = __language__( 30144 )
                    msg1  = __language__( 30149 )%itemName
                    msg2  = ""
                    if self._processOldInstall( itemInstaller ):
                        # Continue install
                        dp = xbmcgui.DialogProgress()
                        dp.create(__language__( 30137 ))
                        status, destination = itemInstaller.installItem( msgFunc=self.message_cb, progressBar=dp )
                        dp.close()
                        del dp
                        title = __language__( 30141 )
                        msg1  = __language__( 30142 )%itemName # should we manage only unicode instead of string?
                        msg2  = __language__( 30143 )
                        if status == "OK":
                            loop = False
                    else:
                        xbmc.log("bypass: %s install has been cancelled by the user" % itemName, xbmc.LOGDEBUG)
                        title = __language__( 30146 )
                        msg1  = __language__( 30147 )%itemName
                        msg2  = ""
                        loop = False


                elif status == "INVALIDNAME":
                    keyboard = xbmc.Keyboard( itemInstallName, __language__( 30154 ) )
                    keyboard.doModal()
                    if ( keyboard.isConfirmed() ):
                        inputText = keyboard.getText()
                        dp = xbmcgui.DialogProgress()
                        dp.create(__language__( 30137 ))
                        status, destination = itemInstaller.installItem( itemName=inputText, msgFunc=self.message_cb, progressBar=dp )
                        dp.close()
                        if status == "OK":
                            title = __language__( 30141 )
                            msg1  = __language__( 30142 )%itemName # should we manage only unicode instead of string?
                            msg2  = __language__( 30143 )
                            loop = False
                    del keyboard

                elif status == "ALREADYINUSE":
                    xbmc.log("%s currently used by XBMC, install impossible" % itemName, xbmc.LOGDEBUG)
                    title = __language__( 30117 )
                    msg1  = __language__( 30117 )
                    msg2  = __language__( 30119 )
                    loop = False

                elif status == "NOT_SUPPORTED":
                    xbmc.log("%s install impossible, type of addon not supported" % itemName, xbmc.LOGDEBUG)
                    title = __language__( 30144 )
                    msg1  = __language__( 30160 )
                    msg2  = __language__( 30136 )%itemName
                    loop = False

                else:
                    title = __language__( 30144 )
                    msg1  = __language__( 30136 )%itemName
                    msg2  = ""
                    loop = False

                xbmcgui.Dialog().ok( title, msg1, msg2 )

            del itemInstaller

        except:
            print_exc()

        return status, destination


    def _processOldInstall( self, itemInstaller ):
        """
        Traite les ancien download suivant les desirs de l'utilisateur
        retourne True si le download peut continuer.
        """
        continueInstall = True

        # Get Item install name
        if itemInstaller:
            itemInstallName = itemInstaller.getItemInstallName()

            exit = False
            while exit == False:
                menuList = [ __language__( 30150 ), __language__( 30151 ), __language__( 30152 ), __language__( 30153 ) ]
                dialog = xbmcgui.Dialog()
                chosenIndex = dialog.select( __language__( 30149 ) % itemInstallName, menuList )
                if chosenIndex == 0:
                    # Delete
                    xbmc.log("Deleting:", xbmc.LOGDEBUG)
                    OK = itemInstaller.deleteInstalledItem()
                    if OK == True:
                        exit = True
                    else:
                        xbmcgui.Dialog().ok( __language__(148), __language__( 117) )
                elif chosenIndex == 1:
                    # Rename
                    xbmc.log("Renaming:", xbmc.LOGDEBUG)
                    keyboard = xbmc.Keyboard( itemInstallName, __language__( 30154 ) )
                    keyboard.doModal()
                    if ( keyboard.isConfirmed() ):
                        inputText = keyboard.getText()
                        OK = itemInstaller.renameInstalledItem( inputText )
                        if OK == True:
                            xbmcgui.Dialog().ok( __language__( 30155 ), inputText  )
                            exit = True
                        else:
                            xbmcgui.Dialog().ok( __language__( 30148 ), __language__( 30117 ) )

                    del keyboard
                    #dp.close()
                elif chosenIndex == 2:
                    # Overwrite
                    xbmc.log("Overwriting:", xbmc.LOGDEBUG)
                    exit = True
                else:
                    # EXIT
                    exit = True
                    continueInstall = False
        else:
            continueInstall = False

        return continueInstall


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
