
__all__ = [
    # public names
    #"copy_func",
    "ListItemObject",
    "fileMgr"
    ]

# Modules general
import os
import sys
from traceback import print_exc

# Modules XBMC
import xbmc

# Modules custom
import shutil2
#from utilities import *


# INITIALISATION CHEMIN RACINE
#ROOTDIR = os.getcwd().replace( ";", "" )

#FONCTION POUR RECUPERER LES LABELS DE LA LANGUE.
_ = sys.modules[ "__main__" ].__language__
ROOTDIR = sys.modules[ "__main__" ].ROOTDIR


############################################################################
# Get actioncodes from keymap.xml
############################################################################
#ACTION_MOVE_LEFT = 1
#ACTION_MOVE_RIGHT = 2
#ACTION_MOVE_UP = 3
#ACTION_MOVE_DOWN = 4
#ACTION_PAGE_UP = 5
#ACTION_PAGE_DOWN = 6
#ACTION_SELECT_ITEM = 7
#ACTION_HIGHLIGHT_ITEM = 8
ACTION_PARENT_DIR = 9
ACTION_PREVIOUS_MENU = 10
ACTION_SHOW_INFO = 11
#ACTION_PAUSE = 12
#ACTION_STOP = 13
#ACTION_NEXT_ITEM = 14
#ACTION_PREV_ITEM = 15
ACTION_CONTEXT_MENU = 117 # ACTION_MOUSE_RIGHT_CLICK *sa marche maintenant avec les derniere SVN*
CLOSE_CONTEXT_MENU = ( ACTION_PARENT_DIR, ACTION_PREVIOUS_MENU, ACTION_CONTEXT_MENU, )




##################################################


#def copy_func( cpt_blk, taille_blk, total_taille, dialogCB=None ):
#    try:
#        updt_val = int( ( cpt_blk * taille_blk ) / 10.0 / total_taille )
#        if updt_val > 100: updt_val = 100
#        if dialogCB != None:
#            dialogCB.update( updt_val )
#    except:
#        pass
#        #dialogCB.update( 100 )
#    # DON'T ALLOW Progress().iscanceled() BUG CREATE, FIXED SOON
#    #if xbmcgui.DialogProgress().iscanceled():
#    #    xbmcgui.DialogProgress().close()

class ListItemObject:
    """
    Structure de donnee definissant un element de la liste
    """
    def __init__( self, type='unknown', name='', local_path=None, thumb='default' ):
        self.type       = type
        self.name       = name
        self.local_path = local_path
        self.thumb      = thumb

    def __repr__(self):
        return "(%s, %s, %s, %s)" % ( self.type, self.name, self.local_path, self.thumb )


