#!/bin/bash
set -x

PKG_NAME=sleepydog
SPEC_FILE=${PKG_NAME}.spec
SPEC_GEN_FILE=${SPEC_FILE}.gen

SCRIPTDIR=$(cd `dirname "$0"` && pwd)

read -r VERSION BUILD commit <<< $(git describe --tag --long | sed 's/[v-]/ /g')
if [ -z "$VERSION" ] ; then
  VERSION=0.0.1
  BUILD=$(git log --pretty=format:'' | wc -l)
fi

mkdir -p ${SCRIPTDIR}/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

sed -e "s/__VERSION__/$VERSION/" -e "s/__BUILD__/$BUILD/" ${SCRIPTDIR}/${SPEC_GEN_FILE} > ${SCRIPTDIR}/SPECS/${SPEC_FILE}

# add changelog
date +"* %a %b %d %Y - commits one by one up to date" >> ${SCRIPTDIR}/SPECS/${SPEC_FILE}
git log --format="- %cd (%h) %s%d" --no-merges | sed -r 's/[0-9]+:[0-9]+:[0-9]+ //' >> ${SCRIPTDIR}/SPECS/${SPEC_FILE}

#create archve for rpmbuild
git archive --format=tar.gz --prefix=${PKG_NAME}-${VERSION}-${BUILD}/ -o ${SCRIPTDIR}/SOURCES/${PKG_NAME}-v${VERSION}-${BUILD}.tar.gz HEAD

#clear old rpms
rm -rf ${SCRIPTDIR}/RPMS/*.rpm
#build new one
rpmbuild -bb --define "_topdir ${SCRIPTDIR}" ${SCRIPTDIR}/SPECS/${SPEC_FILE}
