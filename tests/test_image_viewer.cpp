#include <gtest/gtest.h>
#include <wx/wx.h>
#include <wx/image.h>
#include "ImageDocument.h"
#include "ImageCanvas.h"
#include "ImageViewerWindow.h"

// Test app for wxWidgets initialization
class TestApp : public wxApp {
public:
    virtual bool OnInit() override {
        // Initialize image handlers
        if (!wxImage::FindHandler(wxBITMAP_TYPE_PNG)) {
            wxImage::AddHandler(new wxPNGHandler);
        }
        if (!wxImage::FindHandler(wxBITMAP_TYPE_JPEG)) {
            wxImage::AddHandler(new wxJPEGHandler);
        }
        if (!wxImage::FindHandler(wxBITMAP_TYPE_BMP)) {
            wxImage::AddHandler(new wxBMPHandler);
        }
        return true;
    }
};

// Base test class that initializes wxWidgets
class ImageViewerTestBase : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!wxApp::GetInstance()) {
            int argc = 0;
            char** argv = nullptr;
            wxApp::SetInstance(new TestApp());
            wxEntryStart(argc, argv);
            wxTheApp->CallOnInit();
        }
    }

    static void TearDownTestSuite() {
        // Note: We don't tear down wxApp as other tests might need it
    }
};

// ImageDocument Tests
class ImageDocumentTest : public ImageViewerTestBase {
};

TEST_F(ImageDocumentTest, InitiallyEmpty) {
    ImageDocument doc;
    EXPECT_EQ(doc.getPageCount(), 0);
    EXPECT_FALSE(doc.hasPages());
}

TEST_F(ImageDocumentTest, AddPageIncreasesCount) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    testImage.InitAlpha();  // Initialize to prevent issues
    doc.addPage(testImage, "/path/test.png");
    EXPECT_EQ(doc.getPageCount(), 1);
    EXPECT_TRUE(doc.hasPages());
}

TEST_F(ImageDocumentTest, AddMultiplePagesIncreasesCount) {
    ImageDocument doc;
    wxImage testImage1(100, 100);
    wxImage testImage2(200, 200);
    testImage1.InitAlpha();
    testImage2.InitAlpha();

    doc.addPage(testImage1, "/path/test1.png");
    doc.addPage(testImage2, "/path/test2.png");

    EXPECT_EQ(doc.getPageCount(), 2);
    EXPECT_TRUE(doc.hasPages());
}

TEST_F(ImageDocumentTest, ClearRemovesAllPages) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    testImage.InitAlpha();
    doc.addPage(testImage, "/path/test1.png");
    doc.addPage(testImage, "/path/test2.png");

    doc.clear();

    EXPECT_EQ(doc.getPageCount(), 0);
    EXPECT_FALSE(doc.hasPages());
}

TEST_F(ImageDocumentTest, PageIndexValidation) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    testImage.InitAlpha();
    doc.addPage(testImage, "/path/test.png");

    EXPECT_TRUE(doc.isValidPageIndex(0));
    EXPECT_FALSE(doc.isValidPageIndex(1));
    EXPECT_FALSE(doc.isValidPageIndex(100));
}

TEST_F(ImageDocumentTest, GetPageReturnsCorrectImage) {
    ImageDocument doc;
    wxImage testImage(100, 150);
    testImage.InitAlpha();
    doc.addPage(testImage, "/path/test.png");

    const wxImage& retrieved = doc.getPage(0);
    EXPECT_EQ(retrieved.GetWidth(), 100);
    EXPECT_EQ(retrieved.GetHeight(), 150);
}

TEST_F(ImageDocumentTest, GetPagePathReturnsCorrectPath) {
    ImageDocument doc;
    wxImage testImage(100, 100);
    testImage.InitAlpha();
    std::string path = "/path/to/test.png";
    doc.addPage(testImage, path);

    EXPECT_EQ(doc.getPagePath(0), path);
}

// ImageCanvas Tests
class ImageCanvasTest : public ImageViewerTestBase {
protected:
    wxFrame* m_frame{nullptr};
    ImageCanvas* m_canvas{nullptr};

    void SetUp() override {
        m_frame = new wxFrame(nullptr, wxID_ANY, "Test");
        m_canvas = new ImageCanvas(m_frame);
    }

    void TearDown() override {
        if (m_frame) {
            m_frame->Destroy();
            m_frame = nullptr;
            m_canvas = nullptr;  // Destroyed with frame
        }
    }
};

TEST_F(ImageCanvasTest, InitialZoomIs100Percent) {
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), 1.0);
}

TEST_F(ImageCanvasTest, ZoomInIncreasesZoom) {
    double initialZoom = m_canvas->getZoom();
    m_canvas->zoomIn();
    EXPECT_GT(m_canvas->getZoom(), initialZoom);
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), initialZoom * ImageCanvas::ZOOM_STEP);
}

