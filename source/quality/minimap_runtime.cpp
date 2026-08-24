#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "minimap.hpp"
#include "follower_roster.hpp"
#include "minimap_chests.hpp"
#include "minimap_items.hpp"
#include "minimap_reveal.hpp"
#include "minimap_runtime.hpp"
#include "runtime_layout.hpp"

namespace
{
	namespace layout = quality::runtime::layout;
	constexpr double pi = 3.14159265358979323846;
	constexpr std::size_t relayStride = 256;
	constexpr std::size_t drawHookBytes = 14;
	constexpr std::size_t pingHookBytes = 23;

	constexpr std::uintptr_t drawMinimapRva = 0x00708070;
	constexpr std::uintptr_t drawTriangleRva = 0x007073F0;
	constexpr std::uintptr_t imageDrawRva = 0x008B3320;
	constexpr std::uintptr_t playerColorRva = 0x0086F6D0;
	constexpr std::uintptr_t playSoundRva = 0x00652800;
	constexpr std::uintptr_t loadMapRva = 0x004A7F10;
	constexpr std::uintptr_t languageGetRva = 0x004D37B0;
	constexpr std::uintptr_t createDialogueRva = 0x00840370;
	constexpr std::uintptr_t updateAllyFollowerFrameRva = 0x008859F0;
	constexpr std::uintptr_t getMonsterLocalizedNameRva = 0x00340E90;
	constexpr std::uintptr_t actPlayerRva = 0x00355FE0;
	constexpr std::uintptr_t actMonsterRva = 0x0032D4D0;
	constexpr std::uintptr_t actItemRva = 0x0031B480;
	constexpr std::uintptr_t actGoldBagRva = 0x0030D5B0;
	constexpr std::uintptr_t actColliderDecorationRva = 0x002F1A80;
	constexpr std::uintptr_t actCustomPortalRva = 0x0031F210;
	constexpr std::uintptr_t actWorkbenchRva = 0x002E2050;
	constexpr std::uintptr_t actCauldronRva = 0x002E14D0;
	constexpr std::uintptr_t lootBagColorCallRva = 0x00709DDD;
	constexpr std::uintptr_t lootBagColorblindCallRva = 0x00709E1F;
	constexpr std::uintptr_t pingColorBlockRva = 0x0070A609;
	constexpr std::uintptr_t pingColorBlockReturnRva = 0x0070A620;
	constexpr std::uintptr_t calloutColorCallRva = 0x0070AE75;
	constexpr std::uintptr_t terrainImageDrawCallRva = 0x00708EAF;
	constexpr std::uintptr_t worldTooltipHeightPrimaryRva = 0x008B000C;
	constexpr std::uintptr_t worldTooltipHeightAlternateRva = 0x008B1833;
	constexpr std::array<std::uintptr_t, 4> exitTooltipCallRvas = {
		0x005F451B, 0x005F4597, 0x005F460B, 0x005F4953,
	};
	constexpr std::array<std::uintptr_t, 4> headstoneDialogueCallRvas = {
		0x0030E1C2, 0x0030E2CD, 0x0030E3DA, 0x0030E4F8,
	};
	constexpr std::array<std::uintptr_t, 4> ghostColorCallRvas = {
		0x0070B58A, 0x0070B667, 0x0070B738, 0x0070B809,
	};
	constexpr std::array<std::uintptr_t, 4> ghostTriangleCallRvas = {
		0x0070B606, 0x0070B6DE, 0x0070B7AF, 0x0070B87B,
	};
	constexpr std::uintptr_t minimapTilesRva = 0x01010740;
	constexpr std::uintptr_t mapWidthRva = 0x010539B0;
	constexpr std::uintptr_t mapHeightRva = 0x010539B4;
	constexpr std::uintptr_t mapEntitiesRva = 0x01053A48;
	constexpr std::uintptr_t mapCreaturesRva = 0x01053A50;
	constexpr std::uintptr_t ticksRva = 0x010533E4;
	constexpr std::uintptr_t virtualScreenXPointerRva = 0x00BFEC60;
	constexpr std::uintptr_t virtualScreenYPointerRva = 0x00BFEC68;
	constexpr std::uintptr_t colorblindLobbyRva = 0x00ED1BA5;
	constexpr std::uintptr_t lootBagSpriteIndexRva = 0x01001DA0;
	constexpr std::uintptr_t lootBagSpriteVariationsRva = 0x01001DAC;
	constexpr std::uintptr_t multiplayerRva = 0x010538F4;
	constexpr std::uintptr_t clientDisconnectedRva = 0x01053908;
	constexpr std::uintptr_t currentLevelRva = 0x01053278;
	constexpr std::uintptr_t mapSeedRva = 0x01050740;
	constexpr std::uintptr_t secretLevelRva = 0x01050745;
	constexpr std::uintptr_t netServerRva = 0x0100F270;
	constexpr std::uintptr_t netClientsPointerRva = 0x0100F278;
	constexpr std::uintptr_t netSocketRva = 0x0100F280;
	constexpr std::uintptr_t udpRecvIatRva = 0x00ACCD58;
	constexpr std::uintptr_t udpSendIatRva = 0x00ACCD60;
	constexpr std::uintptr_t mapRva = mapWidthRva - 64;
	constexpr std::uintptr_t followerMenusRva = 0x00C28FB0;

	constexpr std::size_t entityUid = 0x68;
	constexpr std::size_t entityX = 0xD8;
	constexpr std::size_t entityY = 0xE0;
	constexpr std::size_t entityYaw = 0xF0;
	constexpr std::size_t entitySprite = 0x140;
	constexpr std::size_t entitySkill = 0x288;
	constexpr std::size_t entityFlags = 0x378;
	constexpr std::size_t entityChildren = 0x3A0;
	constexpr std::size_t entityParent = 0x3B0;
	constexpr std::size_t entityBehavior = 0x1350;
	constexpr std::size_t playerEntity = 0x18;
	constexpr std::size_t skillPlayerIndex = 2;
	constexpr std::size_t skillMonsterAllyIndex = 42;
	constexpr std::size_t skillShadowTaggedUid = 54;
	constexpr std::size_t skillShowOnMap = 59;
	constexpr std::size_t skillItemOriginalOwner = 21;
	constexpr std::size_t skillItemContainer = 29;
	constexpr std::size_t skillItemType = 10;
	constexpr std::size_t skillItemIdentified = 15;
	constexpr std::size_t worldUiPlayer = 0;
	constexpr std::size_t playerNumber = 0x10;
	constexpr std::size_t skillColliderMaxHp = 9;
	constexpr std::size_t skillColliderCurrentHp = 12;
	constexpr std::size_t skillColliderContainedEntity = 15;
	constexpr std::size_t skillChestVoidState = 17;
	constexpr std::size_t itemCount = 0x0A;
	constexpr std::size_t statType = 0xE0;
	constexpr std::size_t statName = 0xEC;
	constexpr std::size_t statHp = 0x220;
	constexpr std::size_t statMaxHp = 0x224;
	constexpr std::size_t statMp = 0x22C;
	constexpr std::size_t statMaxMp = 0x230;
	constexpr std::size_t statLevel = 0x250;
	constexpr std::size_t followerMenuStride = 0x100;
	constexpr std::size_t followerMenuRecentEntity = 0x08;
	constexpr std::size_t syntheticStatSize = 0x300;

	constexpr std::array<std::uint8_t, 32> drawMinimapSignature = {
		0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x18, 0x55,
		0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56,
		0x41, 0x57, 0x48, 0x8D, 0xA8, 0x78, 0xFE, 0xFF,
		0xFF, 0x48, 0x81, 0xEC, 0x50, 0x02, 0x00, 0x00,
	};
	constexpr std::array<std::uint8_t, 16> playerColorSignature = {
		0x48, 0x83, 0xEC, 0x48, 0x84, 0xD2, 0x74, 0x22,
		0x83, 0xF9, 0x07, 0x77, 0x69, 0x48, 0x63, 0xC1,
	};
	constexpr std::array<std::uint8_t, 16> imageDrawSignature = {
		0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74,
		0x24, 0x18, 0x55, 0x57, 0x41, 0x54, 0x41, 0x56,
	};
	constexpr std::array<std::uint8_t, 16> playSoundSignature = {
		0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x74,
		0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x48,
	};
	constexpr std::array<std::uint8_t, 21> loadMapSignature = {
		0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41,
		0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0xAC,
		0x24, 0xC8, 0xFB, 0xFF, 0xFF,
	};
	constexpr std::size_t loadMapHookBytes = loadMapSignature.size();
	constexpr std::array<std::uint8_t, 16> languageGetSignature = {
		0x89, 0x4C, 0x24, 0x08, 0x48, 0x83, 0xEC, 0x28,
		0x85, 0xC9, 0x79, 0x0C, 0x48, 0x8D, 0x05, 0x3A,
	};
	constexpr std::array<std::uint8_t, 16> createDialogueSignature = {
		0x48, 0x8B, 0xC4, 0x4C, 0x89, 0x48, 0x20, 0x55,
		0x53, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41,
	};
	constexpr std::array<std::uint8_t, 16> updateAllyFollowerFrameSignature = {
		0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x10, 0x48,
		0x89, 0x70, 0x18, 0x48, 0x89, 0x78, 0x20, 0x55,
	};
	constexpr std::array<std::uint8_t, 16> getMonsterLocalizedNameSignature = {
		0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C,
		0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57,
	};
	constexpr std::size_t updateAllyFollowerFrameHookBytes = 15;
	constexpr std::size_t getStatsHookBytes = 14;
	constexpr std::array<std::array<std::uint8_t, 5>, 4> exitTooltipCalls = {{
		{0xE8, 0x90, 0xF2, 0xED, 0xFF},
		{0xE8, 0x14, 0xF2, 0xED, 0xFF},
		{0xE8, 0xA0, 0xF1, 0xED, 0xFF},
		{0xE8, 0x58, 0xEE, 0xED, 0xFF},
	}};
	constexpr std::array<std::array<std::uint8_t, 5>, 4> headstoneDialogueCalls = {{
		{0xE8, 0xA9, 0x21, 0x53, 0x00},
		{0xE8, 0x9E, 0x20, 0x53, 0x00},
		{0xE8, 0x91, 0x1F, 0x53, 0x00},
		{0xE8, 0x73, 0x1E, 0x53, 0x00},
	}};
	constexpr std::array<std::uint8_t, 5> lootBagColorCall = {
		0xE8, 0xEE, 0x58, 0x16, 0x00,
	};
	constexpr std::array<std::uint8_t, 5> lootBagColorblindCall = {
		0xE8, 0xAC, 0x58, 0x16, 0x00,
	};
	constexpr std::array<std::uint8_t, 28> pingColorBlock = {
		0x49, 0xC1, 0xE8, 0x20, 0x41, 0x0F, 0xB6, 0xC8,
		0x45, 0x33, 0xC0, 0x0F, 0xB6, 0x15, 0x8A, 0x75,
		0x7C, 0x00, 0xE8, 0xB0, 0x50, 0x16, 0x00, 0x48,
		0x8D, 0x4C, 0x24, 0x44,
	};
	constexpr std::array<std::uint8_t, 5> calloutColorCall = {
		0xE8, 0x56, 0x48, 0x16, 0x00,
	};
	constexpr std::array<std::uint8_t, 5> terrainImageDrawCall = {
		0xE8, 0x6C, 0xA4, 0x1A, 0x00,
	};
	constexpr std::array<std::uint8_t, 46> worldTooltipHeightPrimary = {
		0x8B, 0x48, 0x30, 0x89, 0x4C, 0x24, 0x78, 0x48,
		0x8D, 0x8B, 0x48, 0x01, 0x00, 0x00, 0x48, 0x83,
		0x79, 0x18, 0x0F, 0x76, 0x03, 0x48, 0x8B, 0x09,
		0xE8, 0x07, 0x86, 0xF6, 0xFF, 0xB2, 0x01, 0x48,
		0x8B, 0xC8, 0xE8, 0x0D, 0x88, 0xF6, 0xFF, 0x83,
		0xC0, 0x08, 0x89, 0x44, 0x24, 0x7C,
	};
	constexpr std::array<std::uint8_t, 50> worldTooltipHeightAlternate = {
		0x8B, 0x40, 0x30, 0x89, 0x44, 0x24, 0x34, 0x89,
		0x44, 0x24, 0x78, 0x49, 0x8D, 0x8F, 0x48, 0x01,
		0x00, 0x00, 0x48, 0x83, 0x79, 0x18, 0x0F, 0x76,
		0x03, 0x48, 0x8B, 0x09, 0xE8, 0xDC, 0x6D, 0xF6,
		0xFF, 0xB2, 0x01, 0x48, 0x8B, 0xC8, 0xE8, 0xE2,
		0x6F, 0xF6, 0xFF, 0x83, 0xC0, 0x08, 0x89, 0x44,
		0x24, 0x7C,
	};
	constexpr std::array<std::array<std::uint8_t, 5>, 4> ghostColorCalls = {{
		{0xE8, 0x41, 0x41, 0x16, 0x00},
		{0xE8, 0x64, 0x40, 0x16, 0x00},
		{0xE8, 0x93, 0x3F, 0x16, 0x00},
		{0xE8, 0xC2, 0x3E, 0x16, 0x00},
	}};
	constexpr std::array<std::array<std::uint8_t, 5>, 4> ghostTriangleCalls = {{
		{0xE8, 0xE5, 0xBD, 0xFF, 0xFF},
		{0xE8, 0x0D, 0xBD, 0xFF, 0xFF},
		{0xE8, 0x3C, 0xBC, 0xFF, 0xFF},
		{0xE8, 0x70, 0xBB, 0xFF, 0xFF},
	}};

	struct Rect { int x; int y; int w; int h; };
	struct Node
	{
		Node* next;
		Node* previous;
		void* list;
		void* element;
		void (*deconstructor)(void*);
		std::uint32_t size;
	};
	struct List { Node* first; Node* last; };
	struct MsvcString
	{
		union
		{
			char inlineData[16];
			char* heapData;
		} storage {};
		std::uint64_t size = 0;
		std::uint64_t capacity = 15;
	};
	static_assert(sizeof(MsvcString) == 32);

	enum class VisualKind
	{
		Exit, Boulder, Workbench, Cauldron, Minotaur, ShadowCreature,
		Player, Follower, Unused,
	};

	struct Visual
	{
		VisualKind kind;
		double x;
		double y;
		double yaw;
		int owner;
		bool shadow;
	};

