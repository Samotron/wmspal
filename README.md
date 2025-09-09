# WMSPal
You're friend for working with WMS services in c.

## What is it?
A c based cli app that wraps GDAL and lets you do the follow with wms tiles.

+ Download the tile as a georefernced image
+ Vectorise the georeferenced image
+ apply atribution to the vectorised areas based on getfeatureinfo queries.

## What does it use?

+ C11
+ vcpkg 
+ a modern cli 
+ GDAL 
+ curl for http
