#!/bin/bash

SCRDIRECTORY="${0%%/*}"
SAVEDIR=`pwd`

cd $SCRDIRECTORY/../indra/newview/skins/default/xui/fr
dos2unix *
sed -i '1 s/^\xef\xbb\xbf//' *
chmod 644 *

cd $SAVEDIR

