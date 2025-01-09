# -*- coding: utf-8 -*-

AUDIODBKEY = u'95424d43204d6564696538'
AUDIODBURL = u'https://www.theaudiodb.com/api/v1/json/%s/%s'
AUDIODBSEARCH = u'searchalbum.php?s=%s&a=%s'
AUDIODBDETAILS = u'album-mb.php?i=%s'

MUSICBRAINZURL = u'https://musicbrainz.org/ws/2/%s'
MUSICBRAINZSEARCH = u'release/?query=release:"%s"%%20AND%%20(artistname:"%s"%%20OR%%20artist:"%s")&fmt=json'
MUSICBRAINZLINKS = u'release-group/%s?inc=url-rels&fmt=json'
MUSICBRAINZDETAILS = u'release/%s?inc=release-groups+artists+labels+ratings&fmt=json'
MUSICBRAINZART = u'https://coverartarchive.org/release-group/%s'

DISCOGSKEY = u'zACPgktOmNegwbwKWMaC'
DISCOGSSECRET = u'wGuSOeMtfdkQxtERKQKPquyBwExSHdQq'
DISCOGSURL = u'https://api.discogs.com/%s'
DISCOGSSEARCH = u'database/search?release_title=%s&type=release&artist=%s&page=1&per_page=100&key=%s&secret=%s'
DISCOGSMASTER = u'masters/%s?key=%s&secret=%s'
DISCOGSDETAILS = u'releases/%s?key=%s&secret=%s'

ALLMUSICURL = u'https://www.allmusic.com/%s'
ALLMUSICSEARCH = u'search/albums/%s+%s'
ALLMUSICDETAILS = u'%s/releases'

FANARTVKEY = u'88ca41db0d6878929f1f9771eade41fd'
FANARTVURL = u'https://webservice.fanart.tv/v3/music/albums/%s?api_key=%s'

WIKIDATAURL = u'https://www.wikidata.org/wiki/Special:EntityData/%s.json'
WIKIPEDIAURL = u'https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=%s&formatversion=2&exsentences=10&exlimit=1&explaintext=1'
