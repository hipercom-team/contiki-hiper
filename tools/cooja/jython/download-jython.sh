#! /bin/sh
#---------------------------------------------------------------------------
# Cedric Adjih - Inria - 2012-2013 
#---------------------------------------------------------------------------

PKG=jython-standalone-2.5.3.jar
JAR=jython.jar
URL=http://contiki-hiper.gforge.inria.fr/static/${PKG}
#URL="http://search.maven.org/remotecontent?filepath=org/python/jython-standalone/2.5.3/${PKG}"
WHERE=lib

test -e "${WHERE}/${JAR}" && exit 0
echo "Downloading jython"
cd ${WHERE} || exit 1
wget -nc ${URL} -O ${PKG} || exit 1
ln -sf ${PKG} ${JAR} || exit 1

#---------------------------------------------------------------------------
