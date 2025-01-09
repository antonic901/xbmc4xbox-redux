
def get_imdb_id(uniqueids):
    imdb_id = uniqueids.get(u'imdb')
    if not imdb_id or not imdb_id.startswith(u'tt'):
        return None
    return imdb_id

# example format for scraper results
_ScraperResults = set([
    u'info',
    u'ratings',
    u'uniqueids',
    u'cast',
    u'available_art',
    u'error',
    u'warning'])
