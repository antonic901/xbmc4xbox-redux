"""
ItemInstaller: this module allows download and install an item from Passion XBMC download center
"""

# Modules general
import os
import sys
from traceback import print_exc

# Modules XBMC
import xbmc

# Modules custom
try:
    from ItemInstaller import ArchItemInstaller#, cancelRequest
    #from globalvars import *
    from Item import TYPE_SYSTEM_ARCHIVE
except:
    print_exc()


class LocalArchiveInstaller(ArchItemInstaller):
    """
    Download an item on Passion XBMC http server and install it
    """

    #def __init__( self , name, type ):
    def __init__( self , path ):
        ArchItemInstaller.__init__( self )

        self.itemInfo [ "raw_item_path" ] = path

        #TODO: support progress bar display

    def GetRawItem( self, msgFunc=None,progressBar=None ):
        """
        Get an item (local or remote)
        Set self.rawItemSysType - the filetype of the item: ARCHIVE | DIRECTORY
        Set self.rawItemPath - the path of the item: path of an archive or a directory
        Returns the status of the retrieval attempt : OK | ERROR
        """
        status      = "ERROR" # Status of download :[OK | ERROR | CANCELED | ERRORFILENAME]

        xbmc.log("LocalArchiveInstaller::GetRawItem - Item to install path: %s"%self.itemInfo [ "raw_item_path" ], xbmc.LOGDEBUG)

        if self.itemInfo [ "raw_item_path" ].endswith( 'zip' ) or self.itemInfo [ "raw_item_path" ].endswith( 'rar' ):
            status      = "OK"
            self.itemInfo [ "name" ]   = os.path.basename( self.itemInfo [ "raw_item_path" ] )
            self.itemInfo [ "raw_item_sys_type" ] = TYPE_SYSTEM_ARCHIVE
        #return status, self.archivePath
        return status