TEST_F(ImageCanvasTest, ZoomOutDecreasesZoom) {
    m_canvas->setZoom(2.0);
    double initialZoom = m_canvas->getZoom();
    m_canvas->zoomOut();
    EXPECT_LT(m_canvas->getZoom(), initialZoom);
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), initialZoom / ImageCanvas::ZOOM_STEP);
}

TEST_F(ImageCanvasTest, ZoomClampedToMinimum) {
    m_canvas->setZoom(0.01);  // Below minimum
    EXPECT_GE(m_canvas->getZoom(), ImageCanvas::MIN_ZOOM);
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), ImageCanvas::MIN_ZOOM);
}

TEST_F(ImageCanvasTest, ZoomClampedToMaximum) {
    m_canvas->setZoom(100.0);  // Above maximum
    EXPECT_LE(m_canvas->getZoom(), ImageCanvas::MAX_ZOOM);
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), ImageCanvas::MAX_ZOOM);
}

TEST_F(ImageCanvasTest, ZoomActualSetsTo100Percent) {
    m_canvas->setZoom(2.5);
    m_canvas->zoomToActual();
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), 1.0);
}

TEST_F(ImageCanvasTest, InitiallyNoImage) {
    EXPECT_FALSE(m_canvas->hasImage());
}

TEST_F(ImageCanvasTest, SetImageSetsHasImage) {
    wxImage testImage(100, 100);
    testImage.InitAlpha();
    m_canvas->setImage(testImage);
    EXPECT_TRUE(m_canvas->hasImage());
}

TEST_F(ImageCanvasTest, ClearImageRemovesImage) {
    wxImage testImage(100, 100);
    testImage.InitAlpha();
    m_canvas->setImage(testImage);
    m_canvas->clearImage();
    EXPECT_FALSE(m_canvas->hasImage());
}

TEST_F(ImageCanvasTest, SetImageResetsZoomTo100Percent) {
    wxImage testImage(100, 100);
    testImage.InitAlpha();
    m_canvas->setZoom(2.5);
    m_canvas->setImage(testImage);
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), 1.0);
}

TEST_F(ImageCanvasTest, MultipleZoomInOperations) {
    double zoom = 1.0;
    for (int i = 0; i < 3; i++) {
        m_canvas->zoomIn();
        zoom *= ImageCanvas::ZOOM_STEP;
    }
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), zoom);
}

TEST_F(ImageCanvasTest, MultipleZoomOutOperations) {
    m_canvas->setZoom(5.0);
    double zoom = 5.0;
    for (int i = 0; i < 3; i++) {
        m_canvas->zoomOut();
        zoom /= ImageCanvas::ZOOM_STEP;
    }
    EXPECT_DOUBLE_EQ(m_canvas->getZoom(), zoom);
}

// ImageViewerWindow Tests
class ImageViewerWindowTest : public ImageViewerTestBase {
protected:
    wxFrame* m_parent{nullptr};
    ImageViewerWindow* m_viewer{nullptr};

    void SetUp() override {
        m_parent = new wxFrame(nullptr, wxID_ANY, "Parent");
        m_viewer = new ImageViewerWindow(m_parent);
    }

    void TearDown() override {
        if (m_viewer) {
            m_viewer->Destroy();
            m_viewer = nullptr;
        }
        if (m_parent) {
            m_parent->Destroy();
            m_parent = nullptr;
        }
    }
};

TEST_F(ImageViewerWindowTest, InitiallyNoPages) {
    EXPECT_EQ(m_viewer->getPageCount(), 0);
    EXPECT_EQ(m_viewer->getCurrentPage(), 0);
}

TEST_F(ImageViewerWindowTest, CloseDocumentClearsPages) {
    // Can't easily test file loading without real files
    // but we can test that closeDocument doesn't crash
    m_viewer->closeDocument();
    EXPECT_EQ(m_viewer->getPageCount(), 0);
}

TEST_F(ImageViewerWindowTest, PageNavigationBoundsChecking) {
    // Without pages, navigation shouldn't crash
    m_viewer->nextPage();
    EXPECT_EQ(m_viewer->getCurrentPage(), 0);

    m_viewer->previousPage();
    EXPECT_EQ(m_viewer->getCurrentPage(), 0);
}

TEST_F(ImageViewerWindowTest, GoToPageWithInvalidIndex) {
    // Should handle invalid indices gracefully
    m_viewer->goToPage(100);
    EXPECT_EQ(m_viewer->getCurrentPage(), 0);
}

// Integration test (disabled by default - requires test image files)
TEST_F(ImageViewerWindowTest, DISABLED_LoadRealImageFile) {
    // This test would require a real image file
    // Enable and provide path when testing with actual files
    wxString testImagePath = "/path/to/test/image.png";
    bool loaded = m_viewer->openFile(testImagePath);

    if (loaded) {
        EXPECT_GT(m_viewer->getPageCount(), 0);
        EXPECT_EQ(m_viewer->getCurrentPage(), 0);
    }
}
