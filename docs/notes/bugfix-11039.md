# Throw error when changing behavior from behavior script

Previously it was theoretically possible to change the behavior of an 
object from that object's existing behavior script. This will now
result in an execution error

	parentScript: can't change parent while parent script is executing
	
This change was necessarily as the engine would occasionally crash when
changing a behavior this way, and would be guaranteed to crash if 
stepping over the behavior script line that changes the behavior.