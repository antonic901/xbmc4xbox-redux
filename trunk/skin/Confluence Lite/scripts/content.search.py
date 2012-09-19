import xbmc
import xbmcgui
from urllib import quote_plus, unquote_plus
import re
import sys
import os
import datetime

ACTION_CLOSE = ( 9, 10, )

_searchString = ''

def log(txt):
	print 'script.global.search: %s' % txt
	#message = 'script.global.search: %s' % txt
	#xbmc.log(msg=message, level=xbmc.LOGDEBUG)

def selectContentType():
	log( '|==================================|' )
	log( 'searchSelect().start' )
	contentList = [ 'All Content', 'Movies', 'Tv Shows', 'Tv Episodes', 'Actors', 'Albums', 'Songs', 'Artists', 'Games' ]
	content = xbmcgui.Dialog().select( 'Select content to search for...', contentList )
	if content in (0,1,2,3,4,5,6,7,8,):
		return content
	else:
		return None
	
class GUI( xbmcgui.WindowXMLDialog ):
	def __init__( self, *args, **kwargs ):
		log( '|==================================|' )
		log( '__init__().start' )
		self.SEARCH_STRING 	= kwargs[ 'searchSTRING' ]
		self.CONTENT_TYPE 	= kwargs[ 'contentTYPE' ]
		self.CONTENT_FOUND 	= 'false'
		
	def onInit(self):
		log( '|==================================|' )
		log( 'onInit().start' )
		self.reset()
		self.searchContent()
	
	def searchContent( self ):
		log( '|==================================|' )
		log( 'searchContent().start' )
				
		content = self.CONTENT_TYPE
		
		if content == 0:
			self.searchAll()
		elif content == 1:
			self.searchMovies()
		elif content == 2:
			self.searchTvShows()
		elif content == 3:
			self.searchTvEpisodes()
		elif content == 4:
			self.searchActors()
		elif content == 5:
			self.searchMusicAlbums()
		elif content == 6:
			self.searchMusicSongs()
		elif content == 7:
			self.searchMusicArtists()
		elif content == 8:
			self.searchGames()
		self.cleanUp()
		
	def searchAll( self ):
		log( '|==================================|' )
		log( 'searchAll().start' )
		log( 'search().string: ' + self.SEARCH_STRING )
		
		self.getControl( 190 ).setLabel( '[B]' + xbmc.getLocalizedString(194) + '[/B]' )
		self.searchMovies()
		self.searchActors()
		self.searchTvShows()
		self.searchTvEpisodes()
		self.searchMusicAlbums()
		self.searchMusicSongs()
		self.searchMusicArtists()
		self.searchGames()
		
		self.setProperty( 'SearchString', self.SEARCH_STRING )
		
		log( 'search().end' )

	def newSearch( self ):
		log( '|==================================|' )
		log( 'newSearch().start' )
		self.reset()
		self.CONTENT_TYPE = selectContentType()
		if self.CONTENT_TYPE == None:
			xbmc.executebuiltin( 'XBMC.Notification(Xbox Content Search Cancelled!,Content not selected!)' )
			self.cleanUp()
		else:
			keyboard = xbmc.Keyboard( '', 'Enter content search string...' , False )
			keyboard.doModal()
			if ( keyboard.isConfirmed() ):
				self.SEARCH_STRING = keyboard.getText()
				if (self.SEARCH_STRING == '') or (self.SEARCH_STRING == ' '):
					xbmc.executebuiltin( 'XBMC.Notification(Xbox Content Search Cancelled!,No search string provided!)' )
					self.cleanUp()
				else:
					self.searchContent()
			else:
				xbmc.executebuiltin( 'XBMC.Notification(Xbox Content Search Cancelled!,User request...)' )
				self.cleanUp()
			log( 'newSearch().end' )
		
	def hideControls( self ):
		log( 'hideControls().start' )
		self.getControl( 119 ).setVisible( False )
		self.getControl( 129 ).setVisible( False )
		self.getControl( 139 ).setVisible( False )
		self.getControl( 149 ).setVisible( False )
		self.getControl( 159 ).setVisible( False )
		self.getControl( 169 ).setVisible( False )
		self.getControl( 179 ).setVisible( False )
		self.getControl( 189 ).setVisible( False )
		self.getControl( 198 ).setVisible( False )
		self.getControl( 199 ).setVisible( False )
		self.getControl( 209 ).setVisible( False )

	def resetControls( self ):
		log( 'resetControls().start' )
		self.getControl( 111 ).reset()
		self.getControl( 121 ).reset()
		self.getControl( 131 ).reset()
		self.getControl( 141 ).reset()
		self.getControl( 151 ).reset()
		self.getControl( 161 ).reset()
		self.getControl( 171 ).reset()
		self.getControl( 181 ).reset()
		self.getControl( 201 ).reset()
	
	def reset( self ):
		self.getControl( 190 ).setLabel( '[B]' + xbmc.getLocalizedString(194) + '[/B]' )
		self.getControl( 198 ).setLabel( '[B]' + xbmc.getLocalizedString(32299) + '[/B]' )
		self.hideControls()
		self.resetControls()
		self.CONTENT_FOUND 		= 'false'
		self.GET_ACTOR_MOVIES 	= 'false'
		self.GET_ACTOR_TVSHOWS 	= 'false'
		self.GET_ACTOR_EPISODES = 'false'
	
	def cleanUp( self ):
		self.getControl( 190 ).setLabel( '' )
		self.getControl( 191 ).setLabel( '' )
		if self.CONTENT_FOUND == 'false':
			self.getControl( 199 ).setVisible( True )
			self.setFocus( self.getControl( 198 ) )
		self.getControl( 198 ).setVisible( True )
	
	def onFocus(self, controlID):
		log( '|==================================|' )
		log( 'onFocus().start' )
		log( 'onFocus().controlID = ' + str( controlID ) )
		
	def onAction( self, action ):
		log( '|==================================|' )
		log( 'onAction().start' )
		log( 'onAction().action = ' + str( action.getId() ) )
		
		if action in ACTION_CLOSE:
			self.close()
		else:
			focusId = self.getFocusId()
			if focusId in ( 111, 121, 141 ):
				fanart = self.getFocus().getSelectedItem().getProperty( 'fanart_image' )
				self.getControl( 99 ).setImage( fanart )
		
	def onClick( self, controlId ):
		log( '|==================================|' )
		log( 'onClick()' )
		log( 'onClick().controlId: ' + str( controlId ) )
		
		if controlId == 111:	# movie.play
			listItem = self.getControl( 111 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			log( 'onClick().movie.play( ' + path + ' )' )
			self.close()
			xbmc.Player().play( path )
		elif controlId ==121:	# tvshow.browse
			listItem = self.getControl( 121 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			fanart = listItem.getProperty( 'fanart_image' )
			log( 'onClick().tvShow.browse.path( ' + path + ' )' )
			self.close()
			xbmc.executebuiltin('ActivateWindow(VideoLibrary,' + path + ',return)')
		elif controlId ==131:	# actor.browse
			listItem = self.getControl( 131 ).getSelectedItem()
			#path = listItem.getProperty( 'path' )
			id = listItem.getProperty( 'id' )
			log( 'onClick().movie.actor.browse( ' + id + ' )' )
			self.hideControls()
			self.resetControls()
			self.SEARCH_STRING = id
			self.GET_ACTOR_MOVIES 	= 'true'
			self.GET_ACTOR_TVSHOWS	= 'true'
			self.GET_ACTOR_EPISODES	= 'true'
			
			self.searchMovies()
			self.searchTvShows()
			self.searchTvEpisodes()
		elif controlId ==141:	# episode.play
			listItem = self.getControl( 141 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			log( 'onClick().episodes.play( ' + path + ' )' )		
			self.close()
			xbmc.Player().play( path )
		elif controlId ==161:	# artist.browse
			listItem = self.getControl( 161 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			log( 'onClick().artist.browse( ' + path + ' )' )
			self.close()
			xbmc.executebuiltin( 'ActivateWindow(MusicLibrary,' + path + ',return)')
		elif controlId ==171:	# album.browse
			listItem = self.getControl( 171 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			log( 'onClick().albums.browse( ' + path + ' )' )
			self.close()
			xbmc.executebuiltin( 'ActivateWindow(MusicLibrary,' + path + ',return)')
		elif controlId ==181:	# song.play
			listItem = self.getControl( 181 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			log( 'onClick().songs.play( ' + path + ' )' )
			self.close()
			xbmc.Player().play( path, listItem )
		elif controlId == 201:
			listItem = self.getControl( 201 ).getSelectedItem()
			path = listItem.getProperty( 'path' )
			log( 'onClick().runXBE( ' + path + ' )' )
			xbmc.executebuiltin('runXBE(' + path + ')' )
		elif controlId == 198:	# newSearch
			self.newSearch()
		
	def searchMovies( self ):
		log( '|==================================|' )
		log( 'searchMovies()' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(342) + '[/B]' )
		# fields: 0=title, 1=rating, 2=year, 3=duration, 4=mpaa, 5=genre, 6=trailer, 7=fileName, 8=path, 9=plotOutline, 10=plot
		if self.GET_ACTOR_MOVIES == 'true':
			moviesSQL = 'SELECT c00, c05, c07, c11, c12, c14, c19, strFileName, strPath, c03, c01 FROM movieview WHERE idMovie IN (SELECT DISTINCT idMovie FROM actorlinkmovie WHERE idActor = %s)' % ( self.SEARCH_STRING, ) 
		else:
			moviesSQL = 'SELECT c00, c05, c07, c11, c12, c14, c19, strFileName, strPath, c03, c01 FROM movieview WHERE c00 LIKE "%' + self.SEARCH_STRING + '%"'
		
		log( 'searchMovies().sql = ' + moviesSQL )
		
		moviesXML 	= xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( moviesSQL ), )
		movies 		= re.findall( "<record>(.+?)</record>", moviesXML, re.DOTALL )
		results 	= len( movies )
		
		log( 'searchMovies().results = ' + str( results ) )
		
		for count, movie in enumerate( movies ):
			fields = re.findall( "<field>(.*?)</field>", movie, re.DOTALL )
			thumb_cache, fanart_cache, play_path = self.getCacheThumb( fields[8], fields[7] )
			thumb 	= "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[0], thumb_cache, )
			fanart	= "special://profile/Thumbnails/Video/Fanart/%s" % ( fanart_cache, )
			
			log( 'searchMovies().results(%d).title = %s' % ( count + 1, fields[0], ) )
			log( 'searchMovies().results(%d).path = %s' % ( count + 1, play_path, ) )
			log( 'searchMovies().results(%d).fanart = %s' % ( count + 1, fanart, ) )
			
			listItem = xbmcgui.ListItem( fields[0], fields[2], thumb, thumb )
			listItem.setProperty( 'year', fields[2] )
			listItem.setProperty( 'duration', fields[3] )
			listItem.setProperty( 'plotOutline', fields[9] )
			listItem.setProperty( 'plot', fields[10] )
			listItem.setProperty( 'starrating', 'rating%1d.png' % round( float( fields[1] ) / 2 ) )
			listItem.setProperty( 'path', play_path )
			listItem.setProperty( 'fanart_image', fanart )
			listItem.setProperty( 'content', 'movie' )
			listItems.append( listItem )
		
		if results > 0:
			self.getControl( 111 ).addItems( listItems )
			self.getControl( 110 ).setLabel( str( results ) )
			self.getControl( 119 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchMovies().end' )
		
	def searchTvShows( self ):
		log( '|==================================|' )
		log( 'searchTvShows().start' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(20343) + '[/B]' )
		# fields: 0=showID, 1=title, 2=plot, 3=rating, 4=year, 5=genre, 6=mpaa, 7=studio, 8=path
		if self.GET_ACTOR_MOVIES == 'true':
			tvshowsSQL = 'SELECT idShow, c00, c01, c04, c05, c08, c13, c14, strPath FROM tvshowview WHERE idShow IN (SELECT DISTINCT idShow FROM actorLinkTvShow WHERE idActor = %s)' % ( self.SEARCH_STRING, )
		else:
			tvshowsSQL = 'SELECT idShow, c00, c01, c04, c05, c08, c13, c14, strPath FROM tvshowview WHERE c00 LIKE "%' + self.SEARCH_STRING + '%"'
		
		log( 'searchTvShows().sql = ' + tvshowsSQL )
		
		tvshowsXML 	= xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( tvshowsSQL ), )
		tvshows 	= re.findall( "<record>(.+?)</record>", tvshowsXML, re.DOTALL )
		results 	= len( tvshows )
		
		log( 'searchTvShows().results = ' + str( results ) )
		
		for count, movie in enumerate( tvshows ):
			fields = re.findall( "<field>(.*?)</field>", movie, re.DOTALL )
			thumb_cache, fanart_cache, play_path = self.getCacheThumb( fields[8], '' )
			thumb 	= "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[0], thumb_cache, )
			fanart 	= "special://profile/Thumbnails/Video/Fanart/%s" % ( fanart_cache, )
			
			log( 'searchTvShows().results(%d).title = %s' % ( count + 1, fields[1], ) )
			log( 'searchTvShows().results(%d).path = %s' % ( count + 1, 'videodb://2/2/' + fields[0], ) )
			
			listItem = xbmcgui.ListItem( fields[1], fields[3], thumb, thumb )
			listItem.setProperty( 'icon', thumb )
			listItem.setProperty( 'plot', fields[2] )
			listItem.setProperty( 'rating', fields[3] )
			listItem.setProperty( 'year', fields[4] )
			listItem.setProperty( 'genre', fields[5] )
			listItem.setProperty( 'mpaa', fields[6] )
			listItem.setProperty( 'studio', fields[7] )
			listItem.setProperty( 'starrating', 'rating%d.png' % round( float( fields[3] ) / 2 ) )
			listItem.setProperty( 'path', 'videodb://2/2/' + fields[0] )
			listItem.setProperty( 'fanart_image', fanart )
			listItem.setProperty( 'content', 'movie' )
			listItems.append( listItem )
		
		if results > 0:
			self.getControl( 121 ).addItems( listItems )
			self.getControl( 120 ).setLabel( str( results ) )
			self.getControl( 129 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchTvShows().end' )
		
	def searchTvEpisodes( self ):
		log( '|==================================|' )
		log( 'searchTvEpisodes().start' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(20360) + '[/B]' )
		# fields: 0=showTitle, 1=episodeTitle, 2=season, 3=episodeNo, 4=rating, 5=fileName, 6=path, 7=mpaa, 8=firstAired, 9=rating, 10=plot
		if self.GET_ACTOR_EPISODES == 'true':
			episodesSQL = 'SELECT strTitle, c00, c12, c13, c03, strFileName, strPath, mpaa, c05, c03, c01 FROM episodeview WHERE idEpisode IN (SELECT DISTINCT idEpisode FROM actorLinkEpisode WHERE idActor = %s)' % ( self.SEARCH_STRING, )
		else:
			episodesSQL = 'SELECT strTitle, c00, c12, c13, c03, strFileName, strPath, mpaa, c05, c03, c01 FROM episodeview WHERE c00 LIKE "%' + self.SEARCH_STRING + '%"'
		
		log( 'searchTvEpisodes().sql = ' + episodesSQL )
		
		episodesXML = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( episodesSQL ), )
		episodes 	= re.findall( "<record>(.+?)</record>", episodesXML, re.DOTALL )
		results 	= len( episodes )
		
		log( 'searchTvEpisodes().results = ' + str( results ) )
		
		for count, episode in enumerate( episodes ):
			fields = re.findall( "<field>(.*?)</field>", episode, re.DOTALL )
			thumb_cache, fanart_cache, play_path = self.getCacheThumb( fields[6], fields[5] )
			thumb 	= "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[0], thumb_cache, )
			fanart 	= "special://profile/Thumbnails/Video/Fanart/%s" % ( fanart_cache, )
			
			log( 'searchTvEpisodes().results(%d).title = %s' % ( count + 1, fields[0], ) )
			log( 'searchTvEpisodes().results(%d).path = %s' % ( count + 1, play_path, ) )
			
			listItem = xbmcgui.ListItem( fields[1], fields[2], thumb, thumb )
			listItem.setProperty( 'icon', thumb )
			listItem.setProperty( 'episode', fields[3] )
			listItem.setProperty( 'plot', fields[10] )
			listItem.setProperty( 'rating', fields[4] )
			listItem.setProperty( 'season', fields[2] )
			listItem.setProperty( 'starrating', 'rating%d.png' % round( float( fields[9] ) / 2 ) )
			listItem.setProperty( 'tvshowtitle', fields[0] )
			listItem.setProperty( 'premiered', fields[8] )
			listItem.setProperty( 'content', 'episode' )
			listItem.setProperty( 'path', play_path )
			listItem.setProperty( 'fanart_image', fanart )
			listItems.append( listItem )
			
		if results > 0:
			self.getControl( 141 ).addItems( listItems )
			self.getControl( 140 ).setLabel( str( results ) )
			self.getControl( 149 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchTvEpisodes().terminate' )
	
	def searchMusicAlbums( self ):
		log( '|==================================|' )
		log( 'searchMusicAlbums().start' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(132) + '[/B]' )
		# fields: 0=albumID, 1=title, 2=year, 3=genre, 4=artist, 5=thumb, 6=rating
		albumsSQL = 'SELECT idAlbum, strAlbum, iYear, strGenre, strArtist, strThumb, iRating FROM albumview WHERE strAlbum LIKE "%' + self.SEARCH_STRING + '%"'
		
		log( 'searchMusicAlbums().sql = ' + albumsSQL )
		
		albumsXML 	= xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( albumsSQL ), )
		albums 		= re.findall( "<record>(.+?)</record>", albumsXML, re.DOTALL )
		results 	= len( albums )
		
		log( 'searchMusicAlbums().results = ' + str( results ) )
		
		for count, album in enumerate( albums ):
			fields = re.findall( "<field>(.*?)</field>", album, re.DOTALL )
			
			log( 'searchMusicAlbums().results(%d).title = %s' % ( count + 1, fields[1], ) )
			log( 'searchMusicAlbums().results(%d).path = %s' % ( count + 1, 'musicdb://3/' + fields[0], ) )
			
			listItem = xbmcgui.ListItem( fields[1], fields[4], fields[5], fields[5], 'ActivateWindow(MusicLibrary,musicdb://3/' + fields[0] + ')' )
			listItem.setProperty( 'icon', 	fields[5] )
			listItem.setProperty( 'artist', fields[4] )
			listItem.setProperty( 'year', 	fields[2] )
			listItem.setProperty( 'genre', 	fields[3] )
			listItem.setProperty( 'path', 'musicdb://3/' + fields[0] )
			listItem.setProperty( 'content', 'albums' )
			listItems.append( listItem )
			
		if results > 0:
			self.getControl( 171 ).addItems( listItems )
			self.getControl( 170 ).setLabel( str( results ) )
			self.getControl( 179 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchMusicAlbums().end' )
	
	def searchMusicSongs( self ):
		log( '|==================================|' )
		log( 'searchMusicSongs()' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(133) + '[/B]' )
		# fields: 0=songTitle, 1=year, 2=artistName, 3=albumName, 4=genre, 5=path, 6=fileName, 7=thumb, 8=duration, 9=rating
		songsSQL = 'SELECT strTitle, iYear, strArtist, strAlbum, strGenre, strPath, strFileName, strThumb, iDuration, rating FROM songview WHERE strTitle LIKE "%' + self.SEARCH_STRING + '%"'
		
		log( 'searchMusicSongs().sql = ' + songsSQL )
		
		songsXML	= xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( songsSQL ), )
		songs 		= re.findall( "<record>(.+?)</record>", songsXML, re.DOTALL )
		results 	= len( songs )
		
		log( 'searchMusicSongs().results = ' + str( results ) )
		
		for count, song in enumerate( songs ):
			fields = re.findall( "<field>(.*?)</field>", song, re.DOTALL )
			
			log( 'searchMusicSongs().results(%d).title: %s' % ( count + 1, fields[0], ) )
			log( 'searchMusicSongs().results(%d).path: %s' % ( count + 1, fields[5] + fields[6], ) )
			
			listItem = xbmcgui.ListItem( fields[0], fields[1], fields[7], fields[7] )
			listItem.setProperty( 'starrating', 'rating%d.png' % round( float( fields[9] ) / 2 ) )
			listItem.setProperty( 'icon', 	fields[7] )
			listItem.setProperty( 'artist', fields[2] )
			listItem.setProperty( 'album', 	fields[3] )
			listItem.setProperty( 'genre', 	fields[4] )
			listItem.setProperty( 'duration', str( datetime.timedelta(seconds=int( fields[8] )) ) )
			listItem.setProperty( 'path',	fields[5] + fields[6] )
			listItem.setProperty( 'content', 'song' )
			listItems.append( listItem )
			
		if results > 0:
			self.getControl( 181 ).addItems( listItems )
			self.getControl( 180 ).setLabel( str( results ) )
			self.getControl( 189 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchMusicSongs().end' )

	def searchActors( self ):
		log( '|==================================|' )
		log( 'searchMovieActors().start' )

		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(20337) + '[/B]' )
		# fields: 0=actorID, 1=actor
		actorsSQL = 'SELECT idActor, strActor FROM actors WHERE strActor LIKE "%' + self.SEARCH_STRING + '%"'
		#actorsSQL = 'SELECT DISTINCT actors.idActor, actors.strActor FROM actors INNER JOIN actorlinkmovie ON actors.idActor = actorlinkmovie.idActor WHERE actors.strActor LIKE "%' + self.SEARCH_STRING + '%"'
		
		log( 'searchMovieActors().sql = ' + actorsSQL )
		
		actorsXML = xbmc.executehttpapi( "QueryVideoDatabase(%s)" % quote_plus( actorsSQL ), )
		actors = re.findall( "<record>(.+?)</record>", actorsXML, re.DOTALL )
		results = len( actors )
		
		log( 'searchMovieActors().results = ' + str( results ) )
		
		for count, actor in enumerate( actors ):
			fields = re.findall( "<field>(.*?)</field>", actor, re.DOTALL )
			thumb_cache, fanart_cache, play_path = self.getCacheThumb( 'actor' + fields[1].lower(), '' )
			thumb 	= "special://profile/Thumbnails/Video/%s/%s" % ( thumb_cache[0], thumb_cache, )
			
			log( 'searchMovieActors().results(%d).title = %s' % ( count + 1, fields[1], ) )
			log( 'searchMovieActors().results(%d).path = %s' % ( count + 1, 'videodb://1/4/' + fields[0], ) )
			
			listItem = xbmcgui.ListItem( fields[1], fields[1], thumb, thumb )
			listItem.setProperty( 'id', fields[0] )
			listItem.setProperty( 'icon', thumb )
			listItem.setProperty( 'path', 'videodb://1/4/' + fields[0] )
			listItem.setProperty( 'content', 'actors' )
			listItems.append( listItem )
		
		if results > 0:
			self.getControl( 131 ).addItems( listItems )
			self.getControl( 130 ).setLabel( str( results ) )
			self.getControl( 139 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchMovieActors().end' )
		
	def searchMusicArtists( self ):
		log( '|==================================|' )
		log( 'searchMusicArtists()' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]' + xbmc.getLocalizedString(133) + '[/B]' )
		# fields: 0=artistID, 1=artist
		artistsSQL = 'SELECT idArtist, strArtist FROM artist WHERE strArtist LIKE "%' + self.SEARCH_STRING + '%"'
		artistsXML = xbmc.executehttpapi( "QueryMusicDatabase(%s)" % quote_plus( artistsSQL ), )
		artists = re.findall( "<record>(.+?)</record>", artistsXML, re.DOTALL )
		results = len( artists )
		
		log( 'searchMusicArtists().results = ' + str( results ) )
		
		for count, artist in enumerate( artists ):
			fields = re.findall( "<field>(.*?)</field>", artist, re.DOTALL )
			thumb_cache, fanart_cache, play_path = self.getCacheThumb( 'artist' + fields[1], '' )
			thumb 	= "special://profile/Thumbnails/Music/Artists/%s" % ( thumb_cache, )
			fanart 	= "special://profile/Thumbnails/Music/Fanart/%s" % ( fanart_cache, )
			
			log( 'searchMusicArtists().results(%d).title: %s' % ( count + 1, fields[1], ) )
			log( 'searchMusicArtists().results(%d).path: %s' % ( count + 1, 'musicdb://2/' + fields[0], ) )
			
			listItem = xbmcgui.ListItem( fields[1], fields[1], thumb, thumb )
			listItem.setProperty( 'icon', thumb )
			listItem.setProperty( 'fanart_image', thumb )
			listItem.setProperty( 'path', 'musicdb://2/' + fields[0] )
			listItem.setProperty( 'content', 'artists' )
			listItems.append( listItem )
		
		if results > 0:
			self.getControl( 161 ).addItems( listItems )
			self.getControl( 160 ).setLabel( str( results ) )
			self.getControl( 169 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchMusicArtists().end' )
		
	def searchGames( self ):
		log( '|==================================|' )
		log( 'searchGames().start' )
		
		listItems = []
		self.getControl( 191 ).setLabel( '[B]Games[/B]' )
		gamesSQL = 'SELECT xbeDescription, strFileName FROM files WHERE strFileName LIKE "F:\GAMES%" AND xbeDescription LIKE "%' + self.SEARCH_STRING + '%"' 
		log( 'searchGames().sql = ' + gamesSQL )
		gamesXML = xbmc.executehttpapi( "QueryProgramDatabase(%s)" % quote_plus( gamesSQL ), )
		games = re.findall( "<record>(.+?)</record>", gamesXML, re.DOTALL )
		results = len( games )
		
		log( 'searchGames().results: ' + str( results ) )

		for count, game in enumerate( games ):
			fields = re.findall( "<field>(.*?)</field>", game, re.DOTALL )
			thumb_cache = xbmc.getCacheThumbName( fields[1] )
			thumb = "special://profile/Thumbnails/Programs/%s/%s" % ( thumb_cache[ 0 ], thumb_cache, )			
			listItem = xbmcgui.ListItem( fields[0], fields[0], thumb, thumb )
			listItem.setProperty( 'path', fields[1] )
			listItem.setProperty( 'content', 'game' )
			listItems.append( listItem )
		
		if results > 0:
			log( 'searchGames().success' )
			self.getControl( 201 ).addItems( listItems )
			self.getControl( 200 ).setLabel( str( results ) )
			self.getControl( 209 ).setVisible( True )
			self.CONTENT_FOUND = 'true'
		log( 'searchGames().end' )
	
	def getCacheThumb( self, path, file ):
		play_path = fanart_path = thumb_path = path + file
		if ( file.startswith( "stack://" ) ):
			play_path = fanart_path = file
			thumb_path = file[ 8 : ].split( " , " )[ 0 ]
		if ( file.startswith( "rar://" ) or file.startswith( "zip://" ) ):
			play_path = fanart_path = thumb_path = file
		return xbmc.getCacheThumbName( thumb_path ), xbmc.getCacheThumbName( fanart_path ), play_path
		
if ( __name__ == "__main__" ):
	_searchContentType = selectContentType()
	log( '__main__().contentType = ' + str( _searchContentType ) )
	if _searchContentType == None:
		xbmc.executebuiltin( 'XBMC.Notification(Xbox Content Search Cancelled, Content not selected!)' )
	else:
		keyboard = xbmc.Keyboard( '', 'Search your Xbox for content...', False )     
		keyboard.doModal()     
		if ( keyboard.isConfirmed() ):         
			_searchString = keyboard.getText()
			log( '__main__()._searchString = ' + _searchString )
			if ( _searchString == '' ) or ( _searchString == ' ' ):
				xbmc.executebuiltin( 'XBMC.Notification(Xbox Content Search Cancelled, Invalid search string!)' )
			else:
				xbmc.executehttpapi( "SetResponseFormat()" )
				xbmc.executehttpapi( "SetResponseFormat(OpenRecord,%s)" % ( "<record>", ) )
				xbmc.executehttpapi( "SetResponseFormat(CloseRecord,%s)" % ( "</record>", ) )
				w = GUI( 'script-Content_Search-main.xml', os.getcwd(), searchSTRING=_searchString, contentTYPE=_searchContentType )
				w.doModal()
				
				del w