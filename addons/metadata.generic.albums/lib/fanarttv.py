# -*- coding: utf-8 -*-

def fanarttv_albumart(data):
    if u'albums' in data:
        albumdata = {}
        thumbs = []
        extras = []
        discs = {}
        for mbid, art in data[u'albums'].items():
            if u'albumcover' in art:
                for thumb in art[u'albumcover']:
                    thumbdata = {}
                    thumbdata[u'image'] = thumb[u'url']
                    thumbdata[u'preview'] = thumb[u'url'].replace(u'/fanart/', u'/preview/')
                    thumbdata[u'aspect'] = u'thumb'
                    thumbs.append(thumbdata)
            if u'cdart' in art:
                for cdart in art[u'cdart']:
                    extradata = {}
                    extradata[u'image'] = cdart[u'url']
                    extradata[u'preview'] = cdart[u'url'].replace(u'/fanart/', u'/preview/')
                    extradata[u'aspect'] = u'discart'
                    extras.append(extradata)
                    # support for multi-disc albums
                    multidata = {}
                    num = cdart[u'disc']
                    multidata[u'image'] = cdart[u'url']
                    multidata[u'preview'] = cdart[u'url'].replace(u'/fanart/', u'/preview/')
                    multidata[u'aspect'] = u'discart%s' % num
                    if not num in discs:
                        discs[num] = [multidata]
                    else:
                        discs[num].append(multidata)
        if thumbs:
            albumdata[u'thumb'] = thumbs
        # only return for multi-discs, not single discs
        if len(discs) > 1:
            for k, v in discs.items():
                for item in v:
                    extras.append(item)
        if extras:
            albumdata[u'extras'] = extras
        return albumdata
