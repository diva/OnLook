#!/bin/bash

SCRDIRECTORY="${0%%/*}"
SAVEDIR=`pwd`

for TRANSLATION_LANGUAGE in fr es 
do
  cd $SCRDIRECTORY/../indra/newview/skins/default/xui/$TRANSLATION_LANGUAGE
  dos2unix *
  sed -i '1 s/^\xef\xbb\xbf//' *
  chmod 644 *
  cd $SAVEDIR
done