	struct Mutation
	{
		std::uint8_t* entity;
		std::uintptr_t behavior;
		std::int32_t sprite;
		std::int32_t showOnMap;
	};

	using DrawMinimapFn = void (*)(int, Rect, bool);
	using ImageDrawFn = void (*)(std::uint32_t, int, int, const Rect*, Rect,
		Rect, const std::uint32_t&);
	using DrawTriangleFn = void (*)(void*, double, double, double, double,
		Rect, std::uint32_t, bool);
	using PlayerColorFn = std::uint32_t (*)(int, bool, bool);
	using GetStatsFn = std::uint8_t* (*)(std::uint8_t*);
	using MonsterIsFriendlyForTooltipFn = bool (*)(int, std::uint8_t*);
	using PlaySoundFn = void (*)(int, int);
	using LoadMapFn = int (*)(const char*, void*, List*, List*, int*);
	using LanguageGetFn = const char* (*)(int);
	using CreateDialogueFn = void (*)(void*, std::uint32_t, int, const char*);
	using UdpSocket = void*;
	struct IpAddress { std::uint32_t host; std::uint16_t port; };
	struct UdpPacket
	{
		int channel;
		std::uint8_t* data;
		int len;
		int maxlen;
		int status;
		IpAddress address;
	};
	using UdpSendFn = int (*)(UdpSocket, int, UdpPacket*);
	using UdpRecvFn = int (*)(UdpSocket, UdpPacket*);
	using UpdateAllyFollowerFrameFn = void (*)(int);
	using GetMonsterLocalizedNameFn = MsvcString* (*)(MsvcString*, int, void*);
	using MsvcStringDestroyFn = void (*)(MsvcString*);
	using GlUseProgramFn = void (APIENTRY*)(GLuint);
	using GlCreateShaderFn = GLuint (APIENTRY*)(GLenum);
	using GlShaderSourceFn = void (APIENTRY*)(GLuint, GLsizei,
		const char* const*, const GLint*);
	using GlCompileShaderFn = void (APIENTRY*)(GLuint);
	using GlGetShaderivFn = void (APIENTRY*)(GLuint, GLenum, GLint*);
	using GlGetShaderInfoLogFn = void (APIENTRY*)(GLuint, GLsizei, GLsizei*, char*);
	using GlDeleteShaderFn = void (APIENTRY*)(GLuint);
	using GlCreateProgramFn = GLuint (APIENTRY*)();
	using GlAttachShaderFn = void (APIENTRY*)(GLuint, GLuint);
	using GlBindAttribLocationFn = void (APIENTRY*)(GLuint, GLuint, const char*);
	using GlLinkProgramFn = void (APIENTRY*)(GLuint);
	using GlGetProgramivFn = void (APIENTRY*)(GLuint, GLenum, GLint*);
	using GlGetProgramInfoLogFn = void (APIENTRY*)(GLuint, GLsizei, GLsizei*, char*);
	using GlDeleteProgramFn = void (APIENTRY*)(GLuint);
	using GlGenVertexArraysFn = void (APIENTRY*)(GLsizei, GLuint*);
	using GlBindVertexArrayFn = void (APIENTRY*)(GLuint);
	using GlGenBuffersFn = void (APIENTRY*)(GLsizei, GLuint*);
	using GlBindBufferFn = void (APIENTRY*)(GLenum, GLuint);
	using GlBufferDataFn = void (APIENTRY*)(GLenum, std::ptrdiff_t,
		const void*, GLenum);
	using GlEnableVertexAttribArrayFn = void (APIENTRY*)(GLuint);
	using GlVertexAttribPointerFn = void (APIENTRY*)(GLuint, GLint, GLenum,
		GLboolean, GLsizei, const void*);
	using GlGetUniformLocationFn = GLint (APIENTRY*)(GLuint, const char*);
	using GlUniform2fFn = void (APIENTRY*)(GLint, GLfloat, GLfloat);
	using GlUniform4fFn = void (APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
	using GlBlendFuncSeparateFn = void (APIENTRY*)(GLenum, GLenum, GLenum, GLenum);

	std::uint8_t* base = nullptr;
	DrawMinimapFn originalDrawMinimap = nullptr;
	ImageDrawFn imageDraw = nullptr;
	DrawTriangleFn drawTriangle = nullptr;
	PlayerColorFn originalPlayerColor = nullptr;
	GetStatsFn getStats = nullptr;
	MonsterIsFriendlyForTooltipFn monsterIsFriendlyForTooltip = nullptr;
	PlaySoundFn playSound = nullptr;
	LoadMapFn originalLoadMap = nullptr;
	LanguageGetFn languageGet = nullptr;
	CreateDialogueFn createDialogue = nullptr;
	UdpSendFn udpSend = nullptr;
	UdpRecvFn udpRecv = nullptr;
	UpdateAllyFollowerFrameFn originalUpdateAllyFollowerFrame = nullptr;
	GetStatsFn originalGetStats = nullptr;
	GetMonsterLocalizedNameFn getMonsterLocalizedName = nullptr;
	MsvcStringDestroyFn destroyMsvcString = nullptr;
	void* relayPage = nullptr;
	void* drawTrampoline = nullptr;
	void* loadMapTrampoline = nullptr;
	void* followerFrameTrampoline = nullptr;
	void* getStatsTrampoline = nullptr;
	Rect currentRect {};
	int currentViewer = 0;
	std::vector<Visual> visuals;
	std::vector<Mutation> mutations;
	std::uintptr_t lastEntityList = 0;
	std::uint32_t lastTicks = 0;
	bool minotaurAlerted = false;
	quality::minimap::reveal::State revealState;
	std::vector<quality::minimap::reveal::Candidate> revealCandidates;
	std::array<char, 512> exitTooltipText {};
	std::pair<int, int> synchronizedExitCounts {};
	bool synchronizedExitCountsValid = false;
	quality::minimap::items::State partyItemState;
	quality::minimap::chests::State chestState;
	std::unordered_set<std::uint32_t> seenWorldItems;
	bool inMinimapDraw = false;
	bool inFollowerRosterDraw = false;
	std::unordered_map<std::uint8_t*, std::array<std::uint8_t,
		syntheticStatSize>> syntheticFollowerStats;
	quality::follower_roster::State sharedFollowerRoster;
	quality::follower_roster::State publishedFollowerRoster;
	std::uint32_t lastFollowerRosterTick = 0;

	GlUseProgramFn useProgram = nullptr;
	GlCreateShaderFn createShader = nullptr;
	GlShaderSourceFn shaderSource = nullptr;
	GlCompileShaderFn compileShaderProcedure = nullptr;
	GlGetShaderivFn getShaderiv = nullptr;
	GlGetShaderInfoLogFn getShaderInfoLog = nullptr;
	GlDeleteShaderFn deleteShader = nullptr;
	GlCreateProgramFn createProgram = nullptr;
	GlAttachShaderFn attachShader = nullptr;
	GlBindAttribLocationFn bindAttribLocation = nullptr;
	GlLinkProgramFn linkProgram = nullptr;
	GlGetProgramivFn getProgramiv = nullptr;
	GlGetProgramInfoLogFn getProgramInfoLog = nullptr;
	GlDeleteProgramFn deleteProgram = nullptr;
	GlGenVertexArraysFn genVertexArrays = nullptr;
	GlBindVertexArrayFn bindVertexArray = nullptr;
	GlGenBuffersFn genBuffers = nullptr;
	GlBindBufferFn bindBuffer = nullptr;
	GlBufferDataFn bufferData = nullptr;
	GlEnableVertexAttribArrayFn enableVertexAttribArray = nullptr;
	GlVertexAttribPointerFn vertexAttribPointer = nullptr;
	GlGetUniformLocationFn getUniformLocation = nullptr;
	GlUniform2fFn uniform2f = nullptr;
	GlUniform4fFn uniform4f = nullptr;
	GlBlendFuncSeparateFn blendFuncSeparate = nullptr;
	HGLRC rendererContext = nullptr;
	GLuint vectorProgram = 0;
	GLuint vectorVao = 0;
	GLuint vectorVbo = 0;
	GLint viewportUniform = -1;
	GLint colorUniform = -1;

	template <typename T>
	T& field(std::uint8_t* object, const std::size_t offset)
	{
		return *reinterpret_cast<T*>(object + offset);
	}

	template <typename T>
	const T& field(const std::uint8_t* object, const std::size_t offset)
	{
		return *reinterpret_cast<const T*>(object + offset);
	}

	std::int32_t& skill(std::uint8_t* entity, const std::size_t index)
	{
		return field<std::int32_t>(entity, entitySkill + index * sizeof(std::int32_t));
	}

	std::uintptr_t behavior(const std::uint8_t* entity)
	{
		return field<std::uintptr_t>(entity, entityBehavior);
	}

	std::uint32_t uid(const std::uint8_t* entity)
	{
		return field<std::uint32_t>(entity, entityUid);
	}

	std::uint64_t addressKey(const IpAddress address)
	{
		return (static_cast<std::uint64_t>(address.host) << 16) | address.port;
	}

	bool sameAddress(const IpAddress left, const IpAddress right)
	{
		return left.host == right.host && left.port == right.port;
	}

	std::uint8_t* findEntity(const List* entities, const std::uint32_t entityUid)
	{
		if ( !entities || entityUid == 0 )
		{
			return nullptr;
		}
		for ( Node* node = entities->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( entity && uid(entity) == entityUid )
			{
				return entity;
			}
		}
		return nullptr;
	}

	enum class PacketKind : std::uint8_t
	{
		Ready = 1,
		Request = 2,
		Refresh = 3,
		Acknowledgement = 4,
		ItemPickedUp = 5,
		ItemDropped = 6,
		ItemRemoved = 7,
		ChestState = 8,
	};
	enum class RosterPacketKind : std::uint8_t
	{
		Upsert = 1,
		Remove = 2,
	};

	struct ItemPacketPayload
	{
		std::uint32_t markerId = 0;
		std::uint32_t entityUid = 0;
		std::uint32_t inventoryKey = 0;
		std::uint16_t x = 0;
		std::uint16_t y = 0;
	};

	constexpr std::size_t packetSize = 48;
	constexpr std::size_t rosterPacketSize = 184;
	struct PendingPacket
	{
		std::vector<std::uint8_t> bytes;
		IpAddress destination {};
		std::uint32_t createdTick = 0;
		std::uint32_t lastSentTick = 0;
	};

	std::uint32_t localGeneration = 0;
	std::uint32_t networkSequence = 1;
	std::unordered_map<std::uint32_t, PendingPacket> pendingPackets;
	std::unordered_map<std::uint64_t, std::uint32_t> clientGenerations;
	std::unordered_set<std::uint64_t> appliedPackets;
	bool flushingPackets = false;

	void writeU32(std::uint8_t* output, const std::size_t offset,
		const std::uint32_t value)
	{
		output[offset] = static_cast<std::uint8_t>(value >> 24);
		output[offset + 1] = static_cast<std::uint8_t>(value >> 16);
		output[offset + 2] = static_cast<std::uint8_t>(value >> 8);
		output[offset + 3] = static_cast<std::uint8_t>(value);
	}

	std::uint32_t readU32(const std::uint8_t* input, const std::size_t offset)
	{
		return (static_cast<std::uint32_t>(input[offset]) << 24)
			| (static_cast<std::uint32_t>(input[offset + 1]) << 16)
			| (static_cast<std::uint32_t>(input[offset + 2]) << 8)
			| static_cast<std::uint32_t>(input[offset + 3]);
	}

	void writeU16(std::uint8_t* output, const std::size_t offset,
		const std::uint16_t value)
	{
		output[offset] = static_cast<std::uint8_t>(value >> 8);
		output[offset + 1] = static_cast<std::uint8_t>(value);
	}

	std::uint16_t readU16(const std::uint8_t* input, const std::size_t offset)
	{
		return static_cast<std::uint16_t>(
			(static_cast<std::uint16_t>(input[offset]) << 8) | input[offset + 1]);
	}

	int multiplayerMode()
	{
		return *reinterpret_cast<int*>(base + multiplayerRva);
	}

	bool floorMatches(const std::uint8_t* bytes)
	{
		return static_cast<std::int32_t>(readU32(bytes, 16))
			== *reinterpret_cast<std::int32_t*>(base + currentLevelRva)
			&& readU32(bytes, 20) == *reinterpret_cast<std::uint32_t*>(
				base + mapSeedRva)
			&& (bytes[6] != 0) == (*reinterpret_cast<std::uint8_t*>(
				base + secretLevelRva) != 0);
	}

	std::array<std::uint8_t, packetSize> makePacket(const PacketKind kind,
		const std::uint32_t sequence, const std::uint32_t acknowledgement,
		const std::uint32_t generation,
		const ItemPacketPayload payload = {})
	{
		std::array<std::uint8_t, packetSize> bytes {};
		bytes[0] = 'Q'; bytes[1] = 'M'; bytes[2] = 'R'; bytes[3] = 'F';
		bytes[4] = 4;
		bytes[5] = static_cast<std::uint8_t>(kind);
		bytes[6] = *reinterpret_cast<std::uint8_t*>(base + secretLevelRva) ? 1 : 0;
		writeU32(bytes.data(), 8, sequence);
		writeU32(bytes.data(), 12, acknowledgement);
		writeU32(bytes.data(), 16, static_cast<std::uint32_t>(
			*reinterpret_cast<std::int32_t*>(base + currentLevelRva)));
		writeU32(bytes.data(), 20,
			*reinterpret_cast<std::uint32_t*>(base + mapSeedRva));
		writeU32(bytes.data(), 24, generation);
		writeU32(bytes.data(), 28, localGeneration);
		writeU32(bytes.data(), 32, payload.markerId);
		writeU32(bytes.data(), 36, payload.entityUid);
		writeU32(bytes.data(), 40, payload.inventoryKey);
		writeU16(bytes.data(), 44, payload.x);
		writeU16(bytes.data(), 46, payload.y);
		return bytes;
	}

	void sendBytes(const std::uint8_t* bytes, const std::size_t size,
		const IpAddress destination)
	{
		if ( !udpSend )
		{
			return;
		}
		UdpSocket socket = *reinterpret_cast<UdpSocket*>(base + netSocketRva);
		if ( !socket || destination.host == 0 || destination.port == 0 )
		{
			return;
		}
		UdpPacket packet {};
		packet.channel = -1;
		packet.data = const_cast<std::uint8_t*>(bytes);
		packet.len = static_cast<int>(size);
		packet.maxlen = packet.len;
		packet.address = destination;
		udpSend(socket, -1, &packet);
	}

	template <typename T>
	void sendBytes(const T& bytes, const IpAddress destination)
	{
		sendBytes(bytes.data(), bytes.size(), destination);
	}

	void queueReliable(const PacketKind kind, const IpAddress destination,
		const std::uint32_t generation,
		const ItemPacketPayload payload = {})
	{
		std::uint32_t sequence = networkSequence++;
		if ( sequence == 0 )
		{
			sequence = networkSequence++;
		}
		PendingPacket pending;
		const auto bytes = makePacket(kind, sequence, 0, generation, payload);
		pending.bytes.assign(bytes.begin(), bytes.end());
		pending.destination = destination;
		pending.createdTick = *reinterpret_cast<std::uint32_t*>(base + ticksRva);
		pendingPackets[sequence] = pending;
		sendBytes(pending.bytes, destination);
		pendingPackets[sequence].lastSentTick = pending.createdTick;
	}

	void sendAcknowledgement(const IpAddress destination,
		const std::uint32_t sequence)
	{
		sendBytes(makePacket(PacketKind::Acknowledgement, 0, sequence,
			localGeneration), destination);
	}

	std::array<std::uint8_t, rosterPacketSize> makeRosterPacket(
		const RosterPacketKind kind, const std::uint32_t sequence,
		const std::uint32_t generation,
		const quality::follower_roster::Entry& entry)
	{
		std::array<std::uint8_t, rosterPacketSize> bytes {};
		bytes[0] = 'Q'; bytes[1] = 'F'; bytes[2] = 'R'; bytes[3] = 'S';
		bytes[4] = 1;
		bytes[5] = static_cast<std::uint8_t>(kind);
		bytes[6] = *reinterpret_cast<std::uint8_t*>(base + secretLevelRva) ? 1 : 0;
		bytes[7] = static_cast<std::uint8_t>(entry.owner);
		writeU32(bytes.data(), 8, sequence);
		writeU32(bytes.data(), 16, static_cast<std::uint32_t>(
			*reinterpret_cast<std::int32_t*>(base + currentLevelRva)));
		writeU32(bytes.data(), 20,
			*reinterpret_cast<std::uint32_t*>(base + mapSeedRva));
		writeU32(bytes.data(), 24, generation);
		writeU32(bytes.data(), 32, entry.uid);
		writeU32(bytes.data(), 36, static_cast<std::uint32_t>(entry.level));
		writeU32(bytes.data(), 40, static_cast<std::uint32_t>(entry.hp));
		writeU32(bytes.data(), 44, static_cast<std::uint32_t>(entry.maxHp));
		writeU32(bytes.data(), 48, static_cast<std::uint32_t>(entry.type));
		writeU32(bytes.data(), 52, static_cast<std::uint32_t>(entry.model));
		writeU32(bytes.data(), 56, static_cast<std::uint32_t>(entry.order));
		const auto length = std::min(entry.name.size(), rosterPacketSize - 61);
		std::memcpy(bytes.data() + 60, entry.name.data(), length);
		bytes[60 + length] = 0;
		return bytes;
	}

	void queueRosterReliable(const RosterPacketKind kind,
		const IpAddress destination, const std::uint32_t generation,
		const quality::follower_roster::Entry& entry)
	{
		std::uint32_t sequence = networkSequence++;
		if ( sequence == 0 ) { sequence = networkSequence++; }
		const auto bytes = makeRosterPacket(kind, sequence, generation, entry);
		PendingPacket pending;
		pending.bytes.assign(bytes.begin(), bytes.end());
		pending.destination = destination;
		pending.createdTick = *reinterpret_cast<std::uint32_t*>(base + ticksRva);
		pending.lastSentTick = pending.createdTick;
		pendingPackets[sequence] = pending;
		sendBytes(pending.bytes, destination);
	}

	void flushPendingPackets()
	{
		if ( flushingPackets || !base )
		{
			return;
		}
		flushingPackets = true;
		const auto now = *reinterpret_cast<std::uint32_t*>(base + ticksRva);
		for ( auto packet = pendingPackets.begin(); packet != pendingPackets.end(); )
		{
			if ( now - packet->second.createdTick > 500U )
			{
				packet = pendingPackets.erase(packet);
				continue;
			}
			if ( now - packet->second.lastSentTick >= 15U )
			{
				sendBytes(packet->second.bytes, packet->second.destination);
				packet->second.lastSentTick = now;
			}
			++packet;
		}
		flushingPackets = false;
	}

	void refreshRevealSnapshot();
	std::pair<int, int> exitCreatureCountsForViewer(int viewer);

	ItemPacketPayload exitCountsPayload(const int viewer)
	{
		const auto counts = exitCreatureCountsForViewer(viewer);
		return {static_cast<std::uint32_t>(counts.first),
			static_cast<std::uint32_t>(counts.second)};
	}

	void broadcastToReadyClients(const PacketKind kind,
		const ItemPacketPayload payload = {})
	{
		if ( multiplayerMode() != 1 )
		{
			return;
		}
		auto* clients = *reinterpret_cast<IpAddress**>(base + netClientsPointerRva);
		const auto* disconnected = base + clientDisconnectedRva;
		if ( !clients )
		{
			return;
		}
		for ( int player = 1; player < 4; ++player )
		{
			if ( disconnected[player] )
			{
				continue;
			}
			const IpAddress destination = clients[player - 1];
			const auto generation = clientGenerations.find(addressKey(destination));
			if ( generation != clientGenerations.end() )
			{
				queueReliable(kind, destination, generation->second, payload);
			}
		}
	}

	void broadcastRosterEntry(const RosterPacketKind kind,
		const quality::follower_roster::Entry& entry)
	{
		if ( multiplayerMode() != 1 ) { return; }
		auto* clients = *reinterpret_cast<IpAddress**>(base + netClientsPointerRva);
		const auto* disconnected = base + clientDisconnectedRva;
		if ( !clients ) { return; }
		for ( int player = 1; player < quality::follower_roster::maximumPlayers;
			++player )
		{
			if ( disconnected[player] ) { continue; }
			const IpAddress destination = clients[player - 1];
			const auto generation = clientGenerations.find(addressKey(destination));
			if ( generation != clientGenerations.end() )
			{
				queueRosterReliable(kind, destination, generation->second, entry);
			}
		}
	}

	quality::follower_roster::State collectFollowerRoster()
	{
		quality::follower_roster::State result;
		auto** playerStats = reinterpret_cast<std::uint8_t**>(base + layout::statsRva);
		const auto* disconnected = base + clientDisconnectedRva;
		for ( int owner = 0; owner < quality::follower_roster::maximumPlayers;
			++owner )
		{
			if ( disconnected[owner] || !playerStats[owner] ) { continue; }
			auto& followers = field<List>(playerStats[owner], layout::statFollowers);
			int order = 0;
			for ( Node* node = followers.first; node; node = node->next, ++order )
			{
				auto* uidPointer = static_cast<std::uint32_t*>(node->element);
				auto* entity = uidPointer ? findEntity(
					*reinterpret_cast<List**>(base + mapCreaturesRva), *uidPointer)
					: nullptr;
				auto* stats = entity ? originalGetStats(entity) : nullptr;
				if ( !entity || !stats ) { continue; }
				quality::follower_roster::Entry entry;
				entry.uid = *uidPointer;
				entry.owner = owner;
				entry.level = field<std::int32_t>(stats, statLevel);
				entry.hp = field<std::int32_t>(stats, statHp);
				entry.maxHp = field<std::int32_t>(stats, statMaxHp);
				entry.type = field<std::int32_t>(stats, statType);
				entry.model = field<std::int32_t>(entity, entitySprite);
				entry.order = order;
				const char* name = reinterpret_cast<const char*>(stats + statName);
				entry.name.assign(name, strnlen(name, 127));
				result.upsert(entry);
			}
		}
		return result;
	}

	void refreshFollowerRoster()
	{
		if ( multiplayerMode() == layout::multiplayerClient ) { return; }
		const auto now = *reinterpret_cast<std::uint32_t*>(base + ticksRva);
		if ( now - lastFollowerRosterTick < 10U ) { return; }
		lastFollowerRosterTick = now;
		auto current = collectFollowerRoster();
		for ( const auto& pair : current.entries() )
		{
			const auto previous = publishedFollowerRoster.entries().find(pair.first);
			if ( previous == publishedFollowerRoster.entries().end()
				|| !(previous->second == pair.second) )
			{
				broadcastRosterEntry(RosterPacketKind::Upsert, pair.second);
			}
		}
		for ( const auto& pair : publishedFollowerRoster.entries() )
		{
			if ( current.entries().find(pair.first) == current.entries().end() )
			{
				broadcastRosterEntry(RosterPacketKind::Remove, pair.second);
			}
		}
		publishedFollowerRoster = current;
		sharedFollowerRoster = std::move(current);
	}

	ItemPacketPayload itemPayload(const quality::minimap::items::Marker& marker)
	{
		return {marker.markerId, marker.entityUid, marker.inventoryKey,
			static_cast<std::uint16_t>(std::clamp(marker.x, 0, 65535)),
			static_cast<std::uint16_t>(std::clamp(marker.y, 0, 65535))};
	}

	void broadcastItem(const PacketKind kind,
		const quality::minimap::items::Marker& marker)
	{
		if ( multiplayerMode() == 1 )
		{
			broadcastToReadyClients(kind, itemPayload(marker));
		}
	}

	ItemPacketPayload chestPayload(const quality::minimap::chests::Update& update)
	{
		return {update.uid, 0,
			(update.interacted ? 1U : 0U) | (update.nonempty ? 2U : 0U),
			static_cast<std::uint16_t>(std::clamp(update.x, 0, 65535)),
			static_cast<std::uint16_t>(std::clamp(update.y, 0, 65535))};
	}

	void broadcastChest(const quality::minimap::chests::Update& update)
	{
		if ( multiplayerMode() == 1 )
		{
			broadcastToReadyClients(PacketKind::ChestState, chestPayload(update));
		}
	}

	void requestLocalRefresh()
	{
		refreshRevealSnapshot();
		if ( multiplayerMode() == 2 )
		{
			queueReliable(PacketKind::Request,
				*reinterpret_cast<IpAddress*>(base + netServerRva), localGeneration);
		}
	}

	void resetRevealFloor()
	{
		revealState.reset();
		revealCandidates.clear();
		synchronizedExitCounts = {};
		synchronizedExitCountsValid = false;
		partyItemState.reset();
		chestState.reset();
		seenWorldItems.clear();
		sharedFollowerRoster.reset();
		publishedFollowerRoster.reset();
		syntheticFollowerStats.clear();
		lastFollowerRosterTick = 0;
		pendingPackets.clear();
		// A client can finish loading and announce readiness before the host.
		// Keep that announcement; floor identity still gates every packet.
		appliedPackets.clear();
		++localGeneration;
		if ( localGeneration == 0 )
		{
			++localGeneration;
		}
		if ( multiplayerMode() == 2 )
		{
			queueReliable(PacketKind::Ready,
				*reinterpret_cast<IpAddress*>(base + netServerRva), localGeneration);
		}
	}

	bool isQualityPacket(const UdpPacket* packet)
	{
		return packet && packet->data && packet->len == static_cast<int>(packetSize)
			&& std::memcmp(packet->data, "QMRF", 4) == 0
			&& packet->data[4] == 4
			&& packet->data[5] >= static_cast<std::uint8_t>(PacketKind::Ready)
			&& packet->data[5] <= static_cast<std::uint8_t>(PacketKind::ChestState);
	}

	bool isRosterPacket(const UdpPacket* packet)
	{
		return packet && packet->data
			&& packet->len == static_cast<int>(rosterPacketSize)
			&& std::memcmp(packet->data, "QFRS", 4) == 0
			&& packet->data[4] == 1
			&& packet->data[5] >= static_cast<std::uint8_t>(
				RosterPacketKind::Upsert)
			&& packet->data[5] <= static_cast<std::uint8_t>(
				RosterPacketKind::Remove);
	}

	void processRosterPacket(const UdpPacket* packet)
	{
		if ( !floorMatches(packet->data) ) { return; }
		const auto sequence = readU32(packet->data, 8);
		sendAcknowledgement(packet->address, sequence);
		const std::uint64_t delivery = addressKey(packet->address)
			^ (static_cast<std::uint64_t>(sequence) << 1);
		if ( !appliedPackets.insert(delivery).second ) { return; }
		if ( multiplayerMode() != layout::multiplayerClient
			|| !quality::minimap::reveal::acceptClientRefresh(sameAddress(
				packet->address, *reinterpret_cast<IpAddress*>(base + netServerRva)),
				true, readU32(packet->data, 24), localGeneration) )
		{
			return;
		}
		quality::follower_roster::Entry entry;
		entry.owner = packet->data[7];
		entry.uid = readU32(packet->data, 32);
		entry.level = static_cast<std::int32_t>(readU32(packet->data, 36));
		entry.hp = static_cast<std::int32_t>(readU32(packet->data, 40));
		entry.maxHp = static_cast<std::int32_t>(readU32(packet->data, 44));
		entry.type = static_cast<std::int32_t>(readU32(packet->data, 48));
		entry.model = static_cast<std::int32_t>(readU32(packet->data, 52));
		entry.order = static_cast<std::int32_t>(readU32(packet->data, 56));
		const char* name = reinterpret_cast<const char*>(packet->data + 60);
		entry.name.assign(name, strnlen(name, rosterPacketSize - 60));
		if ( static_cast<RosterPacketKind>(packet->data[5])
			== RosterPacketKind::Remove )
		{
			sharedFollowerRoster.erase(entry.uid);
		}
		else
		{
			sharedFollowerRoster.upsert(entry);
		}
	}

	void processQualityPacket(const UdpPacket* packet)
	{
		const auto kind = static_cast<PacketKind>(packet->data[5]);
		if ( kind == PacketKind::Acknowledgement )
		{
			const auto acknowledged = readU32(packet->data, 12);
			const auto pending = pendingPackets.find(acknowledged);
			if ( pending != pendingPackets.end()
				&& sameAddress(pending->second.destination, packet->address) )
			{
				pendingPackets.erase(pending);
			}
			return;
		}
		if ( !floorMatches(packet->data) )
		{
			return;
		}
		const auto sequence = readU32(packet->data, 8);
		sendAcknowledgement(packet->address, sequence);
		const std::uint64_t delivery = addressKey(packet->address)
			^ (static_cast<std::uint64_t>(sequence) << 1);
		if ( !appliedPackets.insert(delivery).second )
		{
			return;
		}
		const auto generation = readU32(packet->data, 24);
		if ( multiplayerMode() == 1
			&& (kind == PacketKind::Ready || kind == PacketKind::Request) )
		{
			auto* clients = *reinterpret_cast<IpAddress**>(base + netClientsPointerRva);
			int requestingPlayer = -1;
			if ( clients )
			{
				for ( int index = 0; index < 3; ++index )
				{
					if ( sameAddress(clients[index], packet->address) )
					{
						requestingPlayer = index + 1;
						break;
					}
				}
			}
			if ( requestingPlayer < 0 )
			{
				return;
			}
			auto& accepted = clientGenerations[addressKey(packet->address)];
			if ( kind == PacketKind::Ready )
			{
				// The same address can belong to a new session whose counter restarted.
				accepted = generation;
				for ( const auto& marker : partyItemState.markers() )
				{
					queueReliable(PacketKind::ItemDropped, packet->address,
						accepted, itemPayload(marker));
				}
				for ( const auto& update : chestState.updates() )
				{
					queueReliable(PacketKind::ChestState, packet->address,
						accepted, chestPayload(update));
				}
				for ( const auto& follower : publishedFollowerRoster.entries() )
				{
					queueRosterReliable(RosterPacketKind::Upsert, packet->address,
						accepted, follower.second);
				}
			}
			else if ( quality::minimap::reveal::acceptHostRequest(true, true,
				generation, accepted) )
			{
				queueReliable(PacketKind::Refresh, packet->address, accepted,
					exitCountsPayload(requestingPlayer));
			}
		}
		else if ( multiplayerMode() == 2
			&& (kind == PacketKind::Refresh || kind == PacketKind::ItemPickedUp
				|| kind == PacketKind::ItemDropped || kind == PacketKind::ItemRemoved
				|| kind == PacketKind::ChestState)
			&& quality::minimap::reveal::acceptClientRefresh(sameAddress(
				packet->address, *reinterpret_cast<IpAddress*>(base + netServerRva)),
				true, generation, localGeneration) )
		{
			if ( kind == PacketKind::Refresh )
			{
				synchronizedExitCounts = {
					static_cast<int>(readU32(packet->data, 32)),
					static_cast<int>(readU32(packet->data, 36)),
				};
				synchronizedExitCountsValid = true;
				refreshRevealSnapshot();
			}
			else
			{
				const ItemPacketPayload payload {
					readU32(packet->data, 32), readU32(packet->data, 36),
					readU32(packet->data, 40), readU16(packet->data, 44),
					readU16(packet->data, 46),
				};
				if ( kind == PacketKind::ChestState )
				{
					const std::uint32_t flags = payload.inventoryKey;
					const quality::minimap::chests::Update update {
						payload.markerId, payload.x, payload.y,
						(flags & 1U) != 0, (flags & 2U) != 0,
					};
					chestState.apply(update);
					if ( update.interacted )
					{
						revealState.markUsed(update.uid);
					}
				}
				else if ( kind == PacketKind::ItemDropped )
				{
					partyItemState.applyDrop(payload.markerId, payload.entityUid,
						payload.inventoryKey, payload.x, payload.y);
				}
				else if ( kind == PacketKind::ItemPickedUp )
				{
					partyItemState.pickUp(payload.entityUid, payload.inventoryKey);
					partyItemState.remove(payload.entityUid);
				}
				else
				{
					partyItemState.remove(payload.entityUid);
				}
			}
		}
	}

	int udpSendHook(UdpSocket socket, const int channel, UdpPacket* packet)
	{
		const int result = udpSend(socket, channel, packet);
		refreshFollowerRoster();
		flushPendingPackets();
		return result;
	}

	int udpRecvHook(UdpSocket socket, UdpPacket* packet)
	{
		for ( ;; )
		{
			const int result = udpRecv(socket, packet);
			if ( result <= 0 )
			{
				return result;
			}
			if ( isQualityPacket(packet) )
			{
				processQualityPacket(packet);
				continue;
			}
			if ( isRosterPacket(packet) )
			{
				processRosterPacket(packet);
				continue;
			}
			return result;
		}
	}

	std::uint8_t visibilityAt(const int x, const int y)
	{
		const int width = *reinterpret_cast<int*>(base + mapWidthRva);
		const int height = *reinterpret_cast<int*>(base + mapHeightRva);
		if ( x < 0 || y < 0 || x >= width || y >= height
			|| width > quality::minimap::dimension
			|| height > quality::minimap::dimension )
		{
			return 0;
		}
		const std::uint8_t value = *(base + minimapTilesRva
			+ quality::minimap::tileIndex(x, y));
		return value <= 4 ? value : 0;
	}

	std::vector<quality::minimap::reveal::Candidate> collectRevealCandidates(
		List* entities, const std::unordered_set<std::uint32_t>& partyUids)
	{
		using quality::minimap::reveal::Candidate;
		using quality::minimap::reveal::CandidateKind;
		std::vector<Candidate> candidates;
		if ( !entities )
		{
			return candidates;
		}
		const int lootBagIndex = *reinterpret_cast<int*>(
			base + lootBagSpriteIndexRva);
		const int lootBagVariations = *reinterpret_cast<int*>(
			base + lootBagSpriteVariationsRva);
		for ( Node* node = entities->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const auto entityUid = uid(entity);
			const auto action = behavior(entity);
			const int sprite = field<std::int32_t>(entity, entitySprite);
			Candidate candidate;
			candidate.uid = entityUid;
			candidate.x = static_cast<int>(std::floor(
				field<double>(entity, entityX) / 16.0));
			candidate.y = static_cast<int>(std::floor(
				field<double>(entity, entityY) / 16.0));

			if ( quality::minimap::isExitSprite(sprite)
				|| action == reinterpret_cast<std::uintptr_t>(base + actCustomPortalRva) )
			{
				candidate.kind = CandidateKind::Exit;
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actWorkbenchRva) )
			{
				candidate.kind = CandidateKind::Workbench;
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actCauldronRva) )
			{
				candidate.kind = CandidateKind::Cauldron;
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actGoldBagRva) )
			{
				candidate.kind = CandidateKind::Gold;
				candidate.available = quality::minimap::reveal::eligibleGroundGold(
					skill(entity, 0), static_cast<std::uint32_t>(skill(entity, 4)));
			}
			else if ( sprite == 188 || sprite == 1791 )
			{
				candidate.kind = CandidateKind::Chest;
				if ( skill(entity, 1) == 1 )
				{
					revealState.markUsed(entityUid);
				}
			}
			else if ( sprite == 224 )
			{
				candidate.kind = CandidateKind::Grave;
			}
			else if ( sprite == 163 )
			{
				candidate.kind = CandidateKind::Fountain;
				candidate.available = skill(entity, 0) > 0;
				revealState.observeUses(entityUid, skill(entity, 0));
			}
			else if ( sprite == 164 )
			{
				candidate.kind = CandidateKind::Sink;
				candidate.available = skill(entity, 0) > 0;
				revealState.observeUses(entityUid, skill(entity, 0));
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(
				base + actColliderDecorationRva) )
			{
				candidate.kind = CandidateKind::BreakableContainer;
				candidate.available = skill(entity, skillColliderMaxHp) > 0
					&& skill(entity, skillColliderCurrentHp) > 0;
				auto* contained = findEntity(entities, static_cast<std::uint32_t>(
					skill(entity, skillColliderContainedEntity)));
				candidate.hasLoot = contained
					&& (behavior(contained) == reinterpret_cast<std::uintptr_t>(
						base + actItemRva)
						|| behavior(contained) == reinterpret_cast<std::uintptr_t>(
							base + actGoldBagRva));
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actItemRva)
				&& skill(entity, skillItemContainer) == 0 )
			{
				candidate.kind = CandidateKind::Item;
				candidate.identified = skill(entity, skillItemIdentified) != 0;
				candidate.available = quality::minimap::reveal::eligibleRevealedGroundItem(
					skill(entity, skillItemType), candidate.identified);
				const auto originalOwner = static_cast<std::uint32_t>(
					skill(entity, skillItemOriginalOwner));
				const auto parent = field<std::uint32_t>(entity, entityParent);
				const bool continuedGreenItem =
					partyItemState.isTrackedOrdinary(entityUid);
				candidate.playerOwned = !continuedGreenItem
					&& (partyUids.count(originalOwner) != 0
						|| partyUids.count(parent) != 0
						|| partyItemState.isPartyDropped(entityUid));
				candidate.lootBag = sprite >= lootBagIndex
					&& sprite < lootBagIndex + lootBagVariations;
			}
			if ( candidate.kind != CandidateKind::None )
			{
				candidates.push_back(candidate);
			}
		}
		return candidates;
	}

	std::vector<quality::minimap::chests::Observation> collectChestObservations(
		List* entities, const bool inspectInventories)
	{
		using quality::minimap::chests::Observation;
		std::vector<Observation> observations;
		if ( !entities )
		{
			return observations;
		}
		for ( Node* node = entities->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const int sprite = field<std::int32_t>(entity, entitySprite);
			if ( !quality::minimap::chests::eligibleOrdinary(
				sprite == 188 || sprite == 1791,
				skill(entity, skillChestVoidState)) )
			{
				continue;
			}
			Observation observation;
			observation.uid = uid(entity);
			observation.x = static_cast<int>(std::floor(
				field<double>(entity, entityX) / 16.0));
			observation.y = static_cast<int>(std::floor(
				field<double>(entity, entityY) / 16.0));
			observation.open = skill(entity, 1) == 1;
			if ( inspectInventories )
			{
				const auto& children = field<List>(entity, entityChildren);
				auto* inventory = children.first && children.first->element
					? static_cast<List*>(children.first->element) : nullptr;
				std::int64_t total = 0;
				for ( Node* itemNode = inventory ? inventory->first : nullptr;
					itemNode; itemNode = itemNode->next )
				{
					auto* item = static_cast<std::uint8_t*>(itemNode->element);
					if ( item )
					{
						total += std::max<int>(0, field<std::int16_t>(item, itemCount));
					}
				}
				observation.itemCount = static_cast<int>(std::min<std::int64_t>(
					total, static_cast<std::int64_t>(INT_MAX)));
			}
			observations.push_back(observation);
		}
		return observations;
	}

	void observeChests(List* entities)
	{
		const bool authoritative = multiplayerMode() != 2;
		const auto observations = collectChestObservations(entities, authoritative);
		if ( !authoritative )
		{
			chestState.observeLive(observations);
			return;
		}
		for ( const auto& update : chestState.observeAuthoritative(observations) )
		{
			revealState.markUsed(update.uid);
			broadcastChest(update);
		}
	}

	void refreshRevealSnapshot()
	{
		auto* entities = *reinterpret_cast<List**>(base + mapEntitiesRva);
		auto* creatures = *reinterpret_cast<List**>(base + mapCreaturesRva);
		std::unordered_set<std::uint32_t> partyUids;
		if ( creatures )
		{
			for ( Node* node = creatures->first; node; node = node->next )
			{
				auto* entity = static_cast<std::uint8_t*>(node->element);
				if ( !entity )
				{
					continue;
				}
				const auto action = behavior(entity);
				if ( action == reinterpret_cast<std::uintptr_t>(base + actPlayerRva)
					|| (action == reinterpret_cast<std::uintptr_t>(base + actMonsterRva)
						&& skill(entity, skillMonsterAllyIndex) >= 0) )
				{
					partyUids.insert(uid(entity));
				}
			}
		}
		revealCandidates = collectRevealCandidates(entities, partyUids);
		revealState.refresh(revealCandidates);
	}

	int loadMapHook(const char* filename, void* destination, List* entities,
		List* creatures, int* mapHash)
	{
		const int result = originalLoadMap(filename, destination, entities,
			creatures, mapHash);
		if ( result != -1 && destination == base + mapRva )
		{
			resetRevealFloor();
			auto* loadedEntities = *reinterpret_cast<List**>(base + mapEntitiesRva);
			collectRevealCandidates(loadedEntities, {});
		}
		return result;
	}

	std::pair<int, int> exitCreatureCountsForViewer(const int viewer)
	{
		int hostiles = 0;
		int neutrals = 0;
		if ( viewer < 0 || viewer >= 4 || !getStats
			|| !monsterIsFriendlyForTooltip )
		{
			return {hostiles, neutrals};
		}
		auto* creatures = *reinterpret_cast<List**>(base + mapCreaturesRva);
		if ( !creatures )
		{
			return {hostiles, neutrals};
		}
		for ( Node* node = creatures->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const bool monster = behavior(entity)
				== reinterpret_cast<std::uintptr_t>(base + actMonsterRva);
			auto* stats = monster ? getStats(entity) : nullptr;
			const int hp = stats
				? field<std::int32_t>(stats, layout::statHp) : 0;
			const int allyIndex = skill(entity, skillMonsterAllyIndex);
			const bool countable = monster && allyIndex < 0 && stats && hp > 0;
			const bool friendly = countable
				&& monsterIsFriendlyForTooltip(viewer, entity);
			switch ( quality::minimap::classifyExitCreature(monster, allyIndex,
				stats != nullptr, hp, friendly) )
			{
				case quality::minimap::ExitCreatureDisposition::Hostile:
					++hostiles;
					break;
				case quality::minimap::ExitCreatureDisposition::Neutral:
					++neutrals;
					break;
				default:
					break;
			}
		}
		return {hostiles, neutrals};
	}

	std::pair<int, int> exitCreatureCounts(std::uint8_t* worldUi)
	{
		if ( multiplayerMode() == 2 && synchronizedExitCountsValid )
		{
			return synchronizedExitCounts;
		}
		if ( !worldUi )
		{
			return {};
		}
		auto* player = field<std::uint8_t*>(worldUi, worldUiPlayer);
		if ( !player )
		{
			return {};
		}
		return exitCreatureCountsForViewer(
			field<std::int32_t>(player, playerNumber));
	}

	const char* exitTooltipHook(const int languageId, std::uint8_t* worldUi)
	{
		const char* text = languageGet(languageId);
		if ( revealState.tooltipEdge(
			*reinterpret_cast<std::uint32_t*>(base + ticksRva)) )
		{
			requestLocalRefresh();
		}
		const auto counts = exitCreatureCounts(worldUi);
		quality::minimap::formatExitTooltip(exitTooltipText.data(),
			exitTooltipText.size(), text, counts.first, counts.second);
		return exitTooltipText.data();
	}

	void headstoneDialogueHook(void* dialogue, const std::uint32_t entityUid,
		const int type, const char* text)
	{
		createDialogue(dialogue, entityUid, type, text);
		revealState.markUsed(entityUid);
	}

	void suppressVanilla(std::uint8_t* entity, const bool clearSprite)
	{
		mutations.push_back({entity, behavior(entity),
			field<std::int32_t>(entity, entitySprite), skill(entity, skillShowOnMap)});
		field<std::uintptr_t>(entity, entityBehavior) = 0;
		skill(entity, skillShowOnMap) = 0;
		if ( clearSprite )
		{
			field<std::int32_t>(entity, entitySprite) = 0;
		}
	}

	void restoreVanilla()
	{
		for ( auto mutation = mutations.rbegin(); mutation != mutations.rend(); ++mutation )
		{
			field<std::uintptr_t>(mutation->entity, entityBehavior) = mutation->behavior;
			field<std::int32_t>(mutation->entity, entitySprite) = mutation->sprite;
			skill(mutation->entity, skillShowOnMap) = mutation->showOnMap;
		}
		mutations.clear();
	}

	std::uint32_t itemInventoryKey(std::uint8_t* entity)
	{
		return quality::minimap::items::stackFingerprint({
			static_cast<std::uint32_t>(skill(entity, 10)),
			static_cast<std::uint32_t>(skill(entity, 11)),
			static_cast<std::uint32_t>(skill(entity, 12)),
			static_cast<std::uint32_t>(skill(entity, 14)),
			static_cast<std::uint32_t>(skill(entity, 15)),
		});
	}

	void observePartyItems(List* entities,
		const std::unordered_set<std::uint32_t>& partyUids,
		const std::vector<std::uint8_t*>& partyEntities)
	{
		if ( !entities )
		{
			return;
		}
		const bool authoritative = quality::minimap::items::locallyAuthoritative(
			multiplayerMode());
		struct ItemObservation
		{
			std::uint32_t uid;
			std::uint32_t inventoryKey;
			int x;
			int y;
			bool partyOwned;
		};
		std::vector<ItemObservation> observations;
		std::unordered_set<std::uint32_t> liveItems;
		const int lootBagIndex = *reinterpret_cast<int*>(
			base + lootBagSpriteIndexRva);
		const int lootBagVariations = *reinterpret_cast<int*>(
			base + lootBagSpriteVariationsRva);
		for ( Node* node = entities->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const int sprite = field<std::int32_t>(entity, entitySprite);
			const bool lootBag = sprite >= lootBagIndex
				&& sprite < lootBagIndex + lootBagVariations;
			if ( !quality::minimap::items::eligibleGroundItem(
				behavior(entity) == reinterpret_cast<std::uintptr_t>(base + actItemRva),
				skill(entity, skillItemContainer) != 0, lootBag) )
			{
				continue;
			}
			const auto entityUid = uid(entity);
			const auto inventoryKey = itemInventoryKey(entity);
			const int tileX = static_cast<int>(std::floor(
				field<double>(entity, entityX) / 16.0));
			const int tileY = static_cast<int>(std::floor(
				field<double>(entity, entityY) / 16.0));
			liveItems.insert(entityUid);
			const auto originalOwner = static_cast<std::uint32_t>(
				skill(entity, skillItemOriginalOwner));
			const auto parent = field<std::uint32_t>(entity, entityParent);
			const bool partyDropped = partyUids.count(originalOwner) != 0
				|| partyUids.count(parent) != 0;
			observations.push_back({entityUid, inventoryKey, tileX, tileY,
				partyDropped});
			const auto* existing = partyItemState.find(entityUid);
			if ( existing )
			{
				partyItemState.observe(entityUid, inventoryKey, tileX, tileY,
					existing->partyDropped);
			}
			else if ( !partyDropped )
			{
				partyItemState.observe(entityUid, inventoryKey, tileX, tileY, false);
			}
		}

		std::unordered_set<std::uint32_t> reboundOldItems;
		std::unordered_set<std::uint32_t> reboundNewItems;
		for ( const auto oldUid : seenWorldItems )
		{
			if ( liveItems.count(oldUid) || !revealState.contains(oldUid) )
			{
				continue;
			}
			const auto* oldRecord = partyItemState.find(oldUid);
			if ( !oldRecord || oldRecord->partyDropped )
			{
				continue;
			}
			for ( const auto& observation : observations )
			{
				if ( !observation.partyOwned
					|| partyItemState.find(observation.uid)
					|| reboundNewItems.count(observation.uid)
					|| observation.inventoryKey
						!= oldRecord->marker.inventoryKey )
				{
					continue;
				}
				const int dx = observation.x - oldRecord->marker.x;
				const int dy = observation.y - oldRecord->marker.y;
				if ( dx * dx + dy * dy > 25 )
				{
					continue;
				}
				if ( partyItemState.rebindOrdinary(oldUid, observation.uid,
					observation.inventoryKey, observation.x, observation.y) )
				{
					revealState.rebind(oldUid, observation.uid,
						observation.x, observation.y);
					reboundOldItems.insert(oldUid);
					reboundNewItems.insert(observation.uid);
				}
				break;
			}
		}

		if ( authoritative )
		{
			for ( const auto& observation : observations )
			{
				if ( !observation.partyOwned
					|| reboundNewItems.count(observation.uid)
					|| partyItemState.find(observation.uid) )
				{
					continue;
				}
				partyItemState.observe(observation.uid, observation.inventoryKey,
					observation.x, observation.y, true);
				const auto* record = partyItemState.find(observation.uid);
				if ( record )
				{
					broadcastItem(PacketKind::ItemDropped, record->marker);
				}
			}
			for ( const auto oldUid : seenWorldItems )
			{
				if ( liveItems.count(oldUid) || reboundOldItems.count(oldUid) )
				{
					continue;
				}
				const auto* found = partyItemState.find(oldUid);
				if ( !found )
				{
					continue;
				}
				const auto record = *found;
				bool partyPickup = false;
				for ( const auto* party : partyEntities )
				{
					const double dx = field<double>(party, entityX) / 16.0
						- (record.marker.x + .5);
					const double dy = field<double>(party, entityY) / 16.0
						- (record.marker.y + .5);
					if ( dx * dx + dy * dy <= 6.25 )
					{
						partyPickup = true;
						break;
					}
				}
				if ( partyPickup )
				{
					partyItemState.pickUp(oldUid, record.marker.inventoryKey);
					if ( record.partyDropped )
					{
						broadcastItem(PacketKind::ItemPickedUp, record.marker);
					}
				}
				else
				{
					if ( record.partyDropped )
					{
						broadcastItem(PacketKind::ItemRemoved, record.marker);
					}
					partyItemState.remove(oldUid);
				}
			}
		}
		else
		{
			for ( const auto oldUid : seenWorldItems )
			{
				if ( liveItems.count(oldUid) || reboundOldItems.count(oldUid) )
				{
					continue;
				}
				const auto* record = partyItemState.find(oldUid);
				if ( record && !record->partyDropped )
				{
					partyItemState.remove(oldUid);
				}
			}
		}
		seenWorldItems = std::move(liveItems);
	}

	void observeWorld()
	{
		visuals.clear();
		mutations.clear();
		auto* entities = *reinterpret_cast<List**>(base + mapEntitiesRva);
		auto* creatures = *reinterpret_cast<List**>(base + mapCreaturesRva);
		if ( !entities || !creatures )
		{
			return;
		}
		const std::uint32_t ticks = *reinterpret_cast<std::uint32_t*>(base + ticksRva);
		const auto entityList = reinterpret_cast<std::uintptr_t>(entities);
		if ( entityList != lastEntityList || ticks < lastTicks )
		{
			minotaurAlerted = false;
			lastEntityList = entityList;
		}
		lastTicks = ticks;

		std::unordered_set<std::uint32_t> partyUids;
		std::unordered_set<std::uint32_t> shadowTaggedUids;
		std::vector<std::uint8_t*> partyEntities;
		for ( Node* node = creatures->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const auto action = behavior(entity);
			if ( action == reinterpret_cast<std::uintptr_t>(base + actPlayerRva) )
			{
				partyUids.insert(uid(entity));
				partyEntities.push_back(entity);
				const auto tagged = static_cast<std::uint32_t>(
					skill(entity, skillShadowTaggedUid));
				if ( tagged )
				{
					shadowTaggedUids.insert(tagged);
				}
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actMonsterRva)
				&& skill(entity, skillMonsterAllyIndex) >= 0 )
			{
				partyUids.insert(uid(entity));
				partyEntities.push_back(entity);
			}
		}
		observePartyItems(entities, partyUids, partyEntities);
		observeChests(entities);
		revealCandidates = collectRevealCandidates(entities, partyUids);
		revealState.observeLive(revealCandidates);
		flushPendingPackets();

		for ( Node* node = entities->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const auto action = behavior(entity);
			const int sprite = field<std::int32_t>(entity, entitySprite);
			const double worldX = field<double>(entity, entityX);
			const double worldY = field<double>(entity, entityY);
			const int tileX = static_cast<int>(std::floor(worldX / 16.0));
			const int tileY = static_cast<int>(std::floor(worldY / 16.0));
			const std::uint8_t visibility = visibilityAt(tileX, tileY);
			const bool explored = quality::minimap::ordinarilyExplored(visibility);
			const auto showOnMap = static_cast<std::uint32_t>(
				skill(entity, skillShowOnMap));
			const bool customPortal = action
				== reinterpret_cast<std::uintptr_t>(base + actCustomPortalRva);
			const auto appearance = quality::minimap::classifyEntity(sprite,
				field<bool>(entity, entityFlags + 7), showOnMap,
				customPortal,
				action == reinterpret_cast<std::uintptr_t>(base + actWorkbenchRva),
				action == reinterpret_cast<std::uintptr_t>(base + actCauldronRva));
			if ( partyItemState.isPartyDropped(uid(entity)) )
			{
				suppressVanilla(entity, false);
			}

			if ( appearance == quality::minimap::MarkerAppearance::Exit )
			{
				if ( revealState.contains(uid(entity))
					|| quality::minimap::exitVisible(visibility, showOnMap,
						customPortal) )
				{
					visuals.push_back({VisualKind::Exit, tileX + .5, tileY + .5,
						0.0, -1, false});
				}
				suppressVanilla(entity, true);
				continue;
			}
			if ( appearance == quality::minimap::MarkerAppearance::Boulder )
			{
				if ( explored )
				{
					visuals.push_back({VisualKind::Boulder, tileX + .5, tileY + .5,
						0.0, -1, false});
				}
				suppressVanilla(entity, true);
				continue;
			}
			if ( appearance == quality::minimap::MarkerAppearance::Workbench
				|| appearance == quality::minimap::MarkerAppearance::Cauldron )
			{
				if ( explored || revealState.contains(uid(entity)) )
				{
					visuals.push_back({appearance
						== quality::minimap::MarkerAppearance::Workbench
							? VisualKind::Workbench : VisualKind::Cauldron,
						tileX + .5, tileY + .5, 0.0, -1, false});
				}
				suppressVanilla(entity, false);
				continue;
			}

			if ( shadowTaggedUids.count(uid(entity)) && !partyUids.count(uid(entity)) )
			{
				visuals.push_back({VisualKind::ShadowCreature, worldX / 16.0,
					worldY / 16.0, 0.0, -1, true});
				if ( !quality::minimap::isDetectedUnit(showOnMap) )
				{
					suppressVanilla(entity, false);
				}
			}
		}

		for ( const auto& marker : revealState.markers() )
		{
			if ( marker.kind == quality::minimap::reveal::Kind::Unused )
			{
				visuals.push_back({VisualKind::Unused, marker.x + .5,
					marker.y + .5, 0.0, -1, false});
			}
		}

		for ( Node* node = creatures->first; node; node = node->next )
		{
			auto* entity = static_cast<std::uint8_t*>(node->element);
			if ( !entity )
			{
				continue;
			}
			const auto action = behavior(entity);
			const int sprite = field<std::int32_t>(entity, entitySprite);
			const double x = field<double>(entity, entityX) / 16.0;
			const double y = field<double>(entity, entityY) / 16.0;
			const double yaw = field<double>(entity, entityYaw);
			if ( sprite == 239 )
			{
				visuals.push_back({VisualKind::Minotaur, x, y, 0.0, -1, false});
				suppressVanilla(entity, true);
				if ( !minotaurAlerted && playSound )
				{
					playSound(116, 64);
					minotaurAlerted = true;
				}
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actPlayerRva) )
			{
				visuals.push_back({VisualKind::Player, x, y, yaw,
					skill(entity, skillPlayerIndex), shadowTaggedUids.count(uid(entity)) != 0});
				suppressVanilla(entity, false);
			}
			else if ( action == reinterpret_cast<std::uintptr_t>(base + actMonsterRva)
				&& skill(entity, skillMonsterAllyIndex) >= 0 )
			{
				visuals.push_back({VisualKind::Follower, x, y, yaw,
					skill(entity, skillMonsterAllyIndex),
					shadowTaggedUids.count(uid(entity)) != 0});
				suppressVanilla(entity, false);
			}
		}
	}

	template <typename T>
	bool loadGlProcedure(T& target, const char* name)
	{
		const PROC procedure = wglGetProcAddress(name);
		if ( !procedure || procedure == reinterpret_cast<PROC>(1)
			|| procedure == reinterpret_cast<PROC>(2)
			|| procedure == reinterpret_cast<PROC>(3)
			|| procedure == reinterpret_cast<PROC>(-1) )
		{
			return false;
		}
		target = reinterpret_cast<T>(procedure);
		return true;
	}

	GLuint compileShader(const GLenum type, const char* source)
	{
		const GLuint shader = createShader(type);
		shaderSource(shader, 1, &source, nullptr);
		compileShaderProcedure(shader);
		GLint compiled = GL_FALSE;
		getShaderiv(shader, 0x8B81, &compiled);
		if ( compiled == GL_TRUE )
		{
			return shader;
		}
		deleteShader(shader);
		return 0;
	}

	bool ensureRenderer()
	{
		const HGLRC context = wglGetCurrentContext();
		if ( !context )
		{
			return false;
		}
		if ( context == rendererContext && vectorProgram && vectorVao && vectorVbo )
		{
			return true;
		}
		rendererContext = context;
		vectorProgram = 0;
		vectorVao = 0;
		vectorVbo = 0;
		const bool loaded = loadGlProcedure(useProgram, "glUseProgram")
			&& loadGlProcedure(createShader, "glCreateShader")
			&& loadGlProcedure(shaderSource, "glShaderSource")
			&& loadGlProcedure(compileShaderProcedure, "glCompileShader")
			&& loadGlProcedure(getShaderiv, "glGetShaderiv")
			&& loadGlProcedure(getShaderInfoLog, "glGetShaderInfoLog")
			&& loadGlProcedure(deleteShader, "glDeleteShader")
			&& loadGlProcedure(createProgram, "glCreateProgram")
			&& loadGlProcedure(attachShader, "glAttachShader")
			&& loadGlProcedure(bindAttribLocation, "glBindAttribLocation")
			&& loadGlProcedure(linkProgram, "glLinkProgram")
			&& loadGlProcedure(getProgramiv, "glGetProgramiv")
			&& loadGlProcedure(getProgramInfoLog, "glGetProgramInfoLog")
			&& loadGlProcedure(deleteProgram, "glDeleteProgram")
			&& loadGlProcedure(genVertexArrays, "glGenVertexArrays")
			&& loadGlProcedure(bindVertexArray, "glBindVertexArray")
			&& loadGlProcedure(genBuffers, "glGenBuffers")
			&& loadGlProcedure(bindBuffer, "glBindBuffer")
			&& loadGlProcedure(bufferData, "glBufferData")
			&& loadGlProcedure(enableVertexAttribArray, "glEnableVertexAttribArray")
			&& loadGlProcedure(vertexAttribPointer, "glVertexAttribPointer")
			&& loadGlProcedure(getUniformLocation, "glGetUniformLocation")
			&& loadGlProcedure(uniform2f, "glUniform2f")
			&& loadGlProcedure(uniform4f, "glUniform4f")
			&& loadGlProcedure(blendFuncSeparate, "glBlendFuncSeparate");
		if ( !loaded )
		{
			return false;
		}

		static const char* vertexSource =
			"#version 130\n"
			"in vec2 aPosition;\n"
			"uniform vec2 uViewport;\n"
			"void main(){vec2 p=vec2(aPosition.x/uViewport.x*2.0-1.0,"
			"1.0-aPosition.y/uViewport.y*2.0);gl_Position=vec4(p,0.0,1.0);}\n";
		static const char* fragmentSource =
			"#version 130\n"
			"uniform vec4 uColor;out vec4 fragmentColor;\n"
			"void main(){fragmentColor=uColor;}\n";
		const GLuint vertex = compileShader(0x8B31, vertexSource);
		const GLuint fragment = compileShader(0x8B30, fragmentSource);
		if ( !vertex || !fragment )
		{
			if ( vertex ) { deleteShader(vertex); }
			if ( fragment ) { deleteShader(fragment); }
			return false;
		}
		const GLuint program = createProgram();
		attachShader(program, vertex);
		attachShader(program, fragment);
		bindAttribLocation(program, 0, "aPosition");
		linkProgram(program);
		deleteShader(vertex);
		deleteShader(fragment);
		GLint linked = GL_FALSE;
		getProgramiv(program, 0x8B82, &linked);
		if ( linked != GL_TRUE )
		{
			deleteProgram(program);
			return false;
		}

		GLint previousProgram = 0;
		GLint previousVao = 0;
		GLint previousBuffer = 0;
		glGetIntegerv(0x8B8D, &previousProgram);
		glGetIntegerv(0x85B5, &previousVao);
		glGetIntegerv(0x8894, &previousBuffer);
		vectorProgram = program;
		viewportUniform = getUniformLocation(program, "uViewport");
		colorUniform = getUniformLocation(program, "uColor");
		genVertexArrays(1, &vectorVao);
		genBuffers(1, &vectorVbo);
		bindVertexArray(vectorVao);
		bindBuffer(0x8892, vectorVbo);
		enableVertexAttribArray(0);
		vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		bindBuffer(0x8892, static_cast<GLuint>(previousBuffer));
		bindVertexArray(static_cast<GLuint>(previousVao));
		useProgram(static_cast<GLuint>(previousProgram));
		return viewportUniform >= 0 && colorUniform >= 0;
	}

	struct RenderScope
	{
		GLint program = 0;
		GLint vao = 0;
		GLint buffer = 0;
		GLint viewport[4] = {0, 0, 1, 1};
		GLint blendSourceRgb = GL_ONE;
		GLint blendDestinationRgb = GL_ZERO;
		GLint blendSourceAlpha = GL_ONE;
		GLint blendDestinationAlpha = GL_ZERO;
		GLfloat lineWidth = 1.f;
		GLboolean blend = GL_FALSE;
		GLboolean depth = GL_FALSE;
		GLboolean cull = GL_FALSE;
		bool engaged = false;
		bool valid = false;

		RenderScope()
		{
			if ( !ensureRenderer() ) { return; }
			glGetIntegerv(GL_VIEWPORT, viewport);
			glGetIntegerv(0x8B8D, &program);
			glGetIntegerv(0x85B5, &vao);
			glGetIntegerv(0x8894, &buffer);
			glGetIntegerv(0x80C9, &blendSourceRgb);
			glGetIntegerv(0x80C8, &blendDestinationRgb);
			glGetIntegerv(0x80CB, &blendSourceAlpha);
			glGetIntegerv(0x80CA, &blendDestinationAlpha);
			glGetFloatv(GL_LINE_WIDTH, &lineWidth);
			blend = glIsEnabled(GL_BLEND);
			depth = glIsEnabled(GL_DEPTH_TEST);
			cull = glIsEnabled(GL_CULL_FACE);
			useProgram(vectorProgram);
			bindVertexArray(vectorVao);
			bindBuffer(0x8892, vectorVbo);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			uniform2f(viewportUniform, static_cast<float>(viewport[2]),
				static_cast<float>(viewport[3]));
			engaged = true;
			valid = viewport[2] > 0 && viewport[3] > 0;
		}

		~RenderScope()
		{
			if ( !engaged ) { return; }
			glLineWidth(lineWidth);
			if ( blend ) { glEnable(GL_BLEND); } else { glDisable(GL_BLEND); }
			if ( depth ) { glEnable(GL_DEPTH_TEST); } else { glDisable(GL_DEPTH_TEST); }
			if ( cull ) { glEnable(GL_CULL_FACE); } else { glDisable(GL_CULL_FACE); }
			blendFuncSeparate(blendSourceRgb, blendDestinationRgb,
				blendSourceAlpha, blendDestinationAlpha);
			bindBuffer(0x8892, static_cast<GLuint>(buffer));
			bindVertexArray(static_cast<GLuint>(vao));
			useProgram(static_cast<GLuint>(program));
		}
	};

	void primitive(const std::vector<std::pair<float, float>>& points,
		const std::uint32_t markerColor, const GLenum mode)
	{
		if ( points.empty() ) { return; }
		std::vector<float> vertices;
		vertices.reserve(points.size() * 2);
		for ( const auto& point : points )
		{
			vertices.push_back(point.first);
			vertices.push_back(point.second);
		}
		bufferData(0x8892, static_cast<std::ptrdiff_t>(
			vertices.size() * sizeof(float)), vertices.data(), 0x88E8);
		uniform4f(colorUniform,
			static_cast<float>(markerColor & 0xFF) / 255.f,
			static_cast<float>((markerColor >> 8) & 0xFF) / 255.f,
			static_cast<float>((markerColor >> 16) & 0xFF) / 255.f,
			static_cast<float>((markerColor >> 24) & 0xFF) / 255.f);
		glDrawArrays(mode, 0, static_cast<GLsizei>(points.size()));
	}

	void circle(const float x, const float y, const float radius,
		const std::uint32_t markerColor, const bool filled)
	{
		std::vector<std::pair<float, float>> points;
		if ( filled ) { points.emplace_back(x, y); }
		const int count = filled ? 33 : 32;
		for ( int index = 0; index < count; ++index )
		{
			const double angle = 2.0 * pi * index / 32.0;
			points.emplace_back(x + static_cast<float>(std::cos(angle)) * radius,
				y + static_cast<float>(std::sin(angle)) * radius);
		}
		primitive(points, markerColor, filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
	}

	void line(const std::vector<std::pair<float, float>>& points,
		const std::uint32_t markerColor, const GLenum mode = GL_LINE_STRIP)
	{
		primitive(points, markerColor, mode);
	}

	struct MarkerTransform { float x; float y; float unitX; float unitY; };

	MarkerTransform transform(const double mapX, const double mapY,
		const RenderScope& scope)
	{
		const int width = *reinterpret_cast<int*>(base + mapWidthRva);
		const int height = *reinterpret_cast<int*>(base + mapHeightRva);
		const int mapGcd = std::max(1, std::max(width, height));
		const int xmin = (width - mapGcd) / 2;
		const int ymin = (height - mapGcd) / 2;
		int virtualX = scope.viewport[2];
		int virtualY = scope.viewport[3];
		auto** virtualXPointer = reinterpret_cast<int**>(base + virtualScreenXPointerRva);
		auto** virtualYPointer = reinterpret_cast<int**>(base + virtualScreenYPointerRva);
		if ( *virtualXPointer && **virtualXPointer > 0 ) { virtualX = **virtualXPointer; }
		if ( *virtualYPointer && **virtualYPointer > 0 ) { virtualY = **virtualYPointer; }
		const float scaleX = static_cast<float>(scope.viewport[2]) / virtualX;
		const float scaleY = static_cast<float>(scope.viewport[3]) / virtualY;
		const float unitX = static_cast<float>(currentRect.w) / mapGcd * scaleX;
		const float unitY = static_cast<float>(currentRect.h) / mapGcd * scaleY;
		return {(static_cast<float>(mapX) - xmin) * unitX + currentRect.x * scaleX,
			(static_cast<float>(mapY) - ymin) * unitY + currentRect.y * scaleY,
			unitX, unitY};
	}

	void drawCircledSkull(const MarkerTransform& marker,
		const std::uint32_t markerColor)
	{
		const float radius = std::min(marker.unitX, marker.unitY);
		circle(marker.x, marker.y, radius, markerColor, false);
		circle(marker.x, marker.y - radius * .12f, radius * .55f, markerColor, false);
		line({{marker.x-radius*.35f, marker.y+radius*.25f},
			{marker.x-radius*.35f, marker.y+radius*.55f},
			{marker.x-radius*.1f, marker.y+radius*.55f},
			{marker.x-radius*.1f, marker.y+radius*.38f},
			{marker.x+radius*.1f, marker.y+radius*.38f},
			{marker.x+radius*.1f, marker.y+radius*.55f},
			{marker.x+radius*.35f, marker.y+radius*.55f},
			{marker.x+radius*.35f, marker.y+radius*.25f}}, markerColor);
		circle(marker.x-radius*.2f, marker.y-radius*.15f, radius*.08f,
			markerColor, true);
		circle(marker.x+radius*.2f, marker.y-radius*.15f, radius*.08f,
			markerColor, true);
		line({{marker.x, marker.y}, {marker.x-radius*.1f, marker.y+radius*.18f},
			{marker.x+radius*.1f, marker.y+radius*.18f}, {marker.x, marker.y}},
			markerColor);
	}

	void drawExit(const MarkerTransform& marker)
	{
		const auto markerColor = quality::minimap::color(64, 255, 64);
		const float rx = marker.unitX * .75f;
		const float ry = marker.unitY * .75f;
		line({{marker.x-rx, marker.y-ry}, {marker.x+rx, marker.y-ry},
			{marker.x+rx, marker.y+ry}, {marker.x-rx, marker.y+ry},
			{marker.x-rx, marker.y-ry}}, markerColor);
		circle(marker.x+rx*.1f, marker.y-ry*.65f, std::min(rx, ry)*.2f,
			markerColor, true);
		line({{marker.x, marker.y-ry*.35f}, {marker.x-rx*.2f, marker.y+ry*.15f},
			{marker.x+rx*.2f, marker.y+ry*.55f},
			{marker.x+rx*.5f, marker.y+ry*.85f}}, markerColor);
		line({{marker.x-rx*.1f, marker.y-ry*.25f},
			{marker.x-rx*.5f, marker.y-ry*.1f},
			{marker.x-rx*.7f, marker.y+ry*.2f}}, markerColor);
		line({{marker.x, marker.y-ry*.25f}, {marker.x+rx*.35f, marker.y},
			{marker.x+rx*.65f, marker.y-ry*.2f}}, markerColor);
		line({{marker.x-rx*.2f, marker.y+ry*.15f},
			{marker.x-rx*.4f, marker.y+ry*.65f},
			{marker.x-rx*.8f, marker.y+ry*.75f}}, markerColor);
	}

	void drawArrow(const MarkerTransform& marker, const double yaw,
		const std::uint32_t markerColor, const bool outline,
		const float scale = 1.f)
	{
		constexpr std::array<std::pair<float, float>, 3> points = {{
			{1.f, 0.f}, {-.5f, .5f}, {-.5f, -.5f},
		}};
		const float cosine = static_cast<float>(std::cos(yaw));
		const float sine = static_cast<float>(std::sin(yaw));
		std::vector<std::pair<float, float>> transformed;
		for ( const auto& point : points )
		{
			const float x = point.first * marker.unitX * scale;
			const float y = point.second * marker.unitY * scale;
			transformed.emplace_back(marker.x + cosine*x - sine*y,
				marker.y + sine*x + cosine*y);
		}
		primitive(transformed, markerColor, outline ? GL_LINE_LOOP : GL_TRIANGLES);
	}

	void renderBlueLayer()
	{
		if ( !inMinimapDraw )
		{
			return;
		}
		RenderScope scope;
		if ( !scope.valid )
		{
			return;
		}
		glLineWidth(1.5f);
		for ( const auto& item : partyItemState.markers() )
		{
			const auto marker = transform(item.x + .5, item.y + .5, scope);
			circle(marker.x, marker.y,
				std::min(marker.unitX, marker.unitY) * .5f,
				quality::minimap::interactedBlue, true);
		}
		for ( const auto& chest : chestState.markers() )
		{
			const auto marker = transform(chest.x + .5, chest.y + .5, scope);
			circle(marker.x, marker.y,
				std::min(marker.unitX, marker.unitY) * .5f,
				quality::minimap::interactedBlue, true);
		}
	}

	void renderQualityLayer()
	{
		RenderScope scope;
		if ( !scope.valid ) { return; }
		glLineWidth(1.5f);
		for ( const auto& visual : visuals )
		{
			const auto marker = transform(visual.x, visual.y, scope);
			const float radius = std::min(marker.unitX, marker.unitY) * .5f;
			constexpr float stationScale = 4.0f / 3.0f;
			switch ( visual.kind )
			{
				case VisualKind::Exit:
					drawExit(marker);
					break;
				case VisualKind::Boulder:
					circle(marker.x, marker.y, radius,
						quality::minimap::boulderCyan, false);
					line({{marker.x-radius, marker.y}, {marker.x+radius, marker.y}},
						quality::minimap::boulderCyan);
					line({{marker.x, marker.y-radius}, {marker.x, marker.y+radius}},
						quality::minimap::boulderCyan);
					line({{marker.x-radius*.707f, marker.y-radius*.707f},
						{marker.x+radius*.707f, marker.y+radius*.707f}},
						quality::minimap::boulderCyan);
					line({{marker.x+radius*.707f, marker.y-radius*.707f},
						{marker.x-radius*.707f, marker.y+radius*.707f}},
						quality::minimap::boulderCyan);
					break;
				case VisualKind::Workbench:
				case VisualKind::Cauldron:
					circle(marker.x, marker.y, radius * stationScale,
						quality::minimap::stationBlue, true);
					if ( visual.kind == VisualKind::Workbench )
					{
						line({{marker.x-marker.unitX*.34f*stationScale,
								marker.y-marker.unitY*.16f*stationScale},
							{marker.x-marker.unitX*.22f*stationScale,
								marker.y+marker.unitY*.30f*stationScale},
							{marker.x, marker.y-marker.unitY*.02f*stationScale},
							{marker.x+marker.unitX*.22f*stationScale,
								marker.y+marker.unitY*.30f*stationScale},
							{marker.x+marker.unitX*.34f*stationScale,
								marker.y-marker.unitY*.16f*stationScale}},
							quality::minimap::color(0, 64, 64));
					}
					else
					{
						std::vector<std::pair<float, float>> arc;
						for ( int index = 4; index <= 28; ++index )
						{
							const float angle = static_cast<float>(2.0*pi*index/32.0);
							arc.emplace_back(marker.x
								+ std::sin(angle)*marker.unitX*.31f*stationScale,
								marker.y
								+ std::cos(angle)*marker.unitY*.31f*stationScale);
						}
						line(arc, quality::minimap::color(0, 64, 64));
					}
					break;
				case VisualKind::Minotaur:
					drawCircledSkull(marker, quality::minimap::minotaurRed);
					break;
				case VisualKind::ShadowCreature:
					drawCircledSkull(marker, quality::minimap::shadowGray);
					break;
				case VisualKind::Unused:
					circle(marker.x, marker.y, radius,
						quality::minimap::uninteractedGreen, true);
					break;
				case VisualKind::Player:
					drawArrow(marker, visual.yaw,
						visual.shadow ? quality::minimap::shadowGray
							: quality::minimap::ownerColor(visual.owner, currentViewer), false);
					break;
				case VisualKind::Follower:
					drawArrow(marker, visual.yaw,
						visual.shadow ? quality::minimap::shadowGray
							: quality::minimap::ownerColor(visual.owner, currentViewer), true,
						static_cast<float>(quality::minimap::followerGhostScale));
					break;
			}
		}
	}

	void imageDrawHook(const std::uint32_t texture, const int width,
		const int height, const Rect* source, const Rect destination,
		const Rect viewport, const std::uint32_t& markerColor)
	{
		imageDraw(texture, width, height, source, destination, viewport,
			markerColor);
		renderBlueLayer();
	}

	std::uint32_t playerColorHook(const int player, const bool colorblind,
		const bool ally)
	{
		if ( player >= 0 && player < 4 )
		{
			return quality::minimap::ownerColor(player, currentViewer);
		}
		return originalPlayerColor(player, colorblind, ally);
	}

	void ghostTriangleHook(void* closure, const double x, const double y,
		const double angle, const double size, const Rect rect,
		const std::uint32_t markerColor, const bool outline)
	{
		drawTriangle(closure, x, y, angle,
			size * quality::minimap::followerGhostScale,
			rect, markerColor, outline);
	}

	void drawMinimapHook(const int player, const Rect rect, const bool shared)
	{
		if ( !ensureRenderer() )
		{
			originalDrawMinimap(player, rect, shared);
			return;
		}
		currentViewer = player;
		currentRect = rect;
		inMinimapDraw = true;
		observeWorld();
		originalDrawMinimap(player, rect, shared);
		restoreVanilla();
		renderQualityLayer();
		inMinimapDraw = false;
	}

	std::uint8_t* getStatsHook(std::uint8_t* entity)
	{
		if ( inFollowerRosterDraw )
		{
			const auto found = syntheticFollowerStats.find(entity);
			if ( found != syntheticFollowerStats.end() )
			{
				return found->second.data();
			}
		}
		return originalGetStats ? originalGetStats(entity) : nullptr;
	}

	std::string msvcStringValue(const MsvcString& value)
	{
		const char* data = value.capacity > 15
			? value.storage.heapData : value.storage.inlineData;
		return data ? std::string(data, value.size) : std::string();
	}

	std::string localizedMonsterName(const int type)
	{
		if ( !getMonsterLocalizedName || !destroyMsvcString ) { return {}; }
		MsvcString value;
		getMonsterLocalizedName(&value, type, nullptr);
		const std::string result = msvcStringValue(value);
		destroyMsvcString(&value);
		return result;
	}

	std::string rosterOwnerName(const int owner)
	{
		auto** playerStats = reinterpret_cast<std::uint8_t**>(base + layout::statsRva);
		if ( owner < 0 || owner >= quality::follower_roster::maximumPlayers
			|| !playerStats[owner] )
		{
			return {};
		}
		const char* name = reinterpret_cast<const char*>(playerStats[owner] + statName);
		return std::string(name, strnlen(name, 127));
	}

	void makeSyntheticFollowerStats(std::uint8_t* entity,
		const quality::follower_roster::Entry& entry)
	{
		auto& stats = syntheticFollowerStats[entity];
		stats.fill(0);
		field<std::int32_t>(stats.data(), statType) = entry.type;
		field<std::int32_t>(stats.data(), statHp) = entry.hp;
		field<std::int32_t>(stats.data(), statMaxHp) = entry.maxHp;
		field<std::int32_t>(stats.data(), statMp) = 0;
		field<std::int32_t>(stats.data(), statMaxMp) = 0;
		field<std::int32_t>(stats.data(), statLevel) = entry.level;
		std::string unitName = entry.name.empty()
			? localizedMonsterName(entry.type) : entry.name;
		unitName = quality::follower_roster::displayName(
			rosterOwnerName(entry.owner), unitName, entry.owner);
		const auto length = std::min<std::size_t>(unitName.size(), 127);
		std::memcpy(stats.data() + statName, unitName.data(), length);
		stats[statName + length] = 0;
	}

	void updateAllyFollowerFrameHook(const int player)
	{
		if ( !originalUpdateAllyFollowerFrame || player < 0
			|| player >= quality::follower_roster::maximumPlayers )
		{
			return;
		}
		refreshFollowerRoster();
		auto** playerStats = reinterpret_cast<std::uint8_t**>(base + layout::statsRva);
		if ( !playerStats[player] )
		{
			originalUpdateAllyFollowerFrame(player);
			return;
		}

		auto& realList = field<List>(playerStats[player], layout::statFollowers);
		const List savedList = realList;
		std::vector<std::uint32_t> uids;
		for ( Node* node = realList.first; node; node = node->next )
		{
			auto* uidPointer = static_cast<std::uint32_t*>(node->element);
			if ( uidPointer ) { uids.push_back(*uidPointer); }
		}
		const auto localCount = uids.size();
		const auto remoteEntries = quality::follower_roster::visibleRemoteEntries(
			sharedFollowerRoster.entries(), player);
		for ( const auto& entry : remoteEntries )
		{
			if ( (base + clientDisconnectedRva)[entry.owner] ) { continue; }
			auto* entity = findEntity(
				*reinterpret_cast<List**>(base + mapCreaturesRva), entry.uid);
			if ( !entity || skill(entity, skillMonsterAllyIndex) != entry.owner )
			{
				continue;
			}
			uids.push_back(entry.uid);
			makeSyntheticFollowerStats(entity, entry);
		}
		if ( uids.size() == localCount )
		{
			syntheticFollowerStats.clear();
			originalUpdateAllyFollowerFrame(player);
			return;
		}

		std::vector<Node> nodes(uids.size());
		List displayList {};
		for ( std::size_t index = 0; index < nodes.size(); ++index )
		{
			nodes[index].previous = index ? &nodes[index - 1] : nullptr;
			nodes[index].next = index + 1 < nodes.size() ? &nodes[index + 1] : nullptr;
			nodes[index].list = &displayList;
			nodes[index].element = &uids[index];
			nodes[index].deconstructor = nullptr;
			nodes[index].size = sizeof(std::uint32_t);
		}
		displayList.first = nodes.empty() ? nullptr : &nodes.front();
		displayList.last = nodes.empty() ? nullptr : &nodes.back();

		auto** recentEntity = reinterpret_cast<std::uint8_t**>(base
			+ followerMenusRva + followerMenuStride * player
			+ followerMenuRecentEntity);
		auto* savedRecent = *recentEntity;
		if ( !savedRecent || (field<std::uintptr_t>(savedRecent, entityBehavior)
			== reinterpret_cast<std::uintptr_t>(base + actMonsterRva)
			&& skill(savedRecent, skillMonsterAllyIndex) != player) )
		{
			auto** players = reinterpret_cast<std::uint8_t**>(base + layout::playersRva);
			if ( players[player] )
			{
				*recentEntity = field<std::uint8_t*>(players[player], playerEntity);
			}
		}

		realList = displayList;
		inFollowerRosterDraw = true;
		originalUpdateAllyFollowerFrame(player);
		inFollowerRosterDraw = false;
		realList = savedList;
		*recentEntity = savedRecent;
		syntheticFollowerStats.clear();
	}

	std::vector<std::uint8_t> absoluteJump(const void* destination)
	{
		std::vector<std::uint8_t> bytes = {0xFF, 0x25, 0, 0, 0, 0};
		const auto address = reinterpret_cast<std::uintptr_t>(destination);
		const auto* raw = reinterpret_cast<const std::uint8_t*>(&address);
		bytes.insert(bytes.end(), raw, raw + sizeof(address));
		return bytes;
	}

	void writeRelay(std::uint8_t* destination, const void* target)
	{
		destination[0] = 0x48;
		destination[1] = 0xB8;
		const auto address = reinterpret_cast<std::uintptr_t>(target);
		std::memcpy(destination + 2, &address, sizeof(address));
		destination[10] = 0xFF;
		destination[11] = 0xE0;
	}

	void writeExitTooltipRelay(std::uint8_t* destination, const void* target)
	{
		// All verified exit call sites keep WorldUI_t* in RSI.
		destination[0] = 0x48;
		destination[1] = 0x89;
		destination[2] = 0xF2;
		destination[3] = 0x48;
		destination[4] = 0xB8;
		const auto address = reinterpret_cast<std::uintptr_t>(target);
		std::memcpy(destination + 5, &address, sizeof(address));
		destination[13] = 0xFF;
		destination[14] = 0xE0;
	}

	std::vector<std::uint8_t> relativeCall(const std::uintptr_t fromRva,
		const void* destination)
	{
		std::vector<std::uint8_t> bytes(5);
		bytes[0] = 0xE8;
		const auto from = reinterpret_cast<std::uintptr_t>(base + fromRva + 5);
		const auto to = reinterpret_cast<std::uintptr_t>(destination);
		const auto difference = static_cast<std::intptr_t>(to - from);
		const auto displacement = static_cast<std::int32_t>(difference);
		if ( static_cast<std::intptr_t>(displacement) != difference )
		{
			return {};
		}
		std::memcpy(bytes.data() + 1, &displacement, sizeof(displacement));
		return bytes;
	}

	void* allocateNearModule(const std::size_t bytes)
	{
		SYSTEM_INFO info {};
		GetSystemInfo(&info);
		const auto granularity = static_cast<std::uintptr_t>(info.dwAllocationGranularity);
		const auto module = reinterpret_cast<std::uintptr_t>(base);
		for ( std::uintptr_t distance = 0x02000000;
			distance < 0x70000000; distance += granularity )
		{
			const auto candidate = reinterpret_cast<void*>(
				(module + distance) & ~(granularity - 1));
			if ( void* memory = VirtualAlloc(candidate, bytes,
				MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE) )
			{
				return memory;
			}
		}
		return nullptr;
	}

	std::vector<std::uint8_t> makePingStub()
	{
		std::vector<std::uint8_t> code;
		const auto append = [&](std::initializer_list<std::uint8_t> bytes) {
			code.insert(code.end(), bytes.begin(), bytes.end());
		};
		append({0x49, 0xC1, 0xE8, 0x20});
		append({0x41, 0x0F, 0xB6, 0xC8});
		append({0x41, 0xF7, 0xC0, 0x00, 0x00, 0x00, 0xFF});
		const std::size_t radiusJump = code.size();
		append({0x75, 0x00});
		append({0x45, 0x33, 0xC0});
		append({0x48, 0xB8});
		const auto colorblindAddress = reinterpret_cast<std::uintptr_t>(
			base + colorblindLobbyRva);
		const auto* colorblindRaw = reinterpret_cast<const std::uint8_t*>(
			&colorblindAddress);
		code.insert(code.end(), colorblindRaw, colorblindRaw + sizeof(colorblindAddress));
		append({0x0F, 0xB6, 0x10});
		append({0x48, 0xB8});
		const auto hookAddress = reinterpret_cast<std::uintptr_t>(&playerColorHook);
		const auto* hookRaw = reinterpret_cast<const std::uint8_t*>(&hookAddress);
		code.insert(code.end(), hookRaw, hookRaw + sizeof(hookAddress));
		append({0xFF, 0xD0});
		append({0x49, 0xBA});
		const auto returnAddress = reinterpret_cast<std::uintptr_t>(
			base + pingColorBlockReturnRva);
		const auto* returnRaw = reinterpret_cast<const std::uint8_t*>(&returnAddress);
		code.insert(code.end(), returnRaw, returnRaw + sizeof(returnAddress));
		append({0x41, 0xFF, 0xE2});
		const std::size_t vanilla = code.size();
		code[radiusJump + 1] = static_cast<std::uint8_t>(vanilla - (radiusJump + 2));
		append({0x45, 0x33, 0xC0});
		append({0x48, 0xB8});
		code.insert(code.end(), colorblindRaw,
			colorblindRaw + sizeof(colorblindAddress));
		append({0x0F, 0xB6, 0x10});
		append({0x48, 0xB8});
		const auto vanillaColorAddress = reinterpret_cast<std::uintptr_t>(
			base + playerColorRva);
		const auto* vanillaColorRaw = reinterpret_cast<const std::uint8_t*>(
			&vanillaColorAddress);
		code.insert(code.end(), vanillaColorRaw,
			vanillaColorRaw + sizeof(vanillaColorAddress));
		append({0xFF, 0xD0});
		append({0x49, 0xBA});
		code.insert(code.end(), returnRaw, returnRaw + sizeof(returnAddress));
		append({0x41, 0xFF, 0xE2});
		return code;
	}

	template <std::size_t Size>
	bool matches(const std::uintptr_t rva,
		const std::array<std::uint8_t, Size>& signature)
	{
		return std::memcmp(base + rva, signature.data(), signature.size()) == 0;
	}

	bool validateLayout()
	{
		if ( !matches(drawMinimapRva, drawMinimapSignature)
			|| !matches(imageDrawRva, imageDrawSignature)
			|| !matches(playerColorRva, playerColorSignature)
			|| !matches(playSoundRva, playSoundSignature)
			|| !matches(loadMapRva, loadMapSignature)
			|| !matches(languageGetRva, languageGetSignature)
			|| !matches(createDialogueRva, createDialogueSignature)
			|| !matches(updateAllyFollowerFrameRva,
				updateAllyFollowerFrameSignature)
			|| !matches(getMonsterLocalizedNameRva,
				getMonsterLocalizedNameSignature)
			|| !matches(layout::getStatsRva, layout::getStatsSignature)
			|| !matches(layout::monsterIsFriendlyForTooltipRva,
				layout::monsterIsFriendlyForTooltipSignature)
			|| !matches(lootBagColorCallRva, lootBagColorCall)
			|| !matches(lootBagColorblindCallRva, lootBagColorblindCall)
			|| !matches(pingColorBlockRva, pingColorBlock)
			|| !matches(calloutColorCallRva, calloutColorCall)
			|| !matches(terrainImageDrawCallRva, terrainImageDrawCall)
			|| !matches(worldTooltipHeightPrimaryRva,
				worldTooltipHeightPrimary)
			|| !matches(worldTooltipHeightAlternateRva,
				worldTooltipHeightAlternate) )
		{
			return false;
		}
		for ( std::size_t index = 0; index < exitTooltipCallRvas.size(); ++index )
		{
			if ( !matches(exitTooltipCallRvas[index], exitTooltipCalls[index]) )
			{
				return false;
			}
		}
		for ( std::size_t index = 0; index < ghostColorCallRvas.size(); ++index )
		{
			if ( !matches(ghostColorCallRvas[index], ghostColorCalls[index])
				|| !matches(ghostTriangleCallRvas[index], ghostTriangleCalls[index]) )
			{
				return false;
			}
		}
		for ( std::size_t index = 0; index < headstoneDialogueCallRvas.size(); ++index )
		{
			if ( !matches(headstoneDialogueCallRvas[index],
				headstoneDialogueCalls[index]) )
			{
				return false;
			}
		}
		if ( !*reinterpret_cast<UdpRecvFn*>(base + udpRecvIatRva)
			|| !*reinterpret_cast<UdpSendFn*>(base + udpSendIatRva) )
		{
			return false;
		}
		return true;
	}

	template <std::size_t Size>
	void addPatch(std::vector<quality::runtime::Patch>& patches,
		const std::uintptr_t rva, const std::array<std::uint8_t, Size>& expected,
		std::vector<std::uint8_t> replacement)
	{
		quality::runtime::Patch patch;
		patch.rva = rva;
		patch.expected.assign(expected.begin(), expected.end());
		patch.replacement = std::move(replacement);
		patch.original.assign(base + rva, base + rva + patch.replacement.size());
		patches.push_back(std::move(patch));
	}

	template <typename T>
	void addPointerPatch(std::vector<quality::runtime::Patch>& patches,
		const std::uintptr_t rva, const T expected, const T replacement)
	{
		quality::runtime::Patch patch;
		patch.rva = rva;
		const auto* expectedBytes = reinterpret_cast<const std::uint8_t*>(&expected);
		const auto* replacementBytes = reinterpret_cast<const std::uint8_t*>(&replacement);
		patch.expected.assign(expectedBytes, expectedBytes + sizeof(T));
		patch.replacement.assign(replacementBytes, replacementBytes + sizeof(T));
		patch.original.assign(base + rva, base + rva + sizeof(T));
		patches.push_back(std::move(patch));
	}
}

