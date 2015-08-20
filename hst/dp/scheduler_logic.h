
extern List_t * pxAllTasksList;

struct TaskInfo_DP {
	ListItem_t	xPromotionListItem;	// reference the task from the xPromotionList
	BaseType_t 	xPromotion;
	BaseType_t	xInUpperBand;
};
