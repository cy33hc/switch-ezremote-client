#include <cstdlib>

#include <switch.h>

#include "util.h"
#include "config.h"
#include "lang.h"
#include "gui.h"
//#include "dbglogger.h"

static SetLanguage lang;

namespace Services
{
  int Init(void)
  {
    plInitialize(PlServiceType_User);
    romfsInit();
    socketInitializeDefault();
    nxlinkStdio();
    setInitialize();
    u64 lang_code = -1;
    setGetSystemLanguage(&lang_code);
    setMakeLanguage(lang_code, &lang);
    setExit();

    CONFIG::LoadConfig();
    Lang::SetTranslation(lang);
    FontType fontType = FONT_TYPE_LATIN;
    if (strcasecmp(language, "Simplified Chinese") == 0 || lang == 6 || lang == 15)
    {
      fontType = FONT_TYPE_SIMPLIFIED_CHINESE;
    }
    else if (strcasecmp(language, "Traditional Chinese") == 0 || lang == 11 || lang == 16)
    {
      fontType = FONT_TYPE_TRADITIONAL_CHINESE;
    }
    else if (strcasecmp(language, "Korean") == 0 || lang == 7)
    {
      fontType = FONT_TYPE_KOREAN;
    }
    else if (strcasecmp(language, "Japanese") == 0 || strcasecmp(language, "Ryukyuan") == 0 || lang == 0)
    {
      fontType = FONT_TYPE_JAPANESE;
    }
    else if (strcasecmp(language, "Thai") == 0)
    {
      fontType = FONT_TYPE_THAI;
    }
    else if (strcasecmp(language, "Arabic") == 0)
    {
      fontType = FONT_TYPE_ARABIC;
    }
    else if (strcasecmp(language, "Vietnamese") == 0)
    {
      fontType = FONT_TYPE_VIETNAMESE;
    }
    else if (strcasecmp(language, "Greek") == 0)
    {
      fontType = FONT_TYPE_GREEK;
    }

    GUI::Init(fontType);
    plExit();

    return 0;
  }

  void Exit(void)
  {
    GUI::Exit();
    romfsExit();
    socketExit();
  }
} // namespace Services

int main(int argc, char* argv[])
{
  Services::Init();
	//dbglogger_init();
	//dbglogger_log("If you see this you've set up dbglogger correctly.");

  GUI::RenderLoop();

  Services::Exit();
  return 0;
}
