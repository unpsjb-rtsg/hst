
extern List_t * pxAllTasksList;

struct TaskInfo_DP {
	ListItem_t	xPromotionListItem;	// reference the task from the xPromotionList.
	BaseType_t 	xPromotion;         // relative promotion time.
	BaseType_t	xInUpperBand;       // pdTRUE if the task is executing in the upper band.
};

typedef struct TaskInfo_DP TaskDp_t;
