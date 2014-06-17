/* Define header for application - AppBuilder 2.03  */

#if defined(__cplusplus)
extern "C" {
#endif

/* 'base' Window link */
extern const int ABN_base;
#define ABW_base                             AbGetABW( ABN_base )
extern const int ABN_StartStopButton;
#define ABW_StartStopButton                  AbGetABW( ABN_StartStopButton )
extern const int ABN_NodeNameInput;
#define ABW_NodeNameInput                    AbGetABW( ABN_NodeNameInput )
extern const int ABN_WorkNumberInput;
#define ABW_WorkNumberInput                  AbGetABW( ABN_WorkNumberInput )
extern const int ABN_PtLabelStatus;
#define ABW_PtLabelStatus                    AbGetABW( ABN_PtLabelStatus )

#define AbGetABW( n ) ( AbWidgets[ n ].wgt )

#define AB_OPTIONS "s:x:y:h:w:S:"

#if defined(__cplusplus)
}
#endif

