#ifdef HAVE_PDF_SUPPORT

#include <gtest/gtest.h>
#include "PdfDocument.h"
#include <wx/app.h>

// Test fixture for PdfDocument tests
class PdfDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure wxWidgets is initialized
        if (!wxApp::GetInstance()) {
            int argc = 0;
            char** argv = nullptr;
            wxApp::SetInstance(new wxApp());
            wxEntryStart(argc, argv);
        }
    }
};

// Basic lifecycle tests
TEST_F(PdfDocumentTest, DefaultConstructor) {
    PdfDocument doc;
    EXPECT_FALSE(doc.isLoaded());
    EXPECT_EQ(doc.getPageCount(), 0);
    EXPECT_TRUE(doc.getFilePath().empty());
}

TEST_F(PdfDocumentTest, MoveConstructor) {
    PdfDocument doc1;
    PdfDocument doc2(std::move(doc1));
    EXPECT_FALSE(doc2.isLoaded());
    EXPECT_EQ(doc2.getPageCount(), 0);
}

TEST_F(PdfDocumentTest, MoveAssignment) {
    PdfDocument doc1;
    PdfDocument doc2;
    doc2 = std::move(doc1);
    EXPECT_FALSE(doc2.isLoaded());
    EXPECT_EQ(doc2.getPageCount(), 0);
}

// Error handling tests
TEST_F(PdfDocumentTest, LoadNonExistentFile) {
    PdfDocument doc;
    EXPECT_FALSE(doc.loadFromFile("/nonexistent/file.pdf"));
    EXPECT_FALSE(doc.isLoaded());
    EXPECT_EQ(doc.getPageCount(), 0);
}

TEST_F(PdfDocumentTest, LoadInvalidFile) {
    PdfDocument doc;
    // Try to load a non-PDF file (the test binary itself)
    EXPECT_FALSE(doc.loadFromFile(__FILE__));
    EXPECT_FALSE(doc.isLoaded());
    EXPECT_EQ(doc.getPageCount(), 0);
}

TEST_F(PdfDocumentTest, RenderPageWhenNotLoaded) {
    PdfDocument doc;
    wxImage img = doc.renderPage(0);
    EXPECT_FALSE(img.IsOk());
}

TEST_F(PdfDocumentTest, RenderInvalidPageIndex) {
    PdfDocument doc;
    // Even if a document were loaded, index 999999 would be invalid
    wxImage img = doc.renderPage(999999);
    EXPECT_FALSE(img.IsOk());
}

TEST_F(PdfDocumentTest, CloseWhenNotLoaded) {
    PdfDocument doc;
    doc.close();  // Should not crash
    EXPECT_FALSE(doc.isLoaded());
}

TEST_F(PdfDocumentTest, MultipleClose) {
    PdfDocument doc;
    doc.close();
    doc.close();  // Multiple closes should be safe
    EXPECT_FALSE(doc.isLoaded());
}

// If a valid PDF file is available for testing, uncomment and update these tests:
/*
TEST_F(PdfDocumentTest, LoadValidPdf) {
    PdfDocument doc;
    ASSERT_TRUE(doc.loadFromFile("path/to/test.pdf"));
    EXPECT_TRUE(doc.isLoaded());
    EXPECT_GT(doc.getPageCount(), 0);
    EXPECT_EQ(doc.getFilePath(), "path/to/test.pdf");
}

TEST_F(PdfDocumentTest, RenderFirstPage) {
    PdfDocument doc;
    ASSERT_TRUE(doc.loadFromFile("path/to/test.pdf"));
    wxImage img = doc.renderPage(0);
    EXPECT_TRUE(img.IsOk());
    EXPECT_GT(img.GetWidth(), 0);
    EXPECT_GT(img.GetHeight(), 0);
}

TEST_F(PdfDocumentTest, RenderWithDifferentDpi) {
    PdfDocument doc;
    ASSERT_TRUE(doc.loadFromFile("path/to/test.pdf"));

    wxImage img72 = doc.renderPage(0, 72.0f);
    wxImage img150 = doc.renderPage(0, 150.0f);
    wxImage img300 = doc.renderPage(0, 300.0f);

    EXPECT_TRUE(img72.IsOk());
    EXPECT_TRUE(img150.IsOk());
    EXPECT_TRUE(img300.IsOk());

    // Higher DPI should produce larger images
    EXPECT_LT(img72.GetWidth(), img150.GetWidth());
    EXPECT_LT(img150.GetWidth(), img300.GetWidth());
}

TEST_F(PdfDocumentTest, CloseAndReload) {
    PdfDocument doc;
    ASSERT_TRUE(doc.loadFromFile("path/to/test.pdf"));
    size_t pageCount = doc.getPageCount();

    doc.close();
    EXPECT_FALSE(doc.isLoaded());
    EXPECT_EQ(doc.getPageCount(), 0);

    ASSERT_TRUE(doc.loadFromFile("path/to/test.pdf"));
    EXPECT_TRUE(doc.isLoaded());
    EXPECT_EQ(doc.getPageCount(), pageCount);
}
*/

#endif // HAVE_PDF_SUPPORT
