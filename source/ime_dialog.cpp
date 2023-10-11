#include <switch.h>
#include <string>
#include "ime_dialog.h"

static int ime_dialog_running = 0;
static int ime_dialog_option = 0;

SwkbdConfig swkbd;
char ime_initial_text[512+1];
char ime_title[64];
char ime_input_text[512+1];

namespace Dialog
{
  int initImeDialog(const char *title, const char *initial_text, int max_text_length, SwkbdType type, int option, int password)
  {
    if (ime_dialog_running)
      return IME_DIALOG_ALREADY_RUNNING;

    snprintf(ime_title, 63, "%s", title);
    snprintf(ime_initial_text, 512, "%s", initial_text);

    if (R_FAILED(swkbdCreate(&swkbd, 0)))
    {
      swkbdClose(&swkbd);
      return 0;
    }

    swkbdConfigMakePresetDefault(&swkbd);
    swkbdConfigSetHeaderText(&swkbd, ime_title);
    swkbdConfigSetInitialText(&swkbd, ime_initial_text);
    swkbdConfigSetType(&swkbd, type);
    swkbdConfigSetStringLenMax(&swkbd, max_text_length);

    ime_dialog_running = true;

    return 1;
  }

  int isImeDialogRunning()
  {
    return ime_dialog_running;
  }

  char *getImeDialogInputText()
  {
    return ime_input_text;
  }

  const char *getImeDialogInitialText()
  {
    return ime_initial_text;
  }

  int updateImeDialog()
  {
    if (!ime_dialog_running)
      return IME_DIALOG_RESULT_NONE;

    if (R_FAILED(swkbdShow(&swkbd, ime_input_text, sizeof(ime_input_text))))
    {
      ime_dialog_running = 0;
      swkbdClose(&swkbd);
      return IME_DIALOG_RESULT_CANCELED;
    }

    swkbdClose(&swkbd);
    ime_dialog_running = 0;

    return IME_DIALOG_RESULT_FINISHED;
  }

} // namespace Dialog