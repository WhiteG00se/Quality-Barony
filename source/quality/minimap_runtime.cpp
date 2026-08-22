#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <utility>
#include <vector>

#include "minimap.hpp"
#include "minimap_runtime.hpp"

namespace
{
	constexpr double pi = 3.14159265358979323846;
	constexpr std::size_t relayStride = 256;
	constexpr std::size_t drawHookBytes = 14;
	constexpr std::size_t pingHookBytes = 23;

	constexpr std::uintptr_t drawMinimapRva = 0x00708070;
	constexpr std::uintptr_t drawTriangleRva = 0x007073F0;
	constexpr std::uintptr_t playerColorRva = 0x0086F6D0;
	constexpr std::uintptr_t playSoundRva = 0x00652800;
	constexpr std::uintptr_t actPlayerRva = 0x00355FE0;
	constexpr std::uintptr_t actMonsterRva = 0x0032D4D0;
	constexpr std::uintptr_t actCustomPortalRva = 0x0031F210;
	constexpr std::uintptr_t actWorkbenchRva = 0x002E2050;
	constexpr std::uintptr_t actCauldronRva = 0x002E14D0;
	constexpr std::uintptr_t lootBagColorCallRva = 0x00709DDD;
	constexpr std::uintptr_t lootBagColorblindCallRva = 0x00709E1F;
	constexpr std::uintptr_t pingColorBlockRva = 0x0070A609;
	constexpr std::uintptr_t pingColorBlockReturnRva = 0x0070A620;
	constexpr std::uintptr_t calloutColorCallRva = 0x0070AE75;
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

	constexpr std::size_t entityUid = 0x68;
	constexpr std::size_t entityX = 0xD8;
	constexpr std::size_t entityY = 0xE0;
	constexpr std::size_t entityYaw = 0xF0;
	constexpr std::size_t entitySprite = 0x140;
	constexpr std::size_t entitySkill = 0x288;
	constexpr std::size_t entityFlags = 0x378;
	constexpr std::size_t entityBehavior = 0x1350;
	constexpr std::size_t skillPlayerIndex = 2;
	constexpr std::size_t skillMonsterAllyIndex = 42;
	constexpr std::size_t skillShadowTaggedUid = 54;
	constexpr std::size_t skillShowOnMap = 59;

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
	constexpr std::array<std::uint8_t, 16> playSoundSignature = {
		0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x74,
		0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x48,
	};
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
	struct Node { Node* next; Node* previous; void* list; void* element; };
	struct List { Node* first; Node* last; };

