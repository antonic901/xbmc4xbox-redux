"""
Module de partage des fonctions et des constantes

"""

#En gros seul les fonctions et variables de __all__ vont etre importees lors du "import *"
#The public names defined by a module are determined by checking the module's namespace
#for a variable named __all__; if defined, it must be a sequence of strings which are names defined
#or imported by that module. The names given in __all__ are all considered public and are required to exist.
#If __all__ is not defined, the set of public names includes all names found in the module's namespace
#which do not begin with an underscore character ("_"). __all__ should contain the entire public API.
#It is intended to avoid accidentally exporting items that are not part of the API (such as library modules
#which were imported and used within the module).
__all__ = [
    # public names
    "copy_dir",
    "copy_inside_dir",
    "SYSTEM_PLATFORM",
    "readURL",
    "add_pretty_color",
    "bold_text",
    "italic_text",
    "set_xbmc_carriage_return",
    "unescape",
    "strip_off",
    "get_infos_path",
    "replaceStrs",
    "set_cache_thumb_name",
    "RecursiveDialogProgress",
    "checkURL",
    "PersistentDataCreator",
    "PersistentDataRetriever",
    "PluginMgr"
    "versionsCmp"
    ]

#Modules general
import os
import re
import sys
import time
import htmllib
import pickle

from traceback import print_exc
#from httplib import HTTP
#from urlparse import urlparse
import urllib2
import socket

# timeout in seconds
timeout = 3


#modules XBMC
import xbmc
import xbmcgui
import xbmcplugin

# Modules Custom
import shutil2


#REPERTOIRE RACINE ( default.py )
CWD = os.getcwd().rstrip( ";" )

#BASE_SETTINGS_PATH = os.path.join( sys.modules[ "__main__" ].SPECIAL_SCRIPT_DATA, "settings.txt" )
#RSS_FEEDS_XML = os.path.join( CWD, "resources", "RssFeeds.xml" )

BASE_THUMBS_PATH = os.path.join( sys.modules[ "__main__" ].SPECIAL_SCRIPT_DATA, "Thumbnails" )
MEDIA_PATH         = sys.modules[ "__main__" ].MEDIA_PATH


def copy_dir( dirname, destination, overwrite=True, progressBar=None, percentage=100 ):
    if not overwrite and os.path.isdir( destination ):
        shutil2.rmtree( destination )
    shutil2.copytree( dirname, destination, overwrite=overwrite, progressBar=progressBar, percentage=percentage )


def copy_inside_dir( dirname, destination, overwrite=True, progressBar=None, percentage=100 ):
    list_dir = os.listdir( dirname )
    for file in list_dir:
        src = os.path.join( dirname, file )
        dst = os.path.join( destination, file )
        if os.path.isfile( src ):
            if not os.path.isdir( os.path.dirname( dst ) ):
                os.makedirs( os.path.dirname( dst ) )
            if not overwrite and os.path.isfile( dst ):
                os.unlink( dst )
            shutil2.copyfile( src, dst, overwrite=overwrite, progressBar=progressBar, percentage=percentage )
        elif os.path.isdir( src ):
            if not overwrite and os.path.isdir( dst ):
                shutil2.rmtree( dst )
            shutil2.copytree( src, dst, overwrite=overwrite, progressBar=progressBar, percentage=percentage )


def readURL( url, save=False, localPath=None ):

    if (not url):
        return ""

    body = ""
    req = urllib2.Request(url)
    try:
        resp = urllib2.urlopen(req)
        body = resp.read()
        if save:
            open(localPath,"w").write(body)
    except urllib2.HTTPError, e:
        print "readURL() url: %s - HTTP Error %s" % ( url, e.code )
    except:
        print "readURL() url: %s - URLError %s" % (url, sys.exc_info()[ 1 ])

    return body

def set_cache_thumb_name( path ):
    try:
        fpath = path
        # make the proper cache filename and path so duplicate caching is unnecessary
        filename = xbmc.getCacheThumbName( fpath )
        thumbnail = os.path.join( BASE_THUMBS_PATH, filename[ 0 ], filename )
        preview_pic = os.path.join( BASE_THUMBS_PATH, "originals", filename[ 0 ], filename )
        # if the cached thumbnail does not exist check for dir does not exist create dir
        if not os.path.isdir( os.path.dirname( preview_pic ) ):
            os.makedirs( os.path.dirname( preview_pic ) )
        if not os.path.isdir( os.path.dirname( thumbnail ) ):
            os.makedirs( os.path.dirname( thumbnail ) )
        return thumbnail, preview_pic
    except:
        print_exc()
        return "", ""

def get_system_platform():
    """ fonction: pour recuperer la platform que xbmc tourne """
    platform = "unknown"
    if xbmc.getCondVisibility( "system.platform.linux" ):
        platform = "linux"
    elif xbmc.getCondVisibility( "system.platform.xbox" ):
        platform = "xbox"
    elif xbmc.getCondVisibility( "system.platform.windows" ):
        platform = "windows"
    elif xbmc.getCondVisibility( "system.platform.osx" ):
        platform = "osx"
    return platform

