#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/glad.h>
#include <stdio.h>
#include <switch.h>
#include "windows.h"
#include "gui.h"

bool done = false;
int gui_mode = GUI_MODE_BROWSER;

namespace GUI
{
	static EGLDisplay s_display = EGL_NO_DISPLAY;
	static EGLContext s_context = EGL_NO_CONTEXT;
	static EGLSurface s_surface = EGL_NO_SURFACE;

	static bool InitEGL(NWindow *win)
	{
		s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

		if (!s_display)
		{
			return false;
		}

		eglInitialize(s_display, nullptr, nullptr);

		if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
		{
			eglTerminate(s_display);
			s_display = nullptr;
		}

		EGLConfig config;
		EGLint num_configs;
		static const EGLint framebuffer_attr_list[] = {
			EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_ALPHA_SIZE, 8,
			EGL_DEPTH_SIZE, 24,
			EGL_STENCIL_SIZE, 8,
			EGL_NONE};

		eglChooseConfig(s_display, framebuffer_attr_list, std::addressof(config), 1, std::addressof(num_configs));
		if (num_configs == 0)
		{
			eglTerminate(s_display);
			s_display = nullptr;
		}

		s_surface = eglCreateWindowSurface(s_display, config, win, nullptr);
		if (!s_surface)
		{
			eglTerminate(s_display);
			s_display = nullptr;
		}

		static const EGLint context_attr_list[] = {
			EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
			EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
			EGL_CONTEXT_MINOR_VERSION_KHR, 3,
			EGL_NONE};

		s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, context_attr_list);
		if (!s_context)
		{
			eglDestroySurface(s_display, s_surface);
			s_surface = nullptr;
		}

