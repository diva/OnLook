#!/bin/bash

SAVEDIR=`pwd`

SOURCE="${BASH_SOURCE[0]}"
DIR="$( dirname "$SOURCE" )"
while [ -h "$SOURCE" ]
do 
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
  DIR="$( cd -P "$( dirname "$SOURCE"  )" && pwd )"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
echo $DIR

cd $SAVEDIR

for TRANSLATION_LANGUAGE in fr es it pt de
do
  cd $DIR/../indra/newview/skins/default/xui/$TRANSLATION_LANGUAGE || exit 1;
  dos2unix *
  sed -i '1 s/^\xef\xbb\xbf//' *
  chmod 644 *
  cd $SAVEDIR
done

