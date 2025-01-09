# -*- coding: utf-8 -*-
from __future__ import division
from __future__ import absolute_import
import difflib

def discogs_albumfind(data, artist, album):
    albums = []
    masters = []
    # sort results by lowest release id (first version of a release)
    releases = sorted(data.get(u'results',[]), key=lambda k: k[u'id']) 
    for item in releases:
        masterid = item[u'master_id']
        # we are not interested in multiple versions that belong to the same master release
        if masterid not in masters:
            masters.append(masterid)
            albumdata = {}
            albumdata[u'artist'] = item[u'title'].split(u' - ',1)[0]
            albumdata[u'album'] = item[u'title'].split(u' - ',1)[1]
            albumdata[u'artist_description'] = item[u'title'].split(u' - ',1)[0]
            albumdata[u'year'] = unicode(item.get(u'year', u''))
            albumdata[u'label'] = item[u'label'][0]
            albumdata[u'thumb'] = item[u'thumb']
            albumdata[u'dcalbumid'] = item[u'id']
            # discogs does not provide relevance, use our own
            artistmatch = difflib.SequenceMatcher(None, artist.lower(), albumdata[u'artist'].lower()).ratio()
            albummatch = difflib.SequenceMatcher(None, album.lower(), albumdata[u'album'].lower()).ratio()
            if artistmatch > 0.90 and albummatch > 0.90:
                score = round(((artistmatch + albummatch) / 2), 2)
                albumdata[u'relevance'] = unicode(score)
                albums.append(albumdata)
    return albums

def discogs_albummain(data):
    if data:
        if u'main_release_url' in data:
            url = data[u'main_release_url'].rsplit(u'/', 1)[1]
            return url

def discogs_albumdetails(data):
    albumdata = {}
    albumdata[u'album'] = data[u'title']
    if u'styles' in data:
        albumdata[u'styles'] = u' / '.join(data[u'styles'])
    albumdata[u'genres'] = u' / '.join(data[u'genres'])
    albumdata[u'year'] = unicode(data[u'year'])
    albumdata[u'label'] = data[u'labels'][0][u'name']
    artists = []
    for artist in data[u'artists']:
        artistdata = {}
        artistdata[u'artist'] = artist[u'name']
        artists.append(artistdata)
    albumdata[u'artist'] = artists
    albumdata[u'artist_description'] = data[u'artists_sort']
    albumdata[u'rating'] = unicode(int((float(data[u'community'][u'rating'][u'average']) * 2) + 0.5))
    albumdata[u'votes'] = unicode(data[u'community'][u'rating'][u'count'])
    if u'images' in data:
        thumbs = []
        for thumb in data[u'images']:
            thumbdata = {}
            thumbdata[u'image'] = thumb[u'uri']
            thumbdata[u'preview'] = thumb[u'uri150']
            # not accurate: discogs can provide any art type, there is no indication if it is an album front cover (thumb)
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
        albumdata[u'thumb'] = thumbs
    return albumdata
