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


# URLs
REPO_LIST_URL = "http://wiki.xbmc.org/index.php?title=Unofficial_Add-on_Repositories"

__platform__ = "xbmc media center, [%s]" % xbmc.__platform__
__language__ = xbmc.Language( ROOTDIR ).getLocalizedString


# Custom modules
try:
    from globalvars import SPECIAL_HOME_DIR, OFFICIAL_REPO_ID, DIR_CACHE, DIR_ADDON_REPO
    #from FileManager import fileMgr
    #from Item import *
    from Item import TYPE_ADDON_MODULE, get_install_path
    import LocalArchiveInstaller
    import RemoteArchiveInstaller
    from utilities import RecursiveDialogProgress, versionsCmp, readURL
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

        dp = RecursiveDialogProgress(__language__( 137 ), __language__( 138 ))
        status, destination = addonInstaller.installItem( msgFunc=self.message_cb, progressBar=dp )
        dp.close()
        del dp
        return status, addonName, destination, addonInstaller

    def install_from_zip(self):
        """
        Install an addon from a local zip file
        """
        status = "CANCELED"
        destination = None
        addonInstaller = None

        dialog = xbmcgui.Dialog()
        zipPath = dialog.browse(1, 'XBMC', 'files', '', False, False, SPECIAL_HOME_DIR)
        itemName = os.path.basename(zipPath)
        print("_install_from_zip - installing %s"%zipPath)

        # install from zip file
        addonInstaller = LocalArchiveInstaller.LocalArchiveInstaller( zipPath )
        #dp = xbmcgui.DialogProgress()
        #dp.create(__language__( 137 ))
        dp = RecursiveDialogProgress(__language__( 137 ), __language__( 138 ))
        status, destination = addonInstaller.installItem( msgFunc=self.message_cb, progressBar=dp )

        dp.close()
        del dp
        return status, itemName, destination, addonInstaller


    def _getAddonRequiredLibs ( self, addonIdList, repoId ):
        """
        Display the addons to install for a repository
        """
        print "_getAddonRequiredLibs"
        print addonIdList
        status = "CANCELED"
        #destination = None
        #addonInstaller = None
        #addonInstaller = None

        # For time being we will only look in the offcial repo

        # List of repositories within we will look in order to find the required modules
        if repoId:
            repoList = [ getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, repoId) ),
                         getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, OFFICIAL_REPO_ID) ) ]
        else:
            # if repoId is unknow (i.e install form zip) we will parse only official repo
            repoList = [ getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, OFFICIAL_REPO_ID) )]



        # Check if required lib already exist
        addonIdCheck = [] # Create a copy of addonIdList (We remove an element of the list while looping on it)
        addonIdCheck.extend(addonIdList)
        for requiredlib in addonIdCheck:
            print addonIdCheck
            print "Checking %s if required module with version is installed: %s"%(requiredlib["id"], requiredlib["version"])

            # Check if remote version is the same as requiredlib['version']
            #doInstall= False
            #localLibVersion = self.isLibInstalled( requiredlib["id"] )
            localLibVersion = isLibInstalled( requiredlib["id"] )

            try:
                if localLibVersion:
                    #Check version
                    if localLibVersion == requiredlib["version"]:
                        print "Already install"

                        # Module already installed - Remove it for the list of libs to install
                        addonIdList.remove(requiredlib)
                    else:
                        print "version of lib does not match: %s (local) Vs %s (required)"%( localLibVersion, requiredlib["version"])
                        if versionsCmp( localLibVersion, requiredlib["version"] ) < 0:
                            print "localLibVersion older than lib[\"version\"]"
                            #TODO: delete and brute force install
                            installPath = get_install_path( TYPE_ADDON_MODULE )
                            libpath = os.path.join( installPath, os.path.basename( requiredlib['id'] ) )
                            result = self.fileMgr.deleteItem( libpath )
                            if result == False:
                                print "_getAddonRequiredLibs: Impossible to delete one of the element in the item: %s - we won't try to install it" %libpath
                                addonIdList.remove(requiredlib)
                        else:
                            print "localLibVersion more recent than lib[\"version\"]"
                            addonIdList.remove(requiredlib)
                print addonIdList
                print addonIdCheck
            except:
                print_exc()


        # Parse each repository in the list and try to find in it the required module
        if len(addonIdList) > 0:
            allLibsFound = False
            for repoInfo in repoList:
                # Retrieve info from  addons.xml for the repositories where we will look for the lib addons
                xmlInfofPath = os.path.join( DIR_CACHE, "addons.xml")

                # Delete old version of addons.xml
                if os.path.isfile(xmlInfofPath):
                    os.remove(xmlInfofPath)
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
                                        print "%s module required with version: %s found"%(lib["id"], lib["version"])
                                        # We are going to try to install it even if version does not match (better than nothing)

                                        # 1 - Look for matching repo
                                        # 2 - install
                                        # 3 - notify user if something when wrong or not

    #                                    # Check if remote version is the same as lib['version']
    #                                    doInstall= False
    #                                    localLibVersion = self.isLibInstalled( lib["id"] )
    #                                    if localLibVersion:
    #                                        status = "OK" # For now ...
    #
    #                                        #Check version
    #                                        if localLibVersion == item["version"]:
    #                                            print "Already install"
    #                                        else:
    #                                            print "version of lib does not match: %s Vs %s"%( localLibVersion, lib["version"])
    #                                            if versionsCmp( localLibVersion, item["version"] ) < 0:
    #                                                print "localLibVersion older than lib[\"version\"]"
    #                                                #TODO: delete and brute force install
    #                                                installPath = get_install_path( TYPE_ADDON_MODULE )
    #                                                libpath = os.path.join( installPath, os.path.basename( id ) )
    #                                                result = self.fileMgr.deleteItem( libpath )
    #                                                if result == False:
    #                                                    print "_getAddonRequiredLibs: Impossible to delete one of the element in the item: %s" %libpath
    #                                                else:
    #                                                    doInstall = True
    #                                    else:
    #                                        # Not already installed
    #                                        doInstall = True

                                        doInstall = True
                                        if doInstall:
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

                                            print downloadUrl

                                            # Install lib
                                            installMgr = InstallMgr()
                                            status, itemName, destination, addonInstaller = installMgr.install_from_repo( item['name'].encode('utf8'), downloadUrl, repoInfo[ "repo_format" ], repoInfo[ "repo_datadir" ] )
                                            # force install/overwrite
                                            if status != "OK":
                                                status, destination = addonInstaller.installItem()
                                            if status == "OK":
                                                saveLocalAddonInfo(repoInfo[ "id" ], destination, addonInstaller)
                                            else:
                                                #missingModulesFile = open(MISSING_MODULES_PATH, "a")
                                                #missingModulesFile.write("%s|%s/n"%(lib["id"], lib["version"]))
                                                #missingModulesFile.close()
                                                # Notify user it is impossible to install the current kib and check if he want to continue
                                                if not xbmcgui.Dialog().yesno( item['name'].encode('utf8'), __language__( 30070 ), __language__( 30071 ), __language__( 30072 ) ):
                                                    keepParsingCurrentRepo = False
                                                    allLibsFound = True
                                                    status = "CANCELED"
                                                    print "User cancelled due to error of a lib install"
                                                else:
                                                    # User wants to continue
                                                    status = "OK"
                                            print status
                                            # Module installed or already installed - Remove it for the list of libs to install
                                            addonIdList.remove(lib)
                                            print addonIdList
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
                print "Not all required lib has been installed"
                print addonIdList

                if not xbmcgui.Dialog().yesno( lib['id'], __language__( 30070 ), __language__( 30071 ), __language__( 30072 ) ):
                    print "User cancelled due to error of a lib install"
                    allLibsFound = True
                    status = "CANCELED"
                else:
                    # User wants to continue

                    # Update list of module which fail to install
                    #self.addMissingModules(addonIdList)
                    addMissingModules2DB(addonIdList)

                    status = "OK"
        else:
            print "No required libs"
            status = "OK"

        return status#, itemName, destination, addonInstaller


    def check_install(self, status, itemName, destination, itemInstaller):
        """
        Check if install went well and if not ask the user what to do
        """
        # Default message: error
        title = __language__( 144 )
        msg1  = __language__( 144 )
        msg2  = ""
        loop = True
        # Get Item install name
        if itemInstaller:
            itemInstallName = itemInstaller.getItemInstallName()

        try:
            while loop:
                if status == "OK":
                    #self._save_downloaded_property()
                    title = __language__( 141 )
                    #msg1  = _( 142 )%(unicode(itemName,'cp1252')) # should we manage only unicode instead of string?
                    msg1  = __language__( 142 )%itemName # should we manage only unicode instead of string?
                    #msg1  = _( 142 )%"" + itemName
                    msg2  = __language__( 143 )
                    loop = False

                elif status == "CANCELED":
                    title = __language__( 146 )
                    #msg1  = _( 147 )%(unicode(itemName,'cp1252'))
                    msg1  = __language__( 147 )%itemName
                    msg2  = ""
                    loop = False

                elif status == "ALREADYINSTALLED":
                    title = __language__( 144 )
                    #msg1  = _( 149 )%(unicode(itemName,'cp1252'))
                    msg1  = __language__( 149 )%itemName
                    msg2  = ""
                    if self._processOldInstall( itemInstaller ):
                        # Continue install
                        dp = xbmcgui.DialogProgress()
                        dp.create(__language__( 137 ))
                        status, destination = itemInstaller.installItem( msgFunc=self.message_cb, progressBar=dp )
                        dp.close()
                        del dp
                        #self._save_downloaded_property()
                        title = __language__( 141 )
                        msg1  = __language__( 142 )%itemName # should we manage only unicode instead of string?
                        #msg1  = __language__( 142 )%"" + itemName
                        msg2  = __language__( 143 )
                        if status == "OK":
                            loop = False
                    else:
                        #installCancelled = True
                        print "bypass: %s install has been cancelled by the user" % itemName
                        title = __language__( 146 )
                        msg1  = __language__( 147 )%itemName
                        msg2  = ""
                        loop = False
                        #dp = xbmcgui.DialogProgress()
                        #dp.create(__language__( 153 ))
                        #dp.update(100)
                        #dp.close()

                elif status == "INVALIDNAME":
                    keyboard = xbmc.Keyboard( itemInstallName, __language__( 154 ) )
                    keyboard.doModal()
                    if ( keyboard.isConfirmed() ):
                        inputText = keyboard.getText()
                        dp = xbmcgui.DialogProgress()
                        dp.create(__language__( 137 ))
                        status, destination = itemInstaller.installItem( itemName=inputText, msgFunc=self.message_cb, progressBar=dp )
                        dp.close()
                        if status == "OK":
                            title = __language__( 141 )
                            msg1  = __language__( 142 )%itemName # should we manage only unicode instead of string?
                            msg2  = __language__( 143 )
                            loop = False
                    del keyboard

                elif status == "ALREADYINUSE":
                    print "%s currently used by XBMC, install impossible" % itemName
                    title = __language__( 117 )
                    msg1  = __language__( 117 )
                    msg2  = __language__( 119 )
                    loop = False

                elif status == "NOT_SUPPORTED":
                    print "%s install impossible, type of addon not supported" % itemName
                    title = __language__( 144 )
                    msg1  = __language__( 160)
                    msg2  = __language__( 136 )%itemName
                    loop = False

                else:
                    title = __language__( 144 )
                    msg1  = __language__( 136 )%itemName
                    msg2  = ""
                    loop = False
        #                else:
        #                    title = "Unknown Error"
        #                    msg1  = "Please check the logs"
        #                    msg2  = ""
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
                menuList = [ __language__( 150 ), __language__( 151 ), __language__( 152 ), __language__( 153 ) ]
                dialog = xbmcgui.Dialog()
                chosenIndex = dialog.select( __language__( 149 ) % itemInstallName, menuList )
                if chosenIndex == 0:
                    # Delete
                    print "Deleting:"
                    OK = itemInstaller.deleteInstalledItem()
                    if OK == True:
                        exit = True
                    else:
                        xbmcgui.Dialog().ok( __language__(148), __language__( 117) )
                elif chosenIndex == 1:
                    # Rename
                    print "Renaming:"
                    #dp = xbmcgui.DialogProgress()
                    #dp.create(__language__( 157 ))
                    #dp.update(50)
                    keyboard = xbmc.Keyboard( itemInstallName, __language__( 154 ) )
                    keyboard.doModal()
                    if ( keyboard.isConfirmed() ):
                        inputText = keyboard.getText()
                        OK = itemInstaller.renameInstalledItem( inputText )
                        if OK == True:
                            xbmcgui.Dialog().ok( __language__( 155 ), inputText  )
                            exit = True
                        else:
                            xbmcgui.Dialog().ok( __language__( 148 ), __language__( 117 ) )

                    del keyboard
                    #dp.close()
                elif chosenIndex == 2:
                    # Overwrite
                    print "Overwriting:"
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

    #===========================================================================
    # def updateProgress_cb( self, percent, dp=None ):
    #    """
    #    Met a jour la barre de progression
    #    """
    #    #TODO Dans le futur, veut t'on donner la responsabilite a cette fonction le calcul du pourcentage????
    #    try:
    #        dp.update( percent )
    #    except:
    #        percent = 100
    #        dp.update( percent )
    #===========================================================================
