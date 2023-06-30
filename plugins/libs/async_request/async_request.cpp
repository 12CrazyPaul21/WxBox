#include "async_request.hpp"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <chrono>
#include <future>

#include <curl/curl.h>

static lua_State* g_main_lua_state = nullptr;

static lua_State* g_routine_lua_state = nullptr;
static std::thread g_routine_thread;

static std::queue<RequestContext> g_queue;
static std::mutex g_mutex;
static std::condition_variable g_cv;
static std::atomic_bool g_quit_flag = false;


size_t func_response_chunk(void* buffer, size_t size, size_t nmemb, void* data)
{
	size_t bytesize = size * nmemb;
	PRequestContext context = reinterpret_cast<PRequestContext>(data);

	if (!buffer || !context) {
		return 0;
	}

	if (!g_routine_lua_state) {
		return bytesize;
	}

	lua_getglobal(g_routine_lua_state, "async_request_response_callback");
	if (lua_isfunction(g_routine_lua_state, -1))
	{
		lua_pushlstring(g_routine_lua_state, (const char*)(buffer), bytesize);
		lua_pushstring(g_routine_lua_state, context->context.c_str());
		
		int ret = lua_pcall(g_routine_lua_state, 2, 0, 0);
		if (ret != LUA_OK) {
			std::cerr << "error calling callback : "
				<< lua_tostring(g_routine_lua_state, -1)
				<< std::endl;
		}
	}

	std::cout << "bytesize : " << bytesize << std::endl;
	std::cout << context->url << std::endl;
	std::cout << (char*)buffer << std::endl;

	return bytesize;
}

void func_request(RequestContext& context)
{
	CURL* curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl) {
		return;
	}

	curl_easy_setopt(curl, CURLOPT_URL, context.url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 4096);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, func_response_chunk);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		std::cerr << "curl_easy_perform() failed : "
			<< curl_easy_strerror(res)
			<< std::endl;
	}

	curl_easy_cleanup(curl);
}

static void routine(std::promise<void*>* p)
{
	RequestContext next_context;

	g_routine_lua_state = lua_newthread(g_main_lua_state);
	lua_setglobal(g_main_lua_state, "async_request_thread");
	p->set_value(g_routine_lua_state);

	for (;;)
	{
		{
			std::unique_lock<std::mutex> lock(g_mutex);
			g_cv.wait_for(lock, std::chrono::milliseconds(100), [] {
				return !g_queue.empty() || g_quit_flag;
			});
			
			if (g_quit_flag) {
				break;
			}

			if (g_queue.empty()) {
				continue;
			}

			next_context = g_queue.front();
			g_queue.pop();
		}

		func_request(next_context);
	}

	while (lua_status(g_routine_lua_state) != LUA_OK && lua_status(g_routine_lua_state) != LUA_YIELD)
	{

	}

	lua_close(g_routine_lua_state);

	std::cout << "async request routine it's finished" << std::endl;
}

static void push_mission(RequestContext context)
{
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		g_queue.push(context);
	}

	g_cv.notify_one();
}

static int __ar_register_response_callback(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setglobal(L, "async_request_response_callback");
	lua_pushboolean(L, true);
	return 1;
}

static int __ar_request(lua_State* L)
{
	const char* url = luaL_checkstring(L, 1);
	const char* context = luaL_checkstring(L, 2);

	push_mission(RequestContext{ url, context });

	lua_pushboolean(L, true);
	return 1;
}

const struct luaL_Reg AsyncRequestModuleMethods[] = {
	{"register_response_callback", __ar_register_response_callback},
    {"request", __ar_request},
    {NULL, NULL},
};

int LUA_API luaL_async_request_destroy(lua_State* L)
{
	std::cout << "async request module destroy" << std::endl;

	UNREFERENCED_PARAMETER(L);

	g_quit_flag = true;
	g_cv.notify_all();
	g_routine_thread.join();

	g_routine_lua_state = nullptr;
	g_main_lua_state = nullptr;

	curl_global_cleanup();

	return 0;
}

int LUA_API luaopen_async_request(lua_State* L)
{
	std::cout << "async request module open" << std::endl;

    luaL_newlib(L, AsyncRequestModuleMethods);

	char full_path[MAX_PATH] = { 0 };
	char abs_full_path[MAX_PATH] = { 0 };

	if (GetModuleFileNameA(GetModuleHandleA("async_request.dll"), full_path, MAX_PATH)) {
		if (GetFullPathNameA(full_path, MAX_PATH, abs_full_path, nullptr)) {
			std::string dll_search_path = std::filesystem::path::path(abs_full_path).parent_path().string();
			SetDllDirectoryA(dll_search_path.c_str());
		}
	}

	if (g_routine_lua_state) {
		return 1;
	}

    curl_global_init(CURL_GLOBAL_ALL);

	std::promise<void*> p;
	std::future<void*> f = p.get_future();

	g_main_lua_state = L;
	g_routine_thread = std::thread(routine, &p);

	f.wait();

	//luaL_newmetatable(L, "async_request");
	//lua_pushcfunction(L, luaL_async_request_destroy);
	//lua_setfield(L, -2, "__gc");
	//lua_setmetatable(L, -2);

    return 1;
}