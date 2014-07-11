"""
ItemInstaller: this module allows download and install of an item (addons: script, plugin, scraper, skin ...)
"""

# Modules general
import os
import sys
from traceback import print_exc
from time import sleep

# Modules custom
try:
    from Item import *
    from FileManager import fileMgr
    from XmlParser import parseAddonXml
except:
    print_exc()

# XBMC modules
import xbmc
import xbmcgui

#FONCTION POUR RECUPERER LES LABELS DE LA LANGUE.
_ = sys.modules[ "__main__" ].__language__

class cancelRequest(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)


class ItemInstaller:
    """
    ABSTRACT
    """
    # dictionary to hold addon info
    itemInfo = {}
        # id
        # name
        # type
        # version
        # author
        # disclaimer
        # summary
        # description
        # icon
        # fanart
        # changelog
        # library: path of python script
        # raw_item_sys_type: file | archive | dir
        # raw_item_path
        # install_path
        # temp_item_path
        # provides
        # required_lib

    def __init__( self ):
        from globalvars import DIR_CACHE_ADDONS
        self.CACHEDIR      = DIR_CACHE_ADDONS
        self.fileMgr       = fileMgr()
        self.status        = "INIT" # Status of install :[ INIT | OK | ERROR | DOWNLOADED | EXTRACTED | ALREADYINSTALLED | ALREADYINUSE | CANCELED | INSTALL_DONE ]

        # Clean cache directory
        self.fileMgr.delDirContent(self.CACHEDIR)

    def GetRawItem( self, msgFunc=None,progressBar=None ):
    #def downloadItem( self, msgFunc=None,progressBar=None ):
        """
        Get an item (local or remote)
        Set rawItemSysType - the filetype of the item: ARCHIVE | DIRECTORY
        Set rawItemPath - the path of the item: path of an archive or a directory
        Returns the status of the retrieval attempt : OK | ERROR
        TO IMPLEMENT in a child class
        """
        xbmc.log("ItemInstaller::GetRawItem not implemented", xbmc.LOGNOTICE)
        raise

    def isAlreadyInstalled( self ):
        """
        Check if item is already installed
        Needs to be called after extractItem (destinationPath has to be determined first)
        ==> self.itemInfo [ "install_path" ] need to be set (in a subclass) before calling this method
        """
        result = False
        #if hasattr( self.itemInfo, "install_path" ) and os.path.exists( self.itemInfo [ "install_path" ] ):
        if os.path.exists( self.itemInfo [ "install_path" ] ):
            result = True

        return result

    def isInUse( self ):
        """
        Check if item is currently in use
        Needs to be called after extractItem (destinationPath has to be determined first)
        ==> installName need to be set (in a subclass) before calling this method
        """
        result = False
        return result


    def installItem( self, itemName=None, msgFunc=None,progressBar=None ):
        """
        Install item (download + extract + copy)
        Needs to be called after extractItem
        TO IMPLEMENT in a child class
        """
        xbmc.log("ItemInstaller::installItem not implemented", xbmc.LOGNOTICE)
        raise


    def getItemInstallName( self ):
        """
        Return the real name (not the path) of the item
        """
        return self.itemInfo[ "name" ]

    def getItemId( self ):
        """
        Return the Id the item
        """
        return self.itemInfo[ "id" ]

    def getItemVersion( self ):
        """
        Return the version of the item
        """
        return self.itemInfo[ "version" ]

    def getItemType( self ):
        """
        Return the type of the item
        """
        return self.itemInfo[ "type" ]



    def _getItemPaths( self ):
        """
        Returns the list of path of the current item (dir or file path)
        NOTE 1: A list is necessary in case of scrapers for instance
        NOTE 2: self.itemInfo [ "install_path" ] need to be set (in a subclass) before calling this method
        """

        #TODO: return path or name in both scenario

        paths = []
        paths.append( self.itemInfo[ "install_path" ] )
        return paths

    def deleteInstalledItem( self ):
        """
        Delete an item already installed (file or directory)
        Return True if success, False otherwise
        NOTE: self.itemInfo [ "install_path" ] need to be set (in a subclass) before calling this method
        """
        result = None
        item_paths = self._getItemPaths ()

        if len( item_paths ) > None:
            for path in item_paths:
                result = self.fileMgr.deleteItem( path )
                if result == False:
                    xbmc.log("deleteInstalledItem: Impossible to delete one of the element in the item: %s" %path, xbmc.LOGNOTICE)
                    break
        else:
            result = False
            xbmc.log("deleteInstalledItem: Item invalid - error", xbmc.LOGNOTICE)
        return result

    def renameInstalledItem( self, inputText ):
        """
        Rename an item already installed (file or directory)
        Return True if success, False otherwise
        NOTE: self.itemInfo [ "install_path" ] need to be set (in a subclass) before calling this method
        """

        #TODO; iuse same fomat between delete and rename: path or name


        result = None
        item_paths = self._getItemPaths ()
        xbmc.log("renameInstalledItem", xbmc.LOGDEBUG)

        if len( item_paths ) > None:
            for path in item_paths:
                xbmc.log(u"Renaming %s by %s"%(path, path.replace( os.path.basename( path ), inputText)), xbmc.LOGDEBUG)
                result = self.fileMgr.renameItem( None, path, path.replace( os.path.basename( path ), inputText) )
                if result == False:
                    xbmc.log("renameInstalledItem: Impossible to rename one of the element in the item: %s" % path, xbmc.LOGDEBUG)
                    break
        else:
            result = False
            xbmc.log("renameInstalledItem: Item invalid - error" % path, xbmc.LOGNOTICEs)
        return result

    def copyItem( self, msgFunc=None,progressBar=None ):
        """
        Install item from extracted archive
        Needs to be called after extractItem
        """
        #TODO: update a progress bar during copy
        import extractor
        OK = False

        xbmc.log("copyItem", xbmc.LOGDEBUG)
        # get install path
        process_error = False
        percent = 0
        if progressBar != None:
            progressBar.update( percent, _( 30176 ), self.itemInfo [ "temp_item_path" ] )
        if ( ( self.itemInfo [ "temp_item_path" ] != None ) and ( self.itemInfo [ "install_path" ] != None ) ):
            # Let's get the dir name in the archive
            try:
                #if ( OK == bool( self.itemInfo [ "temp_item_path" ] ) ) and os.path.exists( self.itemInfo [ "temp_item_path" ] ):
                if os.path.exists( self.itemInfo [ "temp_item_path" ] ):
                    self.fileMgr.copyDir( self.itemInfo [ "temp_item_path" ], self.itemInfo [ "install_path" ], progressBar=progressBar )
                    OK = True
                else:
                    xbmc.log("ItemInstaller::installItem - self.itemInfo [ 'temp_item_path' ] = %s does not exist"%self.itemInfo [ 'temp_item_path' ], xbmc.LOGNOTICE)
            except Exception, e:
                xbmc.log("ItemInstaller::installItem - Exception during copy of the directory %s" % self.itemInfo [ "temp_item_path" ], xbmc.LOGNOTICE)
                print_exc()
                process_error = True

        del extractor
        percent = 100
        if progressBar != None:
            progressBar.update( percent, _( 30176 ), ( self.itemInfo [ "temp_item_path" ] ) )
        return OK

    def _renameItem4xbox( self, item, oldpath, newpath, itemName=None ):
        """
        Rename addon directory
        """
        status = "OK"

        # Rename directory
        if not self.fileMgr.renameItem( None, oldpath, newpath ):
            xbmc.log("Impossible to rename the addon directory based on the name in addon.xml, need name from user", xbmc.LOGNOTICE)
            # Rename
            status = "INVALIDNAME"
        return status

    def _prepareItem4xbox( self, item, msgFunc=None,progressBar=None ):
        """
        Prepare an addon in order to be runnable on XBMC4XBOX
        (python script, icon renaming, folder renaming ...)
        """
        status = "OK"
        if  item[ "type" ] not in [TYPE_ADDON_MODULE, TYPE_ADDON_REPO]:
            xbmc.log("_prepareItem4xbox - Renaming addon elements", xbmc.LOGDEBUG)
            # Rename python script
            oldScriptPath = os.path.join( item[ "temp_item_path" ], item[ "library" ] )
            newScriptPath = os.path.join( item[ "temp_item_path" ], "default.py" )
            if oldScriptPath != newScriptPath and os.path.exists( oldScriptPath ):
                result = self.fileMgr.renameItem( None, oldScriptPath, newScriptPath )
                if result == False:
                    status = "ERROR"
                    xbmc.log("Error renaming %s to %s"%(oldScriptPath, newScriptPath), xbmc.LOGNOTICE)
                else:
                    item[ "library" ] = "default.py"
                    xbmc.log("%s renamed to %s"%(oldScriptPath, newScriptPath), xbmc.LOGDEBUG)

            # Rename logo
            oldLogoPath = os.path.join( item[ "temp_item_path" ], "icon.png" )
            newLogoPath = os.path.join( item[ "temp_item_path" ], "default.tbn" )
            if ( oldLogoPath != newLogoPath and os.path.exists( oldLogoPath ) ):
                result = self.fileMgr.renameItem( None, oldLogoPath, newLogoPath, True )
                if result == False:
                    status = "ERROR"
                    xbmc.log("Error renaming %s to %s"%(oldLogoPath, newLogoPath), xbmc.LOGNOTICE)
        else:
            status = "UNCHANGED"

        return status

    def setItemInfo( self, itemName=None ):
        """
        Get Type, name

         id
         name
         type
         version
         author
         disclaimer
         summary
         description
         icon
         fanart
         changelog
         library: path of python script
         raw_item_sys_type: file | archive | dir
         raw_item_path
         install_path
         temp_item_path
         provides
         required_lib
        """


        status = 'OK'
        if self.status != "INVALIDNAME":
            itemExtractedPath = self.itemInfo [ "temp_item_path" ]

            try:
                # Retrieve info from addon.xml
                xmlInfofPath = os.path.join( itemExtractedPath, "addon.xml")
                if ( os.path.exists( xmlInfofPath ) ):
                    xmlData = open( os.path.join( xmlInfofPath ), "r" )
                    statusGetInfo = parseAddonXml( xmlData, self.itemInfo )
                    xmlData.close()
                    sleep(3)
                else:
                    xbmc.log("setItemInfo - addon.xml not found", xbmc.LOGNOTICE)
                    status = 'ERROR'

                # Renaming addon's internal files
                if statusGetInfo == "OK":
                    status = self._prepareItem4xbox( self.itemInfo )
                elif statusGetInfo == "NOT_SUPPORTED":
                    xbmc.log("setItemInfo - Addon not supported", xbmc.LOGDEBUG)
                    status = 'NOT_SUPPORTED'
                else:
                    xbmc.log("setItemInfo - Error parsing addon.xml", xbmc.LOGNOTICE)
                    status = 'ERROR'

            except:
                print_exc()
                status = 'ERROR'

        if status in ["OK", "UNCHANGED"]:
            typeInstallPath = get_install_path( self.itemInfo [ "type" ] )
            status = 'OK'

            # Rename directory
            if  self.itemInfo[ "type" ] not in [TYPE_ADDON_MODULE, TYPE_ADDON_REPO]:
                if itemName:
                    newItemPath = self.itemInfo[ "temp_item_path" ].replace(os.path.basename( self.itemInfo[ "temp_item_path" ] ) , itemName )
                else:
                    newItemPath = self.itemInfo[ "temp_item_path" ].replace(os.path.basename( self.itemInfo[ "temp_item_path" ] ) , self.itemInfo[ "name" ] )

                status = self._renameItem4xbox(self.itemInfo, self.itemInfo[ "temp_item_path" ], newItemPath)
                if status == "OK":
                    if itemName:
                        # Overwriting name with the one given by the user
                        self.itemInfo[ "name" ] = itemName
                    self.itemInfo[ "temp_item_path" ] = newItemPath
                    self.itemInfo[ "install_path" ]   = os.path.join( typeInstallPath, self.itemInfo [ "name" ] )
                else:
                    # Rename failed
                    xbmc.log("Rename failed", xbmc.LOGNOTICE)
                    self.itemInfo [ "install_path" ] = os.path.join( typeInstallPath, os.path.basename( self.itemInfo [ "temp_item_path" ] ) )
            else:
                # We don't rename modules and repos
                #self.itemInfo [ "install_path" ] = os.path.join( typeInstallPath, os.path.basename( self.itemInfo [ "id" ] ) )
                self.itemInfo [ "install_path" ] = os.path.join( typeInstallPath, os.path.basename( self.itemInfo [ "id" ][:42] ) ) # xbox filename limitation
        xbmc.log("setItemInfo - status: %s"%status, xbmc.LOGDEBUG)
        return status



