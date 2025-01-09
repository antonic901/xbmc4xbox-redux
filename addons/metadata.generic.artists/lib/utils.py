# -*- coding: utf-8 -*-

AUDIODBKEY = u'95424d43204d6564696538'
AUDIODBURL = u'https://www.theaudiodb.com/api/v1/json/%s/%s'
AUDIODBSEARCH = u'search.php?s=%s'
AUDIODBDETAILS = u'artist-mb.php?i=%s'
AUDIODBDISCOGRAPHY = u'discography-mb.php?s=%s'

MUSICBRAINZURL = u'https://musicbrainz.org/ws/2/artist/%s'
MUSICBRAINZSEARCH = u'?query="%s"&fmt=json'
MUSICBRAINZDETAILS = u'%s?inc=url-rels+release-groups&type=album&fmt=json'

DISCOGSKEY = u'zACPgktOmNegwbwKWMaC'
DISCOGSSECRET = u'wGuSOeMtfdkQxtERKQKPquyBwExSHdQq'
DISCOGSURL = u'https://api.discogs.com/%s'
DISCOGSSEARCH = u'database/search?q=%s&type=artist&key=%s&secret=%s'
DISCOGSDETAILS = u'artists/%s?key=%s&secret=%s'
DISCOGSDISCOGRAPHY = u'artists/%s/releases?sort=format&page=1&per_page=100&key=%s&secret=%s'

ALLMUSICURL = u'https://www.allmusic.com/search/artists/%s'

FANARTVKEY = u'88ca41db0d6878929f1f9771eade41fd'
FANARTVURL = u'https://webservice.fanart.tv/v3/music/%s?api_key=%s'

WIKIDATAURL = u'https://www.wikidata.org/wiki/Special:EntityData/%s.json'
WIKIPEDIAURL = u'https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&titles=%s&formatversion=2&exsentences=10&exlimit=1&explaintext=1'
