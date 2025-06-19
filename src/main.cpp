// Include project headers first
#include "embedded_translations.h" // Include the embedded translations
#include "i18n.h"                  // Include the i18n header
// Include the precompiled header last among project headers
#include "main.h"
#include "minidocx.hpp"


#include <algorithm>  // for std::reverse
#include <filesystem> // Required for path operations
#include <string>


static const char DEFAULT_LANG[] = "zh-CN";
static const int SERVER_PORT = 8889;

// https://patorjk.com/software/taag/#p=testall&f=ANSI%20Regular&t=WORDAUTO
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

//call this funciton to test the abilities of minidox
void minidocx_testfield()
{
    using namespace md;
    try
    {
        Document doc;
        doc.prop_.author_ = "Cline";
        doc.prop_.title_ = "MiniDocx Comprehensive Test";

        // 1. Section and Page Setup
        SectionPointer sect = doc.addSection();
        sect->prop_.landscape_ = true;

        // 2. Styled Paragraph (Heading)
        ParagraphStyle heading_style;
        heading_style.name_ = "My Heading Style";
        heading_style.align_ = Alignment::Centered;
        heading_style.fontSize_ = 24;
        heading_style.color_ = "FF0000";
        // bold is a character property, not a paragraph style property.
        // We will apply it to the RichText later if needed.
        doc.addParagraphStyle(heading_style);

        ParagraphPointer heading_para = sect->addParagraph();
        heading_para->prop_.style_ = "My Heading Style";
        RichTextPointer heading_text = heading_para->addRichText("Comprehensive Test for minidocx");
        heading_text->prop_.fontStyle_.bold_ = true;


        // 3. Normal Paragraph
        sect->addParagraph()->addRichText(
            "This document serves as a comprehensive test for the minidocx library, "
            "covering various features such as text formatting, images, tables, and lists.");

        // 4. Formatted Text (RichText)
        ParagraphPointer p_formatted = sect->addParagraph();
        p_formatted->addRichText("This paragraph contains ");

        RichTextPointer bold_text = p_formatted->addRichText("bold");
        bold_text->prop_.fontStyle_.bold_ = true;

        p_formatted->addRichText(", ");

        RichTextPointer italic_text = p_formatted->addRichText("italic");
        italic_text->prop_.fontStyle_.italic_ = true;

        p_formatted->addRichText(", and ");

        RichTextPointer underlined_text = p_formatted->addRichText("underlined");
        underlined_text->prop_.underline_.style_ = RichTextProperties::UnderlineStyle::Single;

        p_formatted->addRichText(" text.");

        // 5. Image
        try {
            ParagraphPointer pic_para = sect->addParagraph();
            pic_para->prop_.align_ = Alignment::Centered;
            PicturePointer pict = pic_para->addPicture(doc.addImage("extlib/minidocx/assets/logo.png"));
            pict->prop_.extent_.setSize(128, 128, 96, 100);
        } catch (const Exception& ex) {
            spdlog::warn("Could not add image to test document: {}. (Is the path correct?)", ex.what());
        }


        // 6. Table
        ParagraphPointer table_title = sect->addParagraph();
        table_title->addRichText("Here is a table:");
        table_title->prop_.spacing_ = ParagraphProperties::Spacing();
        table_title->prop_.spacing_->before_.type_ = ParagraphProperties::SpacingType::Absolute;
        table_title->prop_.spacing_->before_.value_ = 400;

        TablePointer table = sect->addTable(3, 4);
        table->prop_.width_.type_ = TableProperties::WidthType::Percent;
        table->prop_.width_.value_ = 80 * 50; // 80%

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 4; ++c) {
                CellPointer cell = table->cellAt(r, c);
                cell->addParagraph()->addRichText("Row " + std::to_string(r + 1) + ", Col " + std::to_string(c + 1));
                // Cannot set cell properties directly, this feature might be missing.
            }
        }

        // 7. Numbered List
        ParagraphPointer num_list_title = sect->addParagraph();
        num_list_title->addRichText("This is a numbered list:");
        num_list_title->prop_.spacing_ = ParagraphProperties::Spacing();
        num_list_title->prop_.spacing_->before_.type_ = ParagraphProperties::SpacingType::Absolute;
        num_list_title->prop_.spacing_->before_.value_ = 400;

        NumberingId num_id = doc.addNumberedListDefinition();
        ParagraphPointer item1 = sect->addParagraph();
        item1->addRichText("First item");
        item1->prop_.numId_ = num_id;
        item1->prop_.numLevel_ = 0;

        ParagraphPointer item2 = sect->addParagraph();
        item2->addRichText("Second item");
        item2->prop_.numId_ = num_id;
        item2->prop_.numLevel_ = 0;

        ParagraphPointer sub_item = sect->addParagraph();
        sub_item->addRichText("Sub-item A");
        sub_item->prop_.numId_ = num_id;
        sub_item->prop_.numLevel_ = 1;

        // 8. Bulleted List
        ParagraphPointer bullet_list_title = sect->addParagraph();
        bullet_list_title->addRichText("And a bulleted list:");
        bullet_list_title->prop_.spacing_ = ParagraphProperties::Spacing();
        bullet_list_title->prop_.spacing_->before_.type_ = ParagraphProperties::SpacingType::Absolute;
        bullet_list_title->prop_.spacing_->before_.value_ = 400;

        NumberingId bullet_id = doc.addBulletedListDefinition();
        ParagraphPointer bullet1 = sect->addParagraph();
        bullet1->addRichText("Apple");
        bullet1->prop_.numId_ = bullet_id;
        bullet1->prop_.numLevel_ = 0;

        ParagraphPointer bullet2 = sect->addParagraph();
        bullet2->addRichText("Orange");
        bullet2->prop_.numId_ = bullet_id;
        bullet2->prop_.numLevel_ = 0;

        ParagraphPointer bullet3 = sect->addParagraph();
        bullet3->addRichText("Banana");
        bullet3->prop_.numId_ = bullet_id;
        bullet3->prop_.numLevel_ = 0;

        doc.saveAs("bin/comprehensive_test.docx");
        spdlog::info("Successfully created comprehensive_test.docx in bin/ directory.");
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Failed to create comprehensive test docx: {}", ex.what());
    }
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

    minidocx_testfield();


    mcp::server server("localhost", SERVER_PORT);
    mcp::set_log_level(mcp::log_level::error); // Keep MCP library logs concise
    s_mcpServer_init(server, true);

    return 0;
}