class ArchItemInstaller(ItemInstaller):
    """
    Installer from an archive
    """

    def __init__( self ):
        ItemInstaller.__init__( self )
        self.itemInfo [ "install_path" ] = None

        #TODO: support progress bar display


    def extractItem( self, msgFunc=None,progressBar=None ):
        """
        Extract item in temp location
        Update:
        temp_item_path
        install_path
        name
        """
        #TODO: update a progress bar during extraction
        status  = "OK" # Status of download :[OK | ERROR | CANCELED]
        percent = 33
        # Check if the archive exists
        xbmc.log("extractItem", xbmc.LOGDEBUG)
        if ( os.path.exists( self.itemInfo[ "raw_item_path" ] ) and TYPE_SYSTEM_ARCHIVE == self.itemInfo[ "raw_item_sys_type" ] ):
            if progressBar != None:
                progressBar.update( percent, "Extraction:", ( self.itemInfo [ "name" ] ) )
                import extractor
                process_error = False
                # Extraction in cache directory (if OK copy later on to the correct location)
                file_path, OK = extractor.extract( self.itemInfo [ "raw_item_path" ], destination=self.CACHEDIR, report=True )

                xbmc.log("extractItem - file_path: %s"%file_path, xbmc.LOGDEBUG)
                if file_path == "":
                    installError = _( 30139 ) % os.path.basename( self.itemInfo[ "raw_item_path" ] )
                    xbmc.log("ArchItemInstaller - extractItem: Error during the extraction of %s - impossible to extract the name of the directory " % os.path.basename( self.itemInfo [ "raw_item_path" ] ), xbmc.LOGNOTICE)
                    status = "ERROR"
                else:
                    # Extraction successful
                    self.itemInfo[ "temp_item_path" ] = file_path
                del extractor

            percent = 100
            if progressBar != None:
                progressBar.update( percent, _( 30182 ), self.itemInfo [ "name" ] )
        else:
            xbmc.log("extractItem - Archive does not exist - extraction impossible", xbmc.LOGNOTICE)
            status = "ERROR"
        return status


    def installItem( self, itemName=None, msgFunc=None,progressBar=None ):
        """
        Install item (download + extract + copy)
        Needs to be called after extractItem
        """
        xbmc.log("installItem: Download and install case", xbmc.LOGDEBUG)
        percent = 0
        result  = "OK" # result after install :[ OK | ERROR | ALREADYINSTALLED |CANCELED]


        xbmc.log("installItem: Get Item", xbmc.LOGDEBUG)
        if ( self.status == "INIT" ) or ( self.status == "ERROR" ):
            #TODO: support message callback in addition of pb callback
            #statusDownload = self.GetRawItem( msgFunc=msgFunc, progressBar=progressBar )
            #statusDownload, self.itemInfo [ "raw_item_path" ] = self.GetRawItem( progressBar=progressBar )
            statusDownload = self.GetRawItem( progressBar=progressBar )
            if statusDownload in ["OK", "ERRORFILENAME"]:
                if self.extractItem( msgFunc=msgFunc, progressBar=progressBar ) == "OK":
                    # Download and extract successful
                    self.status = self.setItemInfo()
                    result = self.status
                    if self.status == "OK":
                        if not self.isAlreadyInstalled():
                            if not self.isInUse():
                                xbmc.log("installItem - Item is not yet installed - installing", xbmc.LOGDEBUG)
                                # TODO: in case of skin check skin is not the one currently used
                                if self.copyItem( msgFunc=msgFunc, progressBar=progressBar ) == False:
                                    result = "ERROR"
                                    self.status = "ERROR"
                                    xbmc.log("installItem - Error during copy", xbmc.LOGNOTICE)
                                else:
                                    self.status = "INSTALL_DONE"
                            else:
                                xbmc.log("installItem - Item is already currently used by XBMC - stopping install", xbmc.LOGNOTICE)
                                result = "ALREADYINUSE"
                                self.status = "EXTRACTED"
                        else:
                            xbmc.log("installItem - Item is already installed - stopping install", xbmc.LOGNOTICE)
                            result = "ALREADYINSTALLED"
                            self.status = "EXTRACTED"
                else:
                    xbmc.log("installItem - unknown error during extraction", xbmc.LOGNOTICE)
                    result = "ERROR"
                    self.status = "ERROR"

