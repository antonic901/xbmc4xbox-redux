# -*- coding: utf-8 -*-

from __future__ import division
def musicbrainz_artistfind(data, artist):
    artists = []
    for item in data.get(u'artists',[]):
        artistdata = {}
        artistdata[u'artist'] = item[u'name']
        artistdata[u'thumb'] = u''
        artistdata[u'genre'] = u''
        artistdata[u'born'] = item[u'life-span'].get(u'begin', u'')
        if u'type' in item:
            artistdata[u'type'] = item[u'type']
        if u'gender' in item:
            artistdata[u'gender'] = item[u'gender']
        if u'disambiguation' in item:
            artistdata[u'disambiguation'] = item[u'disambiguation']
        artistdata[u'mbartistid'] = item[u'id']
        if item.get(u'score',1):
            artistdata[u'relevance'] = unicode(item[u'score'] / 100.00)
        artists.append(artistdata)
    return artists

def musicbrainz_artistdetails(data):
    artistdata = {}
    artistdata[u'artist'] = data[u'name']
    artistdata[u'mbartistid'] = data[u'id']
    artistdata[u'type'] = data[u'type']
    artistdata[u'gender'] = data[u'gender']
    artistdata[u'disambiguation'] = data[u'disambiguation']
    if data.get(u'life-span',u'') and data.get(u'type',u''):
        begin = data[u'life-span'].get(u'begin', u'')
        end = data[u'life-span'].get(u'end', u'')
        if data[u'type'] in [u'Group', u'Orchestra', u'Choir']:
            artistdata[u'formed'] = begin
            artistdata[u'disbanded'] = end
        elif data[u'type'] in [u'Person', u'Character']:
            artistdata[u'born'] = begin
            artistdata[u'died'] = end
    albums = []
    for item in data.get(u'release-groups',[]):
        albumdata = {}
        albumdata[u'title'] = item.get(u'title',u'')
        albumdata[u'year'] = item.get(u'first-release-date',u'')
        albumdata[u'musicbrainzreleasegroupid'] = item.get(u'id',u'')
        albums.append(albumdata)
    if albums:
        artistdata[u'albums'] = albums
    for item in data[u'relations']:
        if item[u'type'] == u'allmusic':
            artistdata[u'allmusic'] = item[u'url'][u'resource']
        elif item[u'type'] == u'discogs':
            dataid = item[u'url'][u'resource'].rsplit(u'/', 1)[1]
            artistdata[u'discogs'] = dataid
        elif item[u'type'] == u'wikidata':
            dataid = item[u'url'][u'resource'].rsplit(u'/', 1)[1]
            artistdata[u'wikidata'] = dataid
        elif item[u'type'] == u'wikipedia':
            dataid = item[u'url'][u'resource'].rsplit(u'/', 1)[1]
            artistdata[u'wikipedia'] = dataid
    return artistdata
