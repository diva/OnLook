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
#echo $DIR
cd $DIR/../indra/newview/

if command -v xmllint >/dev/null 2>&1 ; then
  LINT=xmllint
else
  echo 'Error - xmllint not found'
  exit 1
fi

ESTATUS=0

for FILE in `find . -name \*.xml`
do
  if ! $LINT $FILE >/dev/null 2>&1 ; then
     ESTATUS=1
     $LINT $FILE
  fi
done


cd $SAVEDIR

exit $ESTATUS