#            elif statusDownload == "ERRORFILENAME":
#                pass
            elif statusDownload == "CANCELED":
                result      = "CANCELED"
                self.status = "CANCELED"
                xbmc.log("installItem - Install cancelled by the user", xbmc.LOGDEBUG)
            else:
                result      = "ERROR"
                self.status = "ERROR"
                xbmc.log("installItem - Error during download", xbmc.LOGNOTICE)
        elif self.status == "EXTRACTED":
            xbmc.log("installItem - continue install", xbmc.LOGDEBUG)
            # TODO: in case of skin check skin is not the one currently used
            if self.copyItem( msgFunc=msgFunc, progressBar=progressBar ) == False:
                result      = "ERROR"
                self.status = "ERROR"
                xbmc.log("installItem - Error during copy", xbmc.LOGNOTICE)
            else:
                self.status = "INSTALL_DONE"
        elif self.status == "INVALIDNAME":
            if itemName:
                self.status = self.setItemInfo(itemName=itemName)
                if self.status == "OK":
                    if not self.isAlreadyInstalled():
                        if not self.isInUse():
                            xbmc.log("installItem - Item is not yet installed - installing", xbmc.LOGDEBUG)
                            # TODO: in case of skin check skin is not the one currently used
                            if self.copyItem( msgFunc=msgFunc, progressBar=progressBar ) == False:
                                result = "ERROR"
                                self.status = "ERROR"
                                xbmc.log("installItem - Error during copy", xbmc.LOGNOTICE)
                            else:
                                self.status = "INSTALL_DONE"
                        else:
                            xbmc.log("installItem - Item is already currently used by XBMC - stopping install", xbmc.LOGNOTICE)
                            result = "ALREADYINUSE"
                            self.status = "EXTRACTED"
                    else:
                        xbmc.log("installItem - Item is already installed - stopping install", xbmc.LOGNOTICE)
                        result = "ALREADYINSTALLED"
                        self.status = "EXTRACTED"
            else:
                xbmc.log("installItem - Item name missing after and INVALIDNAME state", xbmc.LOGDEBUG)
                self.status == "ERROR"
                result = "ERROR"


        return result, self.itemInfo [ "install_path" ]



