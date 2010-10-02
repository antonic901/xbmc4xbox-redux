"""
 Common functions
"""

import os,sys,re
import xbmc
from urllib import unquote_plus, urlopen
from xml.sax.saxutils import unescape

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '15-10-2009'

#HOME_DIR = os.getcwd()
HOME_DIR = os.path.dirname(os.path.dirname(__file__))		# until XBMC getwd() bug fixed (affects linux)

#################################################################################################################
def log(msg):
	xbmc.log("[%s]: %s" % (__plugin__, msg), xbmc.LOGDEBUG)

log("Module: %s Dated: %s loaded!" % (__name__, __date__))
log("HOME_DIR lib=" + HOME_DIR)

#################################################################################################################
def logError(msg=""):
	log("ERROR: %s (%d) - %s" % (sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], ) )
	if msg:
		log(msg)
	
#################################################################################################################
def handleException(msg=""):
	import traceback
	import xbmcgui
	traceback.print_exc()
	xbmcgui.Dialog().ok(__plugin__ + " ERROR!", msg, str(sys.exc_info()[ 1 ]))

#################################################################################################################
class Info:
	def __init__( self, *args, **kwargs ):
		self.__dict__.update( kwargs )
		log( "Info() dict=%s" % self.__dict__ )
	def has_key(self, key):
		return self.__dict__.has_key(key)

#################################################################################################################
def loadFileObj( filename ):
	log( "loadFileObj() " + filename)
	try:
		file_handle = open( filename, "r" )
		loadObj = eval( file_handle.read() )
		file_handle.close()
	except Exception, e:
		log( "loadFileObj() " + str(e) )
		loadObj = None
	return loadObj

#################################################################################################################
def saveFileObj( filename, saveObj ):
	log( "saveFileObj() " + filename)
	try:
		file_handle = open( filename, "w" )
		file_handle.write( repr( saveObj ) )
		file_handle.close()
		return True
	except Exception, e:
		log( "save_file_obj() " + str(e) )
		return False

#################################################################################################################
def readURL( url ):
	log("readURL() url=" + url)
	if not url:
		return ""
	try:
		sock = urlopen( url )
		doc = sock.read()
		sock.close()
		if ( "404 Not Found" in doc ):
			log("readURL() 404, Not found")
			doc = ""
		return doc
	except:
		log("readURL() %s" % sys.exc_info()[ 1 ])
		return None

#################################################################################################################
def readFile(filename):
	try:
		f = xbmc.translatePath(filename)
		log("readFile() " + f)
		return file(f).read()
	except:
		log("readFile() not found ")
		return ""

#################################################################################################################
def deleteFile( fn ):
	try:
		os.remove( fn )
		log("deleteFile() deleted: " + fn)
	except: pass

#####################################################################################################
def get_repo_info( repo ):
	# path to info file
#	repopath = os.path.join( os.getcwd(), "resources", "repositories", repo, "repo.xml" )
	repopath = os.path.join( HOME_DIR, "resources", "repositories", repo, "repo.xml" )
	try:
		info = open( repopath, "r" ).read()
		# repo's base url
		REPO_URL = re.findall( '<url>([^<]+)</url>', info )[ 0 ]
		# root of repository
		REPO_ROOT = re.findall( '<root>([^<]*)</root>', info )[ 0 ]
		# structure of repo
		REPO_STRUCTURES = re.findall( '<structure name="([^"]+)" noffset="([^"]+)" install="([^"]*)" ioffset="([^"]+)" voffset="([^"]+)"', info )
		log("get_repo_info() REPO_URL=%s REPO_ROOT=%s REPO_STRUCTURES=%s" % (REPO_URL, REPO_ROOT, REPO_STRUCTURES))
		return ( REPO_URL, REPO_ROOT, REPO_STRUCTURES, )
	except:
		logError("get_repo_info()")
		return None

#####################################################################################################
def load_repos( ):
	repo_list = []
#	repos = os.listdir( os.path.join( os.getcwd(), "resources", "repositories" ) )
	repos = os.listdir( os.path.join( HOME_DIR, "resources", "repositories" ) )
	for repo in repos:
		if ("(tagged)" not in repo and repo != ".svn") and (os.path.isdir( os.path.join( HOME_DIR, "resources", "repositories", repo ) ) ):
			repo_list.append( repo )
	log("load_repos() %s" % repo_list)
	return repo_list

#####################################################################################################
def check_readme( base_url ):
	# try to get readme from: language, resources, root
	if base_url[-1] == '/':
		base_url = base_url[:-1]
	urlList = ( "/".join( [base_url, "resources", "language", xbmc.getLanguage(), "readme.txt"] ),
				"/".join( [base_url, "resources", "readme.txt" ] ),
				"/".join( [base_url, "readme.txt" ] ) )

	for url in urlList:
		url = url.replace(' ','%20')
		doc = readURL( url )
		if doc == None:
			break
		elif doc:
			return url
	return ""

#################################################################################################################
def get_xbmc_revision():
    try:
        rev = int(re.search("r([0-9]+)",  xbmc.getInfoLabel( "System.BuildVersion" ), re.IGNORECASE).group(1))
    except:
        rev = 0
    log("get_xbmc_revision() %d" % rev)
    return rev

#####################################################################################################
def makeLabel2( verState ):
	if xbmc.getLocalizedString( 30014 ) in verState:						# New
		label2 = "[COLOR=FF00FFFF]%s[/COLOR]" % verState
	elif xbmc.getLocalizedString( 30015 ) in verState:						# Incompatible
		label2 = "[COLOR=FFFF0000]%s[/COLOR]" % verState
	elif verState == xbmc.getLocalizedString( 30011 ):						# OK
		label2 = "[COLOR=FF00FF00]%s[/COLOR]" % verState
	elif verState == xbmc.getLocalizedString( 30018 ):						# Deleted
		label2 = "[COLOR=66FFFFFF]%s[/COLOR]" % verStat
	else:
		label2 = "[COLOR=FFFFFF00]%s[/COLOR]" % verState
	return label2

