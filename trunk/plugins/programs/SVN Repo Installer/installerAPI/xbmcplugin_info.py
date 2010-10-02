import os, sys
import xbmc, xbmcgui
import urllib
#from pprint import pprint
from xbmcplugin_lib import *
from shutil import rmtree, copytree

__plugin__ = sys.modules["__main__"].__plugin__
__date__ = '19-10-2009'
log("Module: %s Dated: %s loaded!" % (__name__, __date__))

#################################################################################################################
class InfoDialog( xbmcgui.WindowXMLDialog ):
	""" Show skinned Dialog with our information """

	XML_FILENAME = "script-svnri-iteminfo.xml"
	EXIT_CODES = (9, 10, 216, 257, 275, 216, 61506, 61467,)

	def __init__( self, *args, **kwargs):
		log( "%s init!" % self.__class__ )
		self.action = None
		self.buttons = {}

	def onInit( self ):

		xbmcgui.lock()
		try:
			thumb = self.info.get('thumb')
			if thumb:
				thumb = xbmc.translatePath(thumb)
				self.getControl( 31 ).setImage( thumb )

			self.getControl( 4 ).setLabel("[B]"+self.info.get('title','?')+"[/B]")

			text = ""
			# AUTHORS: from description.xml
			try:
				items = self.info.get('authors')
				for item in items:
					text += ", ".join( item )
					text += "  "
			except:
				logError()
			if not text:
				# get from docTags
				text = self.info.get('author','?')
			self.getControl( 6 ).setLabel(text)

			# VERSION and SVN VERSION
			version = self.info.get('version')
			if not version: version = '?'
			svn_ver = self.info.get('svn_ver')
			if not svn_ver: svn_ver = '?'

			if version != svn_ver:
				verText = "v%s -> v%s" % (version,svn_ver)
			else:
				verText = "v%s" % version
			self.getControl( 8 ).setLabel(verText)

			self.getControl( 10 ).setLabel(self.info.get('date','?'))

			# CATEGORY, cut down to just installation dirname
			cat = os.path.dirname(self.info.get('category','?'))
			self.getControl( 12 ).setLabel(cat)

			self.getControl( 19 ).setLabel(self.info.get('compatibility',''))

			# PLATFORM: from description.xml
			try:
				text = ",".join( self.info.get('platforms') )
			except:
				text = '?'
			self.getControl( 14 ).setLabel(text)

			# display Addon info text, from a pref. order of availablility
			textKeys = ('description','summary','changelog','readme')
			for key in textKeys:
				if self.showText(key):
					break

			# set btns enabled state
			btnIDs = {'install': 20,'uninstall': 21,'readme': 22,'changelog': 23 }
			for btnName, isEnabled in self.buttons.items():
				id = btnIDs[btnName]
				self.getControl( id ).setEnabled( isEnabled )

		except:
			xbmcgui.unlock()
			logError("Failed to init skin controls")
		else:
			xbmcgui.unlock()

	def showText(self, key):
		log("showText()")
		text = self.info.get(key)
		if text:
			self.getControl( 30 ).setText( text )
			return True
		else:
			return False

	def onClick( self, controlId ):
		if controlId in (20,21,22,23,24):
			if controlId == 22:
				self.showText('readme')
			elif controlId == 23:
				self.showText('changelog')
			elif controlId == 20:
				self.action = "install"
			elif controlId == 21:
				self.action = "uninstall"
			else:
				self.action = None

			if controlId not in (22,23):
				self.close()

	def onFocus( self, controlId ):
		pass

	def onAction( self, action ):
		try:
			buttonCode =  action.getButtonCode()
			actionID   =  action.getId()
		except: return
		if actionID in self.EXIT_CODES or buttonCode in self.EXIT_CODES:
			self.close()

	def ask(self, info, buttons ):
#		pprint (info)
		if info:
			self.info = info
			self.buttons = buttons
			self.doModal()
		log("ask() action=%s" % self.action)
		return self.action

########################################################################################################################
class Main:

#	INSTALLED_ITEMS_FILENAME = os.path.join( os.getcwd(), "installed_items.dat" )
	INSTALLED_ITEMS_FILENAME = os.path.join( HOME_DIR, "installed_items.dat" )

	def __init__( self, *args, **kwargs):
		log( "%s started!" % self.__class__ )
		try:
			self._parse_argv()
			if self.args.has_key("show_info"):
				info = self._load_item()
				if info:
					self.show_info( info )
		except Exception, e:
			xbmcgui.Dialog().ok(__plugin__ + " ERROR!", str(e))

	########################################################################################################################
	def _parse_argv(self):
		# call Info() with our formatted argv to create the self.args object
		exec "self.args = Info(%s)" % ( unquote_plus( sys.argv[ 2 ][ 1 : ].replace( "&", ", " ) ), )

	########################################################################################################################
	def _load_item( self ):
		info = {}
		items = loadFileObj(self.INSTALLED_ITEMS_FILENAME)
		filepath = self.args.show_info
		log("looking for filepath=%s" % filepath)
		# find addon from installed list
		for i, item in enumerate(items):
			if item.get('filepath','') == filepath:
				info = item
				break
		log("_load_item() info=%s" % info)
		return info

	########################################################################################################################
	def show_info( self, info ):
		log("> show_info() ")
#		pprint (info)
		quit = False
		buttons = {'install': True,'uninstall': True, 'readme': True, 'changelog': True }

		# fetch changelog
		dialog = xbmcgui.DialogProgress()
		dialog.create( __plugin__, xbmc.getLocalizedString( 30001 ),  xbmc.getLocalizedString( 30017 ))
		from xbmcplugin_logviewer import ChangelogParser
		parser = ChangelogParser( info['repo'] , info['title']  )
		info['changelog'] = parser.fetch_changelog()

		# fetch readme
		readme_url = info.get('readme','')
		if readme_url.startswith('http'):
			dialog.update( 50, xbmc.getLocalizedString( 30001 ),  os.path.basename(readme_url))
			info['readme'] = readURL(readme_url)
		dialog.close()
		if not readme_url or not info.get('readme',''):
			buttons['readme'] = False

		# setup compatibility text
		svn_xbmc_rev = info.get('XBMC_Revision',0)
		info['compatibility'] = "XBMC: %s - " % xbmc.getInfoLabel( "System.BuildVersion" )
		if svn_xbmc_rev and svn_xbmc_rev > get_xbmc_revision():
			info['compatibility'] += "[COLOR=FFFF0000]%s[/COLOR] - Requires XBMC: r%s" % (xbmc.getLocalizedString( 30015 ), svn_xbmc_rev) # incomp
		else:
			info['compatibility'] += "[COLOR=FF00FF00]%s[/COLOR]" % (xbmc.getLocalizedString( 30704 ))		# ok, comp

		# set INSTALL button
		if not info.get('download_url',''):
			buttons['install'] = False

		# set UNINSTALL button
		if not os.path.exists(info.get('filepath','')) or "SVN Repo Installer" in info['title']:
			buttons['uninstall'] = False

		while info:
#			action = InfoDialog( InfoDialog.XML_FILENAME, os.getcwd(), "Default" ).ask( info, buttons )
			action = InfoDialog( InfoDialog.XML_FILENAME, HOME_DIR, "Default" ).ask( info, buttons )
			if not action:
				break
			elif action == "install":
				# install
				url_args = info['download_url']
				if "SVN Repo Installer" in info['title']:
					url_args = "self_update=True&" + url_args
				path = '%s?%s' % ( sys.argv[ 0 ], url_args, )
				command = 'XBMC.RunPlugin(%s)' % path
				xbmc.executebuiltin(command)
				break
			elif action == "uninstall":
				path = '%s?delete=%s&title=%s' % ( sys.argv[ 0 ], urllib.quote_plus( repr(info['filepath']) ), urllib.quote_plus( repr(info['title']) ),)
				command = 'XBMC.RunPlugin(%s)' % path
				xbmc.executebuiltin(command)
				break

		log("< show_info()")
