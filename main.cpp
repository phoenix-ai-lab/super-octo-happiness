#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QColor>
#include <QFont>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QKeySequence>

#include <memory>
#include <string>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>

#include <unicode/brkiter.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/ubrk.h>

class CodeEditor : public QMainWindow {
    Q_OBJECT

public:
    CodeEditor() {
        editor = new QsciScintilla(this);

        setupEditor();
        setupLexer();
        setupStatusBar();
        setupMenuBar();

        setCentralWidget(editor);
        setWindowTitle("Qt6 + QScintilla + ICU Code Editor");
        resize(900, 600);

        connect(editor, &QsciScintilla::textChanged,
                this, &CodeEditor::updateStats);
    }

private:
    QsciScintilla *editor;
    QString currentFile;

private slots:
    void updateStats();

    // File menu slots
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();

    void setupEditor() {
        editor->setUtf8(true);

        // Line numbers
        editor->setMarginType(0, QsciScintilla::NumberMargin);
        editor->setMarginWidth(0, "00000");
        editor->setMarginsForegroundColor(Qt::gray);

        // Brace matching
        editor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

        // Indentation
        editor->setAutoIndent(true);
        editor->setIndentationWidth(4);
        editor->setTabWidth(4);
        editor->setIndentationsUseTabs(false);

        // Caret line visible and background: keep the line you're typing white (readable).
        // Make editor's default paper/text black-on-white so the white caret line remains readable.
        editor->setPaper(Qt::white);     // editor background (non-active lines)
        editor->setColor(Qt::black);     // default text color

        editor->setCaretLineVisible(true);
        editor->setCaretLineBackgroundColor(Qt::white); // active line white

        // Font: Intel One Mono
QFont font("Intel One Mono");
font.setPointSize(11);
font.setStyleHint(QFont::Monospace);
font.setFixedPitch(true);

editor->setFont(font);
editor->setMarginsFont(font);
    }

    void setupLexer() {
        auto *lexer = new QsciLexerCPP(editor);
        lexer->setDefaultFont(editor->font());
        // Ensure default lexer style uses black text on white paper
        lexer->setColor(Qt::black, QsciLexerCPP::Default);
        lexer->setPaper(Qt::white, QsciLexerCPP::Default);
        editor->setLexer(lexer);
    }

    void setupStatusBar() {
        statusBar()->showMessage("Ready");
        updateStats();
    }

    void setupMenuBar() {
        QMenu *fileMenu = menuBar()->addMenu("&File");

        QAction *newAct = new QAction("&New", this);
        newAct->setShortcut(QKeySequence::New);
        connect(newAct, &QAction::triggered, this, &CodeEditor::newFile);
        fileMenu->addAction(newAct);

        QAction *openAct = new QAction("&Open...", this);
        openAct->setShortcut(QKeySequence::Open);
        connect(openAct, &QAction::triggered, this, &CodeEditor::openFile);
        fileMenu->addAction(openAct);

        QAction *saveAct = new QAction("&Save", this);
        saveAct->setShortcut(QKeySequence::Save);
        connect(saveAct, &QAction::triggered, this, &CodeEditor::saveFile);
        fileMenu->addAction(saveAct);

        QAction *saveAsAct = new QAction("Save &As...", this);
        saveAsAct->setShortcut(QKeySequence::SaveAs);
        connect(saveAsAct, &QAction::triggered, this, &CodeEditor::saveFileAs);
        fileMenu->addAction(saveAsAct);

        fileMenu->addSeparator();

        QAction *exitAct = new QAction("E&xit", this);
        exitAct->setShortcut(QKeySequence::Quit);
        connect(exitAct, &QAction::triggered, this, &QWidget::close);
        fileMenu->addAction(exitAct);
    }
};

#include "main.moc"

void CodeEditor::updateStats() {
    QString text = editor->text();

    // Convert to UTF-8 std::string then to ICU UnicodeString
    std::string utf8 = text.toUtf8().constData();
    icu::UnicodeString utext = icu::UnicodeString::fromUTF8(utf8);

    UErrorCode status = U_ZERO_ERROR;

    // Word count (Unicode-aware using break iterator)
    std::unique_ptr<icu::BreakIterator> wordIter(
        icu::BreakIterator::createWordInstance(icu::Locale::getDefault(), status));

    int wordCount = 0;
    if (U_SUCCESS(status) && wordIter) {
        wordIter->setText(utext);

        // Iterate boundaries; count segments whose rule status is not UBRK_WORD_NONE
        wordIter->first();
        int32_t end;
        while ((end = wordIter->next()) != icu::BreakIterator::DONE) {
            int ruleStatus = wordIter->getRuleStatus();
            if (ruleStatus != UBRK_WORD_NONE) {
                ++wordCount;
            }
        }
    }

    // Grapheme (user-perceived character) count
    status = U_ZERO_ERROR;
    std::unique_ptr<icu::BreakIterator> charIter(
        icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), status));

    int graphemes = 0;
    if (U_SUCCESS(status) && charIter) {
        charIter->setText(utext);
        int32_t start = charIter->first();
        int32_t end = charIter->next();
        for (; end != icu::BreakIterator::DONE; start = end, end = charIter->next())
            ++graphemes;
    }

    statusBar()->showMessage(
        QString("Words: %1 | Characters: %2")
            .arg(wordCount)
            .arg(graphemes));
}

void CodeEditor::newFile() {
    if (editor->isModified()) {
        auto ret = QMessageBox::question(this, "Unsaved Changes",
                                         "The document has unsaved changes. Save before creating a new file?",
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) return;
        if (ret == QMessageBox::Yes && !saveFile()) return;
    }

    editor->setText(QString());
    currentFile.clear();
    editor->setModified(false);
    statusBar()->showMessage("New file");
}

void CodeEditor::openFile() {
    if (editor->isModified()) {
        auto ret = QMessageBox::question(this, "Unsaved Changes",
                                         "The document has unsaved changes. Save before opening another file?",
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) return;
        if (ret == QMessageBox::Yes && !saveFile()) return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Open File");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Open Failed", "Cannot open file: " + file.errorString());
        return;
    }

    // Qt6: QTextStream::setCodec was removed. Read raw bytes and decode as UTF-8.
    QByteArray bytes = file.readAll();
    QString contents = QString::fromUtf8(bytes);
    file.close();

    editor->setText(contents);
    currentFile = fileName;
    editor->setModified(false);
    statusBar()->showMessage("Opened: " + fileName);
}

bool CodeEditor::saveFile() {
    if (currentFile.isEmpty()) return saveFileAs();

    QFile file(currentFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save Failed", "Cannot save file: " + file.errorString());
        return false;
    }

    // Qt6: QTextStream::setCodec was removed. Write UTF-8 bytes directly.
    QByteArray bytes = editor->text().toUtf8();
    qint64 written = file.write(bytes);
    file.close();

    if (written == -1) {
        QMessageBox::warning(this, "Save Failed", "Failed to write file: " + currentFile);
        return false;
    }

    editor->setModified(false);
    statusBar()->showMessage("Saved: " + currentFile);
    return true;
}

bool CodeEditor::saveFileAs() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save File As");
    if (fileName.isEmpty()) return false;

    currentFile = fileName;
    return saveFile();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    CodeEditor editor;
    editor.show();
    return app.exec();
}
