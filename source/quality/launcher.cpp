#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <tlhelp32.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	constexpr wchar_t gameExecutableName[] = L"barony.exe";
	constexpr wchar_t runtimeDllName[] = L"QualityBarony.dll";
	constexpr char expectedSha256[] =
		"8566DA37BC39EA5A1ED08A8AD57608AF4F019FB415869258FB3C1D310B4419E4";
	constexpr char initializerName[] = "QualityBaronyInitialize";
	constexpr std::uintptr_t xpCaptureRva = 0x00457E45;
	constexpr std::uintptr_t checkEnemyRva = 0x0045A7C0;
	constexpr std::uintptr_t setHpRva = 0x00485380;
	constexpr std::uintptr_t drawMinimapRva = 0x00708070;
	constexpr std::uintptr_t exitTooltipRva = 0x005F451B;
	constexpr std::uintptr_t terrainImageDrawCallRva = 0x00708EAF;

	struct FoundProcess
	{
		HANDLE handle = nullptr;
		DWORD id = 0;
	};

	struct ProcessBasicInformation
	{
		void* reserved1;
		void* pebBaseAddress;
		void* reserved2[2];
		std::uintptr_t processId;
		void* reserved3;
	};

	using NtQueryInformationProcessFn = LONG (NTAPI*)(HANDLE, ULONG, void*,
		ULONG, ULONG*);

	std::wstring windowsError(const DWORD code = GetLastError())
	{
		wchar_t* buffer = nullptr;
		const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS;
		FormatMessageW(flags, nullptr, code, 0,
			reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
		std::wstring result = buffer ? buffer : L"unknown Windows error";
		if ( buffer )
		{
			LocalFree(buffer);
		}
		return result;
	}

	std::wstring quoteArgument(const std::wstring& value)
	{
		if ( value.find_first_of(L" \t\"") == std::wstring::npos )
		{
			return value;
		}
		std::wstring output = L"\"";
		std::size_t slashes = 0;
		for ( const wchar_t character : value )
		{
			if ( character == L'\\' )
			{
				++slashes;
			}
			else if ( character == L'\"' )
			{
				output.append(slashes * 2 + 1, L'\\');
				output.push_back(L'\"');
				slashes = 0;
			}
			else
			{
				output.append(slashes, L'\\');
				slashes = 0;
				output.push_back(character);
			}
		}
		output.append(slashes * 2, L'\\');
		output.push_back(L'\"');
		return output;
	}

	fs::path launcherDirectory()
	{
		std::vector<wchar_t> buffer(32768);
		const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
			static_cast<DWORD>(buffer.size()));
		return fs::path(std::wstring(buffer.data(), length)).parent_path();
	}

	std::wstring readRegistryString(HKEY root, const wchar_t* key,
		const wchar_t* name)
	{
		DWORD bytes = 0;
		if ( RegGetValueW(root, key, name, RRF_RT_REG_SZ,
			nullptr, nullptr, &bytes) != ERROR_SUCCESS )
		{
			return {};
		}
		std::vector<wchar_t> value(bytes / sizeof(wchar_t) + 1);
		if ( RegGetValueW(root, key, name, RRF_RT_REG_SZ,
			nullptr, value.data(), &bytes) != ERROR_SUCCESS )
		{
			return {};
		}
		return value.data();
	}

	void addSteamLibrary(std::vector<fs::path>& candidates, const fs::path& library)
	{
		const fs::path candidate = library / L"steamapps" / L"common" / L"Barony";
		if ( fs::is_regular_file(candidate / gameExecutableName) )
		{
			candidates.push_back(candidate);
		}
	}

	fs::path locateGame()
	{
		std::vector<fs::path> candidates;
		if ( const wchar_t* environmentPath = _wgetenv(L"BARONY_GAME_DIR");
			environmentPath && *environmentPath )
		{
			candidates.emplace_back(environmentPath);
		}

		const std::array<std::wstring, 3> steamPaths = {
			readRegistryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"),
			readRegistryString(HKEY_LOCAL_MACHINE,
				L"Software\\WOW6432Node\\Valve\\Steam", L"InstallPath"),
			readRegistryString(HKEY_LOCAL_MACHINE,
				L"Software\\Valve\\Steam", L"InstallPath"),
		};
		for ( const auto& steamPath : steamPaths )
		{
			if ( steamPath.empty() )
			{
				continue;
			}
			const fs::path steam(steamPath);
			addSteamLibrary(candidates, steam);
			std::ifstream input(steam / L"steamapps" / L"libraryfolders.vdf");
			if ( input )
			{
				const std::regex pathPattern(R"REGEX("path"\s+"([^"]+)")REGEX");
				std::string line;
				std::smatch match;
				while ( std::getline(input, line) )
				{
					if ( !std::regex_search(line, match, pathPattern) )
					{
						continue;
					}
					std::string text = match[1].str();
					std::string unescaped;
					for ( std::size_t index = 0; index < text.size(); ++index )
					{
						if ( text[index] == '\\' && index + 1 < text.size()
							&& text[index + 1] == '\\' )
						{
							++index;
						}
						unescaped.push_back(text[index]);
					}
					addSteamLibrary(candidates, fs::u8path(unescaped));
				}
			}
		}
		for ( const auto& candidate : candidates )
		{
			if ( fs::is_regular_file(candidate / gameExecutableName) )
			{
				return fs::absolute(candidate);
			}
		}
		return {};
	}

	fs::path locateSteamExecutable()
	{
		const std::array<std::wstring, 4> candidates = {
			readRegistryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamExe"),
			readRegistryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"),
			readRegistryString(HKEY_LOCAL_MACHINE,
				L"Software\\WOW6432Node\\Valve\\Steam", L"InstallPath"),
			readRegistryString(HKEY_LOCAL_MACHINE,
				L"Software\\Valve\\Steam", L"InstallPath"),
		};
		for ( const auto& candidate : candidates )
		{
			if ( candidate.empty() )
			{
				continue;
			}
			fs::path path(candidate);
			if ( _wcsicmp(path.filename().c_str(), L"steam.exe") != 0 )
			{
				path /= L"steam.exe";
			}
			if ( fs::is_regular_file(path) )
			{
				return fs::absolute(path);
			}
		}
		return {};
	}

	bool pathsEqual(const fs::path& left, const fs::path& right)
	{
		std::error_code leftError;
		std::error_code rightError;
		const fs::path normalizedLeft = fs::weakly_canonical(left, leftError);
		const fs::path normalizedRight = fs::weakly_canonical(right, rightError);
		const std::wstring leftText = (leftError ? fs::absolute(left) : normalizedLeft).wstring();
		const std::wstring rightText = (rightError ? fs::absolute(right) : normalizedRight).wstring();
		return _wcsicmp(leftText.c_str(), rightText.c_str()) == 0;
	}

	FoundProcess findBaronyProcess(const fs::path& expectedExecutable)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if ( snapshot == INVALID_HANDLE_VALUE )
		{
			return {};
		}
		PROCESSENTRY32W entry {};
		entry.dwSize = sizeof(entry);
		FoundProcess result;
		if ( Process32FirstW(snapshot, &entry) )
		{
			do
			{
				if ( _wcsicmp(entry.szExeFile, gameExecutableName) != 0 )
				{
					continue;
				}
				HANDLE process = OpenProcess(PROCESS_CREATE_THREAD
					| PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_OPERATION
					| PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE
					| PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
				if ( !process )
				{
					continue;
				}
				std::vector<wchar_t> imagePath(32768);
				DWORD characters = static_cast<DWORD>(imagePath.size());
				if ( QueryFullProcessImageNameW(process, 0, imagePath.data(), &characters)
					&& pathsEqual(fs::path(std::wstring(imagePath.data(), characters)),
						expectedExecutable) )
				{
					result = { process, entry.th32ProcessID };
					break;
				}
				CloseHandle(process);
			}
			while ( Process32NextW(snapshot, &entry) );
		}
		CloseHandle(snapshot);
		return result;
	}

	bool requestSteamLaunch(const fs::path& steamExecutable,
		const std::vector<std::wstring>& gameArguments, std::wstring& error)
	{
		std::wstring commandLine = quoteArgument(steamExecutable.wstring())
			+ L" -applaunch 371970";
		for ( const auto& argument : gameArguments )
		{
			commandLine += L" " + quoteArgument(argument);
		}
		std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
		mutableCommand.push_back(L'\0');
		STARTUPINFOW startup {};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process {};
		if ( !CreateProcessW(steamExecutable.c_str(), mutableCommand.data(),
			nullptr, nullptr, FALSE, 0, nullptr,
			steamExecutable.parent_path().c_str(), &startup, &process) )
		{
			error = L"Steam could not be asked to launch Barony: " + windowsError();
			return false;
		}
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		return true;
	}

	FoundProcess waitForSteamBarony(const fs::path& expectedExecutable)
	{
		const ULONGLONG deadline = GetTickCount64() + 180000;
		while ( GetTickCount64() < deadline )
		{
			if ( auto process = findBaronyProcess(expectedExecutable); process.handle )
			{
				return process;
			}
			Sleep(50);
		}
		return {};
	}

	std::string sha256(const fs::path& path)
	{
		std::ifstream input(path, std::ios::binary);
		if ( !input )
		{
			return {};
		}
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		BCRYPT_HASH_HANDLE hash = nullptr;
		DWORD objectBytes = 0;
		DWORD hashBytes = 0;
		DWORD resultBytes = 0;
		std::vector<std::uint8_t> object;
		std::vector<std::uint8_t> digest;
		std::array<char, 64 * 1024> buffer {};
		std::string result;

		if ( BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
			nullptr, 0) < 0
			|| BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
				reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes),
				&resultBytes, 0) < 0
			|| BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
				reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes),
				&resultBytes, 0) < 0 )
		{
			goto cleanup;
		}
		object.resize(objectBytes);
		digest.resize(hashBytes);
		if ( BCryptCreateHash(algorithm, &hash, object.data(), objectBytes,
			nullptr, 0, 0) < 0 )
		{
			goto cleanup;
		}
		while ( input )
		{
			input.read(buffer.data(), buffer.size());
			const auto count = input.gcount();
			if ( count > 0 && BCryptHashData(hash,
				reinterpret_cast<PUCHAR>(buffer.data()),
				static_cast<ULONG>(count), 0) < 0 )
			{
				goto cleanup;
			}
		}
		if ( BCryptFinishHash(hash, digest.data(), hashBytes, 0) < 0 )
		{
			goto cleanup;
		}
		{
			std::ostringstream text;
			text << std::uppercase << std::hex << std::setfill('0');
			for ( const auto byte : digest )
			{
				text << std::setw(2) << static_cast<unsigned>(byte);
			}
			result = text.str();
		}

	cleanup:
		if ( hash )
		{
			BCryptDestroyHash(hash);
		}
		if ( algorithm )
		{
			BCryptCloseAlgorithmProvider(algorithm, 0);
		}
		return result;
	}

	std::uintptr_t remoteModuleBase(const DWORD processId,
		const wchar_t* moduleName)
	{
		for ( int attempt = 0; attempt < 20; ++attempt )
		{
			HANDLE snapshot = CreateToolhelp32Snapshot(
				TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
			if ( snapshot != INVALID_HANDLE_VALUE )
			{
				MODULEENTRY32W entry {};
				entry.dwSize = sizeof(entry);
				if ( Module32FirstW(snapshot, &entry) )
				{
					do
					{
						if ( _wcsicmp(entry.szModule, moduleName) == 0 )
						{
							const auto result = reinterpret_cast<std::uintptr_t>(
								entry.modBaseAddr);
							CloseHandle(snapshot);
							return result;
						}
					}
					while ( Module32NextW(snapshot, &entry) );
				}
				CloseHandle(snapshot);
			}
			Sleep(25);
		}
		return 0;
	}

	bool runRemoteThread(HANDLE process, const void* start, void* parameter,
		DWORD& result, std::wstring& error)
	{
		HANDLE thread = CreateRemoteThread(process, nullptr, 0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(const_cast<void*>(start)),
			parameter, 0, nullptr);
		if ( !thread )
		{
			error = L"CreateRemoteThread failed: " + windowsError();
			return false;
		}
		const DWORD wait = WaitForSingleObject(thread, 30000);
		const bool success = wait == WAIT_OBJECT_0
			&& GetExitCodeThread(thread, &result);
		if ( !success )
		{
			error = L"The remote initialization thread timed out or failed.";
		}
		CloseHandle(thread);
		return success;
	}

	bool injectAndInitialize(const FoundProcess& process, const fs::path& dllPath,
		std::wstring& error)
	{
		const std::wstring absolutePath = fs::absolute(dllPath).wstring();
		const SIZE_T pathBytes = (absolutePath.size() + 1) * sizeof(wchar_t);
		void* remotePath = VirtualAllocEx(process.handle, nullptr, pathBytes,
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if ( !remotePath )
		{
			error = L"VirtualAllocEx failed: " + windowsError();
			return false;
		}

		bool success = false;
		do
		{
			if ( !WriteProcessMemory(process.handle, remotePath,
				absolutePath.c_str(), pathBytes, nullptr) )
			{
				error = L"WriteProcessMemory failed: " + windowsError();
				break;
			}

			const HMODULE localKernel32 = GetModuleHandleW(L"kernel32.dll");
			const FARPROC localLoadLibrary = localKernel32
				? GetProcAddress(localKernel32, "LoadLibraryW") : nullptr;
			const std::uintptr_t remoteKernel32 = remoteModuleBase(
				process.id, L"kernel32.dll");
			if ( !localKernel32 || !localLoadLibrary )
			{
				error = L"Could not locate LoadLibraryW.";
				break;
			}
			const auto loadLibraryOffset = reinterpret_cast<std::uintptr_t>(
				localLoadLibrary) - reinterpret_cast<std::uintptr_t>(localKernel32);
			// A freshly created suspended process may not expose kernel32.dll to
			// the module snapshot yet. System DLLs use the same mapped address in
			// processes within the current Windows boot session, so the local
			// address is the safe fallback for this diagnostic launch state.
			const auto remoteLoadLibrary = remoteKernel32
				? reinterpret_cast<void*>(remoteKernel32 + loadLibraryOffset)
				: reinterpret_cast<void*>(localLoadLibrary);
			DWORD loadResult = 0;
			if ( !runRemoteThread(process.handle, remoteLoadLibrary, remotePath,
				loadResult, error) || loadResult == 0 )
			{
				if ( error.empty() )
				{
					error = L"Windows could not load QualityBarony.dll.";
				}
				break;
			}

			const std::uintptr_t remoteRuntime = remoteModuleBase(
				process.id, dllPath.filename().c_str());
			HMODULE localRuntime = LoadLibraryExW(dllPath.c_str(), nullptr,
				DONT_RESOLVE_DLL_REFERENCES);
			const FARPROC localInitializer = localRuntime
				? GetProcAddress(localRuntime, initializerName) : nullptr;
			if ( !remoteRuntime || !localRuntime || !localInitializer )
			{
				if ( localRuntime )
				{
					FreeLibrary(localRuntime);
				}
				error = L"QualityBarony.dll does not expose its initializer.";
				break;
			}
			const auto initializerOffset = reinterpret_cast<std::uintptr_t>(
				localInitializer) - reinterpret_cast<std::uintptr_t>(localRuntime);
			FreeLibrary(localRuntime);
			const auto remoteInitializer = reinterpret_cast<void*>(
				remoteRuntime + initializerOffset);
			DWORD initializeResult = 0;
			if ( !runRemoteThread(process.handle, remoteInitializer, nullptr,
				initializeResult, error) || initializeResult != 1 )
			{
				if ( error.empty() )
				{
					error = L"QualityBarony.dll rejected the process because an internal "
						L"v5.0.2 signature did not match.";
				}
				break;
			}
			success = true;
		}
		while ( false );

		VirtualFreeEx(process.handle, remotePath, 0, MEM_RELEASE);
		return success;
	}

	bool corruptSignatureForTest(const FoundProcess& process,
		const std::uintptr_t signatureRva, const wchar_t* signatureName,
		std::wstring& error)
	{
		std::uintptr_t gameBase = remoteModuleBase(process.id, gameExecutableName);
		if ( !gameBase )
		{
			const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
			const auto query = ntdll ? reinterpret_cast<NtQueryInformationProcessFn>(
				GetProcAddress(ntdll, "NtQueryInformationProcess")) : nullptr;
			ProcessBasicInformation information {};
			if ( query && query(process.handle, 0, &information,
				sizeof(information), nullptr) >= 0 && information.pebBaseAddress )
			{
				ReadProcessMemory(process.handle,
					static_cast<std::uint8_t*>(information.pebBaseAddress) + 0x10,
					&gameBase, sizeof(gameBase), nullptr);
			}
		}
		if ( !gameBase )
		{
			error = L"Could not locate the suspended Barony module.";
			return false;
		}
		auto* address = reinterpret_cast<void*>(gameBase + signatureRva);
		std::uint8_t original = 0;
		if ( !ReadProcessMemory(process.handle, address, &original,
			sizeof(original), nullptr) )
		{
			error = std::wstring(L"Could not read the suspended ") + signatureName
				+ L" signature: " + windowsError();
			return false;
		}
		DWORD oldProtection = 0;
		if ( !VirtualProtectEx(process.handle, address, sizeof(original),
			PAGE_EXECUTE_READWRITE, &oldProtection) )
		{
			error = std::wstring(L"Could not unlock the suspended ") + signatureName
				+ L" signature: " + windowsError();
			return false;
		}
		const std::uint8_t altered = original ^ 1U;
		const bool written = WriteProcessMemory(process.handle, address, &altered,
			sizeof(altered), nullptr) != FALSE;
		DWORD ignored = 0;
		VirtualProtectEx(process.handle, address, sizeof(original),
			oldProtection, &ignored);
		FlushInstructionCache(process.handle, address, sizeof(original));
		if ( !written )
		{
			error = std::wstring(L"Could not alter the suspended ") + signatureName
				+ L" signature: " + windowsError();
			return false;
		}
		return true;
	}

	void printUsage()
	{
		std::wcout << L"Quality Barony Launcher for Barony v5.0.2 x64\n\n"
			L"Usage: QualityBaronyLauncher.exe [--game-dir PATH] "
			L"[--verify-only|--test-injection|--test-signature-rejection|"
			L"--test-exp-credit-signature-rejection|"
			L"--test-friendly-fire-signature-rejection|"
			L"--test-minimap-signature-rejection|--test-reveal-signature-rejection|"
			L"--test-item-marker-signature-rejection] "
			L"[Barony arguments...]\n"
			L"Normal startup asks Steam to launch Barony so Steam Cloud tracks the session.\n"
			L"Set BARONY_GAME_DIR if Steam auto-detection does not find the game.\n";
	}
}

int wmain(int argc, wchar_t** argv)
{
	fs::path gameDirectory;
	fs::path runtimeDll;
	bool verifyOnly = false;
	bool injectionTest = false;
	bool signatureRejectionTest = false;
	bool friendlyFireSignatureRejectionTest = false;
	bool expCreditSignatureRejectionTest = false;
	bool minimapSignatureRejectionTest = false;
	bool revealSignatureRejectionTest = false;
	bool itemMarkerSignatureRejectionTest = false;
	std::vector<std::wstring> gameArguments;
	for ( int index = 1; index < argc; ++index )
	{
		const std::wstring argument = argv[index];
		if ( argument == L"--help" || argument == L"-h" )
		{
			printUsage();
			return 0;
		}
		if ( argument == L"--game-dir" )
		{
			if ( ++index >= argc )
			{
				std::wcerr << L"Error: --game-dir requires a path.\n";
				return 2;
			}
			gameDirectory = argv[index];
		}
		else if ( argument == L"--runtime-dll" )
		{
			if ( ++index >= argc )
			{
				std::wcerr << L"Error: --runtime-dll requires a path.\n";
				return 2;
			}
			runtimeDll = argv[index];
		}
		else if ( argument == L"--verify-only" )
		{
			verifyOnly = true;
		}
		else if ( argument == L"--test-injection" )
		{
			injectionTest = true;
		}
		else if ( argument == L"--test-signature-rejection" )
		{
			signatureRejectionTest = true;
		}
		else if ( argument == L"--test-friendly-fire-signature-rejection" )
		{
			friendlyFireSignatureRejectionTest = true;
		}
		else if ( argument == L"--test-exp-credit-signature-rejection" )
		{
			expCreditSignatureRejectionTest = true;
		}
		else if ( argument == L"--test-minimap-signature-rejection" )
		{
			minimapSignatureRejectionTest = true;
		}
		else if ( argument == L"--test-reveal-signature-rejection" )
		{
			revealSignatureRejectionTest = true;
		}
		else if ( argument == L"--test-item-marker-signature-rejection" )
		{
			itemMarkerSignatureRejectionTest = true;
		}
		else if ( argument == L"--" )
		{
			while ( ++index < argc )
			{
				gameArguments.emplace_back(argv[index]);
			}
		}
		else
		{
			gameArguments.push_back(argument);
		}
	}

	if ( gameDirectory.empty() )
	{
		gameDirectory = locateGame();
	}
	if ( gameDirectory.empty() )
	{
		std::wcerr << L"Error: Barony was not found. Use --game-dir PATH.\n";
		return 3;
	}
	gameDirectory = fs::absolute(gameDirectory);
	const fs::path executable = gameDirectory / gameExecutableName;
	if ( runtimeDll.empty() )
	{
		runtimeDll = launcherDirectory() / runtimeDllName;
	}
	if ( !fs::is_regular_file(executable) )
	{
		std::wcerr << L"Error: " << executable << L" does not exist.\n";
		return 4;
	}
	const std::string actualHash = sha256(executable);
	if ( actualHash != expectedSha256 )
	{
		std::wcerr << L"Error: unsupported or altered barony.exe.\nExpected: "
			<< expectedSha256 << L"\nActual:   "
			<< std::wstring(actualHash.begin(), actualHash.end()) << L"\n";
		return 5;
	}
	std::wcout << L"Verified Barony v5.0.2 x64: " << executable << L"\n";
	if ( verifyOnly )
	{
		return 0;
	}
	if ( !fs::is_regular_file(runtimeDll) )
	{
		std::wcerr << L"Error: runtime DLL is missing: " << runtimeDll << L"\n";
		return 6;
	}

	if ( injectionTest || signatureRejectionTest
		|| friendlyFireSignatureRejectionTest || expCreditSignatureRejectionTest
		|| minimapSignatureRejectionTest
		|| revealSignatureRejectionTest || itemMarkerSignatureRejectionTest )
	{
		std::wstring commandLine = quoteArgument(executable.wstring());
		std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
		mutableCommand.push_back(L'\0');
		STARTUPINFOW startup {};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process {};
		if ( !CreateProcessW(executable.c_str(), mutableCommand.data(), nullptr,
			nullptr, FALSE, CREATE_SUSPENDED, nullptr, gameDirectory.c_str(),
			&startup, &process) )
		{
			std::wcerr << L"Error: could not create the suspended test process: "
				<< windowsError() << L"\n";
			return 7;
		}
		const FoundProcess found { process.hProcess, process.dwProcessId };
		std::wstring error;
		const bool rejectionTest = signatureRejectionTest
			|| friendlyFireSignatureRejectionTest
			|| expCreditSignatureRejectionTest
			|| minimapSignatureRejectionTest || revealSignatureRejectionTest
			|| itemMarkerSignatureRejectionTest;
		const std::uintptr_t corruptRva = expCreditSignatureRejectionTest
			? setHpRva
			: (friendlyFireSignatureRejectionTest ? checkEnemyRva
			: (itemMarkerSignatureRejectionTest ? terrainImageDrawCallRva
			: (revealSignatureRejectionTest ? exitTooltipRva
			: (minimapSignatureRejectionTest ? drawMinimapRva : xpCaptureRva))));
		const wchar_t* corruptName = expCreditSignatureRejectionTest
			? L"EXP credit"
			: (friendlyFireSignatureRejectionTest ? L"friendly fire"
			: (itemMarkerSignatureRejectionTest ? L"party item marker"
			: (revealSignatureRejectionTest ? L"exit reveal"
			: (minimapSignatureRejectionTest ? L"minimap" : L"EXP"))));
		if ( rejectionTest && !corruptSignatureForTest(found, corruptRva,
			corruptName, error) )
		{
			TerminateProcess(process.hProcess, 200);
			WaitForSingleObject(process.hProcess, 5000);
			CloseHandle(process.hThread);
			CloseHandle(process.hProcess);
			std::wcerr << L"Error: " << error << L"\n";
			return 8;
		}
		const bool injected = injectAndInitialize(found, runtimeDll, error);
		const bool expectedRejection = rejectionTest && !injected
			&& error.find(L"rejected the process") != std::wstring::npos;
		TerminateProcess(process.hProcess,
			(injected || expectedRejection) ? 0 : 200);
		WaitForSingleObject(process.hProcess, 5000);
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		if ( expectedRejection )
		{
			std::wcout << L"Unsupported in-memory " << corruptName
				<< L" signature was rejected.\n";
			return 0;
		}
		if ( !injected )
		{
			std::wcerr << L"Error: " << error << L"\n";
			return 8;
		}
		std::wcout << L"DLL injection and all runtime signatures verified.\n";
		return 0;
	}

	const fs::path steamExecutable = locateSteamExecutable();
	if ( steamExecutable.empty() )
	{
		std::wcerr << L"Error: Steam was not found. Install or start Steam, then try again.\n";
		return 9;
	}
	if ( auto existing = findBaronyProcess(executable); existing.handle )
	{
		CloseHandle(existing.handle);
		std::wcerr << L"Error: Barony is already running. Close it before using Quality Barony.\n";
		return 10;
	}
	std::wstring steamError;
	if ( !requestSteamLaunch(steamExecutable, gameArguments, steamError) )
	{
		std::wcerr << L"Error: " << steamError << L"\n";
		return 11;
	}
	std::wcout << L"Steam is preparing Barony and synchronizing saves...\n";
	FoundProcess game = waitForSteamBarony(executable);
	if ( !game.handle )
	{
		std::wcerr << L"Error: Steam did not start Barony within three minutes.\n"
			L"Resolve any Steam Cloud conflict or launch dialog, then try again.\n";
		return 12;
	}
	std::wstring injectionError;
	if ( !injectAndInitialize(game, runtimeDll, injectionError) )
	{
		TerminateProcess(game.handle, 200);
		WaitForSingleObject(game.handle, 5000);
		CloseHandle(game.handle);
		std::wcerr << L"Error: " << injectionError
			<< L"\nBarony was stopped because the mod was not active.\n";
		return 13;
	}
	std::wcout << L"Quality Barony is active. Closing launcher.\n";
	CloseHandle(game.handle);
	return 0;
}
