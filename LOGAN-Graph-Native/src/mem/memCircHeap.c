#include "common.h"


/*

	Multi-generation GC:

		Use of one circular buffer per generation (vs multiple blocks):
			Less end of block fragmentation
			No need to compact after partial spill to next generation

		Dead Marking:
			Avoids the need to calculate complete root set or indirect references during stats

	Remaining issues:
		Updating inbound pointers for a given block when copying
			Option A: Scan / Index all inbound pointers (root + indirect) and modify as moved
				Advantages: No persistent additional storage
				Disadvantages: Large effort required even for minor collections

			Option B: In-place Forwarding pointers with immediate scan & apply to inbound
				Advantages:
				Disadvantages:

			Option C: In-place Forwarding pointers with deferred apply
				Advantages:
				Disadvantages:

			Option D: Change List with deferred apply
				Advantages:
				Disadvantages:






	Optimization Goals:
	 	Majority of collections should be one (or few) generations
	 	Each generation should be sized to achieve similar survival percentages (e.g 10-20%)










 */