SYSTEM_PLATFORM = get_system_platform()


#NOTE: CE CODE PEUT ETRE REMPLACER PAR UN CODE MIEUX FAIT
def add_pretty_color( word, start="all", end=None, color=None ):
    """ FONCTION POUR METTRE EN COULEUR UN MOT OU UNE PARTIE """
    try:
        if color and start == "all":
            pretty_word = "[COLOR=" + color + "]" + word + "[/COLOR]"
        else:
            pretty_word = []
            for letter in word:
                if color and letter == start:
                    pretty_word.append( "[COLOR=" + color + "]" )
                elif color and letter == end:
                    pretty_word.append( letter )
                    pretty_word.append( "[/COLOR]" )
                    continue
                pretty_word.append( letter )
            pretty_word = "".join( pretty_word )
        return pretty_word
    except:
        print_exc()
        return word


def bold_text( text ):
    """ FONCTION POUR METTRE UN MOT GRAS """
    return "[B]%s[/B]" % ( text, )


def italic_text( text ):
    """ FONCTION POUR METTRE UN MOT ITALIQUE """
    return "[I]%s[/I]" % ( text, )


def set_xbmc_carriage_return( text ):
    """ only for xbmc """
    text = text.replace( "\r\n", "[CR]" )
    text = text.replace( "\n\n", "[CR]" )
    text = text.replace( "\n",   "[CR]" )
    text = text.replace( "\r\r", "[CR]" )
    text = text.replace( "\r",   "[CR]" )
    text = text.replace( "</br>",   "[CR]" )
    return text


def strip_off( text, by="", xbmc_labels_formatting=False ):
    """ FONCTION POUR RECUPERER UN TEXTE D'UN TAG """
    if xbmc_labels_formatting:
        #text = re.sub( "\[url[^>]*?\]|\[/url\]", by, text )
        text = text.replace( "[", "<" ).replace( "]", ">" )
    return re.sub( "(?s)<[^>]*>", by, text )

def unescape(s):
    """
    remplace les sequences d'echappement par leurs caracteres equivalent
    """
    p = htmllib.HTMLParser(None)
    p.save_bgn()
    p.feed(s)
    return p.save_end()

def replaceStrs( s, *args ):
    """
    Replace all ``(frm, to)`` tuples in `args` in string `s`.
    By Alexander Schmolck ( http://markmail.org/message/r67z77skcqcbo5nr )
    replaceStrs("nothing is better than warm beer", ... ('nothing','warm beer'), ('warm beer','nothing')) 'warm beer is better than nothing'
    """
    if args == ():
        return s
    mapping = dict([(frm, to) for frm, to in args])
    return re.sub("|".join(map(re.escape, mapping.keys())), lambda match:mapping[match.group(0)], s)



def get_infos_path( path, get_size=False, report_progress=None ):
    # Return the system's ctime which, on some systems (like Unix) is the time of the last change, and, on others (like Windows), is the creation time for path. The return value is a number giving the number of seconds since the epoch (see the time module). Raise os.error if the file does not exist or is inaccessible. New in version 2.3.
    try: c_time = time.strftime( DATE_TIME_FORMAT, time.localtime( os.path.getctime( path ) ) )#.replace( " | 0", " |  " )
    except: c_time = ""

    # Return the time of last access of path. The return value is a number giving the number of seconds since the epoch (see the time module). Raise os.error if the file does not exist or is inaccessible. New in version 1.5.2. Changed in version 2.3: If os.stat_float_times() returns True, the result is a floating point number.
    try: last_access = time.strftime( DATE_TIME_FORMAT, time.localtime( os.path.getatime( path ) ) )#.replace( " | 0", " |  " )
    except: last_access = ""

    # Return the time of last modification of path. The return value is a number giving the number of seconds since the epoch (see the time module). Raise os.error if the file does not exist or is inaccessible. New in version 1.5.2. Changed in version 2.3: If os.stat_float_times() returns True, the result is a floating point number.
    try: last_modification = time.strftime( DATE_TIME_FORMAT, time.localtime( os.path.getmtime( path ) ) )#.replace( " | 0", " |  " )
    except: last_modification = ""

    # Return the size, in bytes, of path. Raise os.error if the file does not exist or is inaccessible. New in version 1.5.2.
    # calculate dir size "os.walk( path, topdown=False )"
    try:
        size = 0
        if os.access( path, os.R_OK ):
            if os.path.isfile( path ):
                try:
                    size += os.path.getsize( path )
                    if report_progress:
                        #print "Size: %s", path
                        report_progress.update( -1, sys.modules[ "__main__" ].__language__( 30186 ), path, sys.modules[ "__main__" ].__language__( 361 ) + " %00s KB" % round( size / 1024.0, 2 ) )
                except: pass
            elif get_size:
                for root, dirs, files in os.walk( path ):#, topdown=False ):
                    for file in files:
                        try:
                            fpath = os.path.join( root, file )
                            if os.access( fpath, os.R_OK ):
                                size += os.path.getsize( fpath )
                                if report_progress:
                                    #print "Size: %s", fpath
                                    report_progress.update( -1, sys.modules[ "__main__" ].__language__( 30186 ), fpath, sys.modules[ "__main__" ].__language__( 361 ) + " %00s KB" % round( size / 1024.0, 2 ) )
                        except:
                            print "Size: %s" % fpath
        if size <= 0:
            size = ""#"0.0 KB"
        elif size <= ( 1024.0 * 1024.0 ):
            size = "%00s KB" % round( size / 1024.0, 2 )
        elif size >= ( 1024.0 * 1024.0 ):
            size = "%00s MB" % round( size / 1024.0 / 1024.0, 2 )
        else:
            size = "%00s Bytes" % size
    except:
        print_exc()
        size = "0.0 KB"

    return size, c_time, last_access, last_modification


