#include "SigScan.h"
#include "discord_game_sdk.h"
#include "helpers.h"
#include <chrono>
#include <cstdio>
#include <thread>

typedef struct string {
	union {
		char *ptr;
		char data[16];
	} data;
	size_t length;
	size_t capacity;
} string;

typedef enum difficulty : u8 {
	Easy = 0,
	Normal = 1,
	Hard = 2,
	Extreme = 3,
} difficulty;

typedef enum extra : u8 {
	None = 0,
	Extra = 1,
} extra;

struct IDiscordCore *core = nullptr;
struct IDiscordActivityManager *activities = nullptr;

void
UpdateActivityCallback (void *, enum EDiscordResult result) {
	if (result == DiscordResult_Ok) {
		printf ("[discord] Activity updated successfully\n");
	} else {
		printf ("[discord] Activity update failed: %d\n", result);
	}
}

void
DiscordThread (void *) {
	while (true) {
		if (core != nullptr) {
			core->run_callbacks (core);
		}
		std::this_thread::sleep_for (std::chrono::seconds (1));
	}
}

string *songName = nullptr;

SIG_SCAN (
	sigSongStart, 0x14043B2D0,
	"\x8B\xD1\xE9\xA9\xE8\xFF\xFF\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xE9",
	"xxxxxxx?????????x");

SIG_SCAN (sigSongEnd, 0x14043B000,
		  "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8D\x0D\x00\x00\x00"
		  "\x00\xE8\x00\x00\x00\x00\x48\x8B\x3D\x00\x00\x00\x00",
		  "xxxx?xxxxxxxx????x????xxx????");

SIG_SCAN (sigCopySongTitle, 0x15E65CD80,
		  "\x40\x53\x48\x83\xEC\x30\x48\x89\xCB\xE8\x00\x00\x00\x00\x84\xC0",
		  "xxxxxxxxxx????xx");

void
CreateDiscord () {
	if (core != nullptr && activities != nullptr)
		return;
	struct DiscordCreateParams params = { 0 };
	DiscordCreateParamsSetDefault (&params);
	params.flags = DiscordCreateFlags_NoRequireDiscord;
	params.client_id = 1005179451572752404;
	EDiscordResult result = DiscordCreate (DISCORD_VERSION, &params, &core);
	if (result != DiscordResult_Ok)
		return;
	activities = core->get_activity_manager (core);
	_beginthread (DiscordThread, 20, NULL);
}

HOOK (void, __fastcall, SongStart, sigSongStart (), i32 id) {
	CreateDiscord ();
	if (activities == nullptr || core == nullptr) {
		return originalSongStart (id);
	}

	struct DiscordActivity activity;
	memset (&activity, 0, sizeof (activity));
	strcpy (activity.assets.large_image, "miku");
	strcpy (activity.assets.large_text, "Project DIVA MegaMix+");
	if (songName->capacity > 15) {
		strcpy (activity.state, songName->data.ptr);
	} else {
		strcpy (activity.state, songName->data.data);
	}
	switch (*(difficulty *)0x1416E2B90) {
	case Easy:
		strcpy (activity.assets.small_image, "easy");
		strcpy (activity.assets.small_text, "Easy");
		break;
	case Normal:
		strcpy (activity.assets.small_image, "normal");
		strcpy (activity.assets.small_text, "Normal");
		break;
	case Hard:
		strcpy (activity.assets.small_image, "hard");
		strcpy (activity.assets.small_text, "Hard");
		break;
	case Extreme:
		strcpy (activity.assets.small_image, "extreme");
		strcpy (activity.assets.small_text, "Extreme");
		break;
	}
	if (*(extra *)0x1416E2B94 == Extra) {
		strcpy (activity.assets.small_image, "extra");
		strcpy (activity.assets.small_text, "ExExtreme");
	}
	activity.timestamps.start
		= std::chrono::duration_cast<std::chrono::seconds> (
			  std::chrono::system_clock::now ().time_since_epoch ())
			  .count ();

	activities->update_activity (activities, &activity, 0,
								 UpdateActivityCallback);
	return originalSongStart (id);
}

HOOK (u8, __stdcall, CopySongTitle, sigCopySongTitle (), u64 a1) {
	u8 value = originalCopySongTitle (a1);
	songName = (string *)(a1 + 200);
	return value;
}

HOOK (__int64, __stdcall, SongEnd, sigSongEnd ()) {
	CreateDiscord ();
	if (activities == nullptr || core == nullptr) {
		return originalSongEnd ();
	}

	struct DiscordActivity activity;
	memset (&activity, 0, sizeof (activity));
	strcpy (activity.assets.large_image, "miku");
	strcpy (activity.assets.large_text, "Project DIVA MegaMix+");
	strcpy (activity.state, "In menu");
	activities->update_activity (activities, &activity, 0,
								 UpdateActivityCallback);
	return originalSongEnd ();
}

extern "C" __declspec(dllexport) void Init () {
	INSTALL_HOOK (SongStart);
	INSTALL_HOOK (SongEnd);
	INSTALL_HOOK (CopySongTitle);

	CreateDiscord ();
	if (activities == nullptr || core == nullptr) {
		return;
	}

	struct DiscordActivity activity;
	memset (&activity, 0, sizeof (activity));
	strcpy (activity.assets.large_image, "miku");
	strcpy (activity.assets.large_text, "Project DIVA MegaMix+");
	strcpy (activity.state, "In menu");
	activities->update_activity (activities, &activity, 0,
								 UpdateActivityCallback);
}
