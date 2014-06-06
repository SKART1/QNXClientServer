/* Event header for application - AppBuilder 2.03  */

#if defined(__cplusplus)
extern "C" {
#endif

static const ApEventLink_t AbApplLinks[] = {
	{ 3, 0, 0L, 0L, 0L, &base, NULL, NULL, 0, NULL, 0, 0, 0, 0, },
	{ 0 }
	};

static const ApEventLink_t AbLinks_base[] = {
	{ 8, 0, 0L, 0L, 0L, NULL, NULL, "StartStopButton", 2009, startStop, 0, 0, 0, 0, },
	{ 0 }
	};

const char ApOptions[] = AB_OPTIONS;

#if defined(__cplusplus)
}
#endif

