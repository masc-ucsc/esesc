/**[txh]********************************************************************

  Copyright (c) 2004 by Salvador E. Tropea.
  Covered by the GPL license.

  Module: Symbol query.
  Comments:
  GDB/MI commands for the "Symbol Query" section.@p

@<pre>
gdb command:              Implemented?
-symbol-info-address      N.A. (info address, human readable)
-symbol-info-file         N.A.
-symbol-info-function     N.A.
-symbol-info-line         N.A. (info line, human readable)
-symbol-info-symbol       N.A. (info symbol, human readable)
-symbol-list-functions    N.A. (info functions, human readable)
-symbol-list-types        N.A. (info types, human readable)
-symbol-list-variables    N.A. (info variables, human readable)
-symbol-list-lines        No (gdb 6.x)
-symbol-locate            N.A.
-symbol-type              N.A. (ptype, human readable)
@</pre>

Note:@p

Only one is implemented and not in gdb 5.x.@p

***************************************************************************/

#include "mi_gdb.h"

