# mkfile for Plan9port
<$PLAN9/src/mkhdr

TARG=`ls *.[cy] | sed 's/\.[cy]//'`

<$PLAN9/src/mkmany

