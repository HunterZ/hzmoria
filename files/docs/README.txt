FEATURES.txt
	mentions all of the important user visible changes from
	umoria 4.87 to umoria 5.x.

dragon.txt
	Gives spoiler info on dragons.

exp.txt
	Documents how experience works.  Should be part of the manual.

history.txt
	is a brief history of the program umoria.

moria.6
	is the nroff source for a man page for umoria.  Use the command
	'nroff -man moria.6' to view it.  By default, it assumes that
	umoria was compiled with the original command set.  If you
	compiled umoria with the rogue like command set, then remove
	the three characters defore the .ds command at the start for
	a more appropriate man page.

moria.man
	is an already processed copy of the man page (with bold and underlined
	characters removed) for those without the nroff program.

moria1.ms, moria2.ms
	is the document "The Dungeons of Moria".  It is a complete description
	of the game.  You may want to adjust the margin and line length
	values at the beginning.  To view it, use the command
	'tbl moria1.ms moria1.ms | nroff -ms'.  It is split into two parts
	so that it can be easily mailed.

moria1.txt, moria2.txt
	is an already processed copy of the document "The Dungeons of Moria"
	(with bold and underlined characters removed) for those without
	the nroff program.  It is split into two parts so that it can be
	easily mailed.

pronounc.txt
	explains how to pronounce the name moria.

spells.txt
	Documents how spells work.  Should be part of the manual.

where.txt
	gives info on how to obtain the various versions of moria.
