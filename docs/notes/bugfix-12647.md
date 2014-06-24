# Multiple moves created whilst lock moves in effect fail to be synchronized.
Moves created inside a lock move would not start at the same time (i.e. at the point of unlock moves), instead they would drift by the time between the lock move and the execution of the move command.
