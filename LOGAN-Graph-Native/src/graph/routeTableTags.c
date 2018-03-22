
#include "common.h"

/*

Tags store per-route auxiliary information, beyond the bases within the sequence. In principle, each route can have zero or more bytes associated with it.
Interpretation of the meaning of these bytes, and offering high-level indexing based on them is a future task.

Requirements:

    Tags can be expected to be rare, therefore the feature should have low storage / processing overhead if not used in a given node or route.

    Tag Tables can be expected to store little or no information about most routes (<1 byte), but potentially a lot of information for a small minority of routes.

Limitations:

	During insertion, tags can only be added to the final route (i.e. node) in a sequence, in general, since the association of specific routes may not
	be possible until the final node/route has been determined.

Implementation:

	Low storage / processing overhead suggests storing tags in an optional tag table, which is separate but associated to an existing routing table.

    Empty case requires a 'negative' flag, preferably per routing table, to indicate the absence of the optional tag table.
    	For the RouteTableArray case, this can be one additional byte per node.
    	For the RouteTableTree case, this can be two additional entries in the top-level node.

	Tag tables need to be have positional synchronization with routing tables.

	During insertion, only 'ending' edges can have tags (edge zero and dangling edges). As such, only sequences to these edges need to be synchronized.








*/