class DirItemInstaller(ItemInstaller):
    """
    Installer from a directory
    """

    #def __init__( self , itemId, type, filesize ):
    def __init__( self ):
        #ItemInstaller.__init__( self, itemId, type, filesize )
        #ItemInstaller.__init__( self, name, type )
        ItemInstaller.__init__( self )
        self.itemInfo [ "install_path" ] = None

        #TODO: support progress bar display
        #self.rawItemPath = None # Path of the archive to extract
        #self.destinationPath     = None # Path of the destination directory
        #self.downloadDirPath     = None # Path of the extracted item
        #self.status              = "INIT" # Status of install :[INIT | OK | ERROR | ALREADYINSTALLED |CANCELED]


    def installItem( self, itemName=None, msgFunc=None,progressBar=None ):
        """
        Install item (download + extract + copy)
        Needs to be called after extractItem
        """
        xbmc.log("installItem: Download and install %s"%itemName, xbmc.LOGDEBUG)
        percent = 0
        result  = "OK" # result after install :[ OK | ERROR | ALREADYINSTALLED |CANCELED]
        if ( self.status == "INIT" ) or ( self.status == "ERROR" ):
            #TODO: support message callback in addition of pb callback
            statusGetFile = self.GetRawItem( progressBar=progressBar )
            self.itemInfo [ "temp_item_path" ] = self.itemInfo [ "raw_item_path" ] #Since it is an directory those 2 path are identical
            if statusGetFile == "OK":
                if os.path.exists( self.itemInfo [ "temp_item_path" ] ):
                    # Download successful
                    self.status = self.setItemInfo()
                    result = self.status
                    if self.status == "OK":
                        if not self.isAlreadyInstalled():
                            if not self.isInUse():
                                xbmc.log("installItem - Item is not yet installed - installing", xbmc.LOGDEBUG)
                                # TODO: in case of skin check skin is not the one currently used
                                if self.copyItem( msgFunc=msgFunc, progressBar=progressBar ) == False:
                                    result = "ERROR"
                                    xbmc.log("installItem - Error during copy", xbmc.LOGNOTICE)
                                else:
                                    self.status = "INSTALL_DONE"
                            else:
                                xbmc.log("installItem - Item is already currently used by XBMC - stopping install", xbmc.LOGNOTICE)
                                result = "ALREADYINUSE"
                                self.status = "EXTRACTED"
                        else:
                            xbmc.log("installItem - Item is already installed - stopping install", xbmc.LOGNOTICE)
                            result = "ALREADYINSTALLED"
                            self.status = "DOWNLOADED"
            elif statusGetFile == "CANCELED":
                result      = "CANCELED"
                self.status = "CANCELED"
                xbmc.log("installItem - Install cancelled by the user", xbmc.LOGDEBUG)
            else:
                result      = "ERROR"
                self.status = "ERROR"
                xbmc.log("installItem - Error during download", xbmc.LOGNOTICE)
        elif self.status == "DOWNLOADED":
            xbmc.log("installItem - continue install", xbmc.LOGDEBUG)
            # TODO: in case of skin check skin is not the one currently used
            if self.copyItem( msgFunc=msgFunc, progressBar=progressBar ) == False:
                result      = "ERROR"
                self.status = "ERROR"
                xbmc.log("installItem - Error during copy", xbmc.LOGNOTICE)
            else:
                self.status = "INSTALL_DONE"

        elif self.status == "INVALIDNAME":
            if itemName:
                self.status = self.setItemInfo(itemName=itemName)
                if self.status == "OK":
                    if not self.isAlreadyInstalled():
                        if not self.isInUse():
                            xbmc.log("installItem - Item is not yet installed - installing", xbmc.LOGDEBUG)
                            # TODO: in case of skin check skin is not the one currently used
                            if self.copyItem( msgFunc=msgFunc, progressBar=progressBar ) == False:
                                result = "ERROR"
                                self.status = "ERROR"
                                xbmc.log("installItem - Error during copy", xbmc.LOGNOTICE)
                            else:
                                self.status = "INSTALL_DONE"
                        else:
                            xbmc.log("installItem - Item is already currently used by XBMC - stopping install", xbmc.LOGNOTICE)
                            result = "ALREADYINUSE"
                            self.status = "EXTRACTED"
                    else:
                        xbmc.log("installItem - Item is already installed - stopping install", xbmc.LOGNOTICE)
                        result = "ALREADYINSTALLED"
                        self.status = "EXTRACTED"
            else:
                xbmc.log("installItem - Item name missing after and INVALIDNAME state", xbmc.LOGNOTICE)
                self.status == "ERROR"
                result = "ERROR"

        return result, self.itemInfo [ "install_path" ]


