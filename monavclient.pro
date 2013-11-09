TEMPLATE = subdirs
SUBDIRS = client plugins mapsforgereader mapsforgerenderer osmarender
client.depends = plugins mapsforgereader mapsforgerenderer osmarender
plugins.file = plugins/client_plugins.pro
mapsforgereader.file = lib/mapsforgereader/mapsforgereader.pro
mapsforgerenderer.file = lib/mapsforgerenderer/mapsforgerenderer.pro
osmarender.file = lib/osmarender/osmarender.pro