class fileMgr:
    """
    File manager
    """
    def verifrep(self, r_folder):
        """
        Check a folder exists and make it if necessary
        Return True if success, False otherwise
        """
        result = True
        folder = xbmc.makeLegalFilename( r_folder )
        try:
            #print("verifrep check if directory: " + folder + " exists")
            if not os.path.exists(folder):
                print "verifrep: Impossible to find the directory - Trying to create directory: %s" % folder
                os.makedirs(folder)
        except Exception, e:
            result = False
            print "verifrep: Exception while creating the directory: %s" % folder
            print_exc()
        return result

    def listDirFiles(self, r_path):
        """
        List the files of a directory
        @param path: path of directory we want to list the content of
        """
        path = xbmc.makeLegalFilename( r_path )
        print "listDirFiles: Liste le repertoire: %s" % path
        dirList = os.listdir( str( path ) )

        return dirList

    def renameItem( self, r_base_path, r_old_name, r_new_name, r_force = False ):
        """
        Rename an item (file or directory)
        Return True if success, False otherwise
        """
        result = True
        try:
            if r_base_path == None:
                old_name = r_old_name
                new_name = r_new_name
            else:
                old_name = os.path.join( r_base_path, r_old_name )
                new_name = os.path.join( r_base_path, r_new_name )
            # if doing a force rename, then remove the destination if it exists
            if ( r_force and os.path.exists( new_name ) ):
                self.deleteItem( new_name )
            os.rename( old_name, new_name )
        except OSError, err:
            result = False
            print "renameItem - Couldn't rename"
            print err.errno
            print_exc()
        except:
            result = False
            print "renameItem: Exception renaming Item"
            print_exc()
        return result

    def deleteItem( self, r_item_path):
        """
        Delete an item (file or directory)
        Return True if success, False otherwise
        """
        result = None
        item_path = xbmc.makeLegalFilename( r_item_path )
        if os.path.isdir(item_path):
            result = self.deleteDir(item_path)
        else:
            result = self.deleteFile(item_path)

        return result

    def deleteFile(self, r_file_path):
        """
        Delete a file from download directory
        @param filename: name of the file to delete
        Return True if success, False otherwise
        """
        result = True
        file_path = xbmc.makeLegalFilename( r_file_path )
        try:
            if os.path.exists( file_path ):
                os.remove( file_path )
            else:
                print "deleteFile: File %s does NOT exist" % file_path
                result = False
        except:
            result = False
            print "deleteFile: Exception deleting file: %s" % r_file_path
            print_exc()
        return result

    def deleteDir( self, r_path ):
        """
        Delete a directory and all its content (files and subdirs)
        Note: the directory does NOT need to be empty
        Return True if success, False otherwise
        """
        result = True
        path = xbmc.makeLegalFilename( r_path )
        if os.path.isdir( path ):
            dirItems=os.listdir( path )
            for item in dirItems:
                itemFullPath=os.path.join( path, item )
                try:
                    if os.path.isfile( itemFullPath ):
                        # Fichier
                        os.remove( itemFullPath )
                    elif os.path.isdir( itemFullPath ):
                        # Repertoire
                        self.deleteDir( itemFullPath )
                except:
                    result = False
                    print "deleteDir: Exception deleting directory: %s" % path
                    print_exc()
            # Suppression du repertoire pere
            try:
                os.rmdir( path )
            except:
                result = False
                print "deleteDir: Exception deleting directory: %s" % path
                print_exc()
        else:
            print "deleteDir: %s is not a directory" % path
            result = False

        return result

    def delDirContent( self, r_path ):
        """
        Delete the content of a directory ( file and sub direstories)
        but not the directory itself
        path: directory path
        """
        #print "delDirContent"
        #print path
        result = True
        path = xbmc.makeLegalFilename( r_path )
        if os.path.isdir( path ):
            dirItems=os.listdir( path )
            for item in dirItems:
                itemFullPath=os.path.join( path, item )
                try:
                    if os.path.isfile( itemFullPath ):
                        # Fichier
                        os.remove( itemFullPath )
                    elif os.path.isdir( itemFullPath ):
                        # Repertoire
                        self.deleteDir( itemFullPath )
                except:
                    result = False
                    print "delDirContent: Exception la suppression du contenu du reperoire: %s" % path
                    print_exc()
        else:
            print "delDirContent: %s n'est pas un repertoire" % path
            result = False

        return result

    def extract(self,archive,targetDir):
        """
        Extract an archive in targetDir
        """
        xbmc.executebuiltin('XBMC.Extract(%s,%s)'%(archive,targetDir) )

    def copyDir( self, r_dir_src, r_dir_dest, overwrite=True, progressBar=None ):
        """
        Copy a directory to a new location
        """
        #dir_src  = xbmc.makeLegalFilename( r_dir_src )
        #dir_dest = xbmc.makeLegalFilename( r_dir_dest )
        dir_src  = r_dir_src
        dir_dest = r_dir_dest
        if not overwrite and os.path.isdir( dir_dest ):
            shutil2.rmtree( dir_dest )
        shutil2.copytree( dir_src, dir_dest, overwrite=overwrite, progressBar=progressBar, curPercent=100 )


    def copyInsideDir( self, r_dir_src, r_dir_dest, overwrite=True, progressBar=None ):
        """
        Copy the content a directory to a new location
        """
        dir_src  = xbmc.makeLegalFilename( r_dir_src )
        dir_dest = xbmc.makeLegalFilename( r_dir_dest )
        list_dir = os.listdir( dir_src )
        for file in list_dir:
            src = os.path.join( dir_src, file )
            dst = os.path.join( dir_dest, file )
            if os.path.isfile( src ):
                if not os.path.isdir( os.path.dirname( dst ) ):
                    os.makedirs( os.path.dirname( dst ) )
                if not overwrite and os.path.isfile( dst ):
                    os.unlink( dst )
                shutil2.copyfile( src, dst, overwrite=overwrite, progressBar=progressBar, curPercent=100 )
            elif os.path.isdir( src ):
                if not overwrite and os.path.isdir( dst ):
                    shutil2.rmtree( dst )
                shutil2.copytree( src, dst, overwrite=overwrite, progressBar=progressBar, curPercent=100 )


