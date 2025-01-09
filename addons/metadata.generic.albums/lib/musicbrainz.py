# -*- coding: utf-8 -*-

from __future__ import division
def musicbrainz_albumfind(data, artist, album):
    albums = []
    # count how often each releasegroup occurs in the release results
    # keep track of the release with the highest score and earliest releasedate in each releasegroup
    releasegroups = {}
    for item in data.get(u'releases'):
        mbid = item[u'id']
        score = item.get(u'score', 0)
        releasegroup = item[u'release-group'][u'id']
        if u'date' in item and item[u'date']:
            date = item[u'date'].replace(u'-',u'')
            if len(date) == 4:
                date = date + u'9999'
        else:
            date = u'99999999'
        if releasegroup in releasegroups:
            count = releasegroups[releasegroup][0] + 1
            topmbid = releasegroups[releasegroup][1]
            topdate = releasegroups[releasegroup][2]
            topscore = releasegroups[releasegroup][3]
            if date < topdate and score >= topscore:
                topdate = date
                topmbid = mbid
            releasegroups[releasegroup] = [count, topmbid, topdate, topscore]
        else:
            releasegroups[releasegroup] = [1, mbid, date, score]
    if releasegroups:
        # get the highest releasegroup count
        maxcount = max(releasegroups.values())[0]
        # get the releasegroup(s) that match this highest value
        topgroups = [k for k, v in releasegroups.items() if v[0] == maxcount]
    for item in data.get(u'releases'):
        # only use the 'top' release from each releasegroup
        if item[u'id'] != releasegroups[item[u'release-group'][u'id']][1]:
            continue
        albumdata = {}
        if item.get(u'artist-credit'):
            artists = []
            artistdisp = u""
            for artist in item[u'artist-credit']:
                artistdata = {}
                artistdata[u'artist'] = artist[u'artist'][u'name']
                artistdata[u'mbartistid'] = artist[u'artist'][u'id']
                artistdata[u'artistsort'] = artist[u'artist'][u'sort-name']
                artistdisp = artistdisp + artist[u'artist'][u'name']
                artistdisp = artistdisp + artist.get(u'joinphrase', u'')
                artists.append(artistdata)
            albumdata[u'artist'] = artists
            albumdata[u'artist_description'] = artistdisp
        if item.get(u'label-info',u'') and item[u'label-info'][0].get(u'label',u'') and item[u'label-info'][0][u'label'].get(u'name',u''):
            albumdata[u'label'] = item[u'label-info'][0][u'label'][u'name']
        albumdata[u'album'] = item[u'title']
        if item.get(u'date',u''):
            albumdata[u'year'] = item[u'date'][:4]
        albumdata[u'thumb'] = u'https://coverartarchive.org/release-group/%s/front-250' % item[u'release-group'][u'id']
        if item.get(u'label-info',u'') and item[u'label-info'][0].get(u'label',u'') and item[u'label-info'][0][u'label'].get(u'name',u''):
            albumdata[u'label'] = item[u'label-info'][0][u'label'][u'name']
        if item.get(u'status',u''):
            albumdata[u'releasestatus'] = item[u'status']
        albumdata[u'type'] = item[u'release-group'].get(u'primary-type')
        albumdata[u'mbalbumid'] = item[u'id']
        albumdata[u'mbreleasegroupid'] = item[u'release-group'][u'id']
        if item.get(u'score'):
            releasescore = item[u'score'] / 100.0
            # if the release is in the releasegroup with most releases, it is considered the most accurate one
            # (this also helps with prefering official releases over bootlegs, assuming there are more variations of an official release than of a bootleg)
            if item[u'release-group'][u'id'] not in topgroups:
                releasescore -= 0.001
            # if the release is an album, prefer it over singles/ep's
            # (this needs to be the double of the above, as me might have just given the album a lesser score if the single happened to be in the topgroup)
            if item[u'release-group'].get(u'primary-type') != u'Album':
                releasescore -= 0.002
            albumdata[u'relevance'] = unicode(releasescore)
        albums.append(albumdata)
    return albums

