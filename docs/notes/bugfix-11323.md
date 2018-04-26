# New array commands `difference` and `symmetric difference`

The `difference` command removes all keys from the destination
which are present in the source, and leaves all others alone.

The `symmetric difference` command removes all keys from the
destination which are present in the source, and adds all keys
from the source which are not present in the destination.

Additionally the `into` clause has been added to all array set
set operations (`union`, `intersect`, `difference`, `symmetric difference`)
allowing commands such as:

   intersect tLeft with tRight into tResult

The operation of the commands is the same as the non-into form
except that tLeft does not have to be a variable, and the result
of the operation is placed into tResult rather than mutating
tLeft.