	enum class VisualKind
	{
		Exit, Boulder, Workbench, Cauldron, Minotaur, ShadowCreature,
		DetectedHostile, Player, Follower,
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
	using DrawTriangleFn = void (*)(void*, double, double, double, double,
		Rect, std::uint32_t, bool);
	using PlayerColorFn = std::uint32_t (*)(int, bool, bool);
	using PlaySoundFn = void (*)(int, int);
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
	DrawTriangleFn drawTriangle = nullptr;
	PlayerColorFn originalPlayerColor = nullptr;
	PlaySoundFn playSound = nullptr;
	void* relayPage = nullptr;
	void* drawTrampoline = nullptr;
	Rect currentRect {};
	int currentViewer = 0;
	std::vector<Visual> visuals;
	std::vector<Mutation> mutations;
	std::uintptr_t lastEntityList = 0;
	std::uint32_t lastTicks = 0;
	bool minotaurAlerted = false;

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
			}
		}

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

			if ( appearance == quality::minimap::MarkerAppearance::Exit )
			{
				if ( quality::minimap::exitVisible(visibility, showOnMap,
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
				if ( explored )
				{
					visuals.push_back({appearance
						== quality::minimap::MarkerAppearance::Workbench
							? VisualKind::Workbench : VisualKind::Cauldron,
						tileX + .5, tileY + .5, 0.0, -1, false});
				}
				suppressVanilla(entity, false);
				continue;
			}

			if ( appearance == quality::minimap::MarkerAppearance::DetectedHostile )
			{
				visuals.push_back({VisualKind::DetectedHostile, tileX + .5,
					tileY + .5, 0.0, -1, false});
				suppressVanilla(entity, false);
			}
			if ( shadowTaggedUids.count(uid(entity)) && !partyUids.count(uid(entity)) )
			{
				visuals.push_back({VisualKind::ShadowCreature, worldX / 16.0,
					worldY / 16.0, 0.0, -1, true});
				if ( !quality::minimap::isDetectedHostile(showOnMap) )
				{
					suppressVanilla(entity, false);
				}
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

	void renderQualityLayer()
	{
		RenderScope scope;
		if ( !scope.valid ) { return; }
		glLineWidth(1.5f);
		for ( const auto& visual : visuals )
		{
			const auto marker = transform(visual.x, visual.y, scope);
			const float radius = std::min(marker.unitX, marker.unitY) * .5f;
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
					circle(marker.x, marker.y, radius, quality::minimap::stationBlue, true);
					if ( visual.kind == VisualKind::Workbench )
					{
						line({{marker.x-marker.unitX*.34f, marker.y-marker.unitY*.16f},
							{marker.x-marker.unitX*.22f, marker.y+marker.unitY*.30f},
							{marker.x, marker.y-marker.unitY*.02f},
							{marker.x+marker.unitX*.22f, marker.y+marker.unitY*.30f},
							{marker.x+marker.unitX*.34f, marker.y-marker.unitY*.16f}},
							quality::minimap::color(0, 64, 64));
					}
					else
					{
						std::vector<std::pair<float, float>> arc;
						for ( int index = 4; index <= 28; ++index )
						{
							const float angle = static_cast<float>(2.0*pi*index/32.0);
							arc.emplace_back(marker.x + std::sin(angle)*marker.unitX*.31f,
								marker.y + std::cos(angle)*marker.unitY*.31f);
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
				case VisualKind::DetectedHostile:
					circle(marker.x, marker.y, radius,
						quality::minimap::minotaurRed, true);
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
		observeWorld();
		originalDrawMinimap(player, rect, shared);
		restoreVanilla();
		renderQualityLayer();
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
			|| !matches(playerColorRva, playerColorSignature)
			|| !matches(playSoundRva, playSoundSignature)
			|| !matches(lootBagColorCallRva, lootBagColorCall)
			|| !matches(lootBagColorblindCallRva, lootBagColorblindCall)
			|| !matches(pingColorBlockRva, pingColorBlock)
			|| !matches(calloutColorCallRva, calloutColorCall) )
		{
			return false;
		}
		for ( std::size_t index = 0; index < ghostColorCallRvas.size(); ++index )
		{
			if ( !matches(ghostColorCallRvas[index], ghostColorCalls[index])
				|| !matches(ghostTriangleCallRvas[index], ghostTriangleCalls[index]) )
			{
				return false;
			}
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
		drawTriangle = reinterpret_cast<DrawTriangleFn>(base + drawTriangleRva);
		playSound = reinterpret_cast<PlaySoundFn>(base + playSoundRva);
		relayPage = allocateNearModule(3 * relayStride);
		drawTrampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE);
		if ( !relayPage || !drawTrampoline )
		{
			release();
			return false;
		}

		auto* relay = static_cast<std::uint8_t*>(relayPage);
		auto* playerColorRelay = relay;
		auto* ghostTriangleRelay = relay + relayStride;
		auto* pingRelay = relay + 2 * relayStride;
		writeRelay(playerColorRelay, reinterpret_cast<void*>(&playerColorHook));
		writeRelay(ghostTriangleRelay, reinterpret_cast<void*>(&ghostTriangleHook));
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

		addPatch(patches, drawMinimapRva, drawMinimapSignature,
			absoluteJump(reinterpret_cast<void*>(&drawMinimapHook)));
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
		if ( relayPage )
		{
			VirtualFree(relayPage, 0, MEM_RELEASE);
			relayPage = nullptr;
		}
	}
}