def musicbrainz_albumlinks(data):
    albumlinks = {}
    if u'relations' in data and data[u'relations']:
        for item in data[u'relations']:
            if item[u'type'] == u'allmusic':
                albumlinks[u'allmusic'] = item[u'url'][u'resource']
            elif item[u'type'] == u'discogs':
                albumlinks[u'discogs'] = item[u'url'][u'resource'].rsplit(u'/', 1)[1]
            elif item[u'type'] == u'wikipedia':
                albumlinks[u'wikipedia'] = item[u'url'][u'resource'].rsplit(u'/', 1)[1]
            elif item[u'type'] == u'wikidata':
                albumlinks[u'wikidata'] = item[u'url'][u'resource'].rsplit(u'/', 1)[1]
    return albumlinks

def musicbrainz_albumdetails(data):
    albumdata = {}
    albumdata[u'album'] = data[u'title']
    albumdata[u'mbalbumid'] = data[u'id']
    if data.get(u'release-group',u''):
        albumdata[u'mbreleasegroupid'] = data[u'release-group'][u'id']
        if data[u'release-group'][u'rating'] and data[u'release-group'][u'rating'][u'value']:
            albumdata[u'rating'] = unicode(int((float(data[u'release-group'][u'rating'][u'value']) * 2) + 0.5))
            albumdata[u'votes'] = unicode(data[u'release-group'][u'rating'][u'votes-count'])
        if data[u'release-group'].get(u'primary-type'):
            albumtypes = [data[u'release-group'][u'primary-type']] + data[u'release-group'][u'secondary-types']
            albumdata[u'type'] = u' / '.join(albumtypes)
            if u'Compilation' in albumtypes:
                albumdata[u'compilation'] = u'true'
        if data[u'release-group'].get(u'first-release-date',u''):
            albumdata[u'originaldate'] = data[u'release-group'][u'first-release-date']
    if data.get(u'release-events',u''):
        albumdata[u'year'] = data[u'release-events'][0][u'date'][:4]
        albumdata[u'releasedate'] = data[u'release-events'][0][u'date']
    if data.get(u'label-info',u'') and data[u'label-info'][0].get(u'label',u'') and data[u'label-info'][0][u'label'].get(u'name',u''):
        albumdata[u'label'] = data[u'label-info'][0][u'label'][u'name']
    if data.get(u'status',u''):
        albumdata[u'releasestatus'] = data[u'status']
    if data.get(u'artist-credit'):
        artists = []
        artistdisp = u''
        for artist in data[u'artist-credit']:
            artistdata = {}
            artistdata[u'artist'] = artist[u'name']
            artistdata[u'mbartistid'] = artist[u'artist'][u'id']
            artistdata[u'artistsort'] = artist[u'artist'][u'sort-name']
            artistdisp = artistdisp + artist[u'name']
            artistdisp = artistdisp + artist.get(u'joinphrase', u'')
            artists.append(artistdata)
        albumdata[u'artist'] = artists
        albumdata[u'artist_description'] = artistdisp
    return albumdata

def musicbrainz_albumart(data):
    albumdata = {}
    thumbs = []
    extras = []
    for item in data[u'images']:
        if u'Front' in item[u'types']:
            thumbdata = {}
            thumbdata[u'image'] = item[u'image']
            thumbdata[u'preview'] = item[u'thumbnails'][u'small']
            thumbdata[u'aspect'] = u'thumb'
            thumbs.append(thumbdata)
        if u'Back' in item[u'types']:
            backdata = {}
            backdata[u'image'] = item[u'image']
            backdata[u'preview'] = item[u'thumbnails'][u'small']
            backdata[u'aspect'] = u'back'
            extras.append(backdata)
        if u'Medium' in item[u'types']:
            discartdata = {}
            discartdata[u'image'] = item[u'image']
            discartdata[u'preview'] = item[u'thumbnails'][u'small']
            discartdata[u'aspect'] = u'discart'
            extras.append(discartdata)
        # exculde spine+back images
        if u'Spine' in item[u'types'] and len(item[u'types']) == 1:
            spinedata = {}
            spinedata[u'image'] = item[u'image']
            spinedata[u'preview'] = item[u'thumbnails'][u'small']
            spinedata[u'aspect'] = u'spine'
            extras.append(spinedata)
    if thumbs:
        albumdata[u'thumb'] = thumbs
    if extras:
        albumdata[u'extras'] = extras
    return albumdata
