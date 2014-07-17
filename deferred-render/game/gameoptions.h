#ifndef __GAMEOPTIONS_H__
#define __GAMEOPTIONS_H__

enum
{
	SHADOW_QUALITY_LOW = 0,
	SHADOW_QUALITY_MEDIUM,
	SHADOW_QUALITY_HIGH,

	NUM_SHADOW_QUALITIES
};

struct GameOptions
{
	bool		mbDrawPlayerView;
	bool		mbPlayerViewShadow;
	bool		mbMainViewShadow;

	int			miShadowQuality;
};

typedef struct GameOptions tGameOptions;

#endif // __GAMEOPTIONS_H__