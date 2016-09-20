<SPECIAL>@ {
	BEGIN(0);
    if (MoveToNextFile())
        return NEXT_UNIT;
    yyterminate();
}

