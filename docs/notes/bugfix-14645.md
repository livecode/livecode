# Field tab alignments in htmlText and styledText

The **styledText** and **htmlText** of a field now include tab
alignment information.  The **htmlText** uses a new `tabalign`
attribute with a list of alignments, e.g.

    <p tabalign='left,center,right'>left&09;middle&09;right&09;</p>

The **styledText** stores tab alignment in a "tabalign" key in each
paragraph's "style" array, e.g.

    get tStyledText[1]["style"]["tabalign"]
