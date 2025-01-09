# -*- coding: utf-8 -*-

def theaudiodb_artistdetails(data):
    if data.get(u'artists',[]):
        item = data[u'artists'][0]
        artistdata = {}
        extras = []
        artistdata[u'artist'] = item[u'strArtist']
        # api inconsistent
        if item.get(u'intFormedYear',u'') and item[u'intFormedYear'] != u'0':
            artistdata[u'formed'] = item[u'intFormedYear']
        if item.get(u'intBornYear',u'') and item[u'intBornYear'] != u'0':
            artistdata[u'born'] = item[u'intBornYear']
        if item.get(u'intDiedYear',u'') and item[u'intDiedYear'] != u'0':
            artistdata[u'died'] = item[u'intDiedYear']
        if item.get(u'strDisbanded',u'') and item[u'strDisbanded'] != u'0':
            artistdata[u'disbanded'] = item[u'strDisbanded']
        if item.get(u'strStyle',u''):
            artistdata[u'styles'] = item[u'strStyle']
        if item.get(u'strGenre',u''):
            artistdata[u'genre'] = item[u'strGenre']
        if item.get(u'strMood',u''):
            artistdata[u'moods'] = item[u'strMood']
        if item.get(u'strGender',u''):
            artistdata[u'gender'] = item[u'strGender']
        if item.get(u'strBiographyEN',u''):
            artistdata[u'biographyEN'] = item[u'strBiographyEN']
        if item.get(u'strBiographyDE',u''):
            artistdata[u'biographyDE'] = item[u'strBiographyDE']
        if item.get(u'strBiographyFR',u''):
            artistdata[u'biographyFR'] = item[u'strBiographyFR']
        if item.get(u'strBiographyCN',u''):
            artistdata[u'biographyCN'] = item[u'strBiographyCN']
        if item.get(u'strBiographyIT',u''):
            artistdata[u'biographyIT'] = item[u'strBiographyIT']
        if item.get(u'strBiographyJP',u''):
            artistdata[u'biographyJP'] = item[u'strBiographyJP']
        if item.get(u'strBiographyRU',u''):
            artistdata[u'biographyRU'] = item[u'strBiographyRU']
        if item.get(u'strBiographyES',u''):
            artistdata[u'biographyES'] = item[u'strBiographyES']
        if item.get(u'strBiographyPT',u''):
            artistdata[u'biographyPT'] = item[u'strBiographyPT']
        if item.get(u'strBiographySE',u''):
            artistdata[u'biographySE'] = item[u'strBiographySE']
        if item.get(u'strBiographyNL',u''):
            artistdata[u'biographyNL'] = item[u'strBiographyNL']
        if item.get(u'strBiographyHU',u''):
            artistdata[u'biographyHU'] = item[u'strBiographyHU']
        if item.get(u'strBiographyNO',u''):
            artistdata[u'biographyNO'] = item[u'strBiographyNO']
        if item.get(u'strBiographyIL',u''):
            artistdata[u'biographyIL'] = item[u'strBiographyIL']
        if item.get(u'strBiographyPL',u''):
            artistdata[u'biographyPL'] = item[u'strBiographyPL']
        if item.get(u'strMusicBrainzID',u''):
            artistdata[u'mbartistid'] = item[u'strMusicBrainzID']
        if item.get(u'strArtistFanart',u''):
            fanart = []
            fanartdata = {}
            fanartdata[u'image'] = item[u'strArtistFanart']
            fanartdata[u'preview'] = item[u'strArtistFanart'] + u'/preview'
            fanartdata[u'aspect'] = u'fanart'
            fanart.append(fanartdata)
            if item[u'strArtistFanart2']:
                fanartdata = {}
                fanartdata[u'image'] = item[u'strArtistFanart2']
                fanartdata[u'preview'] = item[u'strArtistFanart2'] + u'/preview'
                fanartdata[u'aspect'] = u'fanart'
                fanart.append(fanartdata)
                if item[u'strArtistFanart3']:
                    fanartdata = {}
                    fanartdata[u'image'] = item[u'strArtistFanart3']
                    fanartdata[u'preview'] = item[u'strArtistFanart3'] + u'/preview'
                    fanartdata[u'aspect'] = u'fanart'
                    fanart.append(fanartdata)
            artistdata[u'fanart'] = fanart
        if item.get(u'strArtistThumb',u''):
            thumbs = []
            thumbdata = {}
            thumbdata[u'image'] = item[u'strArtistThumb']
            thumbdata[u'preview'] = item[u'strArtistThumb'] + u'/preview'
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
            artistdata[u'thumb'] = thumbs
        if item.get(u'strArtistLogo',u''):
            extradata = {}
            extradata[u'image'] = item[u'strArtistLogo']
            extradata[u'preview'] = item[u'strArtistLogo'] + u'/preview'
            extradata[u'aspect'] = u'clearlogo'
            extras.append(extradata)
        if item.get(u'strArtistClearart',u''):
            extradata = {}
            extradata[u'image'] = item[u'strArtistClearart']
            extradata[u'preview'] = item[u'strArtistClearart'] + u'/preview'
            extradata[u'aspect'] = u'clearart'
            extras.append(extradata)
        if item.get(u'strArtistWideThumb',u''):
            extradata = {}
            extradata[u'image'] = item[u'strArtistWideThumb']
            extradata[u'preview'] = item[u'strArtistWideThumb'] + u'/preview'
            extradata[u'aspect'] = u'landscape'
            extras.append(extradata)
        if item.get(u'strArtistBanner',u''):
            extradata = {}
            extradata[u'image'] = item[u'strArtistBanner']
            extradata[u'preview'] = item[u'strArtistBanner'] + u'/preview'
            extradata[u'aspect'] = u'banner'
            extras.append(extradata)
        if extras:
            artistdata[u'extras'] = extras
        return artistdata

def theaudiodb_artistalbums(data):
    albums = []
    albumlist = data.get(u'album',[])
    if albumlist:
        for item in data.get(u'album',[]):
            albumdata = {}
            albumdata[u'title'] = item[u'strAlbum']
            albumdata[u'year'] = item.get(u'intYearReleased', u'')
            albums.append(albumdata)
    return albums
