import xbmc
import xbmcgui
from urllib import quote_plus, unquote_plus
import re
import sys
import os

_searchString = ''

class GUI( xbmcgui.WindowXML ):
	def onInit(self):
		print '|==================================|'
		print 'onInit(): Initialized!' 
		
		self.SEARCH_STRING = _searchString
		print 'SEARCH_STRING: %s' % self.SEARCH_STRING
		
		self.dbSearchMovies()
		#self.dbSearchTvShows()
		self.dbSearchTvEpisodes()
		self.dbSearchMusicAlbums()
		self.dbSearchMusicSongs()
		self.dbSearchGames()
		
		self.setProperty( 'SearchString', self.SEARCH_STRING )
	
	def onFocus(self, controlID):
		print '|==================================|'
		print 'onFocus(): Initialized!'
	
	def onClick( self, controlId ):
		print '|==================================|'
		print 'onClick(): Initialized!'
		item = self.getListItem( self.getCurrentListPosition() )
		print item.getProperty( 'content' ) + ' - ' + item.getProperty( 'path' )
		xbmc.executebuiltin( item.getProperty( 'path' ) )
	
	def dbSearchMovies( self ):
		print '|==================================|'
		print 'dbSearchMovies(): Initialized!' 
		# database sql statement
		# fields: 0=title, 1=rating, 2=year, 3=duration, 4=mpaa, 5=genre, 6=trailer, 7=fileName, 8=path
		sql_movies = 'SELECT c00, c05, c07, c11, c12, c14, c19, strFileName, strPath FROM movieview WHERE c00 LIKE "%' + self.SEARCH_STRING + '%"'
		# query the database
		movies_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_movies ), )
		# separate the records
		movies = re.findall( "<record>(.+?)</record>", movies_xml, re.DOTALL )
		# enumerate thru our records and set our properties
		for count, movie in enumerate( movies ):
			# separate individual fields
			fields = re.findall( "<field>(.*?)</field>", movie, re.DOTALL )
			# get cache names of path to use for thumbnail/fanart and play path
			thumb_cache, fanart_cache, play_path = self._getCacheThumb( fields[8], fields[7] )
			thumb = "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[0], thumb_cache, )
			# set item info
			list_item = xbmcgui.ListItem( fields[0], fields[2], thumb, thumb, fields[8] + fields[7] )
			list_item.setInfo( 'video', { 'title': fields[0], 'mpaa': fields[4], 'year': int( fields[2] ), 'genre': fields[4], 'duration': fields[3] } )
			list_item.setProperty( 'starrating', 'rating' + str(int(round(float(fields[1])/2))) )
			list_item.setProperty( 'content', 'movie' )
			list_item.setProperty( 'path', 'Xbmc.PlayMedia(' + fields[8] + fields[7] + ')' )
			# add item
			self.addItem( list_item )
		print 'dbSearchMovies(): Terminated!'
	
	def dbSearchTvShows( self ):
		print '|==================================|'
		print 'dbSearchTvShows(): Initialized!'
	
	def dbSearchTvEpisodes( self ):
		print '|==================================|'
		print 'dbSearchTvEpisodes(): Initialized!'
		# sql statment
		# fields: 0=showTitle, 1=episodeTitle, 2=season, 3=episodeNo, 4=rating, 5=fileName, 6=path, 7=mpaa, 8=premiered, 9=rating
		sql_episodes = 'SELECT strTitle, c00, c12, c13, c03, strFileName, strPath, mpaa, c05, c03 FROM episodeview WHERE c00 LIKE "%' + self.SEARCH_STRING + '%"'
		# query the database
		episodes_xml = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( sql_episodes ), )
		# separate the records
		episodes = re.findall( "<record>(.+?)</record>", episodes_xml, re.DOTALL )
		# enumerate thru our records and set our properties
		for count, episode in enumerate( episodes ):
			# separate individual fields
			fields = re.findall( "<field>(.*?)</field>", episode, re.DOTALL )
			# get cache names of path to use for thumbnail/fanart and play path
			thumb_cache, fanart_cache, play_path = self._getCacheThumb( fields[6], fields[5] )
			thumb = "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[0], thumb_cache, )
			# set item info
			list_item = xbmcgui.ListItem( fields[0], fields[2], thumb, thumb )
			list_item.setInfo( 'video', { 'title': fields[1], 'mpaa': fields[7], 'tvshowtitle': fields[0], 'premiered': fields[8] } )
			list_item.setProperty( 'starrating', 'rating' + str(int(round(float( fields[9] )) / 2 )) )
			list_item.setProperty( 'content', 'episode' )
			list_item.setProperty( 'path', 'Xbmc.PlayMedia(' + fields[6] + fields[5] + ')' )
			# add item to list
			self.addItem( list_item )
		print 'dbSearchTvEpisodes(): Terminated!'
	
	def dbSearchMusicAlbums( self ):
		print '|==================================|'
		print 'dbSearchMusicAlbums(): Initialized!'
		# fields: 0=albumID, 1=title, 2=year, 3=genre, 4=artist, 5=thumb, 6=rating
		sql_music = 'SELECT idAlbum, strAlbum, iYear, strGenre, strArtist, strThumb, iRating FROM albumview WHERE strAlbum LIKE "%' + self.SEARCH_STRING + '%"'
		# query the database
		music_xml = xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( sql_music ), )
		# separate the records
		items = re.findall( "<record>(.+?)</record>", music_xml, re.DOTALL )
		# enumerate thru our records and set our properties
		for count, item in enumerate( items ):
			# separate individual fields
			fields = re.findall( "<field>(.*?)</field>", item, re.DOTALL )
			# set properties
			list_item = xbmcgui.ListItem( fields[1], fields[4], fields[5], fields[5], 'ActivateWindow(MusicLibrary,musicdb://3/' + fields[0] + ')' )
			list_item.setInfo( 'music', { 'album': fields[1], 'year': int( fields[2] ), 'genre': fields[3] } )
			list_item.setProperty( 'content', 'album' )
			list_item.setProperty( 'path', 'Xbmc.ActivateWindow(MusicLibrary,musicdb://3/' + fields[0] + ')' )
			# add item to list
			self.addItem( list_item )
		print 'dbSearchMusicAlbums(): Terminated!'
	
	def dbSearchMusicSongs( self ):
		print '|==================================|'
		print 'dbSearchMusicSongs(): Intialized!'
		# fields: 0=songTitle, 1=year, 2=artistName, 3=albumName, 4=genre, 5=path, 6=fileName, 7=thumb, 8=duration, 9=rating
		sql_music = 'SELECT strTitle, iYear, strArtist, strAlbum, strGenre, strPath, strFileName, strThumb, iDuration, rating FROM songview WHERE strTitle LIKE "%' + self.SEARCH_STRING + '%"'
		# query the database
		music_xml = xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( sql_music ), )
		# separate the records
		items = re.findall( "<record>(.+?)</record>", music_xml, re.DOTALL )
		# enumerate thru our records and set our properties
		for count, item in enumerate( items ):
			# separate individual fields
			fields = re.findall( "<field>(.*?)</field>", item, re.DOTALL )
			
			# set properties
			list_item = xbmcgui.ListItem( fields[0], fields[1], fields[7], fields[7] )
			list_item.setInfo( 'music', {'duration': int( fields[8] ), 'genre': fields[4], 'album': fields[3] } )
			list_item.setProperty( 'rating', 'rating' + str (int( fields[9] ) / 2 ) + '.png' )
			list_item.setProperty( 'content', 'song' )
			list_item.setProperty( 'path', 'Xbmc.PlayMedia(' + fields[5] + fields[6] + ')' )
			
			self.addItem( list_item )
		print 'dbSearchMusicSongs(): Terminated!'
	
	def dbSearchGames( self ):
		print '|==================================|'
		print 'dbSearchGames(): Initialized!'
		sql_games = 'select xbeDescription, strFileName from files WHERE xbeDescription LIKE "%' + self.SEARCH_STRING + '%"' 
		# query the database
		games_xml = xbmc.executehttpapi( "QueryProgramDatabase(%s)" % quote_plus( sql_games ), )
		# separate the records
		games = re.findall( "<record>(.+?)</record>", games_xml, re.DOTALL )
		# enumerate thru our records and set our properties
		for count, game in enumerate( games ):
			# separate individual fields
			fields = re.findall( "<field>(.*?)</field>", game, re.DOTALL )
			# get cache names of path to use for thumbnail/fanart and play path
			thumb_cache = xbmc.getCacheThumbName( fields[1] )
			thumb = "special://profile/Thumbnails/Programs/%s/%s" % ( thumb_cache[ 0 ], thumb_cache, )			
			# add item info
			list_item = xbmcgui.ListItem( fields[0], fields[0], thumb, thumb, fields[1] )
			list_item.setProperty( 'content', 'game' )
			#list_item.setProperty( 'path', 'Xbmc.PlayMedia(' + fields[8] + fields[7] + ')' )
			self.addItem( list_item )
		print 'dbSearchGames(): Terminated!'
	
	def _getCacheThumb( self, path, file ):
		print '_getCacheThumb(): Intialized'
		# set default values
		play_path = fanart_path = thumb_path = path + file
		# we handle stack:// media special
		if ( file.startswith( "stack://" ) ):
			play_path = fanart_path = file
			thumb_path = file[ 8 : ].split( " , " )[ 0 ]
		# we handle rar:// and zip:// media special
		if ( file.startswith( "rar://" ) or file.startswith( "zip://" ) ):
			play_path = fanart_path = thumb_path = file
		# return media info
		return xbmc.getCacheThumbName( thumb_path ), xbmc.getCacheThumbName( fanart_path ), play_path
	
if ( __name__ == "__main__" ):
	
	keyboard = xbmc.Keyboard( '', 'Search your Xbox for content...', False )     
	keyboard.doModal()     
	if ( keyboard.isConfirmed() ):         
		_searchString = keyboard.getText()
		
		# format our records start and end
		xbmc.executehttpapi( "SetResponseFormat()" )
		xbmc.executehttpapi( "SetResponseFormat(OpenRecord,%s)" % ( "<record>", ) )
		xbmc.executehttpapi( "SetResponseFormat(CloseRecord,%s)" % ( "</record>", ) )

		w = GUI( 'script-GlobalSearch-View.xml', os.getcwd() )
		w.doModal()
	
		del w
