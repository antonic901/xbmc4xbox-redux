# -*- coding: utf-8 -*-

def theaudiodb_albumdetails(data):
    if data.get(u'album'):
        item = data[u'album'][0]
        albumdata = {}
        albumdata[u'album'] = item[u'strAlbum']
        if item.get(u'intYearReleased',u''):
            albumdata[u'year'] = item[u'intYearReleased']
        if item.get(u'strStyle',u''):
            albumdata[u'styles'] = item[u'strStyle']
        if item.get(u'strGenre',u''):
            albumdata[u'genre'] = item[u'strGenre']
        if item.get(u'strLabel',u''):
            albumdata[u'label'] = item[u'strLabel']
        if item.get(u'strReleaseFormat',u''):
            albumdata[u'type'] = item[u'strReleaseFormat']
        if item.get(u'intScore',u''):
            albumdata[u'rating'] = unicode(int(float(item[u'intScore']) + 0.5))
        if item.get(u'intScoreVotes',u''):
            albumdata[u'votes'] = item[u'intScoreVotes']
        if item.get(u'strMood',u''):
            albumdata[u'moods'] = item[u'strMood']
        if item.get(u'strTheme',u''):
            albumdata[u'themes'] = item[u'strTheme']
        if item.get(u'strMusicBrainzID',u''):
            albumdata[u'mbreleasegroupid'] = item[u'strMusicBrainzID']
        # api inconsistent
        if item.get(u'strDescription',u''):
            albumdata[u'descriptionEN'] = item[u'strDescription']
        elif item.get(u'strDescriptionEN',u''):
            albumdata[u'descriptionEN'] = item[u'strDescriptionEN']
        if item.get(u'strDescriptionDE',u''):
            albumdata[u'descriptionDE'] = item[u'strDescriptionDE']
        if item.get(u'strDescriptionFR',u''):
            albumdata[u'descriptionFR'] = item[u'strDescriptionFR']
        if item.get(u'strDescriptionCN',u''):
            albumdata[u'descriptionCN'] = item[u'strDescriptionCN']
        if item.get(u'strDescriptionIT',u''):
            albumdata[u'descriptionIT'] = item[u'strDescriptionIT']
        if item.get(u'strDescriptionJP',u''):
            albumdata[u'descriptionJP'] = item[u'strDescriptionJP']
        if item.get(u'strDescriptionRU',u''):
            albumdata[u'descriptionRU'] = item[u'strDescriptionRU']
        if item.get(u'strDescriptionES',u''):
            albumdata[u'descriptionES'] = item[u'strDescriptionES']
        if item.get(u'strDescriptionPT',u''):
            albumdata[u'descriptionPT'] = item[u'strDescriptionPT']
        if item.get(u'strDescriptionSE',u''):
            albumdata[u'descriptionSE'] = item[u'strDescriptionSE']
        if item.get(u'strDescriptionNL',u''):
            albumdata[u'descriptionNL'] = item[u'strDescriptionNL']
        if item.get(u'strDescriptionHU',u''):
            albumdata[u'descriptionHU'] = item[u'strDescriptionHU']
        if item.get(u'strDescriptionNO',u''):
            albumdata[u'descriptionNO'] = item[u'strDescriptionNO']
        if item.get(u'strDescriptionIL',u''):
            albumdata[u'descriptionIL'] = item[u'strDescriptionIL']
        if item.get(u'strDescriptionPL',u''):
            albumdata[u'descriptionPL'] = item[u'strDescriptionPL']
        if item.get(u'strArtist',u''):
            albumdata[u'artist_description'] = item[u'strArtist']
            artists = []
            artistdata = {}
            artistdata[u'artist'] = item[u'strArtist']
            if item.get(u'strMusicBrainzArtistID',u''):
                artistdata[u'mbartistid'] = item[u'strMusicBrainzArtistID']
            artists.append(artistdata)
            albumdata[u'artist'] = artists
        thumbs = []
        extras = []
        if item.get(u'strAlbumThumb',u''):
            thumbdata = {}
            thumbdata[u'image'] = item[u'strAlbumThumb']
            thumbdata[u'preview'] = item[u'strAlbumThumb'] + u'/preview'
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
        if item.get(u'strAlbumThumbBack',u''):
            extradata = {}
            extradata[u'image'] = item[u'strAlbumThumbBack']
            extradata[u'preview'] = item[u'strAlbumThumbBack'] + u'/preview'
            extradata[u'aspect'] = u'back'
            extras.append(extradata)
        if item.get(u'strAlbumSpine',u''):
            extradata = {}
            extradata[u'image'] = item[u'strAlbumSpine']
            extradata[u'preview'] = item[u'strAlbumSpine'] + u'/preview'
            extradata[u'aspect'] = u'spine'
            extras.append(extradata)
        if item.get(u'strAlbumCDart',u''):
            extradata = {}
            extradata[u'image'] = item[u'strAlbumCDart']
            extradata[u'preview'] = item[u'strAlbumCDart'] + u'/preview'
            extradata[u'aspect'] = u'discart'
            extras.append(extradata)
        if item.get(u'strAlbum3DCase',u''):
            extradata = {}
            extradata[u'image'] = item[u'strAlbum3DCase']
            extradata[u'preview'] = item[u'strAlbum3DCase'] + u'/preview'
            extradata[u'aspect'] = u'3dcase'
            extras.append(extradata)
        if item.get(u'strAlbum3DFlat',u''):
            extradata = {}
            extradata[u'image'] = item[u'strAlbum3DFlat']
            extradata[u'preview'] = item[u'strAlbum3DFlat'] + u'/preview'
            extradata[u'aspect'] = u'3dflat'
            extras.append(extradata)
        if item.get(u'strAlbum3DFace',u''):
            extradata = {}
            extradata[u'image'] = item[u'strAlbum3DFace']
            extradata[u'preview'] = item[u'strAlbum3DFace'] + u'/preview'
            extradata[u'aspect'] = u'3dface'
            extras.append(extradata)
        if thumbs:
            albumdata[u'thumb'] = thumbs
        if extras:
            albumdata[u'extras'] = extras
        return albumdata
