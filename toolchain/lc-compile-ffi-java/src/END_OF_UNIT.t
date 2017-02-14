<<EOF>> {
	BEGIN (SPECIAL);
	unput('@');
	return END_OF_UNIT;
}

