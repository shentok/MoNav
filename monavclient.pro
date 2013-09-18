TEMPLATE = subdirs
SUBDIRS = client plugins mapsforgereader mapsforgerenderer
client.depends = plugins mapsforgereader mapsforgerenderer
plugins.file = plugins/client_plugins.pro
mapsforgereader.file = lib/mapsforgereader/mapsforgereader.pro
mapsforgerenderer.file = lib/mapsforgerenderer/mapsforgerenderer.pro