		eglMakeCurrent(s_display, s_surface, s_surface, s_context);
		return true;
	}

	static void ExitEGL(void)
	{
		if (s_display)
		{
			eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

			if (s_context)
			{
				eglDestroyContext(s_display, s_context);
				s_context = nullptr;
			}

			if (s_surface)
			{
				eglDestroySurface(s_display, s_surface);
				s_surface = nullptr;
			}

			eglTerminate(s_display);
			s_display = nullptr;
		}
	}

	bool SwapBuffers(void)
	{
		return eglSwapBuffers(s_display, s_surface);
	}

	void SetDefaultTheme(void)
	{
		ImGuiIO &io = ImGui::GetIO();
		io.MouseDrawCursor = false;
		auto &style = ImGui::GetStyle();

		style.AntiAliasedLinesUseTex = false;
		style.AntiAliasedLines = true;
		style.AntiAliasedFill = true;
		style.WindowRounding = 0.0f;
		style.FrameRounding = 2.0f;
		style.GrabRounding = 2.0f;

		style.Colors[ImGuiCol_Text] = ImVec4( 236.f / 255.f, 240.f / 255.f, 241.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_TextDisabled] = ImVec4( 236.f / 255.f, 240.f / 255.f, 241.f / 255.f, 0.58f );
		style.Colors[ImGuiCol_WindowBg] = ImVec4( 44.f / 255.f, 62.f / 255.f, 80.f / 255.f, 0.95f );
		style.Colors[ImGuiCol_ChildBg] = ImVec4( 57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 0.58f );
		style.Colors[ImGuiCol_Border] = ImVec4( 44.f / 255.f, 62.f / 255.f, 80.f / 255.f, 0.00f );
		style.Colors[ImGuiCol_BorderShadow] = ImVec4( 44.f / 255.f, 62.f / 255.f, 80.f / 255.f, 0.00f );
		style.Colors[ImGuiCol_FrameBg] = ImVec4( 57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.78f );
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_TitleBg] = ImVec4( 57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 0.75f );
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4( 57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 0.47f );
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.21f );
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.78f );
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_CheckMark] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.80f );
		style.Colors[ImGuiCol_SliderGrab] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.50f );
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_Button] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.50f );
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.86f );
		style.Colors[ImGuiCol_ButtonActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_Header] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.76f );
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.86f );
		style.Colors[ImGuiCol_HeaderActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.15f );
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.78f );
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_PlotLines] = ImVec4( 236.f / 255.f, 240.f / 255.f, 241.f / 255.f, 0.63f );
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 236.f / 255.f, 240.f / 255.f, 241.f / 255.f, 0.63f );
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.00f );
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4( 41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 0.43f );
		style.Colors[ImGuiCol_PopupBg] = ImVec4( 33.f / 255.f, 46.f / 255.f, 60.f / 255.f, 0.92f );
	}

	bool Init(FontType fontType)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO(); (void)io;
		io.Fonts->Clear();

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		if (!GUI::InitEGL(nwindowGetDefault()))
			return false;

		gladLoadGL();

		ImGui_ImplSwitch_Init("#version 130");

		// Load nintendo font
		PlFontData standard, extended, s_chinese, s_chinese_ext, t_chinese, korean;
		ImWchar extended_range[] = {0xe000, 0xe152};
		ImWchar others[] = {
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0100, 0x024F, // Latin Extended
			0x0370, 0x03FF, // Greek
			0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
			0x0590, 0x05FF, // Hebrew
			0x1E00, 0x1EFF, // Latin Extended Additional
			0x1F00, 0x1FFF, // Greek Extended
			0x2000, 0x206F, // General Punctuation
			0x2100, 0x214F, // Letterlike Symbols
			0x2460, 0x24FF, // Enclosed Alphanumerics
			0x2DE0, 0x2DFF, // Cyrillic Extended-A
			0x31F0, 0x31FF, // Katakana Phonetic Extensions
			0xA640, 0xA69F, // Cyrillic Extended-B
			0xFF00, 0xFFEF, // Half-width characters
			0,
		};
		ImWchar symbols[] = {
			0x2000, 0x206F, // General Punctuation
			0x2100, 0x214F, // Letterlike Symbols
			0x2460, 0x24FF, // Enclosed Alphanumerics
			0,
		};
		ImWchar simplified_chinese[] = {
			// All languages with chinese included
			0x3400, 0x4DBF, // CJK Rare
			0x4E00, 0x9FFF, // CJK Ideograms
			0xF900, 0xFAFF, // CJK Compatibility Ideographs
			0,
		};

		const ImWchar arabic[] = { // Arabic
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0100, 0x024F, // Latin Extended
			0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
			0x1E00, 0x1EFF, // Latin Extended Additional
			0x2000, 0x206F, // General Punctuation
			0x2100, 0x214F, // Letterlike Symbols
			0x2460, 0x24FF, // Enclosed Alphanumerics
			0x0600, 0x06FF, // Arabic
			0x0750, 0x077F, // Arabic Supplement
			0x0870, 0x089F, // Arabic Extended-B
			0x08A0, 0x08FF, // Arabic Extended-A
			0xFB50, 0xFDFF, // Arabic Presentation Forms-A
			0xFE70, 0xFEFF, // Arabic Presentation Forms-B
			0,
		};

		static const ImWchar fa_icons[] {
			0xF07B, 0xF07B, // folder
			0xF65E, 0xF65E, // new folder
			0xF15B, 0xF15B, // file
			0xF021, 0xF021, // refresh
			0xF0CA, 0xF0CA, // select all
			0xF0C9, 0xF0C9, // unselect all
			0x2700, 0x2700, // cut
			0xF0C5, 0xF0C5, // copy
			0xF0EA, 0xF0EA, // paste
			0xF31C, 0xF31C, // edit
			0xE0AC, 0xE0AC, // rename
			0xE5A1, 0xE5A1, // delete
			0xF002, 0xF002, // search
			0xF013, 0xF013, // settings
			0xF0ED, 0xF0ED, // download
			0xF0EE, 0xF0EE, // upload
			0xF56E, 0xF56E, // extract
			0xF56F, 0xF56F, // compress
			0xF0F6, 0xF0F6, // properties
			0xF112, 0xF112, // cancel
			0xF0DA, 0xF0DA, // arrow right
			0x0031, 0x0031, // 1
			0x004C, 0x004C, // L
			0x0052, 0x0052, // R
			0,
		};
	
		bool ok = R_SUCCEEDED(plGetSharedFontByType(&standard, PlSharedFontType_Standard)) &&
				  R_SUCCEEDED(plGetSharedFontByType(&extended, PlSharedFontType_NintendoExt));

		if (fontType & FONT_TYPE_SIMPLIFIED_CHINESE)
		{
			ok = ok && R_SUCCEEDED(plGetSharedFontByType(&s_chinese, PlSharedFontType_ChineseSimplified));
			ok = ok && R_SUCCEEDED(plGetSharedFontByType(&s_chinese_ext, PlSharedFontType_ExtChineseSimplified));
		}

		if (fontType & FONT_TYPE_TRADITIONAL_CHINESE)
		{
			ok = ok && R_SUCCEEDED(plGetSharedFontByType(&t_chinese, PlSharedFontType_ChineseTraditional));
			ok = ok && R_SUCCEEDED(plGetSharedFontByType(&s_chinese_ext, PlSharedFontType_ExtChineseSimplified));
		}

		if (fontType & FONT_TYPE_KOREAN)
		{
			ok = ok && R_SUCCEEDED(plGetSharedFontByType(&korean, PlSharedFontType_KO));
		}

		IM_ASSERT(ok);

		u8 *px = nullptr;
		int w = 0, h = 0, bpp = 0;
		ImFontConfig font_cfg;

		font_cfg.FontDataOwnedByAtlas = false;
		io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, others);
		font_cfg.MergeMode = true;
		io.Fonts->AddFontFromMemoryTTF(extended.address, extended.size, 18.0f, &font_cfg, extended_range);
		io.Fonts->AddFontFromFileTTF("romfs:/lang/fa-solid-900.ttf", 18.0f, &font_cfg, fa_icons);

		if (fontType & FONT_TYPE_SIMPLIFIED_CHINESE)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromMemoryTTF(s_chinese.address, s_chinese.size, 18.0f, &font_cfg, simplified_chinese);
			io.Fonts->AddFontFromMemoryTTF(s_chinese_ext.address, s_chinese_ext.size, 18.0f, &font_cfg, simplified_chinese);
		}

		if (fontType & FONT_TYPE_TRADITIONAL_CHINESE)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromMemoryTTF(t_chinese.address, t_chinese.size, 18.0f, &font_cfg, io.Fonts->GetGlyphRangesChineseFull());
			io.Fonts->AddFontFromMemoryTTF(s_chinese_ext.address, s_chinese_ext.size, 18.0f, &font_cfg, simplified_chinese);
		}

		if (fontType & FONT_TYPE_KOREAN)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromMemoryTTF(korean.address, korean.size, 18.0f, &font_cfg, io.Fonts->GetGlyphRangesKorean());
		}

		if (fontType & FONT_TYPE_JAPANESE)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
		}

		if (fontType & FONT_TYPE_THAI)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromFileTTF("romfs:/lang/Roboto_ext.ttf", 18.0f, &font_cfg, io.Fonts->GetGlyphRangesThai());
		}

		if (fontType & FONT_TYPE_ARABIC)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromFileTTF("romfs:/lang/Roboto_ext.ttf", 18.0f, &font_cfg, arabic);
		}

		if (fontType & FONT_TYPE_VIETNAMESE)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromFileTTF("romfs:/lang/Roboto_ext.ttf", 18.0f, &font_cfg, io.Fonts->GetGlyphRangesVietnamese());
		}

		if (fontType & FONT_TYPE_GREEK)
		{
			io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 18.0f, &font_cfg, symbols);
			io.Fonts->AddFontFromFileTTF("romfs:/lang/Roboto_ext.ttf", 18.0f, &font_cfg, io.Fonts->GetGlyphRangesGreek());
		}

		io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
		io.Fonts->Build();

		GUI::SetDefaultTheme();
		return true;
	}

	int RenderLoop(void)
	{
		if (!appletMainLoop())
            return false;

		Windows::Init();
		while (!done)
		{
			u64 up;

			if (gui_mode == GUI_MODE_BROWSER)
			{
				up = ImGui_ImplSwitch_NewFrame();
				ImGui::NewFrame();

				Windows::HandleWindowInput(up);
				Windows::MainWindow();
				Windows::ExecuteActions();

				ImGui::Render();

				ImGuiIO &io = ImGui::GetIO(); (void)io;
				glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
				glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
				glClear(GL_COLOR_BUFFER_BIT);
				ImGui_ImplSwitch_RenderDrawData(ImGui::GetDrawData());
				GUI::SwapBuffers();
			}
			else if (gui_mode == GUI_MODE_IME)
			{
				Windows::HandleImeInput();
			}
		}

		return 0;
	}

    void Exit(void) {
        ImGui_ImplSwitch_Shutdown();
        GUI::ExitEGL();
    }
}