#####################################################################################################
def parseCategory(filepath):
	try:
		if filepath[-1] in ['\\','/']:
			filepath = filepath[:-1]
		log("parseCategory() from " + filepath)
		cat = re.search("(plugins.*|scripts.*)$",  filepath, re.IGNORECASE).group(1)
		cat = cat.replace("\\", "/")
	except:
		logError()
		cat = ""
	log("parseCategory() cat=%s" % cat)
	return cat

#####################################################################################################
def joinFiles(fnList):
	""" Join list of filenames """
	try:
		file_path = ""
		for fn in fnList:
			file_path = os.path.join( file_path, fn )
		log("joinFiles() " + file_path)
		return file_path
	except:
		print "ERROR: %s::%s (%d) - %s" % ( __name__, sys.exc_info()[ 2 ].tb_frame.f_code.co_name, sys.exc_info()[ 2 ].tb_lineno, sys.exc_info()[ 1 ], )
		return ""

#####################################################################################################
def joinFile(fn):
	""" Join filename to current dir """
	return joinFiles( [os.path.dirname(__file__), fn] )

#####################################################################################################
def getcwd(filePath=""):
	if not filePath:
		filePath = __file__
	return os.path.dirname(filePath)

#####################################################################################################
def getParentcwd(filePath=""):
	return getcwd(getcwd(filePath))

#################################################################################################################
def searchRegEx(data, regex, flags=re.IGNORECASE):
	try:
		value = re.search(regex, data, flags).group(1)
	except:
		value = ""
	log("searchRegEx() %s = %s" % (regex, value))
	return value

#################################################################################################################
def findAllRegEx(data, regex, flags=re.MULTILINE+re.IGNORECASE+re.DOTALL):
	try:
		matchList = re.compile(regex, flags).findall(data)
	except:
		matchList = []

	log ("findAllRegEx() %s = %s" % (regex,matchList))
	return matchList

#################################################################################################################
def myunicode( text, encoding="utf-8" ):

	try:
		text = myunescape( text )
		unicode(text, encoding, "replace")
	except:
		logError()
	return text

#################################################################################################################
def myunescape( text ):
	entities = {'%21':"!",
				'%22':'"',
				'%25':"%",
				'%26':"&",
				'%27':"'",
				'%28':"(",
				'%29':",",
				'%2a':"*",
				'%2b':"+",
				'%2c':",",
				'%2d':"-",
				'%2e':".",
				'%3a':":",
				'%3b':";",
				'%3f':"?",
				'%40':"@"}
	return unescape(text, entities)

#####################################################################################################
def parseDocTag(doc, tag):
	try:
		match = re.search("__%s__.*?[\"'](.*?)[\"']" % tag,  doc, re.IGNORECASE).group(1)
		match = match.replace( "$", "" ).replace( "Revision", "" ).replace( "Date", "" ).replace( ":", "" ).strip()
	except:
		match = ""
	log("parseDocTag() %s=%s" % (tag, match))
	return match

#####################################################################################################
def parseAllDocTags( doc ):

	if not doc:
		return None

	tagInfo = {}
	# strings
	for tag in ( "author", "version", "date", ):
		tagInfo[tag] = parseDocTag( doc, tag )

	# title
	title = parseDocTag( doc, "plugin" )
	if not title:
		title = parseDocTag( doc, "scriptname" )
	tagInfo['title'] = title

	# ints
	try:
		value = int(parseDocTag( doc, "XBMC_Revision" ))
	except:
		value = 0
	tagInfo["XBMC_Revision"] = value

	# svn_revision
	try:
		value = int(re.search("\$Revision: (\d+)", doc).group(1))
	except:
		value = 0
	tagInfo["svn_revision"] = value

	log("parseAllDocTags() tagInfo=%s" % tagInfo)
	return tagInfo

#################################################################################################################
def parseXMLTag(doc, tag):
	if doc:
		return myunicode(searchRegEx(doc, "<%s>(.*?)</" % tag))
	else:
		return None
	
#################################################################################################################
def parseDescriptionXML(doc):
	""" Parse Description.xml to get info """

	if not doc:
		return None

	info = {}
	# singles lines
	tags = ("guid","type","title","version","summary","description","minrevision","license")
	for tag in tags:
		info[tag] = parseXMLTag(doc, tag)

	# convert type number to a name
	typeNames=("visualization","skin","pvrdll","script","scraper","screensaver","plugin-pvr",
			   "plugin-video","plugin-music","plugin-program","plugin-pictures","plugin-weather")
	try:
		i = int(info["type"])-1
		info["typename"] = typeNames[i]
	except:
		info["typename"] = ""

	# add XBMC_Revision from minversion
	try:
		info["XBMC_Revision"] = int(info["minrevision"])
	except:
		info["XBMC_Revision"] = 0

	# multiple lines, saved as list
	tags = ("tags","platforms")
	for tag in tags:
		try:
			# may be multiple section cos of an example in comments. Use last section found
			section = findAllRegEx(doc, "<%s>(.*?)</%s>" % (tag, tag))[-1]
			if section:
				info[tag] = findAllRegEx(section, "<%s>(.*?)</" % tag[:-1])
		except: pass

	# unique regex reqd.
	tag = "authors"
	info[tag] = findAllRegEx(doc, 'name="(.*?)" email="(.*?)"')

	log("description.xml info=%s" % info)
	return info	

	