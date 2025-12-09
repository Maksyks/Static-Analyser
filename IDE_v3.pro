TEMPLATE = subdirs
CONFIG  += ordered

SUBDIRS += app slice addrmap

QT += webenginewidgets webchannel
RESOURCES += app/resources.qrcs

# алиас "slice" указывает на каталог plugins/Slice
slice.subdir  = plugins/Slice
slice.depends = app   # <- гарантирует сборку app перед плагином

addrmap.subdir  = plugins/AddrMap
addrmap.depends = app
