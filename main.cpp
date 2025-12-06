#include "MainWindow.h"
#include <wx/image.h>

class MyApp : public wxApp {
public:
  virtual bool OnInit() {
    // Initialize all image handlers (PNG, JPEG, BMP, GIF, etc.)
    wxInitAllImageHandlers();

    MainWindow *frame = new MainWindow();
    frame->Show();
    return true;
  }
};

wxIMPLEMENT_APP(MyApp);
