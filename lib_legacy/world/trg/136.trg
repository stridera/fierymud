#13601
Gremlin_pleeb_follow~
0 g 10
~
* This trigger was supposed to make the gremlin
* follow and group with his leader.  I think 
* perhaps this should be a special proc and
* not a trigger.  These notes are from copying
* the Mob progs to script format.
scratch
~
#13602
gremlin_sarg_group~
0 g 10
~
*
* This was a prog that was to make this mobile
* group his followers after they followed and
* consented to him.  It was messed up and didn't
* work.  Perhaps this should be compiled as a
* spec proc.  These notes exist for clarity of
* zone development in the future.
*
growl
~
#13603
east-blessing~
2 d 0
I pray for a blessing from mother earth, creator of life and bringer of death~
wecho The &9&bstone monoliths&0 begin to &6&bglow&0 with power.
wait 4s
wload obj 2330
wecho The &6&bpower&0 seems to coalesce into a &7&bshining ball&0.
wait 20s
wpurge
wecho The &7&bshining ball&0 dissapates back into nothingness.
wecho The &9&bstones&0 stop glowing.
~
$~
