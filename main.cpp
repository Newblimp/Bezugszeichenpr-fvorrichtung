#include "MainWindow.h"
#include <wx/image.h>

class MyApp : public wxApp {
public:
  virtual bool OnInit() {
    // Initialize image handlers for all supported formats
    wxImage::AddHandler(new wxPNGHandler);
    wxImage::AddHandler(new wxJPEGHandler);
    wxImage::AddHandler(new wxBMPHandler);
    wxImage::AddHandler(new wxTIFFHandler);

    MainWindow *frame = new MainWindow();
    frame->Show();
    return true;
  }
};

wxIMPLEMENT_APP(MyApp);