class RecursiveDialogProgress:
    """
    Wrapper class for displayong progress
    """
    _xbmcdp = None
    _heading = None
    _line1 = None
    _line2 = None
    _line3 = None

    def __init__( self, heading, line1=None, line2=None, line3=None ):
        self._xbmcdp = xbmcgui.DialogProgress()
        self._heading = heading
        self._line1 = line1
        self._line2 = line2
        self._line3 = line3

        if self._line1:
            if self._line2:
                if self._line3:
                    self._xbmcdp.create(self._heading, self._line1, self._line2, self._line3)
                else:
                    self._xbmcdp.create(self._heading, self._line1, self._line2)
            else:
                self._xbmcdp.create(self._heading, self._line1)
        else:
            self._xbmcdp.create(self._heading)


    def update( self, percent, itemname, line2=None, line3=None ):
        """
        Met a jour la barre de progression
        """
        #TODO Dans le futur, veut t'on donner la responsabilite a cette fonction le calcul du pourcentage????
        try:
            #xbmcdp.update( percent )
            self._xbmcdp.update( percent, self._line1 % ( itemname ))
            #xbmcdp.update( percent, _( 30138 ) % ( filename ), _( 30134 ) )
        except:
            percent = 100
            self._xbmcdp.update( percent )

    def iscanceled(self):
        return self._xbmcdp.iscanceled()

    def close(self):
        self._xbmcdp.close()


def checkURL(url):
    """
    Check is a URL exists
    """
    print "Checking URL: %s"%url
    try:
        socket.setdefaulttimeout(timeout)
        f = urllib2.urlopen(urllib2.Request(url))
        ok = True
    except:
        ok = False
    print ok
    return ok

    #===========================================================================
    # p = urlparse(url)
    # h = HTTP(p[1])
    # h.putrequest('HEAD', p[2])
    # h.endheaders()
    # if h.getreply()[0] == 200:
    #    return 1
    # else:
    #    return 0
    #===========================================================================

def fileOlderThan(file, offset):
    ret = False
    if os.path.exists(file):
        ctime = os.path.getctime(file)
        if ctime < time.time() - offset:
            ret = True
    else:
        ret = True
    return ret

def versionsCmp( version1, version2 ):
    """
    returning:
     -1 if version1 is older than version2
      0 if version1 is equal to version2
      1 if version1 is newer than version2,
    Credit to jellybean (http://stackoverflow.com/questions/1714027/version-number-comparison)
    """
    if version1 == None:
        version1 = "0"
    if version2 == None:
        version2 = "0"
    parts1 = [int(x) for x in version1.split('.')]
    parts2 = [int(x) for x in version2.split('.')]

    # fill up the shorter version with zeros ...
    lendiff = len(parts1) - len(parts2)
    if lendiff > 0:
        parts2.extend([0] * lendiff)
    elif lendiff < 0:
        parts1.extend([0] * (-lendiff))

    for i, p in enumerate(parts1):
        ret = cmp(p, parts2[i])
        if ret: return ret
    return 0


class PersistentDataCreator:
    """
    Creates persistent data
    """
    def __init__( self, data, filepath ):
        self._persit_data( data, filepath )

    def _persit_data( self, data, filepath ):
        f = open( filepath, 'w' )
        pickle.dump(data, f)
        f.close()

class PersistentDataRetriever:
    """
    Retrieves persistent data
    """
    import pickle
    def __init__( self, filepath ):
        self.filepath = filepath

    def get_data( self ):
        data = None
        if os.path.isfile( self.filepath ):
            f = open( self.filepath, 'r')
            try:
                data = pickle.load(f)
            except:
                pass
            f.close()
        return data
