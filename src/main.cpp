// Include project headers first
#include "embedded_translations.h" // Include the embedded translations
#include "i18n.h"                  // Include the i18n header
// Include the precompiled header last among project headers
#include "main.h"

#include <algorithm>  // for std::reverse
#include <filesystem> // Required for path operations
#include <string>


static const char DEFAULT_LANG[] = "zh-CN";
static const int SERVER_PORT = 8889;

static const char ASCII_ART[] = "\n\
_ _ _ _____ _____ ____  _____ _____ _____ _____ \n\
| | | |     | __  |    \\|  _  |  |  |_   _|     |\n\
| | | |  |  |    -|  |  |     |  |  | | | |  |  |\n\
|_____|_____|__|__|____/|__|__|_____| |_| |_____|\n\                                        
v0.0.1                              By smileFAace\n";

std::string g_current_excel_file_path;

static void s_spdlog_init()
{

    spdlog::set_pattern("%^%L%$(%H:%M:%S) %v");

    bool console_sink_exists = false;
    for (const auto &sink : spdlog::default_logger()->sinks())
    {
        if (std::dynamic_pointer_cast<spdlog::sinks::stdout_color_sink_mt>(sink))
        {
            console_sink_exists = true;
            break;
        }
    }

    if (!console_sink_exists)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        spdlog::default_logger()->sinks().push_back(console_sink);
    }
}

static void s_mcpServer_init(mcp::server &server, bool blocking_mode)
{
    server.set_server_info("WordAutoCpp", "1.0.0"); // Server name/version likely not translated

    mcp::json capabilities = {
        {"tools", mcp::json::object()}};
    server.set_capabilities(capabilities);

   

    spdlog::info(i18n::t("log.info.server_start", SERVER_PORT));
    spdlog::info(i18n::t("log.info.server_stop_prompt"));

    server.start(blocking_mode);
}

#ifdef _WIN32
#include <windows.h> // Moved include here as it's needed by s_i18n_init
#endif

static void s_i18n_init()
{
    auto &i18n = i18n::I18nManager::getInstance();

    // Load English from embedded string in the dedicated header
    bool en_loaded = i18n.loadLanguageFromString("en", embedded_translations::EN_JSON);
    if (!en_loaded)
    {
        spdlog::error("Failed to load embedded English language string from header.");
    }

    // Determine the path to lang.json relative to the executable
    std::filesystem::path exe_path;
#ifdef _WIN32
    char path_buf[MAX_PATH];
    GetModuleFileNameA(NULL, path_buf, MAX_PATH);
    exe_path = path_buf;
#else
    // For non-Windows, assume current working directory or add platform-specific logic
    exe_path = std::filesystem::current_path();
#endif
    std::filesystem::path lang_json_path = exe_path.parent_path() / "lang.json";

    // Load language from lang.json
    bool lang_json_loaded = false;
    if (std::filesystem::exists(lang_json_path))
    {
        lang_json_loaded = i18n.loadLanguage("custom", lang_json_path.string());
        if (!lang_json_loaded)
        {
            spdlog::error("Failed to load language file from '{}'.", lang_json_path.string());
        }
    }
    else
    {
        spdlog::warn("lang.json not found at '{}'. Using default language.", lang_json_path.string());
    }

    // Set default language
    if (lang_json_loaded)
    {
        if (!i18n.setLanguage("custom"))
        {
            spdlog::error("Failed to set 'custom' language from lang.json. Falling back to English.");
            i18n.setLanguage("en"); // Fallback to English
        }
    }
    else if (!i18n.setLanguage("en"))
    { // Fallback to English if lang.json not loaded or failed
        spdlog::critical("Failed to load ANY language data. Application might not function correctly.");
    }

    spdlog::info("Current language set to: {}", i18n.getCurrentLanguage());
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << ASCII_ART << std::endl;

    spdlog::set_level(spdlog::level::info);
    s_spdlog_init();

    s_i18n_init();

    mcp::server server("localhost", SERVER_PORT);
    mcp::set_log_level(mcp::log_level::error); // Keep MCP library logs concise
    s_mcpServer_init(server, true);

    return 0;
}