namespace quality::minimap_runtime
{
	bool prepare(std::uint8_t* moduleBase,
		std::vector<quality::runtime::Patch>& patches)
	{
		base = moduleBase;
		if ( !base || !validateLayout() )
		{
			return false;
		}
		originalPlayerColor = reinterpret_cast<PlayerColorFn>(base + playerColorRva);
		imageDraw = reinterpret_cast<ImageDrawFn>(base + imageDrawRva);
		drawTriangle = reinterpret_cast<DrawTriangleFn>(base + drawTriangleRva);
		playSound = reinterpret_cast<PlaySoundFn>(base + playSoundRva);
		languageGet = reinterpret_cast<LanguageGetFn>(base + languageGetRva);
		createDialogue = reinterpret_cast<CreateDialogueFn>(base + createDialogueRva);
		getStats = reinterpret_cast<GetStatsFn>(base + layout::getStatsRva);
		getMonsterLocalizedName = reinterpret_cast<GetMonsterLocalizedNameFn>(
			base + getMonsterLocalizedNameRva);
		destroyMsvcString = reinterpret_cast<MsvcStringDestroyFn>(
			base + layout::msvcStringDestroyRva);
		monsterIsFriendlyForTooltip =
			reinterpret_cast<MonsterIsFriendlyForTooltipFn>(
				base + layout::monsterIsFriendlyForTooltipRva);
		udpRecv = *reinterpret_cast<UdpRecvFn*>(base + udpRecvIatRva);
		udpSend = *reinterpret_cast<UdpSendFn*>(base + udpSendIatRva);
		relayPage = allocateNearModule(6 * relayStride);
		drawTrampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE);
		loadMapTrampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE);
		followerFrameTrampoline = VirtualAlloc(nullptr, 64,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		getStatsTrampoline = VirtualAlloc(nullptr, 64,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if ( !relayPage || !drawTrampoline || !loadMapTrampoline
			|| !followerFrameTrampoline || !getStatsTrampoline )
		{
			release();
			return false;
		}

		auto* relay = static_cast<std::uint8_t*>(relayPage);
		auto* playerColorRelay = relay;
		auto* ghostTriangleRelay = relay + relayStride;
		auto* pingRelay = relay + 2 * relayStride;
		auto* exitTooltipRelay = relay + 3 * relayStride;
		auto* headstoneDialogueRelay = relay + 4 * relayStride;
		auto* imageDrawRelay = relay + 5 * relayStride;
		writeRelay(playerColorRelay, reinterpret_cast<void*>(&playerColorHook));
		writeRelay(ghostTriangleRelay, reinterpret_cast<void*>(&ghostTriangleHook));
		writeExitTooltipRelay(exitTooltipRelay,
			reinterpret_cast<void*>(&exitTooltipHook));
		writeRelay(headstoneDialogueRelay,
			reinterpret_cast<void*>(&headstoneDialogueHook));
		writeRelay(imageDrawRelay, reinterpret_cast<void*>(&imageDrawHook));
		const auto pingStub = makePingStub();
		if ( pingStub.empty() || pingStub.size() > relayStride )
		{
			release();
			return false;
		}
		std::memcpy(pingRelay, pingStub.data(), pingStub.size());

		auto* trampoline = static_cast<std::uint8_t*>(drawTrampoline);
		std::memcpy(trampoline, base + drawMinimapRva, drawHookBytes);
		const auto jumpBack = absoluteJump(base + drawMinimapRva + drawHookBytes);
		std::memcpy(trampoline + drawHookBytes, jumpBack.data(), jumpBack.size());
		originalDrawMinimap = reinterpret_cast<DrawMinimapFn>(trampoline);
		auto* loadTrampoline = static_cast<std::uint8_t*>(loadMapTrampoline);
		std::memcpy(loadTrampoline, base + loadMapRva, loadMapHookBytes);
		const auto loadJumpBack = absoluteJump(base + loadMapRva + loadMapHookBytes);
		std::memcpy(loadTrampoline + loadMapHookBytes, loadJumpBack.data(),
			loadJumpBack.size());
		originalLoadMap = reinterpret_cast<LoadMapFn>(loadTrampoline);
		auto* followerTrampoline = static_cast<std::uint8_t*>(
			followerFrameTrampoline);
		std::memcpy(followerTrampoline, base + updateAllyFollowerFrameRva,
			updateAllyFollowerFrameHookBytes);
		const auto followerJumpBack = absoluteJump(base
			+ updateAllyFollowerFrameRva + updateAllyFollowerFrameHookBytes);
		std::memcpy(followerTrampoline + updateAllyFollowerFrameHookBytes,
			followerJumpBack.data(), followerJumpBack.size());
		originalUpdateAllyFollowerFrame =
			reinterpret_cast<UpdateAllyFollowerFrameFn>(followerTrampoline);
		auto* statsTrampoline = static_cast<std::uint8_t*>(getStatsTrampoline);
		std::memcpy(statsTrampoline, base + layout::getStatsRva,
			getStatsHookBytes);
		const auto statsJumpBack = absoluteJump(base + layout::getStatsRva
			+ getStatsHookBytes);
		std::memcpy(statsTrampoline + getStatsHookBytes, statsJumpBack.data(),
			statsJumpBack.size());
		originalGetStats = reinterpret_cast<GetStatsFn>(statsTrampoline);

		addPatch(patches, drawMinimapRva, drawMinimapSignature,
			absoluteJump(reinterpret_cast<void*>(&drawMinimapHook)));
		addPatch(patches, loadMapRva, loadMapSignature,
			absoluteJump(reinterpret_cast<void*>(&loadMapHook)));
		addPatch(patches, updateAllyFollowerFrameRva,
			updateAllyFollowerFrameSignature,
			absoluteJump(reinterpret_cast<void*>(&updateAllyFollowerFrameHook)));
		addPatch(patches, layout::getStatsRva, layout::getStatsSignature,
			absoluteJump(reinterpret_cast<void*>(&getStatsHook)));
		for ( std::size_t index = 0; index < exitTooltipCallRvas.size(); ++index )
		{
			addPatch(patches, exitTooltipCallRvas[index], exitTooltipCalls[index],
				relativeCall(exitTooltipCallRvas[index], exitTooltipRelay));
		}
		addPatch(patches, terrainImageDrawCallRva, terrainImageDrawCall,
			relativeCall(terrainImageDrawCallRva, imageDrawRelay));
		std::vector<std::uint8_t> primaryTooltipHeight = {
			0x8B, 0x48, 0x30,             // width = rendered text width
			0x89, 0x4C, 0x24, 0x78,
			0x8B, 0x48, 0x38,             // rendered line count
			0x0F, 0xAF, 0x48, 0x34,       // height = lines * line height
			0x8D, 0x41, 0x08,             // retain Barony's padding
			0x89, 0x44, 0x24, 0x7C,
		};
		primaryTooltipHeight.resize(worldTooltipHeightPrimary.size(), 0x90);
		addPatch(patches, worldTooltipHeightPrimaryRva,
			worldTooltipHeightPrimary, std::move(primaryTooltipHeight));
		std::vector<std::uint8_t> alternateTooltipHeight = {
			0x8B, 0x48, 0x30,             // width = rendered text width
			0x89, 0x4C, 0x24, 0x34,
			0x89, 0x4C, 0x24, 0x78,
			0x8B, 0x48, 0x38,             // rendered line count
			0x0F, 0xAF, 0x48, 0x34,       // height = lines * line height
			0x8D, 0x41, 0x08,             // retain Barony's padding
			0x89, 0x44, 0x24, 0x7C,
		};
		alternateTooltipHeight.resize(worldTooltipHeightAlternate.size(), 0x90);
		addPatch(patches, worldTooltipHeightAlternateRva,
			worldTooltipHeightAlternate, std::move(alternateTooltipHeight));
		for ( std::size_t index = 0; index < headstoneDialogueCallRvas.size(); ++index )
		{
			addPatch(patches, headstoneDialogueCallRvas[index],
				headstoneDialogueCalls[index], relativeCall(
					headstoneDialogueCallRvas[index], headstoneDialogueRelay));
		}
		addPatch(patches, lootBagColorCallRva, lootBagColorCall,
			relativeCall(lootBagColorCallRva, playerColorRelay));
		addPatch(patches, lootBagColorblindCallRva, lootBagColorblindCall,
			relativeCall(lootBagColorblindCallRva, playerColorRelay));
		auto pingJump = absoluteJump(pingRelay);
		pingJump.resize(pingHookBytes, 0x90);
		addPatch(patches, pingColorBlockRva, pingColorBlock, std::move(pingJump));
		addPatch(patches, calloutColorCallRva, calloutColorCall,
			relativeCall(calloutColorCallRva, playerColorRelay));
		for ( std::size_t index = 0; index < ghostColorCallRvas.size(); ++index )
		{
			addPatch(patches, ghostColorCallRvas[index], ghostColorCalls[index],
				relativeCall(ghostColorCallRvas[index], playerColorRelay));
			addPatch(patches, ghostTriangleCallRvas[index], ghostTriangleCalls[index],
				relativeCall(ghostTriangleCallRvas[index], ghostTriangleRelay));
		}
		addPointerPatch(patches, udpRecvIatRva, udpRecv, &udpRecvHook);
		addPointerPatch(patches, udpSendIatRva, udpSend, &udpSendHook);
		return true;
	}

	void release()
	{
		if ( drawTrampoline )
		{
			VirtualFree(drawTrampoline, 0, MEM_RELEASE);
			drawTrampoline = nullptr;
			originalDrawMinimap = nullptr;
		}
		if ( loadMapTrampoline )
		{
			VirtualFree(loadMapTrampoline, 0, MEM_RELEASE);
			loadMapTrampoline = nullptr;
			originalLoadMap = nullptr;
		}
		if ( followerFrameTrampoline )
		{
			VirtualFree(followerFrameTrampoline, 0, MEM_RELEASE);
			followerFrameTrampoline = nullptr;
			originalUpdateAllyFollowerFrame = nullptr;
		}
		if ( getStatsTrampoline )
		{
			VirtualFree(getStatsTrampoline, 0, MEM_RELEASE);
			getStatsTrampoline = nullptr;
			originalGetStats = nullptr;
		}
		if ( relayPage )
		{
			VirtualFree(relayPage, 0, MEM_RELEASE);
			relayPage = nullptr;
		}
		sharedFollowerRoster.reset();
		publishedFollowerRoster.reset();
		syntheticFollowerStats.clear();
	}
}
